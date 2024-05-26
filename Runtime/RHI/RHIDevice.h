#pragma once

#include "RHIDefinitions.h"

class RHIResourceSet;
struct RHISamplerDesc;
class RHISampler;
struct RHIGraphicsPipelineDesc;
class RHIGraphicsPipeline;
struct RHITextureDesc;
class RHITexture;
struct RHIBufferDesc;
class RHIBuffer;
struct RHIResourceHeapDesc;
class RHIResourceHeap;
class RHIComputePipeline;
class RHIShader;
class RHIPipelineBindingLayout;
class RHICommandList;
struct RHIPipelineBindingLayoutDesc;
struct RHIComputePipelineDesc;
struct RHIFrameBufferDesc;
class RHIFrameBuffer;
class RHIAccelerationStructure;
struct RHIRayTracingInstanceDesc;
struct RHIRayTracingGeometryDesc;

// used for Cpu/Gpu synchronization
class RHIFence : public RHIObject
{
public:
    virtual void Reset() = 0;
    virtual void CpuWait() = 0;
};

// used for CommandQueue/CommandQueue synchronization
class RHISemaphore : public RHIObject
{
public:
    virtual void Reset() = 0;
};

// used for creating rhi objects
class RHIDevice : public RHIObject
{
public:
    virtual ERHIBackend                             GetBackend() const = 0;
    
    virtual RefCountPtr<RHIFence>                   CreateRhiFence() = 0;
    virtual RefCountPtr<RHISemaphore>               CreateRhiSemaphore() = 0;
    virtual RefCountPtr<RHICommandList>             CreateCommandList(ERHICommandQueueType inType = ERHICommandQueueType::Direct) = 0;
    virtual RefCountPtr<RHIPipelineBindingLayout>   CreatePipelineBindingLayout(const RHIPipelineBindingLayoutDesc& inBindingItems) = 0;
    virtual RefCountPtr<RHIShader>                  CreateShader(ERHIShaderType inType) = 0;
    virtual RefCountPtr<RHIComputePipeline>         CreatePipeline(const RHIComputePipelineDesc& inDesc) = 0;
    virtual RefCountPtr<RHIGraphicsPipeline>        CreatePipeline(const RHIGraphicsPipelineDesc& inDesc) = 0;
    virtual RefCountPtr<RHIResourceHeap>            CreateResourceHeap(const RHIResourceHeapDesc& inDesc) = 0;
    virtual RefCountPtr<RHIBuffer>                  CreateBuffer(const RHIBufferDesc& inDesc, bool isVirtual = false) = 0;
    virtual RefCountPtr<RHITexture>                 CreateTexture(const RHITextureDesc& inDesc, bool isVirtual = false) = 0;
    virtual RefCountPtr<RHISampler>                 CreateSampler(const RHISamplerDesc& inDesc) = 0;
    virtual RefCountPtr<RHIAccelerationStructure>   CreateBottomLevelAccelerationStructure(const std::vector<RHIRayTracingGeometryDesc>& inDesc) = 0;
    virtual RefCountPtr<RHIAccelerationStructure>   CreateTopLevelAccelerationStructure(const std::vector<RHIRayTracingInstanceDesc>& inDesc) = 0;
    virtual RefCountPtr<RHIFrameBuffer>             CreateFrameBuffer(const RHIFrameBufferDesc& inDesc) = 0;
    virtual RefCountPtr<RHIResourceSet>             CreateResourceSet(const RHIPipelineBindingLayout* inLayout) = 0;
    
    virtual void AddQueueWaitForSemaphore(ERHICommandQueueType inType, RefCountPtr<RHISemaphore>& inSemaphore) = 0;
    virtual void AddQueueSignalSemaphore(ERHICommandQueueType inType, RefCountPtr<RHISemaphore>& inSemaphore) = 0;
    virtual void ExecuteCommandList(const RefCountPtr<RHICommandList>& inCommandList, const RefCountPtr<RHIFence>& inSignalFence = nullptr) = 0;
};