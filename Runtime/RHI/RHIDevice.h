#pragma once

#include "RHIDefinitions.h"

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
    virtual std::shared_ptr<RHIFence>       CreateRhiFence() = 0;
    virtual std::shared_ptr<RHISemaphore>   CreateRhiSemaphore() = 0;
    virtual std::shared_ptr<RHICommandList> CreateCommandList(ERHICommandQueueType inType = ERHICommandQueueType::Direct) = 0;
    virtual std::shared_ptr<RHIPipelineBindingLayout> CreatePipelineBindingLayout(const RHIPipelineBindingLayoutDesc& inBindingItems) = 0;
    virtual std::shared_ptr<RHIShader> CreateShader(ERHIShaderType inType) = 0;
    virtual std::shared_ptr<RHIComputePipeline> CreateComputePipeline(const RHIComputePipelineDesc& inDesc) = 0;
    virtual std::shared_ptr<RHIResourceHeap> CreateResourceHeap(const RHIResourceHeapDesc& inDesc) = 0;
    virtual std::shared_ptr<RHIBuffer> CreateBuffer(const RHIBufferDesc& inDesc, bool isVirtual = false) = 0;
    virtual std::shared_ptr<RHITexture> CreateTexture(const RHITextureDesc& inDesc, bool isVirtual = false) = 0;
    virtual std::shared_ptr<RHIFrameBuffer> CreateFrameBuffer(const RHIFrameBufferDesc& inDesc) = 0;
    virtual void ExecuteCommandList(const std::shared_ptr<RHICommandList>& inCommandList, const std::shared_ptr<RHIFence>& inSignalFence = nullptr,
                            const std::vector<std::shared_ptr<RHISemaphore>>* inWaitForSemaphores = nullptr, 
                            const std::vector<std::shared_ptr<RHISemaphore>>* inSignalSemaphores = nullptr) = 0;
};