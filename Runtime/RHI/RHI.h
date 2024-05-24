#pragma once

#include "RHIDevice.h"
#include "RHICommandList.h"
#include "RHIPipelineState.h"
#include "RHIResources.h"
#include "RHISwapChain.h"

typedef RefCountPtr<RHIFence>                   RHIFenceRef;
typedef RefCountPtr<RHISemaphore>               RHISemaphoreRef;
typedef RefCountPtr<RHICommandList>             RHICommandListRef;
typedef RefCountPtr<RHIBuffer>                  RHIBufferRef;
typedef RefCountPtr<RHITexture>                 RHITextureRef;
typedef RefCountPtr<RHISampler>                 RHISamplerRef;
typedef RefCountPtr<RHISwapChain>               RHISwapChainRef;
typedef RefCountPtr<RHIPipelineBindingLayout>   RHIPipelineBindingLayoutRef;
typedef RefCountPtr<RHIGraphicsPipeline>        RHIGraphicsPipelineRef;
typedef RefCountPtr<RHIShader>                  RHIShaderRef;
typedef RefCountPtr<RHIComputePipeline>         RHIComputePipelineRef;
typedef RefCountPtr<RHIRayTracingPipeline>      RHIRayTracingPipelineRef;
typedef RefCountPtr<RHISwapChain>               RHISwapChainRef;
typedef RefCountPtr<RHIFrameBuffer>             RHIFrameBufferRef;
typedef RefCountPtr<RHIResourceSet>             RHIResourceSetRef;