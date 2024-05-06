#pragma once

#include "RHIDevice.h"
#include "RHICommandList.h"
#include "RHIPipelineState.h"
#include "RHIResources.h"
#include "RHISwapChain.h"

typedef std::shared_ptr<RHIFence> RHIFenceRef;
typedef std::shared_ptr<RHISemaphore> RHISemaphoreRef;
typedef std::shared_ptr<RHICommandList> RHICommandListRef;
typedef std::shared_ptr<RHIBuffer> RHIBufferRef;
typedef std::shared_ptr<RHITexture> RHITextureRef;