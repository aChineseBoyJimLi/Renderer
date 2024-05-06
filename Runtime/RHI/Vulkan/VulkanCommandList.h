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
    
    bool IsClosed() const override { return m_IsClosed; }
    ERHICommandQueueType GetQueueType() const override { return m_QueueType; }
    VkCommandBuffer GetCommandBuffer() const { return m_CmdBufferHandle; }
    
protected:
    void SetNameInternal() override;

private:
    friend class VulkanDevice;
    VulkanCommandList(VulkanDevice& inDevice, ERHICommandQueueType inType);
    void ShutdownInternal();

    VulkanDevice& m_Device;
    const ERHICommandQueueType m_QueueType;
    VkCommandPool m_CmdPoolHandle;
    VkCommandBuffer m_CmdBufferHandle;
    bool m_IsClosed;
};