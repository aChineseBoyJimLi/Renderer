#include "D3D12PipelineState.h"
#include "D3D12Device.h"
#include "D3D12Resources.h"
#include "../../Core/Log.h"
#include "../../Core/Misc.h"
#include "../../Core/Templates.h"

D3D12RayTracingPipeline::D3D12RayTracingPipeline(D3D12Device& inDevice, const RHIRayTracingPipelineDesc& inDesc)
    : m_Device(inDevice), m_Desc(inDesc), m_ShaderTable()
{
    
}

D3D12RayTracingPipeline::~D3D12RayTracingPipeline()
{
    ShutdownInternal();
}

bool D3D12RayTracingPipeline::Init()
{
    if(IsValid())
    {
        Log::Warning("[D3D12] The ray tracing pipeline is already initialized");
        return true;
    }
    

    CD3DX12_STATE_OBJECT_DESC pipelineDesc(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

    // Create global root signature sub object
    const D3D12PipelineBindingLayout* bindingLayout = CheckCast<D3D12PipelineBindingLayout*>(m_Desc.GlobalBindingLayout);
    if(bindingLayout != nullptr && bindingLayout->IsValid())
    {
        auto globalSignatureSubObject = pipelineDesc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
        globalSignatureSubObject->SetRootSignature(bindingLayout->GetRootSignature());
    }
    
    auto shaderConfigSubObject = pipelineDesc.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
    shaderConfigSubObject->Config(m_Desc.MaxPayloadSize, m_Desc.MaxAttributeSize);

    auto pipelineConfigSubObject = pipelineDesc.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
    pipelineConfigSubObject->Config(m_Desc.MaxRecursionDepth);

    std::vector<std::wstring> exportGroupName;
    std::vector<std::wstring> shaderGroupNames;
    
    auto ImportShader = [&](const RHIShader* inShader)
    {
        const D3D12Shader* shader = CheckCast<const D3D12Shader*>(inShader);
        if(shader && shader->IsValid())
        {
            auto shaderSubObject = pipelineDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
            shaderSubObject->SetDXILLibrary(shader->GetByteCodeD3D());
            shaderGroupNames.emplace_back(inShader->GetEntryName().begin(), inShader->GetEntryName().end());
            shaderSubObject->DefineExport(shaderGroupNames.back().c_str());
        }
    };

    auto AssociateLocalRootSignature = [&](const RHIPipelineBindingLayout* inLayout, const std::wstring& inExportName)
    {
        const D3D12PipelineBindingLayout* localBindingLayout = CheckCast<const D3D12PipelineBindingLayout*>(inLayout);
        if(localBindingLayout)
        {
            auto localRootSignatureSubObject = pipelineDesc.CreateSubobject<CD3DX12_LOCAL_ROOT_SIGNATURE_SUBOBJECT>();
            localRootSignatureSubObject->SetRootSignature(localBindingLayout->GetRootSignature());
            auto associationSubObject = pipelineDesc.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
            associationSubObject->SetSubobjectToAssociate(*localRootSignatureSubObject);
            associationSubObject->AddExport(inExportName.c_str());
        }
    };

    // Create raygen shader sub object
    const size_t ragGenShaderCount = 1;
    if(m_Desc.RayGenShaderGroup.RayGenShader == nullptr)
    {
        Log::Error("[D3D12] Failed to create ray tracing pipeline without raygen shader");
        return false;
    }

    ImportShader(m_Desc.RayGenShaderGroup.RayGenShader);
    AssociateLocalRootSignature(m_Desc.RayGenShaderGroup.LocalBindingLayout, shaderGroupNames.back());
    exportGroupName.push_back(shaderGroupNames.back());

    // Create miss shader sub object
    const size_t missShaderCount = m_Desc.MissShaderGroups.size();
    for(uint32_t i = 0; i < missShaderCount; ++i)
    {
        ImportShader(m_Desc.MissShaderGroups[i].MissShader);
        AssociateLocalRootSignature(m_Desc.RayGenShaderGroup.LocalBindingLayout, shaderGroupNames.back());
        exportGroupName.push_back(shaderGroupNames.back());
    }

    // Create hit group sub object
    const size_t hitGroupCount = m_Desc.HitGroups.size();
    for(uint32_t i = 0; i < hitGroupCount; ++i)
    {
        auto& hitGroup = m_Desc.HitGroups[i];
        ImportShader(hitGroup.ClosestHitShader);
        ImportShader(hitGroup.AnyHitShader);
        ImportShader(hitGroup.IntersectionShader);
        auto hitGroupSubObject = pipelineDesc.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
        if(hitGroup.ClosestHitShader)
        {
            hitGroupSubObject->SetClosestHitShaderImport(std::wstring(hitGroup.ClosestHitShader->GetEntryName().begin()
                , hitGroup.ClosestHitShader->GetEntryName().end()).c_str());
        }
        std::wstring hitGroupName(StringFormat(L"HitGroup%d", i));
        hitGroupSubObject->SetHitGroupExport(hitGroupName.c_str());
        if(hitGroup.isProceduralPrimitive)
        {
            hitGroupSubObject->SetHitGroupType(D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE);
        }
        else
        {
            hitGroupSubObject->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
        }
        AssociateLocalRootSignature(hitGroup.LocalBindingLayout, hitGroupName);
        exportGroupName.push_back(hitGroupName);
    }
    
    HRESULT hr = m_Device.GetDevice()->CreateStateObject(pipelineDesc, IID_PPV_ARGS(&m_PipelineState));
    if(FAILED(hr))
    {
        Log::Error("Failed to create a DXR pipeline state object");
        OUTPUT_D3D12_FAILED_RESULT(hr)
        return false;
    }

    // Create Shader Table
    hr = m_PipelineState->QueryInterface(IID_PPV_ARGS(&m_PipelineProperties));
    if(FAILED(hr))
    {
        Log::Error("Failed to get a DXR pipeline info interface from a PSO");
        OUTPUT_D3D12_FAILED_RESULT(hr)
        return false;
    }

    const size_t shaderIdentifierCount = ragGenShaderCount + missShaderCount + hitGroupCount;
    const size_t shaderTableStride = Align(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, 64);
    const size_t shaderTableSize = shaderTableStride * shaderIdentifierCount;

    RHIBufferDesc shaderTableDesc = RHIBufferDesc::ShaderTable(shaderTableSize);
    m_ShaderTable.m_ShaderTableBuffer = m_Device.CreateD3D12Buffer(shaderTableDesc);
    
    RHIResourceGpuAddress shaderTableAddress = m_ShaderTable.m_ShaderTableBuffer->GetGpuAddress();
    void* pData = m_ShaderTable.m_ShaderTableBuffer->Map(shaderTableSize);

    uint32_t index = 0;
    memcpy(static_cast<uint8_t*>(pData) + index * shaderTableStride
        , m_PipelineProperties->GetShaderIdentifier(exportGroupName[index].c_str())
        , shaderTableStride);
    index++;
    m_ShaderTable.m_RayGenerationShaderRecord.StartAddress = shaderTableAddress;
    m_ShaderTable.m_RayGenerationShaderRecord.StrideInBytes = shaderTableStride;
    m_ShaderTable.m_RayGenerationShaderRecord.SizeInBytes = shaderTableStride;


    uint32_t missGroupStartIndex = index;
    for(uint32_t i = 0; i < missShaderCount; ++i)
    {
        memcpy(static_cast<uint8_t*>(pData) + index * shaderTableStride
            , m_PipelineProperties->GetShaderIdentifier(exportGroupName[index].c_str())
            , shaderTableStride);
        index++;
    }
    m_ShaderTable.m_MissShaderRecord.StartAddress = shaderTableAddress + missGroupStartIndex * shaderTableStride;
    m_ShaderTable.m_MissShaderRecord.StrideInBytes = shaderTableStride;
    m_ShaderTable.m_MissShaderRecord.SizeInBytes = missShaderCount * shaderTableStride;

    uint32_t hitGroupStartIndex = index;
    for(uint32_t i = 0; i < hitGroupCount; ++i)
    {
        memcpy(static_cast<uint8_t*>(pData) + index * shaderTableStride
            , m_PipelineProperties->GetShaderIdentifier(exportGroupName[index].c_str())
            , shaderTableStride);
        index++;
    }

    m_ShaderTable.m_HitGroupRecord.StartAddress = shaderTableAddress + hitGroupStartIndex * shaderTableStride;
    m_ShaderTable.m_HitGroupRecord.StrideInBytes = shaderTableStride;
    m_ShaderTable.m_HitGroupRecord.SizeInBytes = hitGroupCount * shaderTableStride;
    
    m_ShaderTable.m_ShaderTableBuffer->Unmap();
    
    return true;
}

void D3D12RayTracingPipeline::Shutdown()
{
    ShutdownInternal();
}

bool D3D12RayTracingPipeline::IsValid() const
{
    return m_PipelineState != nullptr && m_PipelineProperties != nullptr;
}

void D3D12RayTracingPipeline::ShutdownInternal()
{
    m_PipelineState.Reset();
    m_PipelineProperties.Reset();
}

void D3D12RayTracingPipeline::SetNameInternal()
{
    if(IsValid())
    {
        std::wstring name(m_Name.begin(), m_Name.end());
        m_PipelineState->SetName(name.c_str());
    }
}

D3D12ShaderTable::~D3D12ShaderTable()
{
    m_ShaderTableBuffer.SafeRelease();
}
