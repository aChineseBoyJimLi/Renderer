#pragma once

#include "../RHISwapChain.h"
#include "D3D12Definitions.h"
#include "D3D12Resources.h"


class D3D12SwapChain : public RHISwapChain
{
public:
    ~D3D12SwapChain() override;
    bool Init() override;
    void Shutdown() override;
    bool IsValid() const override;
    const RHISwapChainDesc& GetDesc() const override { return m_Desc; }
    void Present() override;
    void Resize(uint32_t inWidth, uint32_t inHeight) override;
    std::shared_ptr<RHITexture> GetBackBuffer(uint32_t index) override;
    std::shared_ptr<RHITexture> GetCurrentBackBuffer() override;
    uint32_t GetCurrentBackBufferIndex() override { return m_CurrentBackBufferIndex; }
    uint32_t GetBackBufferCount() override { return m_Desc.BufferCount; }
    IDXGISwapChain1* GetSwapChainHandle() const { return m_SwapChainHandle.Get(); }
    
private:
    friend std::shared_ptr<RHISwapChain> RHI::CreateSwapChain(const RHISwapChainDesc& inDesc);
    D3D12SwapChain(D3D12Device& inDevice, const RHISwapChainDesc& inDesc);
    void ShutdownInternal();
    void InitBackBuffers();
    
    D3D12Device& m_Device;
    RHISwapChainDesc m_Desc;
    DXGI_FORMAT m_SwapChainFormat;
    Microsoft::WRL::ComPtr<IDXGISwapChain1> m_SwapChainHandle;
    std::vector<std::shared_ptr<D3D12Texture>> m_BackBuffers;
    uint32_t m_CurrentBackBufferIndex;
};