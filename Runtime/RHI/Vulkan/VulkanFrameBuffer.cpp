#include "VulkanPipelineState.h"
#include "VulkanDevice.h"
#include "VulkanResources.h"
#include "../../Core/Log.h"
#include "../../Core/Templates.h"

RefCountPtr<RHIFrameBuffer> VulkanDevice::CreateFrameBuffer(const RHIFrameBufferDesc& inDesc)
{
    RefCountPtr<RHIFrameBuffer> frameBuffer(new VulkanFrameBuffer(*this, inDesc));
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
    , m_FrameBufferWidth(0)
    , m_FrameBufferHeight(0)
    , m_NumRenderTargets(0)
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

    VulkanGraphicsPipeline* pipelineState = CheckCast<VulkanGraphicsPipeline*>(m_Desc.PipelineState);
    if(pipelineState == nullptr || !pipelineState->IsValid())
    {
        Log::Error("[Vulkan] Failed to create framebuffer, pipeline state is invalid");
        return false;
    }

    m_NumRenderTargets = m_Desc.PipelineState->GetDesc().NumRenderTarget;
    m_AttachmentCount = HasDepthStencil() ? m_NumRenderTargets + 1 : m_NumRenderTargets;

    std::vector<VkImageView> imageViews;
    for(uint32_t i = 0; i < GetNumRenderTargets(); i++)
    {
        const RHITextureDesc& textureDesc = m_Desc.RenderTargets[i]->GetDesc();
        m_FrameBufferWidth = textureDesc.Width;
        m_FrameBufferHeight = textureDesc.Height;
        
        m_RenderTargets[i] = CheckCast<VulkanTexture*>(m_Desc.RenderTargets[i]);
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
        m_DepthStencil = CheckCast<VulkanTexture*>(m_Desc.DepthStencil);
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
    frameBufferCreateInfo.renderPass = pipelineState->GetRenderPass();
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

void VulkanFrameBuffer::Shutdown()
{
    ShutdownInternal();
}

void VulkanFrameBuffer::ShutdownInternal()
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

bool VulkanFrameBuffer::IsValid() const
{
    return m_FrameBufferHandle != VK_NULL_HANDLE;
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
    if(inIndex < m_NumRenderTargets && m_Desc.RenderTargets[inIndex] != nullptr)
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