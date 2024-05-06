#pragma once

#if _WIN32 || WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>
#include <array>
#include <iostream>

class Blob;

#define OUTPUT_VULKAN_FAILED_RESULT(Result) {VkResult Re = Result;\
    if(Re != VK_SUCCESS)\
    {\
        std::string message = std::system_category().message(Re);\
        Log::Error("[Vulkan] Error: %s, In File: %s line %d", message.c_str(), __FILE__, __LINE__);\
    }}

struct RHIPipelineBindingItem;

namespace RHI::Vulkan
{
    VkDescriptorSetLayoutBinding ConvertBindingLayoutItem(const RHIPipelineBindingItem& inItem);
    bool CreateShaderModule(const std::shared_ptr<Blob>& inByteCode, VkDevice inDevice, VkShaderModule& outShaderModule);
}
