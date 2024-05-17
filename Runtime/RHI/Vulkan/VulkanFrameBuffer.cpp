#include "VulkanResources.h"
#include "VulkanDevice.h"
#include "../../Core/Log.h"
#include "../../Core/Templates.h"

std::shared_ptr<RHIFrameBuffer> VulkanDevice::CreateFrameBuffer(const RHIFrameBufferDesc& inDesc)
{
    std::shared_ptr<RHIFrameBuffer> frameBuffer(new VulkanFrameBuffer(*this, inDesc));
    if(!frameBuffer->Init())
    {
        Log::Error("[Vulkan] Failed to create frame buffer");
    }
    return frameBuffer;
}

VulkanFrameBuffer::VulkanFrameBuffer(VulkanDevice& inDevice, const RHIFrameBufferDesc& inDesc)
    : m_Device(inDevice)
    , m_Desc(inDesc)
    , m_AttachmentCount(0)
    , m_RenderPassHandle(VK_NULL_HANDLE)
    , m_FrameBufferHandle(VK_NULL_HANDLE)
{
    for(uint32_t i = 0; i < RHIRenderTargetsMaxCount; ++i)
    {
        m_RenderTargets[i] = nullptr;
        m_RTVHandles[i] = VK_NULL_HANDLE;
    }
    m_DepthStencil = nullptr;
    m_DSVHandle = VK_NULL_HANDLE;
}

VulkanFrameBuffer::~VulkanFrameBuffer()
{
    ShutdownInternal();
}

