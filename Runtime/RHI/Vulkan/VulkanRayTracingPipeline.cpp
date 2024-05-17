#include "VulkanPipelineState.h"
#include "VulkanDevice.h"
#include "../../Core/Log.h"
#include "../../Core/Templates.h"

VulkanRayTracingPipeline::VulkanRayTracingPipeline(VulkanDevice& inDevice, const RHIRayTracingPipelineDesc& inDesc)
    : m_Device(inDevice)
    , m_Desc(inDesc)
    , m_ShaderTable(nullptr)
    , m_PipelineState(VK_NULL_HANDLE)
    , m_PipelineProperties{}
{
    
}

VulkanRayTracingPipeline::~VulkanRayTracingPipeline()
{
    ShutdownInternal();
}

bool VulkanRayTracingPipeline::Init()
{
    if(IsValid())
    {
        Log::Warning("[Vulkan] The ray tracing pipeline already initialized");
        return true;
    }

    const VulkanPipelineBindingLayout* bindingLayout = CheckCast<VulkanPipelineBindingLayout*>(m_Desc.GlobalBindingLayout.get());
    if(bindingLayout == nullptr || !bindingLayout->IsValid())
    {
        Log::Error("[Vulkan] The ray tracing pipeline binding layout is invalid");
        return false;
    }
    
    VkPipelineShaderStageCreateInfo shaderStage{};
    VkShaderModule shaderModule = VK_NULL_HANDLE;

    // RayGen
    if(!m_Desc.RayGenShaderGroup.RayGenShader || !m_Desc.RayGenShaderGroup.RayGenShader->IsValid())
    {
        Log::Error("[Vulkan] The rayGen shader is invalid");
        return false;
    }
    if(!RHI::Vulkan::CreateShaderShage(m_Device.GetDevice(), m_Desc.RayGenShaderGroup.RayGenShader, VK_SHADER_STAGE_RAYGEN_BIT_KHR, shaderModule, shaderStage))
    {
        Log::Error("[Vulkan] Failed to create rayGen shader stage");
        return false;
    }
    m_ShaderStages.push_back(shaderStage);
    m_ShaderModules.push_back(shaderModule);
    shaderModule = VK_NULL_HANDLE;
    m_ShaderGroups.push_back({
        VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
        nullptr,
        VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
        0,
        VK_SHADER_UNUSED_KHR,
        VK_SHADER_UNUSED_KHR,
        VK_SHADER_UNUSED_KHR,
        nullptr
    });

    // RayMiss
    for(uint32_t i = 0; i < m_Desc.MissShaderGroups.size(); i++)
    {
        const RHIMissShaderGroup& missShaderGroup = m_Desc.MissShaderGroups[i];
        if(missShaderGroup.MissShader && missShaderGroup.MissShader->IsValid())
        {
            if(!RHI::Vulkan::CreateShaderShage(m_Device.GetDevice(), missShaderGroup.MissShader, VK_SHADER_STAGE_MISS_BIT_KHR, shaderModule, shaderStage))
            {
                Log::Error("[Vulkan] Failed to create rayGen shader stage");
                return false;
            }
            m_ShaderStages.push_back(shaderStage);
            m_ShaderModules.push_back(shaderModule);
            shaderModule = VK_NULL_HANDLE;

            const uint32_t groupIndex = static_cast<uint32_t>(m_ShaderStages.size()) - 1;
            m_ShaderGroups.push_back({
                VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
                nullptr,
                VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
                groupIndex,
                VK_SHADER_UNUSED_KHR,
                VK_SHADER_UNUSED_KHR,
                VK_SHADER_UNUSED_KHR,
                nullptr
            });
        }
    }

    // HitGroup
    for(uint32_t i = 0; i < m_Desc.HitGroups.size(); i++)
    {
        const RHIHitGroup& hitGroup = m_Desc.HitGroups[i];
        uint32_t clsShaderIndex = 0, anyShaderIndex = 0, interShaderIndiex = 0;
        if(hitGroup.ClosestHitShader && hitGroup.ClosestHitShader->IsValid())
        {
            if(!RHI::Vulkan::CreateShaderShage(m_Device.GetDevice(), hitGroup.ClosestHitShader, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, shaderModule, shaderStage))
            {
                Log::Error("[Vulkan] Failed to create closest hit shader stage");
                return false;
            }
            m_ShaderStages.push_back(shaderStage);
            m_ShaderModules.push_back(shaderModule);
            shaderModule = VK_NULL_HANDLE;
            clsShaderIndex = static_cast<uint32_t>(m_ShaderStages.size()) - 1;
        }

        if(hitGroup.AnyHitShader && hitGroup.AnyHitShader->IsValid())
        {
            if(!RHI::Vulkan::CreateShaderShage(m_Device.GetDevice(), hitGroup.AnyHitShader, VK_SHADER_STAGE_ANY_HIT_BIT_KHR, shaderModule, shaderStage))
            {
                Log::Error("[Vulkan] Failed to create any hit shader stage");
                return false;
            }
            m_ShaderStages.push_back(shaderStage);
            m_ShaderModules.push_back(shaderModule);
            shaderModule = VK_NULL_HANDLE;
            anyShaderIndex = static_cast<uint32_t>(m_ShaderStages.size()) - 1;
        }

        if(hitGroup.IntersectionShader && hitGroup.IntersectionShader->IsValid())
        {
            if(!RHI::Vulkan::CreateShaderShage(m_Device.GetDevice(), hitGroup.IntersectionShader, VK_SHADER_STAGE_INTERSECTION_BIT_KHR, shaderModule, shaderStage))
            {
                Log::Error("[Vulkan] Failed to create intersection shader stage");
                return false;
            }
            m_ShaderStages.push_back(shaderStage);
            m_ShaderModules.push_back(shaderModule);
            shaderModule = VK_NULL_HANDLE;
            interShaderIndiex = static_cast<uint32_t>(m_ShaderStages.size()) - 1;
        }

        clsShaderIndex = clsShaderIndex > 0 ? clsShaderIndex : VK_SHADER_UNUSED_KHR;
        anyShaderIndex = anyShaderIndex > 0 ? anyShaderIndex : VK_SHADER_UNUSED_KHR;
        interShaderIndiex = interShaderIndiex > 0 ? interShaderIndiex : VK_SHADER_UNUSED_KHR;

        VkRayTracingShaderGroupTypeKHR shaderGroupType = hitGroup.isProceduralPrimitive ?
            VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR :
            VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
        
        m_ShaderGroups.push_back({
           VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
           nullptr,
           shaderGroupType,
           VK_SHADER_UNUSED_KHR,
           clsShaderIndex,
           anyShaderIndex,
           interShaderIndiex,
           nullptr
        });
    }

    VkRayTracingPipelineCreateInfoKHR pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
    pipelineInfo.stageCount = static_cast<uint32_t>(m_ShaderStages.size());
    pipelineInfo.pStages = m_ShaderStages.data();
    pipelineInfo.groupCount = static_cast<uint32_t>(m_ShaderGroups.size());
    pipelineInfo.pGroups = m_ShaderGroups.data();
    pipelineInfo.maxPipelineRayRecursionDepth = m_Desc.MaxRecursionDepth;
    pipelineInfo.layout = bindingLayout->GetPipelineLayout();
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    VkResult result = m_Device.vkCreateRayTracingPipelinesKHR(m_Device.GetDevice(), VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_PipelineState);
    if(result != VK_SUCCESS)
    {
        OUTPUT_VULKAN_FAILED_RESULT(result)
        return false;
    }
    
    return true;
}

void VulkanRayTracingPipeline::Shutdown()
{
    ShutdownInternal();
}

bool VulkanRayTracingPipeline::IsValid() const
{
    return m_PipelineState != VK_NULL_HANDLE;
}

void VulkanRayTracingPipeline::ShutdownInternal()
{
    if(m_PipelineState != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(m_Device.GetDevice(), m_PipelineState, nullptr);
        m_PipelineState = VK_NULL_HANDLE;
    }

    for(auto shaderModule : m_ShaderModules)
    {
        if(shaderModule != VK_NULL_HANDLE)
        {
            vkDestroyShaderModule(m_Device.GetDevice(), shaderModule, nullptr);
        }
    }
    m_ShaderModules.clear();
}

void VulkanRayTracingPipeline::SetNameInternal()
{
    if(IsValid())
    {
        m_Device.SetDebugName(VK_OBJECT_TYPE_PIPELINE, (uint64_t)m_PipelineState, m_Name);
    }
}
