#include "D3D12PipelineState.h"
#include "D3D12Device.h"
#include "D3D12Resources.h"
#include "../../Core/Log.h"
#include "../../Core/Templates.h"

D3D12MeshPipeline::D3D12MeshPipeline(D3D12Device& inDevice, const RHIMeshPipelineDesc& inDesc)
    : m_Device(inDevice)
    , m_Desc(inDesc)
    , m_PipelineState(nullptr)
{
    
}

D3D12MeshPipeline::~D3D12MeshPipeline()
{
    ShutdownInternal();
}

bool D3D12MeshPipeline::Init()
{
    if(IsValid())
    {
        Log::Warning("[D3D12] The mesh pipeline already initialized");
        return true;
    }

    D3DX12_MESH_SHADER_PIPELINE_STATE_DESC pipelineStateDesc{};
    
    const D3D12PipelineBindingLayout* bindingLayout = CheckCast<D3D12PipelineBindingLayout*>(m_Desc.BindingLayout.get());
    if(bindingLayout != nullptr && bindingLayout->IsValid())
    {
        pipelineStateDesc.pRootSignature = bindingLayout->GetRootSignature();
    }
    else
    {
        pipelineStateDesc.pRootSignature = nullptr;
    }
    pipelineStateDesc.AS.pShaderBytecode = m_Desc.AmplificationShader->GetData();
    pipelineStateDesc.AS.BytecodeLength = m_Desc.AmplificationShader->GetSize();
    pipelineStateDesc.MS.pShaderBytecode = m_Desc.MeshShader->GetData();
    pipelineStateDesc.MS.BytecodeLength = m_Desc.MeshShader->GetSize();
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

    D3D12_PIPELINE_STATE_STREAM_DESC streamDesc{};
    streamDesc.SizeInBytes = sizeof(pipelineStateDesc);
    streamDesc.pPipelineStateSubobjectStream = &pipelineStateDesc;
    const HRESULT hr = m_Device.GetDevice()->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&m_PipelineState));
    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        return false;
    }
    return true;
}

void D3D12MeshPipeline::Shutdown()
{
    ShutdownInternal();
}

bool D3D12MeshPipeline::IsValid() const
{
    return m_PipelineState.Get() != nullptr;
}

void D3D12MeshPipeline::ShutdownInternal()
{
    m_PipelineState.Reset();
}

void D3D12MeshPipeline::SetNameInternal()
{
    if(IsValid())
    {
        std::wstring name(m_Name.begin(), m_Name.end());
        m_PipelineState->SetName(name.c_str());
    }
}