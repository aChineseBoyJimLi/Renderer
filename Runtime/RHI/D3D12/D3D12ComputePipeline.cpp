#include "D3D12PipelineState.h"
#include "D3D12Device.h"
#include "../../Core/Log.h"
#include "../../Core/Templates.h"

RefCountPtr<RHIShader> D3D12Device::CreateShader(ERHIShaderType inType)
{
    RefCountPtr<RHIShader> shader(new D3D12Shader(inType));
    if(!shader->Init())
    {
        Log::Error("[D3D12] Failed to create shader");
    }
    return shader;
}

void D3D12Shader::SetByteCode(std::shared_ptr<Blob> inByteCode)
{
    m_ShaderBlob = inByteCode;
    m_ShaderByteCodeD3D.pShaderBytecode = m_ShaderBlob->GetData();
    m_ShaderByteCodeD3D.BytecodeLength = m_ShaderBlob->GetSize();
}

RefCountPtr<RHIComputePipeline> D3D12Device::CreatePipeline(const RHIComputePipelineDesc& inDesc)
{
    RefCountPtr<RHIComputePipeline> pipeline(new D3D12ComputePipeline(*this, inDesc));
    if(!pipeline->Init())
    {
        Log::Error("[D3D12] Failed to create compute pipeline");
    }
    return pipeline;
}

D3D12ComputePipeline::D3D12ComputePipeline(D3D12Device& inDevice, const RHIComputePipelineDesc& inDesc)
    : m_Device(inDevice), m_Desc(inDesc), m_PipelineState(nullptr)
{
    
}

D3D12ComputePipeline::~D3D12ComputePipeline()
{
    ShutdownInternal();
}

bool D3D12ComputePipeline::Init()
{
    if(IsValid())
    {
        Log::Warning("[D3D12] The compute pipeline already initialized");
        return true;
    }

    if(m_Desc.ComputeShader == nullptr || !m_Desc.ComputeShader->IsValid())
    {
        Log::Error("[D3D12] The compute shader is invalid");
        return false;
    }

    D3D12PipelineBindingLayout* bindingLayout = CheckCast<D3D12PipelineBindingLayout*>(m_Desc.BindingLayout);
    if(bindingLayout == nullptr || !bindingLayout->IsValid())
    {
        Log::Error("[D3D12] The compute pipeline binding layout is invalid");
        return false;
    }
    
    D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineDesc{};
    pipelineDesc.pRootSignature = bindingLayout->GetRootSignature();
    pipelineDesc.CS.pShaderBytecode = m_Desc.ComputeShader->GetData();
    pipelineDesc.CS.BytecodeLength = m_Desc.ComputeShader->GetSize();
    pipelineDesc.NodeMask = D3D12Device::GetNodeMask();

    HRESULT hr = m_Device.GetDevice()->CreateComputePipelineState(&pipelineDesc, IID_PPV_ARGS(&m_PipelineState));
    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        return false;
    }

    return true;
}

void D3D12ComputePipeline::Shutdown()
{
    ShutdownInternal();
}

bool D3D12ComputePipeline::IsValid() const
{
    return m_PipelineState != nullptr;
}

void D3D12ComputePipeline::ShutdownInternal()
{
    m_PipelineState.Reset();
}

void D3D12ComputePipeline::SetNameInternal()
{
    if(IsValid())
    {
        std::wstring name(m_Name.begin(), m_Name.end());
        m_PipelineState->SetName(name.c_str());
    }
}
