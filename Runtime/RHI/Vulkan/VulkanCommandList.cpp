#include "VulkanCommandList.h"
#include "VulkanDevice.h"
#include "VulkanResources.h"
#include "VulkanPipelineState.h"
#include "../../Core/Log.h"
#include "../../Core/Templates.h"

RefCountPtr<RHICommandList> VulkanDevice::CreateCommandList(ERHICommandQueueType inType)
{
    RefCountPtr<RHICommandList> commandList(new VulkanCommandList(*this, inType));
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

void VulkanCommandList::BeginMark(const char* name)
{
    m_Device.BeginDebugMarker(name, m_CmdBufferHandle);
}

void VulkanCommandList::EndMark()
{
    m_Device.EndDebugMarker( m_CmdBufferHandle );
}

void VulkanCommandList::Begin()
{
    if (IsValid() && IsClosed())
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
    if (IsValid() && !IsClosed())
    {
        EndRenderPass();
        FlushBarriers();
        vkEndCommandBuffer(m_CmdBufferHandle);
        m_IsClosed = true;
        m_Context.Clear();
    }
}

void VulkanCommandList::SetPipelineState(const RefCountPtr<RHIComputePipeline>& inPipelineState)
{
    if(IsValid() && !IsClosed())
    {
        if(m_Context.pipelineState)
        {
            EndRenderPass();
        }
        const VulkanComputePipeline* pipeline = CheckCast<VulkanComputePipeline*>(inPipelineState.GetReference());
        VulkanPipelineBindingLayout* bindingLayout = CheckCast<VulkanPipelineBindingLayout*>(inPipelineState->GetDesc().BindingLayout);
        m_Context.pipelineLayout = bindingLayout->GetPipelineLayout();
        vkCmdBindPipeline(m_CmdBufferHandle, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetPipeline());
        m_Context.bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
    }
}

void VulkanCommandList::SetPipelineState(const RefCountPtr<RHIGraphicsPipeline>& inPipelineState)
{
    if (IsValid() && !IsClosed())
    {
        if(m_Context.pipelineState)
        {
            EndRenderPass();
        }

        const VulkanGraphicsPipeline* pipeline = CheckCast<VulkanGraphicsPipeline*>(inPipelineState.GetReference());
        const VulkanPipelineBindingLayout* bindingLayout = CheckCast<VulkanPipelineBindingLayout*>(inPipelineState->GetDesc().BindingLayout);
        m_Context.pipelineLayout = bindingLayout->GetPipelineLayout();
        m_Context.renderPass = pipeline->GetRenderPass();
        m_Context.pipelineState = pipeline->GetPipeline();
        m_Context.bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    }
}

void VulkanCommandList::SetFrameBuffer(const RefCountPtr<RHIFrameBuffer>& inFrameBuffer)
{
    uint32_t numRenderTargets = inFrameBuffer->GetNumRenderTargets();
    std::vector<RHIClearValue> clearColors(numRenderTargets);
    for(uint32_t i = 0; i < numRenderTargets; i++)
    {
        clearColors[i] = inFrameBuffer->GetRenderTarget(i)->GetClearValue();
    }
    SetFrameBuffer(inFrameBuffer, clearColors.data(), numRenderTargets);
}

void VulkanCommandList::SetFrameBuffer(const RefCountPtr<RHIFrameBuffer>& inFrameBuffer
    , const RHIClearValue* inColor , uint32_t inNumRenderTargets)
{
    float depth = 1;
    uint8_t stencil = 0;
    if(inFrameBuffer->HasDepthStencil())
    {
        const RHIClearValue& clearValue = inFrameBuffer->GetDepthStencil()->GetClearValue();
        depth = clearValue.DepthStencil.Depth;
        stencil = clearValue.DepthStencil.Stencil;
    }
    SetFrameBuffer(inFrameBuffer, inColor, inNumRenderTargets, depth, stencil);
}

void VulkanCommandList::SetFrameBuffer(const RefCountPtr<RHIFrameBuffer>& inFrameBuffer
    , const RHIClearValue* inColor , uint32_t inNumRenderTargets
    , float inDepth, uint8_t inStencil)
{
    if (IsValid() && !IsClosed())
    {
        FlushBarriers();

        if(m_Context.renderPass == VK_NULL_HANDLE || m_Context.pipelineState == VK_NULL_HANDLE)
        {
            Log::Error("[Vulkan] The graphics pipeline must be bound before setting the framebuffer");
            return;
        }

        const VulkanFrameBuffer* frameBuffer = CheckCast<VulkanFrameBuffer*>(inFrameBuffer.GetReference());
        if(frameBuffer && frameBuffer->IsValid())
        {
            VkRenderPassBeginInfo renderPassBeginInfo{};
            renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassBeginInfo.renderPass = m_Context.renderPass;
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
            vkCmdBindPipeline(m_CmdBufferHandle, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Context.pipelineState);

            m_Context.frameBuffer = renderPassBeginInfo.framebuffer;
        }
    }
}

void VulkanCommandList::EndRenderPass()
{
    if (IsValid() && !IsClosed())
    {
        if(m_Context.pipelineState && m_Context.frameBuffer && m_Context.renderPass)
        {
            vkCmdEndRenderPass(m_CmdBufferHandle);
            FlushBarriers();
            m_Context.Clear();
        }
    }
}

void VulkanCommandList::SetViewports(const std::vector<RHIViewport>& inViewports)
{
    if (IsValid() && !IsClosed() && !inViewports.empty())
    {
        std::vector<VkViewport> viewports(inViewports.size());

        for(uint32_t i = 0; i < inViewports.size(); ++i)
        {
            viewports[i].x = inViewports[i].X;
            viewports[i].y = inViewports[i].Y;
            viewports[i].width = inViewports[i].Width;
            viewports[i].height = inViewports[i].Height;
            viewports[i].minDepth = inViewports[i].MinDepth;
            viewports[i].maxDepth = inViewports[i].MaxDepth;
        }
        
        vkCmdSetViewport(m_CmdBufferHandle, 0, static_cast<uint32_t>(viewports.size()), viewports.data());
    }
}

void VulkanCommandList::SetScissorRects(const std::vector<RHIRect>& inRects)
{
    if (IsValid() && !IsClosed() && !inRects.empty())
    {
        std::vector<VkRect2D> scissorRects(inRects.size());
        for(uint32_t i = 0; i < inRects.size(); ++i)
        {
            scissorRects[i].offset.x = inRects[i].MinX;
            scissorRects[i].offset.y = inRects[i].MinY;
            scissorRects[i].extent.width = inRects[i].Width;
            scissorRects[i].extent.height = inRects[i].Height;
        }

        vkCmdSetScissor(m_CmdBufferHandle, 0, static_cast<uint32_t>(scissorRects.size()), scissorRects.data());
    }
}

void VulkanCommandList::CopyBuffer(RefCountPtr<RHIBuffer>& dstBuffer, size_t dstOffset, RefCountPtr<RHIBuffer>& srcBuffer, size_t srcOffset, size_t size)
{
    if (IsValid() && !IsClosed())
    {
        FlushBarriers();
        
        VulkanBuffer* buffer0 = CheckCast<VulkanBuffer*>(dstBuffer.GetReference());
        VulkanBuffer* buffer1 = CheckCast<VulkanBuffer*>(srcBuffer.GetReference());

        VkBufferCopy copyDesc;
        copyDesc.dstOffset = dstOffset;
        copyDesc.srcOffset = srcOffset;
        copyDesc.size = size;
        vkCmdCopyBuffer(m_CmdBufferHandle, buffer1->GetBuffer(), buffer0->GetBuffer(), 1, &copyDesc);
    }
}

void VulkanCommandList::CopyTexture(RefCountPtr<RHITexture>& dstTexture, RefCountPtr<RHITexture>& srcTexture)
{
    if(IsValid() && !IsClosed())
    {
        ResourceBarrier(srcTexture, ERHIResourceStates::CopySrc);
        ResourceBarrier(dstTexture, ERHIResourceStates::CopyDst);
        VulkanTexture* texture0 = CheckCast<VulkanTexture*>(dstTexture.GetReference());
        VulkanTexture* texture1 = CheckCast<VulkanTexture*>(srcTexture.GetReference());

        const VulkanTextureState& dstCurrentState = texture0->GetCurrentState();
        const VulkanTextureState& srcCurrentState = texture1->GetCurrentState();

        const RHITextureDesc& textureDesc0 = texture0->GetDesc();
        const RHITextureDesc& textureDesc1 = texture1->GetDesc();

        assert(textureDesc0.Width == textureDesc1.Width);
        assert(textureDesc0.Height == textureDesc1.Height);
        assert(textureDesc0.Depth == textureDesc1.Depth);

        VkImageCopy copyRegion;
        copyRegion.extent.width = textureDesc0.Width;
        copyRegion.extent.height = textureDesc0.Height;
        copyRegion.extent.depth = textureDesc0.Depth;
        copyRegion.dstOffset.x = 0;
        copyRegion.dstOffset.y = 0;
        copyRegion.dstOffset.z = 0;
        copyRegion.srcOffset.x = 0;
        copyRegion.srcOffset.y = 0;
        copyRegion.srcOffset.z = 0;
        copyRegion.dstSubresource.aspectMask = RHI::Vulkan::GuessImageAspectFlags(texture0->GetDesc().Format);
        copyRegion.dstSubresource.layerCount = textureDesc0.ArraySize;
        copyRegion.dstSubresource.mipLevel = 0;
        copyRegion.dstSubresource.baseArrayLayer = 0;
        copyRegion.srcSubresource.aspectMask = RHI::Vulkan::GuessImageAspectFlags(texture1->GetDesc().Format);
        copyRegion.srcSubresource.layerCount = textureDesc0.ArraySize;
        copyRegion.srcSubresource.mipLevel = 0;
        copyRegion.srcSubresource.baseArrayLayer = 0;

        vkCmdCopyImage(m_CmdBufferHandle
            , texture1->GetTexture()
            , VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
            , texture0->GetTexture()
            , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
            , 1, &copyRegion);

        
        texture1->ChangeState({dstCurrentState.AccessFlags, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL});
        // texture0->ChangeState({srcCurrentState.AccessFlags, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL});
    }
}

void VulkanCommandList::CopyTexture(RefCountPtr<RHITexture>& dstTexture, const RHITextureSlice& dstSlice, RefCountPtr<RHITexture>& srcTexture, const RHITextureSlice& srcSlice)
{
    if(IsValid() && !IsClosed())
    {
        ResourceBarrier(srcTexture, ERHIResourceStates::CopySrc);
        ResourceBarrier(dstTexture, ERHIResourceStates::CopyDst);
        FlushBarriers();

        VulkanTexture* texture0 = CheckCast<VulkanTexture*>(dstTexture.GetReference());
        VulkanTexture* texture1 = CheckCast<VulkanTexture*>(srcTexture.GetReference());

        const VulkanTextureState& dstCurrentState = texture0->GetCurrentState();
        const VulkanTextureState& srcCurrentState = texture1->GetCurrentState();

        assert(dstSlice.Width == srcSlice.Width);
        assert(dstSlice.Height == srcSlice.Height);

        VkImageCopy copyRegion;
        copyRegion.extent.width = dstSlice.Width;
        copyRegion.extent.height = dstSlice.Height;
        copyRegion.extent.depth = dstSlice.Depth;
        copyRegion.dstOffset.x = (int)dstSlice.X;
        copyRegion.dstOffset.y = (int)dstSlice.Y;
        copyRegion.dstOffset.z = (int)dstSlice.Z;
        copyRegion.srcOffset.x = (int)srcSlice.X;
        copyRegion.srcOffset.y = (int)srcSlice.Y;
        copyRegion.srcOffset.z = (int)srcSlice.Z;
        copyRegion.dstSubresource.aspectMask = RHI::Vulkan::GuessImageAspectFlags(texture0->GetDesc().Format);
        copyRegion.dstSubresource.layerCount = 1;
        copyRegion.dstSubresource.mipLevel = dstSlice.MipLevel;
        copyRegion.dstSubresource.baseArrayLayer = dstSlice.ArraySlice;
        copyRegion.srcSubresource.aspectMask = RHI::Vulkan::GuessImageAspectFlags(texture1->GetDesc().Format);
        copyRegion.srcSubresource.layerCount = 1;
        copyRegion.srcSubresource.mipLevel = srcSlice.MipLevel;
        copyRegion.srcSubresource.baseArrayLayer = srcSlice.ArraySlice;
        
        vkCmdCopyImage(m_CmdBufferHandle
            , texture1->GetTexture()
            , VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
            , texture0->GetTexture()
            , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
            , 1, &copyRegion);

        
        texture1->ChangeState({dstCurrentState.AccessFlags, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL});
        // texture0->ChangeState({srcCurrentState.AccessFlags, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL});
    }
}

void VulkanCommandList::CopyBufferToTexture(RefCountPtr<RHITexture>& dstTexture, RefCountPtr<RHIBuffer>& srcBuffer)
{
    if(IsValid() && !IsClosed())
    {
        ResourceBarrier(dstTexture, ERHIResourceStates::CopyDst);
        FlushBarriers();

        VulkanTexture* texture0 = CheckCast<VulkanTexture*>(dstTexture.GetReference());
        VulkanBuffer* buffer1 = CheckCast<VulkanBuffer*>(srcBuffer.GetReference());

        const RHITextureDesc& textureDesc = dstTexture->GetDesc();
        const VulkanTextureState& dstCurrentState = texture0->GetCurrentState();
        
        VkBufferImageCopy copyInfo{};
        copyInfo.bufferOffset = 0;
        copyInfo.bufferRowLength = 0;
        copyInfo.bufferImageHeight = 0;
        copyInfo.imageOffset.x = 0;
        copyInfo.imageOffset.y = 0;
        copyInfo.imageOffset.z = 0;
        copyInfo.imageExtent.width = textureDesc.Width;
        copyInfo.imageExtent.height = textureDesc.Height;
        copyInfo.imageExtent.depth = textureDesc.Depth;
        copyInfo.imageSubresource.aspectMask = RHI::Vulkan::GuessImageAspectFlags(texture0->GetDesc().Format);
        copyInfo.imageSubresource.layerCount = textureDesc.ArraySize;
        copyInfo.imageSubresource.mipLevel = 0;
        copyInfo.imageSubresource.baseArrayLayer = 0;
        
        vkCmdCopyBufferToImage(m_CmdBufferHandle
            , buffer1->GetBuffer()
            , texture0->GetTexture()
            , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
            , 1
            , &copyInfo);

        // texture0->ChangeState({dstCurrentState.AccessFlags, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL});
    }
}

void VulkanCommandList::CopyBufferToTexture(RefCountPtr<RHITexture>& dstTexture, const RHITextureSlice& dstSlice, RefCountPtr<RHIBuffer>& srcBuffer, size_t srcOffset)
{
    if(IsValid() && !IsClosed())
    {
        ResourceBarrier(dstTexture, ERHIResourceStates::CopyDst);
        FlushBarriers();

        VulkanTexture* texture0 = CheckCast<VulkanTexture*>(dstTexture.GetReference());
        VulkanBuffer* buffer1 = CheckCast<VulkanBuffer*>(srcBuffer.GetReference());

        const VulkanTextureState& dstCurrentState = texture0->GetCurrentState();
        
        VkBufferImageCopy copyInfo{};
        copyInfo.bufferOffset = srcOffset;
        copyInfo.bufferRowLength = 0;
        copyInfo.bufferImageHeight = 0;
        copyInfo.imageOffset.x = (int)dstSlice.X;
        copyInfo.imageOffset.y = (int)dstSlice.Y;
        copyInfo.imageOffset.z = (int)dstSlice.Z;
        copyInfo.imageExtent.width = dstSlice.Width;
        copyInfo.imageExtent.height = dstSlice.Height;
        copyInfo.imageExtent.depth = dstSlice.Depth;
        copyInfo.imageSubresource.aspectMask = RHI::Vulkan::GuessImageAspectFlags(texture0->GetDesc().Format);
        copyInfo.imageSubresource.layerCount = 1;
        copyInfo.imageSubresource.mipLevel = dstSlice.MipLevel;
        copyInfo.imageSubresource.baseArrayLayer = dstSlice.ArraySlice;
        
        vkCmdCopyBufferToImage(m_CmdBufferHandle
            , buffer1->GetBuffer()
            , texture0->GetTexture()
            , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
            , 1
            , &copyInfo);

        // texture0->ChangeState({dstCurrentState.AccessFlags, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL});
    }
}

void VulkanCommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t vertexOffset, uint32_t firstInstance)
{
    if(IsValid() && !IsClosed())
    {
        vkCmdDraw(m_CmdBufferHandle, vertexCount, instanceCount, vertexOffset, firstInstance);
    }
}

