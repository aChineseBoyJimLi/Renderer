#include "VulkanResources.h"
#include "VulkanDevice.h"
#include "../../Core/Log.h"
#include "../../Core/Templates.h"

RefCountPtr<RHITexture> VulkanDevice::CreateTexture(const RHITextureDesc& inDesc, bool isVirtual)
{
    RefCountPtr<RHITexture> texture(new VulkanTexture(*this, inDesc, isVirtual));
    if(!texture->Init())
    {
        Log::Error("[Vulkan] Failed to create texture");
    }
    return texture;
}

RefCountPtr<VulkanTexture> VulkanDevice::CreateTexture(const RHITextureDesc& inDesc, VkImage inImage)
{
    RefCountPtr<VulkanTexture> texture(new VulkanTexture(*this, inDesc, inImage));
    return texture;
}

VulkanTexture::VulkanTexture(VulkanDevice& inDevice, const RHITextureDesc& inDesc, bool inIsVirtual)
    : IsVirtualTexture(inIsVirtual)
    , IsManagedTexture(true)
    , m_Device(inDevice)
    , m_Desc(inDesc)
    , m_TextureHandle(VK_NULL_HANDLE)
    , m_InitialAccessFlags(VK_ACCESS_NONE)
    , m_InitialLayout(VK_IMAGE_LAYOUT_UNDEFINED)
    , m_MemRequirements{inDesc.Width * inDesc.Height * inDesc.Depth * 4, 0, UINT32_MAX}
    , m_ResourceHeap(nullptr)
    , m_OffsetInHeap(0)
{
    
}

VulkanTexture::VulkanTexture(VulkanDevice& inDevice, const RHITextureDesc& inDesc, VkImage inImage)
    : IsVirtualTexture(false)
    , IsManagedTexture(false)
    , m_Device(inDevice)
    , m_Desc(inDesc)
    , m_TextureHandle(inImage)
    , m_InitialAccessFlags(VK_ACCESS_NONE)
    , m_InitialLayout(VK_IMAGE_LAYOUT_UNDEFINED)
    , m_MemRequirements{inDesc.Width * inDesc.Height * inDesc.Depth * 4, 0, UINT32_MAX}
    , m_ResourceHeap(nullptr)
    , m_OffsetInHeap(0)
{
    
}

VulkanTexture::~VulkanTexture()
{
    ShutdownInternal();
}

bool VulkanTexture::Init()
{
    if(!IsManaged())
    {
        Log::Warning("[Vulkan] Texture is not managed, initialization is not allowed.");
        return true;
    }

    if(IsValid())
    {
        Log::Warning("[Vulkan] Texture already initialized.");
        return true;
    }
    
    // m_InitialAccessFlags = RHI::Vulkan::ConvertAccessFlags(m_Desc.InitialState);
    // m_InitialLayout = RHI::Vulkan::ConvertImageLayout(m_Desc.InitialState);

    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.extent.width = m_Desc.Width;
    imageCreateInfo.extent.height = m_Desc.Height;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.arrayLayers = 1;
    switch (m_Desc.Dimension)
    {
    case ERHITextureDimension::Texture2D:
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        break;
    case ERHITextureDimension::Texture3D:
        imageCreateInfo.imageType = VK_IMAGE_TYPE_3D;
        imageCreateInfo.extent.depth = m_Desc.Depth;
        break;
    case ERHITextureDimension::Texture2DArray:
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.arrayLayers = m_Desc.ArraySize;
        imageCreateInfo.flags = VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT;
        break;
    case ERHITextureDimension::TextureCube:
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.arrayLayers = 6;
        imageCreateInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        break;
    }
    imageCreateInfo.mipLevels = m_Desc.MipLevels;
    imageCreateInfo.format = RHI::Vulkan::ConvertFormat(m_Desc.Format);
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    // VkImage initialLayout must be VK_IMAGE_LAYOUT_UNDEFINED or VK_IMAGE_LAYOUT_PREINITIALIZED
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VkImageUsageFlags usages = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    if((m_Desc.Usages & ERHITextureUsage::RenderTarget) == ERHITextureUsage::RenderTarget) usages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if((m_Desc.Usages & ERHITextureUsage::DepthStencil) == ERHITextureUsage::DepthStencil) usages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    if((m_Desc.Usages & ERHITextureUsage::ShaderResource) == ERHITextureUsage::ShaderResource) usages |= VK_IMAGE_USAGE_SAMPLED_BIT;
    if((m_Desc.Usages & ERHITextureUsage::UnorderedAccess) == ERHITextureUsage::UnorderedAccess) usages |= VK_IMAGE_USAGE_STORAGE_BIT;

    if(m_Device.SupportVariableRateShading())
    {
        if((m_Desc.Usages & ERHITextureUsage::ShadingRateSource) == ERHITextureUsage::ShadingRateSource)
            usages |= VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
    }
    
    imageCreateInfo.usage = usages;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.samples = RHI::Vulkan::ConvertSampleBits(m_Desc.SampleCount);
    imageCreateInfo.pNext = nullptr;

    VkResult result = vkCreateImage(m_Device.GetDevice(), &imageCreateInfo, nullptr, &m_TextureHandle);
    if(result != VK_SUCCESS)
    {
        OUTPUT_VULKAN_FAILED_RESULT(result);
        return false;
    }

    vkGetImageMemoryRequirements(m_Device.GetDevice(), m_TextureHandle, &m_MemRequirements);
    m_ResourceHeap = nullptr;

    if(!IsVirtual() && IsManaged())
    {
        RHIResourceHeapDesc heapDesc{};
        heapDesc.Size = GetSizeInByte();
        heapDesc.Alignment = GetAlignment();
        heapDesc.TypeFilter = GetMemTypeFilter();
        heapDesc.Usage = ERHIHeapUsage::Texture;
        heapDesc.Type = ERHIResourceHeapType::DeviceLocal;

        m_ResourceHeap = m_Device.CreateResourceHeap(heapDesc);
        if(!m_ResourceHeap->IsValid())
        {
            Log::Error("[Vulkan] Failed to create resource heap for texture");
            return false;
        }
        VulkanResourceHeap* heap = CheckCast<VulkanResourceHeap*>(m_ResourceHeap.GetReference());
        result = vkBindImageMemory(m_Device.GetDevice(), m_TextureHandle, heap->GetHeap(), 0);
        if(result != VK_SUCCESS)
        {
            OUTPUT_VULKAN_FAILED_RESULT(result)
            return false;
        }
    }
    
    return true;
}

