#include "VulkanPipelineState.h"
#include "VulkanDevice.h"
#include "VulkanResources.h"
#include "../../Core/Log.h"
#include "../../Core/Templates.h"

VulkanMeshPipeline::VulkanMeshPipeline(VulkanDevice& inDevice, const RHIMeshPipelineDesc& inDesc)
    : m_Device(inDevice)
    , m_Desc(inDesc)
    , m_TaskShaderModule(VK_NULL_HANDLE)
    , m_MeshShaderModule(VK_NULL_HANDLE)
    , m_FragmentShaderModule(VK_NULL_HANDLE)
    , m_PipelineState(VK_NULL_HANDLE)
{
    
}

VulkanMeshPipeline::~VulkanMeshPipeline()
{
    ShutdownInternal();
}

bool VulkanMeshPipeline::Init()
{
    if(IsValid())
    {
        Log::Warning("[Vulkan] The mesh pipeline already initialized");
        return true;
    }

    VulkanPipelineBindingLayout* bindingLayout = CheckCast<VulkanPipelineBindingLayout*>(m_Desc.BindingLayout.get());
    if(bindingLayout == nullptr || !bindingLayout->IsValid())
    {
        Log::Error("[Vulkan] The mesh pipeline binding layout is invalid");
        return false;
    }

    m_ShaderStages.clear();

    if(!m_Desc.MeshShader || !m_Desc.PixelShader || !m_Desc.MeshShader->IsValid() || !m_Desc.PixelShader->IsValid())
    {
        Log::Error("[Vulkan] The mesh pipeline must have a mesh and a fragment shader");
        return false;
    }

    VkPipelineShaderStageCreateInfo shaderStage{};
    if(!RHI::Vulkan::CreateShaderShage(m_Device.GetDevice(), m_Desc.MeshShader, VK_SHADER_STAGE_MESH_BIT_EXT, m_MeshShaderModule, shaderStage))
    {
        Log::Error("[Vulkan] Failed to create mesh shader stage");
        return false;
    }
    m_ShaderStages.push_back(shaderStage);

    if(!RHI::Vulkan::CreateShaderShage(m_Device.GetDevice(), m_Desc.PixelShader, VK_SHADER_STAGE_FRAGMENT_BIT, m_FragmentShaderModule, shaderStage))
    {
        Log::Error("[Vulkan] Failed to create fragment shader stage");
        return false;
    }
    m_ShaderStages.push_back(shaderStage);

    if(m_Desc.AmplificationShader && m_Desc.AmplificationShader->IsValid())
    {
        if(!RHI::Vulkan::CreateShaderShage(m_Device.GetDevice(), m_Desc.AmplificationShader, VK_SHADER_STAGE_TASK_BIT_EXT, m_TaskShaderModule, shaderStage))
        {
            Log::Error("[Vulkan] Failed to create mesh shader stage");
            return false;
        }
        m_ShaderStages.push_back(shaderStage);
    }

    const VulkanFrameBuffer* frameBuffer = CheckCast<VulkanFrameBuffer*>(m_Desc.FrameBuffer.get());
    if(frameBuffer == nullptr || !frameBuffer->IsValid())
    {
        Log::Error("[Vulkan] The graphics pipeline frame buffer is invalid");
        return false;
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = bindingLayout->GetPipelineLayout();
    pipelineInfo.renderPass = frameBuffer->GetRenderPass();
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    VkPipelineViewportStateCreateInfo viewportState;
    viewportState.scissorCount = 1;
    viewportState.viewportCount = 1;
    pipelineInfo.pViewportState = &viewportState;

    VkPipelineDepthStencilStateCreateInfo depthStencilCreanInfo{};
    RHI::Vulkan::TranslateDepthStencilState(m_Desc.DepthStencilState, depthStencilCreanInfo);
    pipelineInfo.pDepthStencilState = &depthStencilCreanInfo;

    VkPipelineColorBlendStateCreateInfo blendStateCreateInfo{};
    RHI::Vulkan::TranslateBlendState(m_Desc.BlendState, blendStateCreateInfo);
    pipelineInfo.pColorBlendState = &blendStateCreateInfo;

    VkPipelineRasterizationStateCreateInfo rasterizerState{};
    RHI::Vulkan::TranslateRasterizerState(m_Desc.RasterizerState, rasterizerState);
    VkPipelineRasterizationConservativeStateCreateInfoEXT conservativeRasterInfo{};
    conservativeRasterInfo.conservativeRasterizationMode = VK_CONSERVATIVE_RASTERIZATION_MODE_OVERESTIMATE_EXT;
    if(m_Desc.RasterizerState.ConservativeRasterEnable)
    {
        rasterizerState.pNext = &conservativeRasterInfo;
    }

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();
    pipelineInfo.pDynamicState = &dynamicState;

    
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = m_Desc.SampleCount > 1 ? VK_TRUE : VK_FALSE;
    multisampling.rasterizationSamples = RHI::Vulkan::ConvertSampleBits(m_Desc.SampleCount);
    pipelineInfo.pMultisampleState = &multisampling;
    
    VkResult result = vkCreateGraphicsPipelines(m_Device.GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_PipelineState);
    if(result != VK_SUCCESS)
    {
        OUTPUT_VULKAN_FAILED_RESULT(result)
        return false;
    }
    
    return true;
}

void VulkanMeshPipeline::Shutdown()
{
    ShutdownInternal();
}

bool VulkanMeshPipeline::IsValid() const
{
    return m_PipelineState != VK_NULL_HANDLE;
}

void VulkanMeshPipeline::ShutdownInternal()
{
    if(m_PipelineState != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(m_Device.GetDevice(), m_PipelineState, nullptr);
        m_PipelineState = VK_NULL_HANDLE;
    }

    if(m_FragmentShaderModule != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(m_Device.GetDevice(), m_FragmentShaderModule, nullptr);
        m_FragmentShaderModule = VK_NULL_HANDLE;
    }

    if(m_MeshShaderModule != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(m_Device.GetDevice(), m_MeshShaderModule, nullptr);
        m_MeshShaderModule = VK_NULL_HANDLE;
    }

    if(m_TaskShaderModule != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(m_Device.GetDevice(), m_TaskShaderModule, nullptr);
        m_TaskShaderModule = VK_NULL_HANDLE;
    }
}

void VulkanMeshPipeline::SetNameInternal()
{
    if(IsValid())
    {
        m_Device.SetDebugName(VK_OBJECT_TYPE_PIPELINE, (uint64_t)m_PipelineState, m_Name);
    }
}
