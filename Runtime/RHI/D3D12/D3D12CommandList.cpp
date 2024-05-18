#include "D3D12CommandList.h"
#include "D3D12Device.h"
#include "D3D12PipelineState.h"
#include "D3D12Resources.h"
#include "../../Core/Log.h"
#include "../../Core/Templates.h"

RefCountPtr<RHICommandList> D3D12Device::CreateCommandList(ERHICommandQueueType inType)
{
    RefCountPtr<RHICommandList> cmdList(new D3D12CommandList(*this, inType));
    if(!cmdList->Init())
    {
        Log::Error("[D3D12] Failed to create a command list");
    }
    return cmdList;
}

D3D12CommandList::D3D12CommandList(D3D12Device& inDevice, ERHICommandQueueType inType)
    : m_Device(inDevice)
    , m_QueueType(inType)
    , m_CmdAllocatorHandle(nullptr)
    , m_CmdListHandle(nullptr)
    , m_IsClosed(false)
{
    
}

D3D12CommandList::~D3D12CommandList()
{
    ShutdownInternal();
}

bool D3D12CommandList::Init()
{
    if(IsValid())
    {
        Log::Warning("[D3D12] Command list is already initialized");
        return true;
    }

    m_CachedBarriers.clear();
    m_StagingBuffers.clear();

    const D3D12_COMMAND_LIST_TYPE d3dQueueType = RHI::D3D12::ConvertCommandQueueType(m_QueueType);

    HRESULT hr = m_Device.GetDevice()->CreateCommandAllocator(d3dQueueType, IID_PPV_ARGS(&m_CmdAllocatorHandle));
    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        return false;
    }
    
    hr = m_Device.GetDevice()->CreateCommandList(D3D12Device::GetNodeMask(), d3dQueueType, m_CmdAllocatorHandle.Get(), nullptr, IID_PPV_ARGS(&m_CmdListHandle));
    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        return false;
    }

    return true;
}

void D3D12CommandList::Shutdown()
{
    ShutdownInternal();
}

bool D3D12CommandList::IsValid() const
{
    bool valid = m_CmdAllocatorHandle != nullptr && m_CmdListHandle != nullptr;
    return valid;
}

void D3D12CommandList::Begin()
{
    if(IsValid() && m_IsClosed)
    {
        m_CmdAllocatorHandle->Reset();
        m_CmdListHandle->Reset(m_CmdAllocatorHandle.Get(), nullptr);
        m_IsClosed = false;
    }
}

void D3D12CommandList::End()
{
    if(IsValid() && !m_IsClosed)
    {
        FlushBarriers();
        m_CmdListHandle->Close();
        m_IsClosed = true;
    }
}

void D3D12CommandList::SetPipelineState(const RefCountPtr<RHIGraphicsPipeline>& inPipelineState)
{
    if(IsValid() && !m_IsClosed)
    {
        FlushBarriers();
        const D3D12GraphicsPipeline* pipeline = CheckCast<D3D12GraphicsPipeline*>(inPipelineState.GetReference());
        const D3D12PipelineBindingLayout* layout = CheckCast<D3D12PipelineBindingLayout*>(pipeline->GetDesc().BindingLayout);
        if(pipeline && pipeline->IsValid())
        {
            const RHIGraphicsPipelineDesc& pipelineDesc = pipeline->GetDesc();
            m_CmdListHandle->SetPipelineState(pipeline->GetPipelineState());
            m_CmdListHandle->IASetPrimitiveTopology(RHI::D3D12::ConvertPrimitiveType(pipelineDesc.PrimitiveType));
        }
        if(layout && layout->IsValid())
        {
            m_CmdListHandle->SetGraphicsRootSignature(layout->GetRootSignature());
        }
    }
}

void D3D12CommandList::SetFrameBuffer(const RefCountPtr<RHIFrameBuffer>& inFrameBuffer)
{
    if(IsValid())
    {
         uint32_t numRenderTargets = inFrameBuffer->GetNumRenderTargets();
         std::vector<RHIClearValue> clearColors(numRenderTargets);
         for(uint32_t i = 0; i < numRenderTargets; i++)
         {
             clearColors[i] = inFrameBuffer->GetRenderTarget(i)->GetClearValue();
         }
         SetFrameBuffer(inFrameBuffer, clearColors.data(), numRenderTargets);
    }
}