bool VulkanTexture::BindMemory(RefCountPtr<RHIResourceHeap> inHeap)
{
    if(!IsManaged())
    {
        Log::Warning("[Vulkan] Texture is not managed, memory binding is not allowed.");
        return true;    
    }
    
    if(!IsVirtual())
    {
        Log::Warning("[Vulkan] Texture is not virtual, memory binding is not allowed.");
        return true;
    }

    if(m_ResourceHeap != nullptr)
    {
        Log::Warning("[Vulkan] Texture already bound to a heap.");
        return true;
    }

    VulkanResourceHeap* heap = CheckCast<VulkanResourceHeap*>(inHeap.GetReference());
    if(heap == nullptr || !heap->IsValid())
    {
        Log::Error("[Vulkan] Failed to bind texture memory, the heap is invalid");
        return false;
    }

    const RHIResourceHeapDesc& heapDesc = heap->GetDesc();

    if(heapDesc.Usage != ERHIHeapUsage::Texture)
    {
        Log::Error("[Vulkan] Failed to bind texture memory, the heap is not a texture heap");
        return false;
    }

    if(!inHeap->TryAllocate(GetSizeInByte(), m_OffsetInHeap))
    {
        Log::Error("[Vulkan] Failed to bind texture memory, the heap size is not enough");
        return false;
    }

    VkResult result = vkBindImageMemory(m_Device.GetDevice(), m_TextureHandle, heap->GetHeap(), m_OffsetInHeap);
    if(result != VK_SUCCESS)
    {
        OUTPUT_VULKAN_FAILED_RESULT(result)
        inHeap->Free(m_OffsetInHeap, GetSizeInByte());
        m_OffsetInHeap = 0;
        return false;
    }

    m_ResourceHeap = inHeap;
    return true;
}

void VulkanTexture::Shutdown()
{
    ShutdownInternal();
}

void VulkanTexture::ShutdownInternal()
{
    for(auto rtv : m_RenderTargetViews)
    {
        if(rtv.second != VK_NULL_HANDLE)
        {
            vkDestroyImageView(m_Device.GetDevice(), rtv.second, nullptr);
        }
    }
    for(auto dsv : m_DepthStencilViews)
    {
        if(dsv.second != VK_NULL_HANDLE)
        {
            vkDestroyImageView(m_Device.GetDevice(), dsv.second, nullptr);
        }
    }
    for(auto srv : m_ShaderResourceViews)
    {
        if(srv.second != VK_NULL_HANDLE)
        {
            vkDestroyImageView(m_Device.GetDevice(), srv.second, nullptr);
        }
    }
    for(auto uav : m_UnorderedAccessViews)
    {
        if(uav.second != VK_NULL_HANDLE)
        {
            vkDestroyImageView(m_Device.GetDevice(), uav.second, nullptr);
        }
    }
    m_RenderTargetViews.clear();
    m_DepthStencilViews.clear();
    m_ShaderResourceViews.clear();
    m_UnorderedAccessViews.clear();
    
    if(m_ResourceHeap != nullptr)
    {
        m_ResourceHeap->Free(m_OffsetInHeap, GetSizeInByte());
        m_ResourceHeap.SafeRelease();
        m_OffsetInHeap = 0;
    }
    
    if(IsManaged())
    {
        if(m_TextureHandle != VK_NULL_HANDLE)
        {
            vkDestroyImage(m_Device.GetDevice(), m_TextureHandle, nullptr);
        }
    }
    m_TextureHandle = VK_NULL_HANDLE;
}

