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
    void BeginRenderPass(const std::shared_ptr<RHIFrameBuffer>& inFrameBuffer) override;
    void BeginRenderPass(const std::shared_ptr<RHIFrameBuffer>& inFrameBuffer
        , const RHIClearValue* inColor , uint32_t inNumRenderTargets) override;
    void BeginRenderPass(const std::shared_ptr<RHIFrameBuffer>& inFrameBuffer
        , const RHIClearValue* inColor , uint32_t inNumRenderTargets
        , float inDepth, uint8_t inStencil) override;
    void EndRenderPass() override;
    void ResourceBarrier(std::shared_ptr<RHITexture>& inResource , ERHIResourceStates inAfterState) override;
    
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
};