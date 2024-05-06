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
    
    bool IsClosed() const override { return m_IsClosed; }
    ERHICommandQueueType GetQueueType() const override { return m_QueueType; }
    ID3D12GraphicsCommandList6* GetCommandList() const { return m_CmdListHandle.Get(); }
    
protected:
    void SetNameInternal() override;

private:
    friend class D3D12Device;
    D3D12CommandList(D3D12Device& inDevice, ERHICommandQueueType inType);
    void ShutdownInternal();

    D3D12Device& m_Device;
    const ERHICommandQueueType m_QueueType;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_CmdAllocatorHandle;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> m_CmdListHandle;
    bool m_IsClosed;
};