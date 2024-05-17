#include "VulkanCommandList.h"
#include "VulkanDevice.h"
#include "VulkanResources.h"
#include "../../Core/Log.h"
#include "../../Core/Templates.h"

std::shared_ptr<RHICommandList> VulkanDevice::CreateCommandList(ERHICommandQueueType inType)
{
    std::shared_ptr<RHICommandList> commandList(new VulkanCommandList(*this, inType));
    if(!commandList->Init())
    {
        Log::Error("[Vulkan] Failed to create a command list");
    }
    return commandList;
}

VulkanCommandList::VulkanCommandList(VulkanDevice& inDevice, ERHICommandQueueType inType)
    : m_Device(inDevice)
    , m_QueueType(inType)
    , m_CmdPoolHandle(VK_NULL_HANDLE)
    , m_CmdBufferHandle(VK_NULL_HANDLE)
    , m_IsClosed(true)
{
    
}

VulkanCommandList::~VulkanCommandList()
{
    ShutdownInternal();
}

bool VulkanCommandList::Init()
{
    if(IsValid())
    {
        Log::Warning("[Vulkan] Command list is already initialized");
        return true;
    }

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = m_Device.GetQueueFamilyIndex(m_QueueType);
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    VkResult result = vkCreateCommandPool(m_Device.GetDevice(), &poolInfo, nullptr, &m_CmdPoolHandle);

    if(result != VK_SUCCESS)
    {
        OUTPUT_VULKAN_FAILED_RESULT(result);
        return false;
    }

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_CmdPoolHandle;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    result = vkAllocateCommandBuffers(m_Device.GetDevice(), &allocInfo, &m_CmdBufferHandle);
    if(result != VK_SUCCESS)
    {
        OUTPUT_VULKAN_FAILED_RESULT(result);
        return false;
    }

    return true;
}

void VulkanCommandList::Shutdown()
{
    ShutdownInternal();
}

void VulkanCommandList::Begin()
{
    if (IsValid() && m_IsClosed)
    {
        vkResetCommandPool(m_Device.GetDevice(), m_CmdPoolHandle, 0);
        vkResetCommandBuffer(m_CmdBufferHandle, 0);
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        vkBeginCommandBuffer(m_CmdBufferHandle, &beginInfo);
        m_IsClosed = false;
    }
}

void VulkanCommandList::End()
{
    if (IsValid() && !m_IsClosed)
    {
        FlushBarriers();
        vkEndCommandBuffer(m_CmdBufferHandle);
        m_IsClosed = true;
    }
}

void VulkanCommandList::BeginRenderPass(const std::shared_ptr<RHIFrameBuffer>& inFrameBuffer)
{
    if(IsValid())
    {
        uint32_t numRenderTargets = inFrameBuffer->GetNumRenderTargets();
        std::vector<RHIClearValue> clearColors(numRenderTargets);
        for(uint32_t i = 0; i < numRenderTargets; i++)
        {
            clearColors[i] = inFrameBuffer->GetRenderTarget(i)->GetClearValue();
        }
        BeginRenderPass(inFrameBuffer, clearColors.data(), numRenderTargets);
    }
}

void VulkanCommandList::BeginRenderPass(const std::shared_ptr<RHIFrameBuffer>& inFrameBuffer
    , const RHIClearValue* inColor , uint32_t inNumRenderTargets)
{
    if(IsValid())
    {
        float depth = 1;
        uint8_t stencil = 0;
        if(inFrameBuffer->HasDepthStencil())
        {
            const RHIClearValue& clearValue = inFrameBuffer->GetDepthStencil()->GetClearValue();
            depth = clearValue.DepthStencil.Depth;
            stencil = clearValue.DepthStencil.Stencil;
        }
        BeginRenderPass(inFrameBuffer, inColor, inNumRenderTargets, depth, stencil);
    }
}

