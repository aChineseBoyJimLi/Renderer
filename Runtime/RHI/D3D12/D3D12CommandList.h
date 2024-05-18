#pragma once

#include "../RHICommandList.h"
#include "D3D12Definitions.h"

class D3D12CommandList : public RHICommandList
{
public:
    ~D3D12CommandList() override;
    bool Init() override;
    void Shutdown() override;
    bool IsValid() const override;
    void Begin() override;
    void End() override;
    void SetPipelineState(const RefCountPtr<RHIGraphicsPipeline>& inPipelineState) override;
    void SetFrameBuffer(const RefCountPtr<RHIFrameBuffer>& inFrameBuffer) override;
    void SetFrameBuffer(const RefCountPtr<RHIFrameBuffer>& inFrameBuffer
        , const RHIClearValue* inColor , uint32_t inNumRenderTargets) override;
    void SetFrameBuffer(const RefCountPtr<RHIFrameBuffer>& inFrameBuffer
        , const RHIClearValue* inColor , uint32_t inNumRenderTargets
        , float inDepth, uint8_t inStencil) override;
    void SetViewports(const std::vector<RHIViewport>& inViewports) override;
    void SetScissorRects(const std::vector<RHIRect>& inRects) override;

    void SetVertexBuffer(const RefCountPtr<RHIBuffer>& inBuffer) override;
    void SetIndexBuffer(const RefCountPtr<RHIBuffer>& inBuffer) override;
    
    void ResourceBarrier(RefCountPtr<RHITexture>& inResource , ERHIResourceStates inAfterState) override;
    void CopyBuffer(RefCountPtr<RHIBuffer>& dstBuffer, size_t dstOffset, RefCountPtr<RHIBuffer>& srcBuffer, size_t srcOffset, size_t size) override;
    
    bool IsClosed() const override { return m_IsClosed; }
    ERHICommandQueueType GetQueueType() const override { return m_QueueType; }
    ID3D12GraphicsCommandList6* GetCommandList() const { return m_CmdListHandle.Get(); }
    
protected:
    void SetNameInternal() override;

private:
    friend class D3D12Device;
    D3D12CommandList(D3D12Device& inDevice, ERHICommandQueueType inType);
    void ShutdownInternal();
    void FlushBarriers();

    
    D3D12Device& m_Device;
    const ERHICommandQueueType m_QueueType;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_CmdAllocatorHandle;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> m_CmdListHandle;
    bool m_IsClosed;
    std::vector<CD3DX12_RESOURCE_BARRIER> m_CachedBarriers;
    std::vector<RefCountPtr<RHIBuffer>> m_StagingBuffers;
};