bool VulkanTexture::IsValid() const
{
    bool valid = m_TextureHandle != VK_NULL_HANDLE;
    if(IsManaged())
    {
        valid = valid && m_ResourceHeap != nullptr && m_ResourceHeap->IsValid();
    }
    
    return valid;
}

void VulkanTexture::SetNameInternal()
{
    if(IsValid())
    {
        m_Device.SetDebugName(VK_OBJECT_TYPE_IMAGE, reinterpret_cast<uint64_t>(m_TextureHandle), m_Name);
    }
}

bool VulkanTexture::CreateImageView(VkImageView& outImageView, const RHITextureSubResource& inSubResource, bool depth, bool stencil) const
{
    uint32_t firstMipLevel = inSubResource.FirstMipSlice;
    uint32_t mipLevels = inSubResource.NumMipSlices;
    uint32_t firstArraySlice = inSubResource.FirstArraySlice;
    uint32_t arraySize = inSubResource.NumArraySlices;
    if(inSubResource == RHITextureSubResource::All)
    {
        firstMipLevel = 0;
        mipLevels = m_Desc.MipLevels;
        firstArraySlice = 0;
        arraySize = m_Desc.ArraySize;
    }

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_TextureHandle;
    viewInfo.format = RHI::Vulkan::ConvertFormat(m_Desc.Format);
    viewInfo.subresourceRange.baseMipLevel = firstMipLevel;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    if(depth)
    {
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if(stencil) viewInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    else
    {
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    
    switch(m_Desc.Dimension)
    {
    case ERHITextureDimension::Texture2D:
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        break;
    case ERHITextureDimension::Texture3D:
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_3D;
    case ERHITextureDimension::Texture2DArray:
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        viewInfo.subresourceRange.baseArrayLayer = firstArraySlice;
        viewInfo.subresourceRange.layerCount = arraySize;
        break;
    case ERHITextureDimension::TextureCube:
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 6;
        break;
    }

    VkResult result = vkCreateImageView(m_Device.GetDevice(), &viewInfo, nullptr, &outImageView);
    if(result != VK_SUCCESS)
    {
        OUTPUT_VULKAN_FAILED_RESULT(result)
        return false;
    }

    return true;
}

bool VulkanTexture::CreateRTV(VkImageView& outHandle, const RHITextureSubResource& inSubResource)
{
    if(!IsValid())
    {
        Log::Error("[Vulkan] Texture is not valid.");
        return false;
    }

    if(TryGetRTVHandle(outHandle, inSubResource))
    {
        Log::Warning("[Vulkan] Render target view already created on this subresource.");
        return true;
    }

    if((m_Desc.Usages & ERHITextureUsage::RenderTarget) == 0)
    {
        Log::Error("[Vulkan] Texture is not allowed to create render target view.");
        return false;
    }
    
    if(!CreateImageView(outHandle, inSubResource))
    {
        Log::Error("[Vulkan] Failed to create image view for render target.");
        return false;
    }

    m_RenderTargetViews.emplace(inSubResource, outHandle);
    
    return true;
}

bool VulkanTexture::CreateDSV(VkImageView& outHandle, const RHITextureSubResource& inSubResource)
{
    if(!IsValid())
    {
        Log::Error("[Vulkan] Texture is not valid.");
        return false;
    }

    if(TryGetDSVHandle(outHandle, inSubResource))
    {
        Log::Warning("[Vulkan] Depth stencil view already created on this subresource.");
        return true;
    }

    if((m_Desc.Usages & ERHITextureUsage::DepthStencil) == 0)
    {
        Log::Error("[Vulkan] Texture is not allowed to create depth stencil view.");
        return false;
    }

    const RHIFormatInfo formatInfo = RHI::GetFormatInfo(m_Desc.Format);
    if(!CreateImageView(outHandle, inSubResource, formatInfo.HasDepth, formatInfo.HasStencil))
    {
        Log::Error("[Vulkan] Failed to create image view for depth stencil.");
        return false;
    }

    m_DepthStencilViews.emplace(inSubResource, outHandle);
    
    return true;
}

bool VulkanTexture::CreateSRV(VkImageView& outHandle, const RHITextureSubResource& inSubResource)
{
    if(!IsValid())
    {
        Log::Error("[Vulkan] Texture is not valid.");
        return false;
    }

    if(TryGetSRVHandle(outHandle, inSubResource))
    {
        Log::Warning("[Vulkan] Shader resource view already created on this subresource.");
        return true;
    }

    if((m_Desc.Usages & ERHITextureUsage::ShaderResource) == 0)
    {
        Log::Error("[Vulkan] Texture is not allowed to create shader resource view.");
        return false;
    }
    
    if(!CreateImageView(outHandle, inSubResource))
    {
        Log::Error("[Vulkan] Failed to create image view for shader resource.");
        return false;
    }

    m_ShaderResourceViews.emplace(inSubResource, outHandle);
    
    return true;
}

bool VulkanTexture::CreateUAV(VkImageView& outHandle, const RHITextureSubResource& inSubResource)
{
    if(!IsValid())
    {
        Log::Error("[Vulkan] Texture is not valid.");
        return false;
    }

    if(TryGetUAVHandle(outHandle, inSubResource))
    {
        Log::Warning("[Vulkan] Unordered access view already created on this subresource.");
        return true;
    }

    if((m_Desc.Usages & ERHITextureUsage::UnorderedAccess) == 0)
    {
        Log::Error("[Vulkan] Texture is not allowed to create unordered access view.");
        return false;
    }
    
    if(!CreateImageView(outHandle, inSubResource))
    {
        Log::Error("[Vulkan] Failed to create image view for unordered access.");
        return false;
    }

    m_UnorderedAccessViews.emplace(inSubResource, outHandle);
    
    return true;
}

bool VulkanTexture::TryGetRTVHandle(VkImageView& outHandle, const RHITextureSubResource& inSubResource)
{
    if(m_RenderTargetViews.find(inSubResource) == m_RenderTargetViews.end())
    {
        return false;
    }
    outHandle = m_RenderTargetViews[inSubResource];
    return true;
}

bool VulkanTexture::TryGetDSVHandle(VkImageView& outHandle, const RHITextureSubResource& inSubResource)
{
    if(m_DepthStencilViews.find(inSubResource) == m_DepthStencilViews.end())
    {
        return false;
    }
    outHandle = m_DepthStencilViews[inSubResource];
    return true;
}

bool VulkanTexture::TryGetSRVHandle(VkImageView& outHandle, const RHITextureSubResource& inSubResource)
{
    if(m_ShaderResourceViews.find(inSubResource) == m_ShaderResourceViews.end())
    {
        return false;
    }
    outHandle = m_ShaderResourceViews[inSubResource];
    return true;
}

bool VulkanTexture::TryGetUAVHandle(VkImageView& outHandle, const RHITextureSubResource& inSubResource)
{
    if(m_UnorderedAccessViews.find(inSubResource) == m_UnorderedAccessViews.end())
    {
        return false;
    }
    outHandle = m_UnorderedAccessViews[inSubResource];
    return true;
}

VulkanTextureState VulkanTexture::GetCurrentState(const RHITextureSubResource& inSubResource)
{
    auto currentStateIter = m_SubResourceStates.find(inSubResource);
    if(currentStateIter == m_SubResourceStates.end())
    {
        auto allSubResourceState = m_SubResourceStates.find(RHITextureSubResource::All);
        if(allSubResourceState == m_SubResourceStates.end())
        {
            VulkanTextureState initialState(m_InitialAccessFlags, m_InitialLayout);
            m_SubResourceStates.emplace(inSubResource, initialState);
            if(inSubResource != RHITextureSubResource::All)
                m_SubResourceStates.emplace(inSubResource, initialState);
            return initialState;
        }
        m_SubResourceStates.emplace(inSubResource, allSubResourceState->second);
        return allSubResourceState->second;
    }
    return currentStateIter->second;
}

void VulkanTexture::ChangeState(const VulkanTextureState& inAfterState, const RHITextureSubResource& inSubResource)
{
    auto currentStateIter = m_SubResourceStates.find(inSubResource);
    if(currentStateIter == m_SubResourceStates.end())
    {
        m_SubResourceStates.emplace(inSubResource, inAfterState);
    }
    else
    {
        currentStateIter->second = inAfterState;
    }
}