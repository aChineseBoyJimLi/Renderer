#include "VulkanPipelineState.h"
#include "VulkanDevice.h"
#include "../../Core/Log.h"
#include <map>

std::shared_ptr<RHIPipelineBindingLayout> VulkanDevice::CreatePipelineBindingLayout(const RHIPipelineBindingLayoutDesc& inBindingItems)
{
    std::shared_ptr<RHIPipelineBindingLayout> layout(new VulkanPipelineBindingLayout(*this, inBindingItems));
    if(!layout->Init())
    {
        Log::Error("[Vulkan] Failed to create pipeline binding layout");
    }
    return layout;
}

VulkanPipelineBindingLayout::VulkanPipelineBindingLayout(VulkanDevice& inDevice, const RHIPipelineBindingLayoutDesc& inBindingItems)
    : m_Device(inDevice)
    , m_BindingItems(inBindingItems)
    , m_PipelineLayout(VK_NULL_HANDLE)
{
    
}

VulkanPipelineBindingLayout::~VulkanPipelineBindingLayout()
{
    ShutdownInternal();
}

bool VulkanPipelineBindingLayout::Init()
{
    if(IsValid())
    {
        Log::Warning("[Vulkan] The pipeline binding layout already initialized");
        return true;
    }

    m_DescriptorSetLayouts.clear();
    
    std::map<uint32_t, std::vector<std::pair<RHIPipelineBindingItem, VkDescriptorSetLayoutBinding>>> descriptorSetLayouts;
    for(const auto& bindingItem : m_BindingItems.Items)
    {
        descriptorSetLayouts[bindingItem.Space].emplace_back(std::make_pair(bindingItem
            , RHI::Vulkan::ConvertBindingLayoutItem(bindingItem)));
    }

    std::vector<VkDescriptorSetLayoutBinding> vkBindings;
    std::vector<VkDescriptorBindingFlagsEXT> bindingFlags;

    for(auto descriptorSet : descriptorSetLayouts)
    {
        const uint32_t bindingCount = (uint32_t)descriptorSet.second.size();

        vkBindings.resize(bindingCount);
        bindingFlags.resize(bindingCount);
        
        bool hasBindless = false;
        
        for(uint32_t i = 0; i < bindingCount; ++i)
        {
            const auto& bindingPair = descriptorSet.second[i];

            if(bindingPair.first.IsBindless)
            {
                bindingFlags[i] = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;
                hasBindless = true;
            }
            else
            {
                bindingFlags[i] = 0;
            }
            vkBindings[i] = descriptorSet.second[i].second;
        }

        VkDescriptorSetLayoutBindingFlagsCreateInfoEXT bindingFlagsInfo{};
        bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
        bindingFlagsInfo.bindingCount = bindingCount;
        bindingFlagsInfo.pBindingFlags = bindingFlags.data();
        
        VkDescriptorSetLayoutCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        createInfo.bindingCount = bindingCount;
        createInfo.pBindings = vkBindings.data();
        createInfo.pNext = hasBindless ?  &bindingFlagsInfo : nullptr;

        VkDescriptorSetLayout layout;
        VkResult result = vkCreateDescriptorSetLayout(m_Device.GetDevice(), &createInfo, nullptr, &layout);
        if(result != VK_SUCCESS)
        {
            OUTPUT_VULKAN_FAILED_RESULT(result);
            return false;
        }
        
        m_DescriptorSetLayouts.push_back(layout);
    }

    if(m_DescriptorSetLayouts.empty())
    {
        Log::Error("[Vulkan] Failed to create descriptor set layout");
        return false;
    }

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = (uint32_t)m_DescriptorSetLayouts.size();
    pipelineLayoutInfo.pSetLayouts = m_DescriptorSetLayouts.data();
    VkResult result = vkCreatePipelineLayout(m_Device.GetDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout);
    if(result != VK_SUCCESS)
    {
        OUTPUT_VULKAN_FAILED_RESULT(result);
        return false;
    }
    
    return true;
}

void VulkanPipelineBindingLayout::Shutdown()
{
    ShutdownInternal();
}

bool VulkanPipelineBindingLayout::IsValid() const
{
    bool bValid = m_PipelineLayout != VK_NULL_HANDLE;
    bValid = bValid && !m_DescriptorSetLayouts.empty();
    return bValid;
}

void VulkanPipelineBindingLayout::ShutdownInternal()
{
    if(m_PipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(m_Device.GetDevice(), m_PipelineLayout, nullptr);
        m_PipelineLayout = VK_NULL_HANDLE;
    }

    for(auto descriptorSetLayout : m_DescriptorSetLayouts)
    {
        if(descriptorSetLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(m_Device.GetDevice(), descriptorSetLayout, nullptr);
        }
    }
    m_DescriptorSetLayouts.clear();
}

void VulkanPipelineBindingLayout::SetNameInternal()
{
    if(IsValid())
    {
        m_Device.SetDebugName(VK_OBJECT_TYPE_PIPELINE_LAYOUT, reinterpret_cast<uint64_t>(m_PipelineLayout), m_Name);
    }
}
