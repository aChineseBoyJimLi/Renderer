#include "D3D12PipelineState.h"
#include "D3D12Device.h"
#include "D3D12Resources.h"
#include "../../Core/Log.h"
#include "../../Core/Templates.h"

D3D12VertexInputLayout::D3D12VertexInputLayout(const RHIVertexInputLayoutDesc& inDesc)
    : m_Desc(inDesc)
{
    m_D3DInputElements.resize(m_Desc.Items.size());
    uint32_t alignedByteOffset = 0;
    for(uint32_t i = 0; i < m_Desc.Items.size(); ++i)
    {
        const RHIVertexInputItem& item = m_Desc.Items[i];
        m_D3DInputElements[i].Format = RHI::D3D12::ConvertFormat(item.Format);
        m_D3DInputElements[i].SemanticName = item.SemanticName;
        m_D3DInputElements[i].SemanticIndex = item.SemanticIndex;
        m_D3DInputElements[i].AlignedByteOffset = alignedByteOffset;
        m_D3DInputElements[i].InputSlot = 0;
        m_D3DInputElements[i].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        m_D3DInputElements[i].InstanceDataStepRate = 0;

        const RHIFormatInfo& formatInfo = RHI::GetFormatInfo(m_Desc.Items[i].Format);
        alignedByteOffset += formatInfo.BlockSize;
    }
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
    
    D3D12PipelineBindingLayout* bindingLayout = CheckCast<D3D12PipelineBindingLayout*>(m_Desc.BindingLayout.get());
    if(bindingLayout == nullptr || !bindingLayout->IsValid())
    {
        Log::Error("[D3D12] The graphics pipeline binding layout is invalid");
        return false;
    }

    D3D12VertexInputLayout* inputLayout = CheckCast<D3D12VertexInputLayout*>(m_Desc.VertexInputLayout.get());
    if(inputLayout == nullptr)
    {
        Log::Error("[D3D12] The graphics pipeline vertex input layout is invalid");
        return false;
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc{};

    pipelineStateDesc.pRootSignature = bindingLayout->GetRootSignature();
    pipelineStateDesc.InputLayout.pInputElementDescs = inputLayout->GetInputElements();
    pipelineStateDesc.InputLayout.NumElements = inputLayout->GetElementsCount();
    pipelineStateDesc.NodeMask = D3D12Device::GetNodeMask();
    pipelineStateDesc.VS.pShaderBytecode = m_Desc.VertexShader->GetData();
    pipelineStateDesc.VS.BytecodeLength = m_Desc.VertexShader->GetSize();
    pipelineStateDesc.HS.pShaderBytecode = m_Desc.HullShader->GetData();
    pipelineStateDesc.HS.BytecodeLength = m_Desc.HullShader->GetSize();
    pipelineStateDesc.DS.pShaderBytecode = m_Desc.DomainShader->GetData();
    pipelineStateDesc.DS.BytecodeLength = m_Desc.DomainShader->GetSize();
    pipelineStateDesc.GS.pShaderBytecode = m_Desc.GeometryShader->GetData();
    pipelineStateDesc.GS.BytecodeLength = m_Desc.GeometryShader->GetSize();
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

    if(m_Desc.FrameBuffer == nullptr || !m_Desc.FrameBuffer->IsValid())
    {
        Log::Error("[D3D12] The graphics pipeline frame buffer is invalid");
        return false;
    }
    pipelineStateDesc.DSVFormat = RHI::D3D12::ConvertFormat(m_Desc.FrameBuffer->GetDepthStencilFormat());
    pipelineStateDesc.NumRenderTargets = m_Desc.FrameBuffer->GetNumRenderTargets();
    for(uint32_t i = 0; i < pipelineStateDesc.NumRenderTargets; i++)
    {
        pipelineStateDesc.RTVFormats[i] = RHI::D3D12::ConvertFormat(m_Desc.FrameBuffer->GetRenderTargetFormat(i));
    }
    
    const HRESULT hr = m_Device.GetDevice()->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(&m_PipelineState));
    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        return false;
    }
    return true;
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
