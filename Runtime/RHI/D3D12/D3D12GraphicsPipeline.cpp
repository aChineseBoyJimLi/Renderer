#include "D3D12PipelineState.h"
#include "D3D12Device.h"
#include "D3D12Resources.h"
#include "../../Core/Log.h"
#include "../../Core/Templates.h"

RefCountPtr<RHIGraphicsPipeline> D3D12Device::CreatePipeline(const RHIGraphicsPipelineDesc& inDesc)
{
    RefCountPtr<RHIGraphicsPipeline> pipeline(new D3D12GraphicsPipeline(*this, inDesc));
    if(!pipeline->Init())
    {
        Log::Error("[D3D12] Failed to create graphics pipeline");
    }
    return pipeline;
}

D3D12GraphicsPipeline::D3D12GraphicsPipeline(D3D12Device& inDevice, const RHIGraphicsPipelineDesc& inDesc)
    : m_Device(inDevice)
    , m_Desc(inDesc)
    , m_PipelineState(nullptr)
{
    
}

D3D12GraphicsPipeline::~D3D12GraphicsPipeline()
{
    ShutdownInternal();
}

bool D3D12GraphicsPipeline::Init()
{
    if(IsValid())
    {
        return true;
    }
    
    D3D12PipelineBindingLayout* bindingLayout = CheckCast<D3D12PipelineBindingLayout*>(m_Desc.BindingLayout);
    if(bindingLayout == nullptr || !bindingLayout->IsValid())
    {
        Log::Error("[D3D12] The graphics pipeline binding layout is invalid");
        return false;
    }

    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements;
    InitInputElements(inputElements);

    HRESULT hr;
    
    if(m_Desc.UsingMeshShader)
    {
        D3DX12_MESH_SHADER_PIPELINE_STATE_DESC pipelineStateDesc{};
        pipelineStateDesc.pRootSignature = bindingLayout->GetRootSignature();
        if(m_Desc.AmplificationShader && m_Desc.AmplificationShader->IsValid())
        {
            pipelineStateDesc.AS.pShaderBytecode = m_Desc.AmplificationShader->GetData();
            pipelineStateDesc.AS.BytecodeLength = m_Desc.AmplificationShader->GetSize();
        }
        pipelineStateDesc.MS.pShaderBytecode = m_Desc.MeshShader->GetData();
        pipelineStateDesc.MS.BytecodeLength = m_Desc.MeshShader->GetSize();
        pipelineStateDesc.PS.pShaderBytecode = m_Desc.PixelShader->GetData();
        pipelineStateDesc.PS.BytecodeLength = m_Desc.PixelShader->GetSize();
        pipelineStateDesc.NodeMask = D3D12Device::GetNodeMask();
        RHI::D3D12::TranslateBlendState(m_Desc.BlendState, pipelineStateDesc.BlendState);
        RHI::D3D12::TranslateRasterizerState(m_Desc.RasterizerState, pipelineStateDesc.RasterizerState);
        RHI::D3D12::TranslateDepthStencilState(m_Desc.DepthStencilState, pipelineStateDesc.DepthStencilState);

        switch (m_Desc.PrimitiveType)
        {
        case ERHIPrimitiveType::PointList:
            pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
            break;
        case ERHIPrimitiveType::LineList:
            pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
            break;
        case ERHIPrimitiveType::TriangleList:
        case ERHIPrimitiveType::TriangleStrip:
            pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            break;
        case ERHIPrimitiveType::PatchList:
            pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
            break;
        case ERHIPrimitiveType::TriangleListWithAdjacency:
        case ERHIPrimitiveType::TriangleStripWithAdjacency:
        case ERHIPrimitiveType::TriangleFan:
            pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
        }
    
        pipelineStateDesc.SampleMask = ~0u;
        pipelineStateDesc.SampleDesc.Count = m_Desc.SampleCount;
        pipelineStateDesc.SampleDesc.Quality = m_Desc.SampleQuality;
    
        pipelineStateDesc.DSVFormat = RHI::D3D12::ConvertFormat(m_Desc.DSVFormat);
        pipelineStateDesc.NumRenderTargets = m_Desc.NumRenderTarget;
        for(uint32_t i = 0; i < pipelineStateDesc.NumRenderTargets; i++)
        {
            pipelineStateDesc.RTVFormats[i] = RHI::D3D12::ConvertFormat(m_Desc.RTVFormats[i]);
        }

        D3D12_PIPELINE_STATE_STREAM_DESC streamDesc{};
        streamDesc.SizeInBytes = sizeof(pipelineStateDesc);
        streamDesc.pPipelineStateSubobjectStream = &pipelineStateDesc;
        hr = m_Device.GetDevice()->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&m_PipelineState));
    }
    else
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc{};
        pipelineStateDesc.pRootSignature = bindingLayout->GetRootSignature();
        pipelineStateDesc.InputLayout.pInputElementDescs = inputElements.data();
        pipelineStateDesc.InputLayout.NumElements = (uint32_t)inputElements.size();
        pipelineStateDesc.NodeMask = D3D12Device::GetNodeMask();
        pipelineStateDesc.VS.pShaderBytecode = m_Desc.VertexShader->GetData();
        pipelineStateDesc.VS.BytecodeLength = m_Desc.VertexShader->GetSize();
        if(m_Desc.HullShader && m_Desc.HullShader->IsValid())
        {
            pipelineStateDesc.HS.pShaderBytecode = m_Desc.HullShader->GetData();
            pipelineStateDesc.HS.BytecodeLength = m_Desc.HullShader->GetSize();
        }
        if(m_Desc.DomainShader && m_Desc.DomainShader->IsValid())
        {
            pipelineStateDesc.DS.pShaderBytecode = m_Desc.DomainShader->GetData();
            pipelineStateDesc.DS.BytecodeLength = m_Desc.DomainShader->GetSize();
        }
        if(m_Desc.GeometryShader && m_Desc.GeometryShader->IsValid())
        {
            pipelineStateDesc.GS.pShaderBytecode = m_Desc.GeometryShader->GetData();
            pipelineStateDesc.GS.BytecodeLength = m_Desc.GeometryShader->GetSize();
        }
        pipelineStateDesc.PS.pShaderBytecode = m_Desc.PixelShader->GetData();
        pipelineStateDesc.PS.BytecodeLength = m_Desc.PixelShader->GetSize();
        
        RHI::D3D12::TranslateBlendState(m_Desc.BlendState, pipelineStateDesc.BlendState);
        RHI::D3D12::TranslateRasterizerState(m_Desc.RasterizerState, pipelineStateDesc.RasterizerState);
        RHI::D3D12::TranslateDepthStencilState(m_Desc.DepthStencilState, pipelineStateDesc.DepthStencilState);

        switch (m_Desc.PrimitiveType)
        {
        case ERHIPrimitiveType::PointList:
            pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
            break;
        case ERHIPrimitiveType::LineList:
            pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
            break;
        case ERHIPrimitiveType::TriangleList:
        case ERHIPrimitiveType::TriangleStrip:
            pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            break;
        case ERHIPrimitiveType::PatchList:
            pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
            break;
        case ERHIPrimitiveType::TriangleListWithAdjacency:
        case ERHIPrimitiveType::TriangleStripWithAdjacency:
        case ERHIPrimitiveType::TriangleFan:
            pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
        }
    
        pipelineStateDesc.SampleMask = ~0u;
        pipelineStateDesc.SampleDesc.Count = m_Desc.SampleCount;
        pipelineStateDesc.SampleDesc.Quality = m_Desc.SampleQuality;
    
        pipelineStateDesc.DSVFormat = RHI::D3D12::ConvertFormat(m_Desc.DSVFormat);
        pipelineStateDesc.NumRenderTargets = m_Desc.NumRenderTarget;
        for(uint32_t i = 0; i < pipelineStateDesc.NumRenderTargets; i++)
        {
            pipelineStateDesc.RTVFormats[i] = RHI::D3D12::ConvertFormat(m_Desc.RTVFormats[i]);
        }

        hr = m_Device.GetDevice()->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(&m_PipelineState));
    }
    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        return false;
    }
    return true;
}

