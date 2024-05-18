#pragma once

#include "RHIDefinitions.h"

class RHIBuffer;
class RHITexture;
class RHIFrameBuffer;
class RHIGraphicsPipeline;
class RHIPipelineBindingLayout;

class RHICommandList : public RHIObject
{
public:
    virtual bool IsClosed() const = 0;
    virtual ERHICommandQueueType GetQueueType() const = 0;
    virtual void Begin() = 0;
    virtual void End() = 0;
    virtual void SetPipelineState(const RefCountPtr<RHIGraphicsPipeline>& inPipelineState) = 0;
    virtual void SetFrameBuffer(const RefCountPtr<RHIFrameBuffer>& inFrameBuffer) = 0;
    virtual void SetFrameBuffer(const RefCountPtr<RHIFrameBuffer>& inFrameBuffer
        , const RHIClearValue* inColor , uint32_t inNumRenderTargets) = 0;
    virtual void SetFrameBuffer(const RefCountPtr<RHIFrameBuffer>& inFrameBuffer
        , const RHIClearValue* inColor , uint32_t inNumRenderTargets
        , float inDepth, uint8_t inStencil) = 0;
    virtual void SetVertexBuffer(const RefCountPtr<RHIBuffer>& inBuffer) = 0;
    virtual void SetIndexBuffer(const RefCountPtr<RHIBuffer>& inBuffer) = 0;
    virtual void SetViewports(const std::vector<RHIViewport>& inViewports) = 0;
    virtual void SetScissorRects(const std::vector<RHIRect>& inRects) = 0;
    virtual void ResourceBarrier(RefCountPtr<RHITexture>& inResource, ERHIResourceStates inAfterState) = 0;
    virtual void CopyBuffer(RefCountPtr<RHIBuffer>& dstBuffer, size_t dstOffset, RefCountPtr<RHIBuffer>& srcBuffer, size_t srcOffset, size_t size) = 0;
};