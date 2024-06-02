#pragma once

#include "RHIDefinitions.h"

class RHIShaderTable;
class RHIComputePipeline;
class RHIResourceSet;
struct RHITextureSlice;
class RHIBuffer;
class RHITexture;
class RHIFrameBuffer;
class RHIGraphicsPipeline;
class RHIPipelineBindingLayout;

class RHICommandList : public RHIObject
{
public:
    virtual ERHICommandQueueType GetQueueType() const = 0;
    virtual bool IsClosed() const = 0;
    virtual void BeginMark(const char* name) = 0;
    virtual void EndMark() = 0;
    virtual void Begin() = 0;
    virtual void End() = 0;

    virtual void SetPipelineState(const RefCountPtr<RHIComputePipeline>& inPipelineState) = 0;
    virtual void SetPipelineState(const RefCountPtr<RHIGraphicsPipeline>& inPipelineState) = 0;
    
    virtual void SetFrameBuffer(const RefCountPtr<RHIFrameBuffer>& inFrameBuffer) = 0;
    virtual void SetFrameBuffer(const RefCountPtr<RHIFrameBuffer>& inFrameBuffer
        , const RHIClearValue* inColor , uint32_t inNumRenderTargets) = 0;
    virtual void SetFrameBuffer(const RefCountPtr<RHIFrameBuffer>& inFrameBuffer
        , const RHIClearValue* inColor , uint32_t inNumRenderTargets
        , float inDepth, uint8_t inStencil) = 0;
    
    virtual void SetViewports(const std::vector<RHIViewport>& inViewports) = 0;
    virtual void SetScissorRects(const std::vector<RHIRect>& inRects) = 0;
    
    virtual void ResourceBarrier(RefCountPtr<RHITexture>& inResource, ERHIResourceStates inAfterState) = 0;
    virtual void ResourceBarrier(RefCountPtr<RHIBuffer>& inResource, ERHIResourceStates inAfterState) = 0;
    virtual void SetResourceSet(RefCountPtr<RHIResourceSet>& inResourceSet) = 0;
    virtual void SetVertexBuffer(const RefCountPtr<RHIBuffer>& inBuffer, size_t inOffset = 0) = 0;
    virtual void SetIndexBuffer(const RefCountPtr<RHIBuffer>& inBuffer, size_t inOffset = 0) = 0;
    
    virtual void CopyBuffer(RefCountPtr<RHIBuffer>& dstBuffer, size_t dstOffset, RefCountPtr<RHIBuffer>& srcBuffer, size_t srcOffset, size_t size) = 0;
    virtual void CopyBufferToTexture(RefCountPtr<RHITexture>& dstTexture, RefCountPtr<RHIBuffer>& srcBuffer) = 0;
    virtual void CopyBufferToTexture(RefCountPtr<RHITexture>& dstTexture, const RHITextureSlice& dstSlice, RefCountPtr<RHIBuffer>& srcBuffer, size_t srcOffset) = 0;
    virtual void CopyTexture(RefCountPtr<RHITexture>& dstTexture, RefCountPtr<RHITexture>& srcTexture) = 0;
    virtual void CopyTexture(RefCountPtr<RHITexture>& dstTexture, const RHITextureSlice& dstSlice, RefCountPtr<RHITexture>& srcTexture, const RHITextureSlice& srcSlice) = 0;

    virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t vertexOffset = 0, uint32_t firstInstance = 0) = 0;
    virtual void DrawIndirect(RefCountPtr<RHIBuffer>& indirectCommands, uint32_t drawCount, size_t commandsBufferOffset = 0) = 0;
    virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex = 0, uint32_t vertexOffset = 0, uint32_t firstInstance = 0) = 0;
    virtual void DrawIndexedIndirect(RefCountPtr<RHIBuffer>& indirectCommands, uint32_t drawCount, size_t commandsBufferOffset = 0) = 0;
    virtual void Dispatch(uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ) = 0;
    virtual void DispatchIndirect(RefCountPtr<RHIBuffer>& indirectCommands, uint32_t count, size_t commandsBufferOffset = 0) = 0;
    virtual void DispatchMesh(uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ) = 0;
    virtual void DispatchMeshIndirect(RefCountPtr<RHIBuffer>& indirectCommands, uint32_t count, size_t commandsBufferOffset = 0) = 0;
    virtual void DispatchRays(uint32_t width, uint32_t height, uint32_t depth, const RHIShaderTable& shaderTable) = 0;
};