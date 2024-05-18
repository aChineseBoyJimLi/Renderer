#include "VulkanPipelineState.h"
#include "VulkanDevice.h"
#include "VulkanResources.h"
#include "../../Core/Log.h"
#include "../../Core/Templates.h"

RefCountPtr<RHIGraphicsPipeline> VulkanDevice::CreatePipeline(const RHIGraphicsPipelineDesc& inDesc)
{
    RefCountPtr<RHIGraphicsPipeline> pipeline(new VulkanGraphicsPipeline(*this, inDesc));
    if(!pipeline->Init())
    {
        Log::Error("[Vulkan] Failed to create graphics pipeline");
    }
    return pipeline;
}

VulkanGraphicsPipeline::VulkanGraphicsPipeline(VulkanDevice& inDevice, const RHIGraphicsPipelineDesc& inDesc)
    : m_Device(inDevice)
    , m_Desc(inDesc)
    , m_VertexShaderModule(VK_NULL_HANDLE)
    , m_FragmentShaderModule(VK_NULL_HANDLE)
    , m_GeometryShaderModule(VK_NULL_HANDLE)
    , m_TessControlShaderModule(VK_NULL_HANDLE)
    , m_TessEvalShaderModule(VK_NULL_HANDLE)
    , m_TaskShaderModule(VK_NULL_HANDLE)
    , m_MeshShaderModule(VK_NULL_HANDLE)
    , m_RenderPass(VK_NULL_HANDLE)
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

    const VulkanPipelineBindingLayout* bindingLayout = CheckCast<VulkanPipelineBindingLayout*>(m_Desc.BindingLayout);
    if(bindingLayout == nullptr || !bindingLayout->IsValid())
    {
        Log::Error("[Vulkan] The graphics pipeline binding layout is invalid");
        return false;
    }

    m_ShaderStages.clear();

    if(!m_Desc.PixelShader ||  !m_Desc.PixelShader->IsValid())
    {
        Log::Error("[Vulkan] The graphics pipeline must have a fragment shader");
        return false;
    }
    VkPipelineShaderStageCreateInfo shaderStage{};
    

    if(!RHI::Vulkan::CreateShaderShage(m_Device.GetDevice(), m_Desc.PixelShader, VK_SHADER_STAGE_FRAGMENT_BIT, m_FragmentShaderModule, shaderStage))
    {
        Log::Error("[Vulkan] Failed to create fragment shader stage");
        return false;
    }
    m_ShaderStages.push_back(shaderStage);

    if(m_Desc.UsingMeshShader)
    {
        if(!m_Desc.MeshShader  || !m_Desc.MeshShader->IsValid() )
        {
            Log::Error("[Vulkan] The mesh pipeline must have a mesh  shader");
            return false;
        }
        if(!RHI::Vulkan::CreateShaderShage(m_Device.GetDevice(), m_Desc.MeshShader, VK_SHADER_STAGE_MESH_BIT_EXT, m_MeshShaderModule, shaderStage))
        {
            Log::Error("[Vulkan] Failed to create mesh shader stage");
            return false;
        }
        if(m_Desc.AmplificationShader && m_Desc.AmplificationShader->IsValid())
        {
            if(!RHI::Vulkan::CreateShaderShage(m_Device.GetDevice(), m_Desc.AmplificationShader, VK_SHADER_STAGE_TASK_BIT_EXT, m_TaskShaderModule, shaderStage))
            {
                Log::Error("[Vulkan] Failed to create mesh shader stage");
                return false;
            }
            m_ShaderStages.push_back(shaderStage);
        }
        m_ShaderStages.push_back(shaderStage);
    }
    else
    {
        if(!m_Desc.VertexShader || !m_Desc.VertexShader->IsValid() )
        {
            Log::Error("[Vulkan] The graphics pipeline must have a vertex shader");
            return false;
        }
        if(!RHI::Vulkan::CreateShaderShage(m_Device.GetDevice(), m_Desc.VertexShader, VK_SHADER_STAGE_VERTEX_BIT, m_VertexShaderModule, shaderStage))
        {
            Log::Error("[Vulkan] Failed to create vertex shader stage");
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
    }

    if(!InitRenderPass())
    {
        Log::Error("[Vulkan] Failed to create render pass");
        return false;
    }
    
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.pStages = m_ShaderStages.data();
    pipelineInfo.stageCount = (uint32_t)m_ShaderStages.size();
    pipelineInfo.layout = bindingLayout->GetPipelineLayout();
    pipelineInfo.renderPass = m_RenderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    VkPipelineVertexInputStateCreateInfo vertexInputState{};
    VkVertexInputBindingDescription bindingDescription{};
    std::vector<VkVertexInputAttributeDescription> attributes;
    if(!m_Desc.UsingMeshShader && m_Desc.VertexInputLayout && !m_Desc.VertexInputLayout->Items.empty())
    {
        bindingDescription.binding = 0;
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        bindingDescription.stride = InitVertexInputAttributes(attributes);
        
        vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputState.vertexAttributeDescriptionCount = (uint32_t) attributes.size();
        vertexInputState.pVertexAttributeDescriptions = attributes.data();
        vertexInputState.vertexBindingDescriptionCount = 1;
        vertexInputState.pVertexBindingDescriptions = &bindingDescription;
        
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = RHI::Vulkan::ConvertPrimitiveTopology(m_Desc.PrimitiveType);
        inputAssembly.primitiveRestartEnable = VK_FALSE;
        pipelineInfo.pVertexInputState = &vertexInputState;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
    }
    else
    {
        pipelineInfo.pVertexInputState = nullptr;
        pipelineInfo.pInputAssemblyState = nullptr;
    }
    
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
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
    pipelineInfo.pRasterizationState = &rasterizerState;

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

bool VulkanGraphicsPipeline::InitRenderPass()
{
    const bool hasDepthStencil = m_Desc.DepthStencilState.DepthTestEnable || m_Desc.DepthStencilState.DepthWriteEnable;
    const uint32_t renderTargetsCount = m_Desc.NumRenderTarget;
    const uint32_t attachmentCount = hasDepthStencil ? renderTargetsCount + 1 : renderTargetsCount;
    
    std::vector<VkAttachmentDescription> attachmentDescs(attachmentCount);
    std::vector<VkAttachmentReference> colorAttachmentRefs(renderTargetsCount);

    VkSampleCountFlagBits sampleCount = RHI::Vulkan::ConvertSampleBits(m_Desc.SampleCount);
    for(uint32_t i = 0; i < renderTargetsCount; i++)
    {
        attachmentDescs[i].samples = sampleCount;
        attachmentDescs[i].format = RHI::Vulkan::ConvertFormat(m_Desc.RTVFormats[i]);
        attachmentDescs[i].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        // attachmentDescs[i].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachmentDescs[i].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentDescs[i].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachmentDescs[i].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        
        colorAttachmentRefs[i].attachment = i;
        colorAttachmentRefs[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    VkAttachmentReference depthStencilAttachmentRef{};
    if(hasDepthStencil)
    {
        const RHIFormatInfo& formatInfo = RHI::GetFormatInfo(m_Desc.DSVFormat);
        const uint32_t depthStencilAttachmentIndex = renderTargetsCount;
        attachmentDescs[depthStencilAttachmentIndex].samples = sampleCount;
        attachmentDescs[depthStencilAttachmentIndex].format = RHI::Vulkan::ConvertFormat(m_Desc.DSVFormat);
        attachmentDescs[depthStencilAttachmentIndex].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachmentDescs[depthStencilAttachmentIndex].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachmentDescs[depthStencilAttachmentIndex].stencilLoadOp = formatInfo.HasStencil ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachmentDescs[depthStencilAttachmentIndex].stencilStoreOp = formatInfo.HasStencil ? VK_ATTACHMENT_STORE_OP_STORE : VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachmentDescs[depthStencilAttachmentIndex].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachmentDescs[depthStencilAttachmentIndex].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthStencilAttachmentRef.attachment = depthStencilAttachmentIndex;
        depthStencilAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    // Only one subpass for now
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachmentRefs.size());
    subpass.pColorAttachments = colorAttachmentRefs.data();
    subpass.pDepthStencilAttachment = hasDepthStencil ? &depthStencilAttachmentRef : nullptr;
    VkRenderPassCreateInfo renderPassCreateInfo{};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachmentDescs.size());
    renderPassCreateInfo.pAttachments = attachmentDescs.data();
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;
    renderPassCreateInfo.dependencyCount = 0;
    renderPassCreateInfo.pDependencies = nullptr;

    VkResult result = vkCreateRenderPass(m_Device.GetDevice(), &renderPassCreateInfo, nullptr, &m_RenderPass);
    if(result != VK_SUCCESS)
    {
        OUTPUT_VULKAN_FAILED_RESULT(result)
        return false;
    }

    return true;
}

uint32_t VulkanGraphicsPipeline::InitVertexInputAttributes(std::vector<VkVertexInputAttributeDescription>& outAttributes) const
{
    uint32_t stride = 0;
    if(m_Desc.VertexInputLayout && !m_Desc.VertexInputLayout->Items.empty())
    {
        outAttributes.resize(m_Desc.VertexInputLayout->Items.size());
        uint32_t alignedByteOffset = 0;
        for(uint32_t i = 0; i < m_Desc.VertexInputLayout->Items.size(); ++i)
        {
            const RHIVertexInputItem& item = m_Desc.VertexInputLayout->Items[i];
            outAttributes[i].binding = item.SemanticIndex;
            outAttributes[i].location = i;
            outAttributes[i].format = RHI::Vulkan::ConvertFormat(item.Format);
            outAttributes[i].offset = alignedByteOffset;

            const RHIFormatInfo& formatInfo = RHI::GetFormatInfo(m_Desc.VertexInputLayout->Items[i].Format);
            alignedByteOffset += formatInfo.BytesPerBlock;
        }
        stride = alignedByteOffset;
    }
    else
    {
        outAttributes.clear();
    }
    return stride;
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

    if(m_RenderPass != VK_NULL_HANDLE)
    {
        vkDestroyRenderPass(m_Device.GetDevice(), m_RenderPass, nullptr);
        m_RenderPass = VK_NULL_HANDLE;
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