void VulkanCommandList::BeginRenderPass(const std::shared_ptr<RHIFrameBuffer>& inFrameBuffer
        , const RHIClearValue* inColor , uint32_t inNumRenderTargets
        , float inDepth, uint8_t inStencil)
{
    if (IsValid())
    {
        FlushBarriers();
        const VulkanFrameBuffer* frameBuffer = CheckCast<VulkanFrameBuffer*>(inFrameBuffer.get());
        if(frameBuffer && frameBuffer->IsValid())
        {
            VkRenderPassBeginInfo renderPassBeginInfo{};
            renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassBeginInfo.renderPass = frameBuffer->GetRenderPass();
            renderPassBeginInfo.framebuffer = frameBuffer->GetFrameBuffer();
            renderPassBeginInfo.renderArea.extent.width = frameBuffer->GetFrameBufferWidth();
            renderPassBeginInfo.renderArea.extent.height = frameBuffer->GetFrameBufferHeight();
            renderPassBeginInfo.renderArea.offset.x = 0;
            renderPassBeginInfo.renderArea.offset.y = 0;

            uint32_t numClearValues = inFrameBuffer->HasDepthStencil() ? inNumRenderTargets + 1 : inNumRenderTargets;
            std::vector<VkClearValue> clearValues(numClearValues);
            for(uint32_t i = 0; i < inNumRenderTargets; i++)
            {
                clearValues[i].color.float32[0] = inColor[i].Color[0];
                clearValues[i].color.float32[1] = inColor[i].Color[1];
                clearValues[i].color.float32[2] = inColor[i].Color[2];
                clearValues[i].color.float32[3] = inColor[i].Color[3];
            }
            if(inFrameBuffer->HasDepthStencil())
            {
                clearValues[inNumRenderTargets].depthStencil.depth = inDepth;
                clearValues[inNumRenderTargets].depthStencil.stencil = inStencil;
            }
            renderPassBeginInfo.clearValueCount = numClearValues;
            renderPassBeginInfo.pClearValues = clearValues.data();
            vkCmdBeginRenderPass(m_CmdBufferHandle, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        }
    }
}

void VulkanCommandList::EndRenderPass()
{
    if (IsValid())
    {
        vkCmdEndRenderPass(m_CmdBufferHandle);
        FlushBarriers();
    }
}

void VulkanCommandList::ResourceBarrier(std::shared_ptr<RHITexture>& inResource , ERHIResourceStates inAfterState)
{
    if(IsValid())
    {
        VulkanTexture* texture = CheckCast<VulkanTexture*>(inResource.get());
        const RHITextureDesc& desc = inResource->GetDesc();
        if(texture && texture->IsValid())
        {
            VulkanTextureState currentState = texture->GetCurrentState();
            const RHIFormatInfo& formatInfo = RHI::GetFormatInfo(desc.Format);
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = currentState.Layout;
            barrier.newLayout = RHI::Vulkan::ConvertImageLayout(inAfterState);
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = texture->GetTextureHandle();
            if(formatInfo.HasDepth)
            {
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                if(formatInfo.HasStencil)
                    barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
            }
            else
            {
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            }
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = desc.MipLevels;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = desc.ArraySize;
            barrier.srcAccessMask = currentState.AccessFlags;
            barrier.dstAccessMask = RHI::Vulkan::ConvertAccessFlags(inAfterState);
            m_ImageBarriers.push_back(barrier);
            currentState.Layout = barrier.newLayout;
            currentState.AccessFlags = barrier.newLayout;
            texture->ChangeState(currentState);
        }
    }
}

void VulkanCommandList::FlushBarriers(VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage)
{
    if(IsValid())
    {
        if(!m_ImageBarriers.empty() || !m_BufferBarriers.empty())
        {
            vkCmdPipelineBarrier(m_CmdBufferHandle
                , srcStage, dstStage, 0
                , 0, nullptr
                , (uint32_t)m_BufferBarriers.size(), m_BufferBarriers.data()
                , (uint32_t)m_ImageBarriers.size(), m_ImageBarriers.data());
        }
        m_BufferBarriers.clear();
        m_ImageBarriers.clear();
    }
}

bool VulkanCommandList::IsValid() const
{
    bool valid = m_CmdPoolHandle != VK_NULL_HANDLE && m_CmdBufferHandle != VK_NULL_HANDLE;
    return valid;
}

void VulkanCommandList::ShutdownInternal()
{
    if(m_CmdBufferHandle != VK_NULL_HANDLE)
    {
        vkFreeCommandBuffers(m_Device.GetDevice(), m_CmdPoolHandle, 1, &m_CmdBufferHandle);
        m_CmdBufferHandle = VK_NULL_HANDLE;
    }

    if(m_CmdPoolHandle != VK_NULL_HANDLE)
    {
        vkDestroyCommandPool(m_Device.GetDevice(), m_CmdPoolHandle, nullptr);
        m_CmdPoolHandle = VK_NULL_HANDLE;
    }
}

void VulkanCommandList::SetNameInternal()
{
    if(IsValid())
    {
        m_Device.SetDebugName(VK_OBJECT_TYPE_COMMAND_BUFFER, reinterpret_cast<uint64_t>(m_CmdBufferHandle), m_Name);
    }
}