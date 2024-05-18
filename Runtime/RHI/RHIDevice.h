#pragma once

#include "RHIDefinitions.h"

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
    virtual ERHIBackend GetBackend() const = 0;
    virtual RefCountPtr<RHIFence>       CreateRhiFence() = 0;
    virtual RefCountPtr<RHISemaphore>   CreateRhiSemaphore() = 0;
    virtual RefCountPtr<RHICommandList> CreateCommandList(ERHICommandQueueType inType = ERHICommandQueueType::Direct) = 0;
    virtual RefCountPtr<RHIPipelineBindingLayout> CreatePipelineBindingLayout(const RHIPipelineBindingLayoutDesc& inBindingItems) = 0;
    virtual RefCountPtr<RHIShader> CreateShader(ERHIShaderType inType) = 0;
    virtual RefCountPtr<RHIComputePipeline> CreatePipeline(const RHIComputePipelineDesc& inDesc) = 0;
    virtual RefCountPtr<RHIGraphicsPipeline> CreatePipeline(const RHIGraphicsPipelineDesc& inDesc) = 0;
    virtual RefCountPtr<RHIResourceHeap> CreateResourceHeap(const RHIResourceHeapDesc& inDesc) = 0;
    virtual RefCountPtr<RHIBuffer> CreateBuffer(const RHIBufferDesc& inDesc, bool isVirtual = false) = 0;
    virtual RefCountPtr<RHITexture> CreateTexture(const RHITextureDesc& inDesc, bool isVirtual = false) = 0;
    virtual RefCountPtr<RHIFrameBuffer> CreateFrameBuffer(const RHIFrameBufferDesc& inDesc) = 0;
    virtual void ExecuteCommandList(const RefCountPtr<RHICommandList>& inCommandList, const RefCountPtr<RHIFence>& inSignalFence = nullptr,
                            const std::vector<RefCountPtr<RHISemaphore>>* inWaitForSemaphores = nullptr, 
                            const std::vector<RefCountPtr<RHISemaphore>>* inSignalSemaphores = nullptr) = 0;
};