void VulkanCommandList::DrawIndirect(RefCountPtr<RHIBuffer>& indirectCommands, uint32_t drawCount, size_t commandsBufferOffset)
{
    if(IsValid() && !IsClosed())
    {
        VulkanBuffer* buffer = CheckCast<VulkanBuffer*>(indirectCommands.GetReference());
        if(buffer && buffer->IsValid())
        {
            vkCmdDrawIndirect(m_CmdBufferHandle, buffer->GetBuffer(), commandsBufferOffset, drawCount, sizeof(RHIDrawArguments));
        }
    }
}

void VulkanCommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t vertexOffset, uint32_t firstInstance)
{
    if(IsValid() && !IsClosed())
    {
        vkCmdDrawIndexed(m_CmdBufferHandle, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }
}

void VulkanCommandList::DrawIndexedIndirect(RefCountPtr<RHIBuffer>& indirectCommands, uint32_t drawCount, size_t commandsBufferOffset)
{
    if(IsValid() && !IsClosed())
    {
        VulkanBuffer* buffer = CheckCast<VulkanBuffer*>(indirectCommands.GetReference());
        if(buffer && buffer->IsValid())
        {
            vkCmdDrawIndexedIndirect(m_CmdBufferHandle, buffer->GetBuffer(), commandsBufferOffset, drawCount, sizeof(RHIDrawIndexedArguments));
        }
    }
}

void VulkanCommandList::Dispatch(uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)
{
    if(IsValid() && !IsClosed())
    {
        vkCmdDispatch(m_CmdBufferHandle, threadGroupX, threadGroupY, threadGroupZ);
    }
}

void VulkanCommandList::DispatchIndirect(RefCountPtr<RHIBuffer>& indirectCommands, uint32_t count, size_t commandsBufferOffset)
{
    if(IsValid() && !IsClosed())
    {
        VulkanBuffer* buffer = CheckCast<VulkanBuffer*>(indirectCommands.GetReference());
        if(buffer && buffer->IsValid())
        {
            vkCmdDispatchIndirect(m_CmdBufferHandle, buffer->GetBuffer(), commandsBufferOffset);
        }
    }
}

void VulkanCommandList::DispatchMesh(uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)
{
    if(IsValid() && !IsClosed())
    {
        m_Device.vkCmdDrawMeshTasksEXT(m_CmdBufferHandle, threadGroupX, threadGroupY, threadGroupZ);
    }
}
void VulkanCommandList::DispatchMeshIndirect(RefCountPtr<RHIBuffer>& indirectCommands, uint32_t count, size_t commandsBufferOffset)
{
    if(IsValid() && !IsClosed())
    {
        VulkanBuffer* buffer = CheckCast<VulkanBuffer*>(indirectCommands.GetReference());
        if(buffer && buffer->IsValid())
        {
            m_Device.vkCmdDrawMeshTasksIndirectEXT(m_CmdBufferHandle, buffer->GetBuffer(), commandsBufferOffset, count, sizeof(RHIDispatchMeshArguments));
        }
    }
}

void VulkanCommandList::DispatchRays(uint32_t width, uint32_t height, uint32_t depth, const RHIShaderTable& shaderTable)
{
    if(IsValid() && !IsClosed())
    {
        VkStridedDeviceAddressRegionKHR rayGenEntry;
        rayGenEntry.deviceAddress = shaderTable.GetRayGenShaderEntry().StartAddress;
        rayGenEntry.size = shaderTable.GetRayGenShaderEntry().SizeInBytes;
        rayGenEntry.stride = shaderTable.GetRayGenShaderEntry().StrideInBytes;

        VkStridedDeviceAddressRegionKHR hitGroupEntry;
        hitGroupEntry.deviceAddress = shaderTable.GetHitGroupEntry().StartAddress;
        hitGroupEntry.size = shaderTable.GetHitGroupEntry().SizeInBytes;
        hitGroupEntry.stride = shaderTable.GetHitGroupEntry().StrideInBytes;

        VkStridedDeviceAddressRegionKHR missEntry;
        missEntry.deviceAddress = shaderTable.GetMissShaderEntry().StartAddress;
        missEntry.size = shaderTable.GetMissShaderEntry().SizeInBytes;
        missEntry.stride = shaderTable.GetMissShaderEntry().StrideInBytes;

        VkStridedDeviceAddressRegionKHR callableEntry;
        callableEntry.deviceAddress = shaderTable.GetCallableShaderEntry().StartAddress;
        callableEntry.size = shaderTable.GetCallableShaderEntry().SizeInBytes;
        callableEntry.stride = shaderTable.GetCallableShaderEntry().StrideInBytes;
        
        m_Device.vkCmdTraceRaysKHR(m_CmdBufferHandle, &rayGenEntry, &missEntry, &hitGroupEntry, &callableEntry, width, height, depth);
    }
}

void VulkanCommandList::SetVertexBuffer(const RefCountPtr<RHIBuffer>& inBuffer)
{
    if(IsValid() && !IsClosed())
    {
        const VulkanBuffer* buffer = CheckCast<VulkanBuffer*>(inBuffer.GetReference());
        VkBuffer bufferVk = buffer->GetBuffer();
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(m_CmdBufferHandle, 0, 1, &bufferVk, &offset);
    }
}

void VulkanCommandList::SetIndexBuffer(const RefCountPtr<RHIBuffer>& inBuffer)
{
    if(IsValid() && !IsClosed())
    {
        const VulkanBuffer* buffer = CheckCast<VulkanBuffer*>(inBuffer.GetReference());
        ERHIFormat format = buffer->GetDesc().Format;

        const RHIFormatInfo& formatInfo = RHI::GetFormatInfo(format);
        
        VkIndexType type = VK_INDEX_TYPE_NONE_KHR;
        if(formatInfo.BytesPerBlock == sizeof(uint32_t))
            type = VK_INDEX_TYPE_UINT32;
        else if(formatInfo.BytesPerBlock == sizeof(uint16_t))
            type = VK_INDEX_TYPE_UINT16;
        else
            type = VK_INDEX_TYPE_UINT8_EXT;
        vkCmdBindIndexBuffer(m_CmdBufferHandle, buffer->GetBuffer(), 0, type);
    }
}

void VulkanCommandList::ResourceBarrier(RefCountPtr<RHITexture>& inResource , ERHIResourceStates inAfterState)
{
    if(IsValid() && !IsClosed())
    {
        VulkanTexture* texture = CheckCast<VulkanTexture*>(inResource.GetReference());
        const RHITextureDesc& desc = inResource->GetDesc();
        if(texture && texture->IsValid())
        {
            VulkanTextureState currentState = texture->GetCurrentState();
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = currentState.Layout;
            barrier.newLayout = RHI::Vulkan::ConvertImageLayout(inAfterState);
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = texture->GetTexture();
            barrier.subresourceRange.aspectMask = RHI::Vulkan::GuessImageAspectFlags(desc.Format);
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

void VulkanCommandList::ResourceBarrier(RefCountPtr<RHIBuffer>& inResource, ERHIResourceStates inAfterState)
{
    
}

void VulkanCommandList::SetResourceSet(RefCountPtr<RHIResourceSet>& inResourceSet)
{
    if(IsValid() && !IsClosed())
    {
        VulkanResourceSet* resourceSet = CheckCast<VulkanResourceSet*>(inResourceSet.GetReference());
        if(!m_Context.pipelineLayout)
        {
            Log::Error("[Vulkan] The pipeline must be bound before setting the resource set");
            return;
        }
        if(resourceSet && resourceSet->IsValid())
        {
            vkCmdBindDescriptorSets(m_CmdBufferHandle, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Context.pipelineLayout, 0, resourceSet->GetDescriptorSetsCount(), resourceSet->GetDescriptorSets(), 0, nullptr);
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