void D3D12CommandList::SetFrameBuffer(const RefCountPtr<RHIFrameBuffer>& inFrameBuffer
    , const RHIClearValue* inColor , uint32_t inNumRenderTargets)
{
    if(IsValid() && !m_IsClosed)
    {
         float depth = 1;
         uint8_t stencil = 0;
         if(inFrameBuffer->HasDepthStencil())
         {
             const RHIClearValue& clearValue = inFrameBuffer->GetDepthStencil()->GetClearValue();
             depth = clearValue.DepthStencil.Depth;
             stencil = clearValue.DepthStencil.Stencil;
         }
         SetFrameBuffer(inFrameBuffer, inColor, inNumRenderTargets, depth, stencil);
    }
}

void D3D12CommandList::SetFrameBuffer(const RefCountPtr<RHIFrameBuffer>& inFrameBuffer
    , const RHIClearValue* inColor , uint32_t inNumRenderTargets
    , float inDepth, uint8_t inStencil)
{
    if (IsValid() && !m_IsClosed)
    {
        const D3D12FrameBuffer* frameBuffer = CheckCast<D3D12FrameBuffer*>(inFrameBuffer.GetReference());
        if(frameBuffer && frameBuffer->IsValid() && inNumRenderTargets == frameBuffer->GetNumRenderTargets())
        {
            m_CmdListHandle->OMSetRenderTargets(frameBuffer->GetNumRenderTargets()
                , frameBuffer->GetRenderTargetViews()
                , false
                , frameBuffer->GetDepthStencilViews());
            for(uint32_t i = 0; i < inNumRenderTargets; ++i)
            {
                m_CmdListHandle->ClearRenderTargetView(frameBuffer->GetRenderTargetView(i), inColor[i].Color, 0, nullptr);
            }
            if(frameBuffer->HasDepthStencil())
            {
                D3D12_CLEAR_FLAGS flags = D3D12_CLEAR_FLAG_DEPTH;
                const RHIFormatInfo& formatInfo = RHI::GetFormatInfo(frameBuffer->GetDepthStencilFormat());
                if(formatInfo.HasStencil) flags |= D3D12_CLEAR_FLAG_STENCIL;
                m_CmdListHandle->ClearDepthStencilView(frameBuffer->GetDepthStencilView(), flags, inDepth, inStencil, 0, nullptr);
            }
        }
    }
}

void D3D12CommandList::SetViewports(const std::vector<RHIViewport>& inViewports)
{
    if(IsValid() && !IsClosed() && !inViewports.empty())
    {
        std::vector<D3D12_VIEWPORT> viewports(inViewports.size());
        for(uint32_t i = 0; i < inViewports.size(); ++i)
        {
            viewports[i].TopLeftX = inViewports[i].X;
            viewports[i].TopLeftY = inViewports[i].Y;
            viewports[i].Width = inViewports[i].Width;
            viewports[i].Height = inViewports[i].Height;
            viewports[i].MinDepth = inViewports[i].MinDepth;
            viewports[i].MaxDepth = inViewports[i].MaxDepth;
        }
        m_CmdListHandle->RSSetViewports((uint32_t) inViewports.size(), viewports.data());
    }
}

void D3D12CommandList::SetScissorRects(const std::vector<RHIRect>& inRects)
{
    if(IsValid() && !IsClosed() && !inRects.empty())
    {
        std::vector<D3D12_RECT> scissorRects(inRects.size());
        for(uint32_t i = 0; i < inRects.size(); ++i)
        {
            scissorRects[i].left = inRects[i].Left();
            scissorRects[i].top = inRects[i].Top();
            scissorRects[i].right = inRects[i].Right();
            scissorRects[i].bottom = inRects[i].Bottom();
        }
        m_CmdListHandle->RSSetScissorRects((uint32_t) inRects.size(), scissorRects.data());
    }
}

