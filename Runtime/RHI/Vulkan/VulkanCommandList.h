#pragma once

#include "../RHICommandList.h"
#include "VulkanDefinitions.h"

class VulkanCommandList : public RHICommandList
{
public:
    ~VulkanCommandList() override;
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
    VkCommandBuffer GetCommandBuffer() const { return m_CmdBufferHandle; }
    
protected:
    void SetNameInternal() override;

private:
    friend class VulkanDevice;
    VulkanCommandList(VulkanDevice& inDevice, ERHICommandQueueType inType);
    void ShutdownInternal();
    void FlushBarriers(VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT
        , VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

    VulkanDevice& m_Device;
    const ERHICommandQueueType m_QueueType;
    VkCommandPool m_CmdPoolHandle;
    VkCommandBuffer m_CmdBufferHandle;
    bool m_IsClosed;

    std::vector<VkImageMemoryBarrier> m_ImageBarriers;
    std::vector<VkBufferMemoryBarrier> m_BufferBarriers;
};