#pragma once
#include "VulkanDefinitions.h"
#include "VulkanResources.h"
#include "../RHISwapChain.h"

class VulkanSemaphore;
class VulkanSwapChain : public RHISwapChain
{
public:
    ~VulkanSwapChain() override;
    bool Init() override;
    void Shutdown() override;
    bool IsValid() const override;
    const RHISwapChainDesc& GetDesc() const override { return m_Desc; }
    void Present() override;
    void Resize(uint32_t inWidth, uint32_t inHeight) override;
    RHITexture* GetBackBuffer(uint32_t index) override;
    RHITexture* GetCurrentBackBuffer() override;
    uint32_t GetCurrentBackBufferIndex() override { return m_CurrentBackBufferIndex; }
    uint32_t GetBackBufferCount() override { return m_Desc.BufferCount; }
    VkSurfaceKHR GetSurface() const { return m_SurfaceHandle; }
    VkSwapchainKHR GetSwapChain() const { return m_SwapChainHandle; }
    
private:
    // friend VulkanDevice;
    friend RefCountPtr<RHISwapChain> RHI::CreateSwapChain(const RHISwapChainDesc& inDesc);
    VulkanSwapChain(VulkanDevice& inDevice, const RHISwapChainDesc& inDesc);
    void ShutdownInternal();
    bool CreateSwapChain();
    void DestroySwapChain();
    void AcquireNextImageIndex();
    
    VulkanDevice& m_Device;
    RHISwapChainDesc m_Desc;
    VkFormat m_SwapChainFormat;
    VkColorSpaceKHR m_SwapChainColorSpace;
    VkPresentModeKHR m_SwapChainPresentMode;
    VkSurfaceKHR m_SurfaceHandle;
    VkSwapchainKHR m_SwapChainHandle;
    std::vector<RefCountPtr<VulkanTexture>> m_BackBuffers;
    uint32_t m_CurrentBackBufferIndex;
    RefCountPtr<VulkanSemaphore> m_ImageAvailableSemaphore;
};