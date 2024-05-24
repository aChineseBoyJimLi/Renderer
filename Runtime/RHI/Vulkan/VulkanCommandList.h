#pragma once

#include "../RHICommandList.h"
#include "VulkanDefinitions.h"

struct VulkanGraphicsPipelineContext
{
    VkPipeline pipelineState = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkFramebuffer frameBuffer = VK_NULL_HANDLE;

    void Clear()
    {
        pipelineState = VK_NULL_HANDLE;
        pipelineLayout = VK_NULL_HANDLE;
        renderPass = VK_NULL_HANDLE;
        frameBuffer = VK_NULL_HANDLE;
    }
};

class VulkanCommandList : public RHICommandList
{
public:
    ~VulkanCommandList() override;
    bool Init() override;
    void Shutdown() override;
    bool IsValid() const override;
    void BeginMark(const char* name) override;
    void EndMark() override;
    void Begin() override;
    void End() override;
    void SetPipelineState(const RefCountPtr<RHIComputePipeline>& inPipelineState) override;
    void SetPipelineState(const RefCountPtr<RHIGraphicsPipeline>& inPipelineState) override;
    void SetFrameBuffer(const RefCountPtr<RHIFrameBuffer>& inFrameBuffer) override;
    void SetFrameBuffer(const RefCountPtr<RHIFrameBuffer>& inFrameBuffer
        , const RHIClearValue* inColor , uint32_t inNumRenderTargets) override;
    void SetFrameBuffer(const RefCountPtr<RHIFrameBuffer>& inFrameBuffer
        , const RHIClearValue* inColor , uint32_t inNumRenderTargets
        , float inDepth, uint8_t inStencil) override;
    void ResourceBarrier(RefCountPtr<RHITexture>& inResource , ERHIResourceStates inAfterState) override;
    void ResourceBarrier(RefCountPtr<RHIBuffer>& inResource, ERHIResourceStates inAfterState) override;
    void SetResourceSet(RefCountPtr<RHIResourceSet>& inResourceSet) override;
    
    void CopyBuffer(RefCountPtr<RHIBuffer>& dstBuffer, size_t dstOffset, RefCountPtr<RHIBuffer>& srcBuffer, size_t srcOffset, size_t size) override;
    void CopyTexture(RefCountPtr<RHITexture>& dstTexture, RefCountPtr<RHITexture>& srcTexture) override;
    void CopyTexture(RefCountPtr<RHITexture>& dstTexture, const RHITextureSlice& dstSlice, RefCountPtr<RHITexture>& srcTexture, const RHITextureSlice& srcSlice) override;
    void CopyBufferToTexture(RefCountPtr<RHITexture>& dstTexture, RefCountPtr<RHIBuffer>& srcBuffer) override;
    void CopyBufferToTexture(RefCountPtr<RHITexture>& dstTexture, const RHITextureSlice& dstSlice, RefCountPtr<RHIBuffer>& srcBuffer, size_t srcOffset) override;

    void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t vertexOffset = 0, uint32_t firstInstance = 0) override;
    void DrawIndirect(RefCountPtr<RHIBuffer>& indirectCommands, uint32_t drawCount, size_t commandsBufferOffset = 0) override;
    void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex = 0, uint32_t vertexOffset = 0, uint32_t firstInstance = 0) override;
    void DrawIndexedIndirect(RefCountPtr<RHIBuffer>& indirectCommands, uint32_t drawCount, size_t commandsBufferOffset = 0) override;
    void Dispatch(uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ) override;
    void DispatchIndirect(RefCountPtr<RHIBuffer>& indirectCommands, uint32_t count, size_t commandsBufferOffset = 0) override;
    void DispatchMesh(uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ) override;
    void DispatchMeshIndirect(RefCountPtr<RHIBuffer>& indirectCommands, uint32_t count, size_t commandsBufferOffset = 0) override;
    void DispatchRays(uint32_t width, uint32_t height, uint32_t depth, RefCountPtr<RHIShaderTable>& shaderTable) override;
    
    void SetVertexBuffer(const RefCountPtr<RHIBuffer>& inBuffer) override;
    void SetIndexBuffer(const RefCountPtr<RHIBuffer>& inBuffer) override;
    void SetViewports(const std::vector<RHIViewport>& inViewports) override;
    void SetScissorRects(const std::vector<RHIRect>& inRects) override;
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

    void EndRenderPass();
    
    VulkanDevice& m_Device;
    const ERHICommandQueueType m_QueueType;
    VkCommandPool m_CmdPoolHandle;
    VkCommandBuffer m_CmdBufferHandle;
    bool m_IsClosed;

    std::vector<VkImageMemoryBarrier> m_ImageBarriers;
    std::vector<VkBufferMemoryBarrier> m_BufferBarriers;

    VulkanGraphicsPipelineContext m_Context;
};