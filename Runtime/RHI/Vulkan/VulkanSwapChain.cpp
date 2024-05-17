#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "../../Core/Log.h"

VulkanSwapChain::VulkanSwapChain(VulkanDevice& inDevice, const RHISwapChainDesc& inDesc)
    : m_Device(inDevice)
    , m_Desc(inDesc)
    , m_SwapChainFormat(VK_FORMAT_UNDEFINED)
    , m_SwapChainColorSpace(VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    , m_SwapChainPresentMode(VK_PRESENT_MODE_MAILBOX_KHR)
    , m_SurfaceHandle(VK_NULL_HANDLE)
    , m_SwapChainHandle(VK_NULL_HANDLE)
    , m_CurrentBackBufferIndex(0)
    , m_ImageAvailableSemaphore(nullptr)
{
    
}

VulkanSwapChain::~VulkanSwapChain()
{
    ShutdownInternal();
}

bool VulkanSwapChain::Init()
{
    if(IsValid())
    {
        Log::Warning("[Vulkan] SwapChain already initialized.");
        return true;
    }

    m_ImageAvailableSemaphore = m_Device.CreateVulkanSemaphore();
    if(!m_ImageAvailableSemaphore->IsValid())
    {
        return false;
    }

#if WIN32 || _WIN32
    if(!(m_Desc.hwnd && m_Desc.hInstance))
    {
        Log::Error("HWND or HINSTANCE is nullptr");
        return false;
    }

    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.hwnd = m_Desc.hwnd;
    surfaceCreateInfo.hinstance = m_Desc.hInstance;
    VkResult result = vkCreateWin32SurfaceKHR(m_Device.GetInstance(), &surfaceCreateInfo, nullptr, &m_SurfaceHandle);
    if(result != VK_SUCCESS)
    {
        OUTPUT_VULKAN_FAILED_RESULT(result)
        return false;
    }
#endif

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_Device.GetPhysicalDevice(), m_SurfaceHandle, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> availableSurfaceformats;
    if (formatCount > 0) {
        availableSurfaceformats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_Device.GetPhysicalDevice(), m_SurfaceHandle, &formatCount, availableSurfaceformats.data());
    }

    m_SwapChainFormat = RHI::Vulkan::ConvertFormat(m_Desc.Format);
    m_SwapChainColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    for (const auto& AvailableFormat : availableSurfaceformats) {
        if (AvailableFormat.format == m_SwapChainFormat) {
            m_SwapChainColorSpace = AvailableFormat.colorSpace;
            break;
        }
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(m_Device.GetPhysicalDevice(), m_SurfaceHandle, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> availablePresentModes;
    if (presentModeCount > 0) {
        availablePresentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(m_Device.GetPhysicalDevice(), m_SurfaceHandle, &presentModeCount, availablePresentModes.data());
    }
    m_SwapChainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto& availablePresentMode : availablePresentModes) {
        if(!m_Desc.IsVSync && availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
        {
            m_SwapChainPresentMode = availablePresentMode;
            break;
        }
    }
    
    if(!CreateSwapChain())
    {
        Log::Error("[Vulkan] Failed to create swap chain");
        return false;
    }

    AcquireNextImageIndex();
    
    return true;
}

bool VulkanSwapChain::CreateSwapChain()
{
    if(m_SurfaceHandle == VK_NULL_HANDLE)
    {
        Log::Error("[Vulkan] Window surface is null");
        return false;
    }

    m_BackBuffers.clear();
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_Device.GetPhysicalDevice(), m_SurfaceHandle, &surfaceCapabilities);
    if (surfaceCapabilities.maxImageCount > 0 && m_Desc.BufferCount > surfaceCapabilities.maxImageCount)
    {
        Log::Warning("Requested swap chain image count is greater than supported max image count %d", surfaceCapabilities.maxImageCount);
        m_Desc.BufferCount = surfaceCapabilities.maxImageCount;
    }
    if (surfaceCapabilities.minImageCount > 0 && m_Desc.BufferCount < surfaceCapabilities.minImageCount)
    {
        Log::Warning("Requested swap chain image count is less equal than supported min image count %d", surfaceCapabilities.minImageCount);
        m_Desc.BufferCount = surfaceCapabilities.minImageCount;
    }

    m_Desc.Width = surfaceCapabilities.currentExtent.width;
    m_Desc.Height = surfaceCapabilities.currentExtent.height;

    VkSwapchainCreateInfoKHR CreateInfo{};
    CreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    CreateInfo.surface = m_SurfaceHandle;
    CreateInfo.minImageCount = m_Desc.BufferCount;
    CreateInfo.imageFormat = m_SwapChainFormat;
    CreateInfo.imageColorSpace = m_SwapChainColorSpace;
    CreateInfo.imageExtent = surfaceCapabilities.currentExtent;
    CreateInfo.imageArrayLayers = 1;
    CreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    CreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    CreateInfo.preTransform = surfaceCapabilities.currentTransform;
    CreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    CreateInfo.presentMode = m_SwapChainPresentMode;
    CreateInfo.clipped = VK_TRUE;

    VkResult result = vkCreateSwapchainKHR(m_Device.GetDevice(), &CreateInfo, nullptr, &m_SwapChainHandle);
    if(result != VK_SUCCESS)
    {
        OUTPUT_VULKAN_FAILED_RESULT(result)
        return false;
    }
    uint32_t imageCount;
    vkGetSwapchainImagesKHR(m_Device.GetDevice(), m_SwapChainHandle, &imageCount, nullptr);
    if(imageCount != GetBackBufferCount())
    {
        Log::Error("[Vulkan] Requested swap chain image count does not match actual image count");
        m_Desc.BufferCount = imageCount;
    }
    std::vector<VkImage> swapChainImages;
    m_BackBuffers.resize(GetBackBufferCount());
    swapChainImages.resize(GetBackBufferCount());
    vkGetSwapchainImagesKHR(m_Device.GetDevice(), m_SwapChainHandle, &imageCount, swapChainImages.data());
    for (uint32_t i = 0; i < GetBackBufferCount(); i++)
    {
        RHITextureDesc backBufferDesc{};
        backBufferDesc.Width = m_Desc.Width;
        backBufferDesc.Height = m_Desc.Height;
        backBufferDesc.Format = m_Desc.Format;
        backBufferDesc.Usages = ERHITextureUsage::RenderTarget | ERHITextureUsage::ShaderResource;
        // backBufferDesc.InitialState = ERHIResourceStates::Present;
        backBufferDesc.SampleCount = m_Desc.SampleCount;
        m_BackBuffers[i] = m_Device.CreateTexture(backBufferDesc, swapChainImages[i]);
    }
    return true;
}

void VulkanSwapChain::Shutdown()
{
    ShutdownInternal();
}

void VulkanSwapChain::DestroySwapChain()
{
    m_BackBuffers.clear();
    if(m_SwapChainHandle != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(m_Device.GetDevice(), m_SwapChainHandle, nullptr);
        m_SwapChainHandle = VK_NULL_HANDLE;
    }
}

void VulkanSwapChain::ShutdownInternal()
{
    DestroySwapChain();
    if(m_SurfaceHandle != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(m_Device.GetInstance(), m_SurfaceHandle, nullptr);
        m_SurfaceHandle = VK_NULL_HANDLE;
    }

}

bool VulkanSwapChain::IsValid() const
{
    return m_SwapChainHandle != VK_NULL_HANDLE && m_SurfaceHandle != VK_NULL_HANDLE;
}

void VulkanSwapChain::Present()
{
    if(IsValid())
    {
        VkSemaphore semaphore = m_ImageAvailableSemaphore->GetSemaphore();
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.swapchainCount = 1;
        presentInfo.waitSemaphoreCount = 0;
        presentInfo.pWaitSemaphores = nullptr;
        presentInfo.pSwapchains = &m_SwapChainHandle;
        presentInfo.pImageIndices = &m_CurrentBackBufferIndex;

        vkQueuePresentKHR(m_Device.GetCommandQueue(ERHICommandQueueType::Direct), &presentInfo);

        AcquireNextImageIndex();
    }
    else
    {
        Log::Error("[Vulkan] Failed to present back buffer to screen, swapChain is not valid");
    }
}

void VulkanSwapChain::AcquireNextImageIndex()
{
    vkAcquireNextImageKHR(m_Device.GetDevice(), m_SwapChainHandle, UINT64_MAX, m_ImageAvailableSemaphore->GetSemaphore(), VK_NULL_HANDLE, &m_CurrentBackBufferIndex);
}

void VulkanSwapChain::Resize(uint32_t inWidth, uint32_t inHeight)
{
    DestroySwapChain();
    CreateSwapChain();
}

std::shared_ptr<RHITexture> VulkanSwapChain::GetBackBuffer(uint32_t index)
{
    if(index < m_BackBuffers.size())
    {
        return m_BackBuffers[index];
    }
    return nullptr;
}

std::shared_ptr<RHITexture> VulkanSwapChain::GetCurrentBackBuffer()
{
    return GetBackBuffer(m_CurrentBackBufferIndex);
}