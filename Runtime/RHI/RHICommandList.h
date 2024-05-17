#pragma once

#include "RHIDefinitions.h"
#include "RHIResources.h"

class RHICommandList : public RHIObject
{
public:
    virtual ERHICommandQueueType GetQueueType() const = 0;
    virtual void Begin() = 0;
    virtual void End() = 0;
    virtual bool IsClosed() const = 0;
    virtual void BeginRenderPass(const std::shared_ptr<RHIFrameBuffer>& inFrameBuffer) = 0;
    virtual void BeginRenderPass(const std::shared_ptr<RHIFrameBuffer>& inFrameBuffer
        , const RHIClearValue* inColor , uint32_t inNumRenderTargets) = 0;
    virtual void BeginRenderPass(const std::shared_ptr<RHIFrameBuffer>& inFrameBuffer
        , const RHIClearValue* inColor , uint32_t inNumRenderTargets
        , float inDepth, uint8_t inStencil) = 0;
    virtual void EndRenderPass() = 0;
    virtual void ResourceBarrier(std::shared_ptr<RHITexture>& inResource, ERHIResourceStates inAfterState) = 0;
};