#pragma once
#include <memory>
#include "RHIDefinitions.h"

class RHIComputePipeline;
class RHIShader;
class RHIPipelineBindingLayout;
class RHICommandList;
struct RHIPipelineBindingLayoutDesc;
struct RHIComputePipelineDesc;

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
};