bool VulkanFrameBuffer::Init()
{
    if(IsValid())
    {
        Log::Warning("[Vulkan] FrameBuffer already initialized.");
        return true;
    }

    const bool hasDepthStencil = HasDepthStencil();
    const uint32_t renderTargetsCount = GetNumRenderTargets();
    m_AttachmentCount = hasDepthStencil ? renderTargetsCount + 1 : renderTargetsCount;

    std::vector<VkAttachmentDescription> attachmentDescs(m_AttachmentCount);
    std::vector<VkAttachmentReference> colorAttachmentRefs(renderTargetsCount);

    for(uint32_t i = 0; i < renderTargetsCount; i++)
    {
        const RHITextureDesc& textureDesc = m_Desc.RenderTargets[i]->GetDesc();
        attachmentDescs[i].samples = RHI::Vulkan::ConvertSampleBits(textureDesc.SampleCount);
        attachmentDescs[i].format = RHI::Vulkan::ConvertFormat(textureDesc.Format);
        attachmentDescs[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        // attachmentDescs[i].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachmentDescs[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentDescs[i].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachmentDescs[i].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        
        colorAttachmentRefs[i].attachment = i;
        colorAttachmentRefs[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    VkAttachmentReference depthStencilAttachmentRef{};
    if(hasDepthStencil)
    {
        const RHITextureDesc& textureDesc = m_Desc.DepthStencil->GetDesc();
        const RHIFormatInfo& formatInfo = RHI::GetFormatInfo(textureDesc.Format);
        const uint32_t depthStencilAttachmentIndex = renderTargetsCount;
        attachmentDescs[depthStencilAttachmentIndex].samples = RHI::Vulkan::ConvertSampleBits(textureDesc.SampleCount);
        attachmentDescs[depthStencilAttachmentIndex].format = RHI::Vulkan::ConvertFormat(textureDesc.Format);
        attachmentDescs[depthStencilAttachmentIndex].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachmentDescs[depthStencilAttachmentIndex].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentDescs[depthStencilAttachmentIndex].stencilLoadOp = formatInfo.HasStencil ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDescs[depthStencilAttachmentIndex].stencilStoreOp = formatInfo.HasStencil ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentDescs[depthStencilAttachmentIndex].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachmentDescs[depthStencilAttachmentIndex].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthStencilAttachmentRef.attachment = depthStencilAttachmentIndex;
        depthStencilAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    // Only one subpass for now
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentRefs.size());
    subpass.pColorAttachments = colorAttachmentRefs.data();
    subpass.pDepthStencilAttachment = hasDepthStencil ? &depthStencilAttachmentRef : nullptr;
    VkRenderPassCreateInfo renderPassCreateInfo{};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachmentDescs.size());
    renderPassCreateInfo.pAttachments = attachmentDescs.data();
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;
    renderPassCreateInfo.dependencyCount = 0;
    renderPassCreateInfo.pDependencies = nullptr;

    VkResult result = vkCreateRenderPass(m_Device.GetDevice(), &renderPassCreateInfo, nullptr, &m_RenderPassHandle);
    if(result != VK_SUCCESS)
    {
        OUTPUT_VULKAN_FAILED_RESULT(result)
        return false;
    }
    
    return CreateFrameBuffer();
}

void VulkanFrameBuffer::Shutdown()
{
    ShutdownInternal();
}

bool VulkanFrameBuffer::Resize(std::shared_ptr<RHITexture>* inRenderTargets, std::shared_ptr<RHITexture> inDepthStencil)
{
    DestroyFrameBuffer();
    for(uint32_t i = 0; i < GetNumRenderTargets(); i++)
    {
        if(!inRenderTargets[i] || !inRenderTargets[i]->IsValid())
        {
            Log::Error("[Vulkan] Failed to resize framebuffer, new render targets %d is invalid", i);
            return false;
        }
        m_Desc.RenderTargets[i] = inRenderTargets[i];
    }
    if(HasDepthStencil())
    {
        if(!inDepthStencil || !inDepthStencil->IsValid())
        {
            Log::Error("[Vulkan] Failed to resize framebuffer, new depth stencil texture is invalid");
            return false;
        }
        m_Desc.DepthStencil = inDepthStencil;
    }
    return CreateFrameBuffer();
}

bool VulkanFrameBuffer::CreateFrameBuffer()
{
    std::vector<VkImageView> imageViews;
    for(uint32_t i = 0; i < GetNumRenderTargets(); i++)
    {
        const RHITextureDesc& textureDesc = m_Desc.RenderTargets[i]->GetDesc();
        m_FrameBufferWidth = textureDesc.Width;
        m_FrameBufferHeight = textureDesc.Height;
        
        m_RenderTargets[i] = CheckCast<VulkanTexture*>(m_Desc.RenderTargets[i].get());
        if(!m_RenderTargets[i] || !m_RenderTargets[i]->IsValid())
        {
            Log::Error("[Vulkan] Render target %u is invalid", i);
            return false;
        }
        
        if(!m_RenderTargets[i]->TryGetRTVHandle(m_RTVHandles[i]))
        {
            if(!m_RenderTargets[i]->CreateRTV(m_RTVHandles[i]))
            {
                Log::Error("[Vulkan] Failed to create RTV for render target %u", i);
                return false;       
            }
        }

        imageViews.push_back(m_RTVHandles[i]);
    }

    if(HasDepthStencil())
    {
        const RHITextureDesc& textureDesc = m_Desc.DepthStencil->GetDesc();
        m_DepthStencil = CheckCast<VulkanTexture*>(m_Desc.DepthStencil.get());
        m_FrameBufferWidth = textureDesc.Width;
        m_FrameBufferHeight = textureDesc.Height;
        if(!m_DepthStencil || !m_DepthStencil->IsValid())
        {
            Log::Error("[Vulkan] Depth stencil buffer is invalid");
            return false;
        }
        if(!m_DepthStencil->TryGetDSVHandle(m_DSVHandle))
        {
            if(!m_DepthStencil->CreateDSV(m_DSVHandle))
            {
                Log::Error("[Vulkan] Failed to create DSV for render target");
                return false;       
            }
        }

        imageViews.push_back(m_DSVHandle);
    }
    
    VkFramebufferCreateInfo frameBufferCreateInfo{};
    frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    frameBufferCreateInfo.renderPass = m_RenderPassHandle;
    frameBufferCreateInfo.attachmentCount = m_AttachmentCount;
    frameBufferCreateInfo.pAttachments = imageViews.data();
    frameBufferCreateInfo.width = m_FrameBufferWidth;
    frameBufferCreateInfo.height = m_FrameBufferHeight;
    frameBufferCreateInfo.layers = 1;
    VkResult result = vkCreateFramebuffer(m_Device.GetDevice(), &frameBufferCreateInfo, nullptr, &m_FrameBufferHandle);
    if(result != VK_SUCCESS)
    {
        OUTPUT_VULKAN_FAILED_RESULT(result)
        return false;
    }
    
    return true;
}

void VulkanFrameBuffer::DestroyFrameBuffer()
{
    if(m_FrameBufferHandle != VK_NULL_HANDLE)
    {
        vkDestroyFramebuffer(m_Device.GetDevice(), m_FrameBufferHandle, nullptr);
        m_FrameBufferHandle = VK_NULL_HANDLE;
    }

    for(uint32_t i = 0; i < RHIRenderTargetsMaxCount; ++i)
    {
        m_RenderTargets[i] = nullptr;
        m_RTVHandles[i] = VK_NULL_HANDLE;
    }
    m_DepthStencil = nullptr;
    m_DSVHandle = VK_NULL_HANDLE;
}

void VulkanFrameBuffer::ShutdownInternal()
{
    if(m_RenderPassHandle != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(m_Device.GetDevice(), m_RenderPassHandle, nullptr);
        m_RenderPassHandle = VK_NULL_HANDLE;
    }
    DestroyFrameBuffer();
}

bool VulkanFrameBuffer::IsValid() const
{
    return m_FrameBufferHandle != VK_NULL_HANDLE && m_RenderPassHandle != VK_NULL_HANDLE;
}

void VulkanFrameBuffer::SetNameInternal()
{
    if(IsValid())
    {
        m_Device.SetDebugName(VK_OBJECT_TYPE_FRAMEBUFFER, reinterpret_cast<uint64_t>(m_FrameBufferHandle), m_Name);
    }
}

ERHIFormat VulkanFrameBuffer::GetRenderTargetFormat(uint32_t inIndex) const
{
    if(inIndex < m_Desc.NumRenderTargets && m_Desc.RenderTargets[inIndex] != nullptr)
    {
        return m_Desc.RenderTargets[inIndex]->GetDesc().Format;
    }
    else
    {
        return ERHIFormat::Unknown;
    }
}

ERHIFormat VulkanFrameBuffer::GetDepthStencilFormat() const
{
    if(HasDepthStencil())
    {
        return m_Desc.DepthStencil->GetDesc().Format;
    }
    else
    {
        return ERHIFormat::Unknown;
    }
}