// void D3D12CommandList::UploadBuffer(RefCountPtr<RHIBuffer>& dstBuffer, const void* inData, size_t size, size_t dstOffset)
// {
//     RefCountPtr<RHIBuffer> stagingBuffer = AcquireStagingBuffer(size);
//     stagingBuffer->WriteData(inData, size);
//     CopyBuffer(dstBuffer, dstOffset, stagingBuffer, 0, size);
// }

void D3D12CommandList::CopyBuffer(RefCountPtr<RHIBuffer>& dstBuffer, size_t dstOffset, RefCountPtr<RHIBuffer>& srcBuffer, size_t srcOffset, size_t size)
{
    if(IsValid() && !IsClosed())
    {
        D3D12Buffer* buffer0 = CheckCast<D3D12Buffer*>(dstBuffer.GetReference());
        D3D12Buffer* buffer1 = CheckCast<D3D12Buffer*>(srcBuffer.GetReference());
    
        ID3D12Resource* dstRes = buffer0->GetBuffer();
        ID3D12Resource* srcRes = buffer1->GetBuffer();
        m_CmdListHandle->CopyBufferRegion(dstRes, dstOffset, srcRes, srcOffset, size);
    }
}

void D3D12CommandList::SetVertexBuffer(const RefCountPtr<RHIBuffer>& inBuffer)
{
    if(IsValid() && !IsClosed())
    {
        const RHIBufferDesc& bufferDesc = inBuffer->GetDesc();
        D3D12_VERTEX_BUFFER_VIEW vbv{};
        vbv.BufferLocation = inBuffer->GetGpuAddress();
        vbv.StrideInBytes = (uint32_t)bufferDesc.Stride;
        vbv.SizeInBytes = (uint32_t)bufferDesc.Size;
        m_CmdListHandle->IASetVertexBuffers(0, 1, &vbv);
    }
}

void D3D12CommandList::SetIndexBuffer(const RefCountPtr<RHIBuffer>& inBuffer)
{
    if(IsValid() && !IsClosed())
    {
        const RHIBufferDesc& bufferDesc = inBuffer->GetDesc();
        D3D12_INDEX_BUFFER_VIEW ibv{};
        ibv.SizeInBytes = (uint32_t)bufferDesc.Size;
        ibv.BufferLocation = inBuffer->GetGpuAddress();
        ibv.Format = RHI::D3D12::ConvertFormat(bufferDesc.Format);
        m_CmdListHandle->IASetIndexBuffer(&ibv);
    }
}

// RefCountPtr<RHIBuffer> D3D12CommandList::AcquireStagingBuffer(size_t inSize)
// {
//     RHIBufferDesc desc;
//     desc.Size = inSize;
//     desc.CpuAccess = ERHICpuAccessMode::Write;
//     RefCountPtr<RHIBuffer> stagingBuffer = m_Device.CreateBuffer(desc);
//     m_StagingBuffers.push_back(stagingBuffer);
//     return stagingBuffer;
// }

void D3D12CommandList::ResourceBarrier(RefCountPtr<RHITexture>& inResource , ERHIResourceStates inAfterState)
{
    if(IsValid() && !IsClosed())
    {
        D3D12Texture* texture = CheckCast<D3D12Texture*>(inResource.GetReference());
        if(texture && texture->IsValid())
        {
            D3D12_RESOURCE_STATES beforeState = texture->GetCurrentState();
            D3D12_RESOURCE_STATES afterState = RHI::D3D12::ConvertResourceStates(inAfterState);
            m_CachedBarriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(texture->GetTexture(), beforeState, afterState));
            texture->ChangeState(afterState);
        }
    }
}

void D3D12CommandList::FlushBarriers()
{
    if(IsValid() && !m_CachedBarriers.empty())
    {
        m_CmdListHandle->ResourceBarrier((uint32_t)m_CachedBarriers.size(), m_CachedBarriers.data());
        m_CachedBarriers.clear();
    }
}

void D3D12CommandList::ShutdownInternal()
{
    m_CmdAllocatorHandle.Reset();
    m_CmdListHandle.Reset();
}

void D3D12CommandList::SetNameInternal()
{
    if(IsValid())
    {
        std::wstring wname(m_Name.begin(), m_Name.end());
        m_CmdListHandle->SetName(wname.c_str());
    }
}
