#include "VulkanPipelineState.h"
#include "VulkanDevice.h"
#include "../../Core/Log.h"
#include "../../Core/Templates.h"

RefCountPtr<RHIShader> VulkanDevice::CreateShader(ERHIShaderType inType)
{
    RefCountPtr<RHIShader> shader(new VulkanShader(inType));
    if(!shader->Init())
    {
        Log::Error("[Vulkan] Failed to create shader");
    }
    return shader;
}

RefCountPtr<RHIComputePipeline> VulkanDevice::CreatePipeline(const RHIComputePipelineDesc& inDesc)
{
    RefCountPtr<RHIComputePipeline> pipeline(new VulkanComputePipeline(*this, inDesc));
    if(!pipeline->Init())
    {
        Log::Error("[Vulkan] Failed to create compute pipeline");
    }
    return pipeline;
}

VulkanComputePipeline::VulkanComputePipeline(VulkanDevice& inDevice, const RHIComputePipelineDesc& inDesc)
    : m_Device(inDevice), m_Desc(inDesc), m_PipelineState(VK_NULL_HANDLE), m_ComputeShaderModule(VK_NULL_HANDLE)
{
    
}

VulkanComputePipeline::~VulkanComputePipeline()
{
    ShutdownInternal();
}

bool VulkanComputePipeline::Init()
{
    if(IsValid())
    {
        Log::Warning("[Vulkan] The compute pipeline already initialized");
        return true;
    }

    if(m_Desc.ComputeShader == nullptr || !m_Desc.ComputeShader->IsValid())
    {
        Log::Error("[Vulkan] The compute shader is invalid");
        return false;
    }

    VulkanPipelineBindingLayout* bindingLayout = CheckCast<VulkanPipelineBindingLayout*>(m_Desc.BindingLayout);
    if(bindingLayout == nullptr || !bindingLayout->IsValid())
    {
        Log::Error("[Vulkan] The compute pipeline binding layout is invalid");
        return false;
    }

    if(!RHI::Vulkan::CreateShaderModule(m_Desc.ComputeShader->GetByteCode(), m_Device.GetDevice(), m_ComputeShaderModule))
    {
        return false;
    }

    VkComputePipelineCreateInfo computePipelineInfo{};
    computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    computePipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computePipelineInfo.stage.module = m_ComputeShaderModule;
    computePipelineInfo.stage.pName = m_Desc.ComputeShader->GetEntryName().c_str();
    computePipelineInfo.stage.pSpecializationInfo = nullptr;
    computePipelineInfo.stage.flags = 0;
    computePipelineInfo.stage.pNext = nullptr;
    computePipelineInfo.layout = bindingLayout->GetPipelineLayout();
    computePipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    computePipelineInfo.basePipelineIndex = -1;
    VkResult result = vkCreateComputePipelines(m_Device.GetDevice(), VK_NULL_HANDLE, 1,  &computePipelineInfo, nullptr, &m_PipelineState);
    if(result != VK_SUCCESS)
    {
        OUTPUT_VULKAN_FAILED_RESULT(result)
        return false;
    }
	
    return true;
}

void VulkanComputePipeline::Shutdown()
{
    ShutdownInternal();
}

bool VulkanComputePipeline::IsValid() const
{
    return m_PipelineState != VK_NULL_HANDLE;
}

void VulkanComputePipeline::SetNameInternal()
{
    if(IsValid())
    {
        m_Device.SetDebugName(VK_OBJECT_TYPE_PIPELINE, (uint64_t)m_PipelineState, m_Name);
    }
}

void VulkanComputePipeline::ShutdownInternal()
{
    if(m_ComputeShaderModule != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(m_Device.GetDevice(), m_ComputeShaderModule, nullptr);
        m_ComputeShaderModule = VK_NULL_HANDLE;
    }
    
    if(m_PipelineState != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(m_Device.GetDevice(), m_PipelineState, nullptr);
        m_PipelineState = VK_NULL_HANDLE;
    }
}
