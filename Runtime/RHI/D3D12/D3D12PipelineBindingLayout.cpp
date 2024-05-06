#include "D3D12PipelineState.h"
#include "D3D12Device.h"
#include "../../Core/Log.h"

std::shared_ptr<RHIPipelineBindingLayout> D3D12Device::CreatePipelineBindingLayout(const RHIPipelineBindingLayoutDesc& inBindingItems)
{
    std::shared_ptr<RHIPipelineBindingLayout> layout(new D3D12PipelineBindingLayout(*this, inBindingItems));
    if(!layout->Init())
    {
        Log::Error("Failed to create pipeline binding layout");
    }
    return layout;
}

D3D12PipelineBindingLayout::D3D12PipelineBindingLayout(D3D12Device& inDevice, const RHIPipelineBindingLayoutDesc& inBindingItems)
    : m_Device(inDevice)
    , m_Desc(inBindingItems)
    , m_RootSignature(nullptr)
{
    
}

D3D12PipelineBindingLayout::~D3D12PipelineBindingLayout()
{
    ShutdownInternal();
}

bool D3D12PipelineBindingLayout::Init()
{
    if(IsValid())
    {
        Log::Warning("[D3D12] The pipeline binding layout already initialized");
        return true;
    }

    std::array<CD3DX12_DESCRIPTOR_RANGE, s_RootSignatureMaxSize> descriptorRanges;
    std::vector<CD3DX12_ROOT_PARAMETER> rootParamters;
    uint8_t totalRootSignaturesSize = 0;
    uint8_t totalCbvDescriptorsCount = 0;
    uint8_t totalDescriptorRangesCount = 0;
    
    auto AddDescriptorTable = [&](D3D12_DESCRIPTOR_RANGE_TYPE inType, uint32_t inNumDescriptors, uint32_t inBaseRegister, uint32_t inSpace)
    {
        descriptorRanges[totalDescriptorRangesCount].Init(inType, inNumDescriptors, inBaseRegister, inSpace);
        rootParamters.emplace_back();
        rootParamters.back().InitAsDescriptorTable(1, &descriptorRanges[totalDescriptorRangesCount], D3D12_SHADER_VISIBILITY_ALL);
        totalRootSignaturesSize += 1;
        totalDescriptorRangesCount++;
    };
    
    for(const auto& bindingItem : m_Desc.Items)
    {
        if(totalRootSignaturesSize > s_RootSignatureMaxSize)
        {
            Log::Error("[D3D12] The root signature size exceeds the maximum size of %d DWORDs", s_RootSignatureMaxSize);
            return false;
        }
        
        if(bindingItem.Type == ERHIBindingResourceType::Buffer_CBV && !bindingItem.IsBindless && totalCbvDescriptorsCount < s_CbvRootDescriptorsMaxCount)
        {
            for(uint32_t i = 0; i < bindingItem.NumResources; i++)
            {
                rootParamters.emplace_back();
                rootParamters.back().InitAsConstantBufferView(bindingItem.BaseRegister + i, bindingItem.Space, D3D12_SHADER_VISIBILITY_ALL);
                totalRootSignaturesSize += 2;
                totalCbvDescriptorsCount++;
            }
            continue;
        }

        if(bindingItem.Type == ERHIBindingResourceType::Buffer_CBV)
        {
            AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, bindingItem.NumResources, bindingItem.BaseRegister, bindingItem.Space);
        }
        else if(bindingItem.Type == ERHIBindingResourceType::Buffer_SRV ||
            bindingItem.Type == ERHIBindingResourceType::Texture_SRV)
        {
            AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, bindingItem.NumResources, bindingItem.BaseRegister, bindingItem.Space);
        }
        else if(bindingItem.Type == ERHIBindingResourceType::Buffer_UAV ||
            bindingItem.Type == ERHIBindingResourceType::Texture_UAV ||
            bindingItem.Type == ERHIBindingResourceType::AccelerationStructure)
        {
            AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, bindingItem.NumResources, bindingItem.BaseRegister, bindingItem.Space);
        }
        else if(bindingItem.Type == ERHIBindingResourceType::Sampler)
        {
            AddDescriptorTable(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, bindingItem.NumResources, bindingItem.BaseRegister, bindingItem.Space);
        }
    }

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
    
    if(m_Desc.IsRayTracingLocalLayout)
        rootSignatureFlags |= D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;
    if(m_Desc.AllowInputLayout)
        rootSignatureFlags |= D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    rootSignatureDesc.Init((uint32_t)rootParamters.size()
           , rootParamters.data()
           , 0, nullptr
           , rootSignatureFlags);

    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
    HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        Log::Error("[D3D12] Failed to serialize the root signature: %s", static_cast<const char*>(errorBlob->GetBufferPointer()));
        return false;
    }

    hr = m_Device.GetDevice()->CreateRootSignature(D3D12Device::GetNodeMask(),
    signatureBlob->GetBufferPointer()
        , signatureBlob->GetBufferSize()
        , IID_PPV_ARGS(&m_RootSignature));

    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        Log::Error("[D3D12] Failed to create the root signature");
        return false;
    }
    
    return true;
}

void D3D12PipelineBindingLayout::Shutdown()
{
    ShutdownInternal();   
}

void D3D12PipelineBindingLayout::ShutdownInternal()
{
    m_RootSignature.Reset();
}

bool D3D12PipelineBindingLayout::IsValid() const
{
    return m_RootSignature != nullptr;
}

void D3D12PipelineBindingLayout::SetNameInternal()
{
    if(IsValid())
    {
        std::wstring name(m_Name.begin(), m_Name.end());
        m_RootSignature->SetName(name.c_str());
    }
}

