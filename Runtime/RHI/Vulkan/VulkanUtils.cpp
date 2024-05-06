#include "VulkanDefinitions.h"
#include "VulkanPipelineState.h"
#include "../../Core/Log.h"

namespace RHI::Vulkan
{
    uint32_t GetBindingSlot(ERHIBindingResourceType resourceType, uint32_t inRegisterSlot)
    {
        switch (resourceType)
        {
        case ERHIBindingResourceType::Buffer_SRV:
        case ERHIBindingResourceType::Texture_SRV:
            return SPIRV_SRV_BINDING_OFFSET + inRegisterSlot;
        case ERHIBindingResourceType::Buffer_UAV:
        case ERHIBindingResourceType::Texture_UAV:
        case ERHIBindingResourceType::AccelerationStructure:
            return SPIRV_UAV_BINDING_OFFSET + inRegisterSlot;
        case ERHIBindingResourceType::Buffer_CBV:
        // case ERHIBindingResourceType::PushConstants:
            return SPIRV_CBV_BINDING_OFFSET + inRegisterSlot;
        case ERHIBindingResourceType::Sampler:
            return SPIRV_SAMPLER_BINDING_OFFSET + inRegisterSlot;
        }
        return 0;
    }

    VkDescriptorType ConvertDescriptorType(ERHIBindingResourceType resourceType)
    {
        switch (resourceType)
        {
        case ERHIBindingResourceType::Buffer_SRV:
        case ERHIBindingResourceType::Buffer_UAV:
            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case ERHIBindingResourceType::Texture_SRV:
            return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case ERHIBindingResourceType::Texture_UAV:
            return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case ERHIBindingResourceType::Sampler:
            return VK_DESCRIPTOR_TYPE_SAMPLER;
        case ERHIBindingResourceType::AccelerationStructure:
            return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        case ERHIBindingResourceType::Buffer_CBV:
        // case ERHIBindingResourceType::PushConstants:
            return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        }
        return VK_DESCRIPTOR_TYPE_MAX_ENUM;
    }
    
    VkDescriptorSetLayoutBinding ConvertBindingLayoutItem(const RHIPipelineBindingItem& inItem)
    {
        VkDescriptorSetLayoutBinding layoutBinding = {};
        layoutBinding.binding = GetBindingSlot(inItem.Type, inItem.BaseRegister);
        layoutBinding.descriptorType = ConvertDescriptorType(inItem.Type);
        layoutBinding.descriptorCount = inItem.NumResources;
        layoutBinding.stageFlags = VK_SHADER_STAGE_ALL;
        layoutBinding.pImmutableSamplers = nullptr;
        return layoutBinding;
    }

    bool CreateShaderModule(const std::shared_ptr<Blob>& inByteCode, VkDevice inDevice, VkShaderModule& outShaderModule)
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = inByteCode->GetSize();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(inByteCode->GetData());

        VkResult result = vkCreateShaderModule(inDevice, &createInfo, nullptr, &outShaderModule);
        if (result != VK_SUCCESS)
        {
            OUTPUT_VULKAN_FAILED_RESULT(result)
            Log::Error("[Vulkan] Failed to create shader module");
            return false;
        }
        return true;
    }
}
