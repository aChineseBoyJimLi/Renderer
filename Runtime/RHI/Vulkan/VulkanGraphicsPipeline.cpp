#include "VulkanPipelineState.h"
#include "VulkanDevice.h"
#include "VulkanResources.h"
#include "../../Core/Log.h"
#include "../../Core/Templates.h"

VulkanVertexInputLayout::VulkanVertexInputLayout(const RHIVertexInputLayoutDesc& inDesc)
    : m_Desc(inDesc)
{
    m_Attributes.resize(m_Desc.Items.size());
    uint32_t alignedByteOffset = 0;
    for(uint32_t i = 0; i < m_Desc.Items.size(); ++i)
    {
        const RHIVertexInputItem& item = m_Desc.Items[i];
        m_Attributes[i].binding = item.SemanticIndex;
        m_Attributes[i].location = i;
        m_Attributes[i].format = RHI::Vulkan::ConvertFormat(item.Format);
        m_Attributes[i].offset = alignedByteOffset;

        const RHIFormatInfo& formatInfo = RHI::GetFormatInfo(m_Desc.Items[i].Format);
        alignedByteOffset += formatInfo.BlockSize;
    }
}

VulkanGraphicsPipeline::VulkanGraphicsPipeline(VulkanDevice& inDevice, const RHIGraphicsPipelineDesc& inDesc)
    : m_Device(inDevice)
    , m_Desc(inDesc)
    , m_VertexShaderModule(VK_NULL_HANDLE)
    , m_FragmentShaderModule(VK_NULL_HANDLE)
    , m_GeometryShaderModule(VK_NULL_HANDLE)
    , m_TessControlShaderModule(VK_NULL_HANDLE)
    , m_TessEvalShaderModule(VK_NULL_HANDLE)
    , m_PipelineState(VK_NULL_HANDLE)
{
    
}

VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
{
    ShutdownInternal();
}

bool VulkanGraphicsPipeline::Init()
{
    if(IsValid())
    {
        Log::Warning("[Vulkan] The graphics pipeline already initialized");
        return true;
    }

    const VulkanPipelineBindingLayout* bindingLayout = CheckCast<VulkanPipelineBindingLayout*>(m_Desc.BindingLayout.get());
    if(bindingLayout == nullptr || !bindingLayout->IsValid())
    {
        Log::Error("[Vulkan] The graphics pipeline binding layout is invalid");
        return false;
    }

    m_ShaderStages.clear();

    if(!m_Desc.VertexShader || !m_Desc.PixelShader || !m_Desc.VertexShader->IsValid() || !m_Desc.PixelShader->IsValid())
    {
        Log::Error("[Vulkan] The graphics pipeline must have a vertex and a fragment shader");
        return false;
    }

    VkPipelineShaderStageCreateInfo shaderStage{};
    if(!RHI::Vulkan::CreateShaderShage(m_Device.GetDevice(), m_Desc.VertexShader, VK_SHADER_STAGE_VERTEX_BIT, m_VertexShaderModule, shaderStage))
    {
        Log::Error("[Vulkan] Failed to create vertex shader stage");
        return false;
    }
    m_ShaderStages.push_back(shaderStage);

    if(!RHI::Vulkan::CreateShaderShage(m_Device.GetDevice(), m_Desc.PixelShader, VK_SHADER_STAGE_FRAGMENT_BIT, m_FragmentShaderModule, shaderStage))
    {
        Log::Error("[Vulkan] Failed to create fragment shader stage");
        return false;
    }
    m_ShaderStages.push_back(shaderStage);

    if(m_Desc.GeometryShader && m_Desc.GeometryShader->IsValid())
    {
        if(!RHI::Vulkan::CreateShaderShage(m_Device.GetDevice(), m_Desc.GeometryShader, VK_SHADER_STAGE_GEOMETRY_BIT, m_GeometryShaderModule, shaderStage))
        {
            Log::Error("[Vulkan] Failed to create geometry shader stage");
            return false;
        }
        m_ShaderStages.push_back(shaderStage);
    }
    
    if(m_Desc.HullShader && m_Desc.HullShader->IsValid())
    {
        if(!RHI::Vulkan::CreateShaderShage(m_Device.GetDevice(), m_Desc.HullShader, VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, m_TessControlShaderModule, shaderStage))
        {
            Log::Error("[Vulkan] Failed to create tessellation control shader stage");
            return false;
        }
        m_ShaderStages.push_back(shaderStage);
    }
    
    if(m_Desc.DomainShader && m_Desc.DomainShader->IsValid())
    {
        if(!RHI::Vulkan::CreateShaderShage(m_Device.GetDevice(), m_Desc.DomainShader, VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, m_TessEvalShaderModule, shaderStage))
        {
            Log::Error("[Vulkan] Failed to create tessellation evaluation shader stage");
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

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    if(m_Desc.VertexInputLayout)
    {
        const VulkanVertexInputLayout* inputLayout = CheckCast<VulkanVertexInputLayout*>(m_Desc.VertexInputLayout.get());
        if(inputLayout == nullptr)
        {
            Log::Error("[Vulkan] The graphics pipeline vertex input layout is invalid");
            return false;
        }
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = RHI::Vulkan::ConvertPrimitiveTopology(m_Desc.PrimitiveType);
        inputAssembly.primitiveRestartEnable = VK_FALSE;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
    }
    else
    {
        pipelineInfo.pVertexInputState = nullptr;
        pipelineInfo.pInputAssemblyState = nullptr;
    }
    
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

void VulkanGraphicsPipeline::Shutdown()
{
    ShutdownInternal();
}

void VulkanGraphicsPipeline::ShutdownInternal()
{
    if(m_PipelineState != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(m_Device.GetDevice(), m_PipelineState, nullptr);
        m_PipelineState = VK_NULL_HANDLE;
    }
    
    if(m_TessEvalShaderModule != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(m_Device.GetDevice(), m_TessEvalShaderModule, nullptr);
        m_TessEvalShaderModule = VK_NULL_HANDLE;
    }

    if(m_TessControlShaderModule != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(m_Device.GetDevice(), m_TessControlShaderModule, nullptr);
        m_TessControlShaderModule = VK_NULL_HANDLE;
    }

    if(m_GeometryShaderModule != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(m_Device.GetDevice(), m_GeometryShaderModule, nullptr);
        m_GeometryShaderModule = VK_NULL_HANDLE;
    }

    if(m_GeometryShaderModule != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(m_Device.GetDevice(), m_GeometryShaderModule, nullptr);
        m_GeometryShaderModule = VK_NULL_HANDLE;
    }

    if(m_FragmentShaderModule != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(m_Device.GetDevice(), m_FragmentShaderModule, nullptr);
        m_FragmentShaderModule = VK_NULL_HANDLE;
    }

    if(m_VertexShaderModule != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(m_Device.GetDevice(), m_VertexShaderModule, nullptr);
        m_VertexShaderModule = VK_NULL_HANDLE;
    }
}

bool VulkanGraphicsPipeline::IsValid() const
{
    return m_PipelineState != VK_NULL_HANDLE;
}

void VulkanGraphicsPipeline::SetNameInternal()
{
    if(IsValid())
    {
        m_Device.SetDebugName(VK_OBJECT_TYPE_PIPELINE, (uint64_t)m_PipelineState, m_Name);
    }
}