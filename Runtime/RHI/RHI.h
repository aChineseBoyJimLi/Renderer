#pragma once

#include "RHIDevice.h"
#include "RHICommandList.h"
#include "RHIPipelineState.h"
#include "RHIResources.h"
#include "RHISwapChain.h"

typedef std::shared_ptr<RHIFence>           RHIFenceRef;
typedef std::shared_ptr<RHISemaphore>       RHISemaphoreRef;
typedef std::shared_ptr<RHICommandList>     RHICommandListRef;
typedef std::shared_ptr<RHIBuffer>          RHIBufferRef;
typedef std::shared_ptr<RHITexture>         RHITextureRef;
typedef std::shared_ptr<RHISampler>         RHISamplerRef;
typedef std::shared_ptr<RHISwapChain>       RHISwapChainRef;
typedef std::shared_ptr<RHIPipelineBindingLayout> RHIPipelineBindingLayoutRef;