void D3D12GraphicsPipeline::InitInputElements(std::vector<D3D12_INPUT_ELEMENT_DESC>& outInputElements) const
{
    if(m_Desc.VertexInputLayout && !m_Desc.VertexInputLayout->Items.empty())
    {
        outInputElements.resize(m_Desc.VertexInputLayout->Items.size());
        uint32_t alignedByteOffset = 0;
        for(uint32_t i = 0; i < m_Desc.VertexInputLayout->Items.size(); ++i)
        {
            const RHIVertexInputItem& item = m_Desc.VertexInputLayout->Items[i];
            outInputElements[i].Format = RHI::D3D12::ConvertFormat(item.Format);
            outInputElements[i].SemanticName = item.SemanticName;
            outInputElements[i].SemanticIndex = item.SemanticIndex;
            outInputElements[i].AlignedByteOffset = alignedByteOffset;
            outInputElements[i].InputSlot = 0;
            outInputElements[i].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            outInputElements[i].InstanceDataStepRate = 0;

            const RHIFormatInfo& formatInfo = RHI::GetFormatInfo(m_Desc.VertexInputLayout->Items[i].Format);
            alignedByteOffset += formatInfo.BytesPerBlock;
        }
    }
    else
    {
        outInputElements.clear();
    }
}

void D3D12GraphicsPipeline::Shutdown()
{
    ShutdownInternal();
}

bool D3D12GraphicsPipeline::IsValid() const
{
    return m_PipelineState.Get() != nullptr;
}

void D3D12GraphicsPipeline::ShutdownInternal()
{
    m_PipelineState.Reset();
}

void D3D12GraphicsPipeline::SetNameInternal()
{
    if(IsValid())
    {
        std::wstring name(m_Name.begin(), m_Name.end());
        m_PipelineState->SetName(name.c_str());
    }
}
