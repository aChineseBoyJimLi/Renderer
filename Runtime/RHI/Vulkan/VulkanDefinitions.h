#pragma once

#include "../RHIDefinitions.h"
#if _WIN32 || WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#include <vulkan/vulkan.h>
#include <iostream>

class Blob;

#define OUTPUT_VULKAN_FAILED_RESULT(Result) {VkResult Re = Result;\
    if(Re != VK_SUCCESS)\
    {\
        std::string message = std::system_category().message(Re);\
        Log::Error("[Vulkan] Error: %s, In File: %s line %d", message.c_str(), __FILE__, __LINE__);\
    }}

struct RHIPipelineBindingItem;
struct RHIDepthStencilDesc;
struct RHIRasterizerDesc;
struct RHIBlendStateDesc;
class RHIShader;

namespace RHI::Vulkan
{
    VkDescriptorSetLayoutBinding ConvertBindingLayoutItem(const RHIPipelineBindingItem& inItem);
    VkFormat                ConvertFormat(ERHIFormat inFormat);
    VkSampleCountFlagBits   ConvertSampleBits(uint32_t inSampleCount);
    bool                    CreateShaderModule(const std::shared_ptr<Blob>& inByteCode, VkDevice inDevice, VkShaderModule& outShaderModule);
    VkMemoryPropertyFlags   ConvertHeapType(ERHIResourceHeapType inType);
    VkAccessFlags           ConvertAccessFlags(ERHIResourceStates inState);
    VkImageLayout           ConvertImageLayout(ERHIResourceStates inState);
    bool                    CreateShaderShage(VkDevice inDevice
                                , const std::shared_ptr<RHIShader>& inShader
                                , VkShaderStageFlagBits inShaderType
                                , VkShaderModule& outShaderModule
                                , VkPipelineShaderStageCreateInfo& outShaderStage);
    VkPrimitiveTopology     ConvertPrimitiveTopology(ERHIPrimitiveType inType);
    VkSamplerAddressMode    ConvertSamplerAddressMode(ERHISamplerAddressMode inMode);
    VkPolygonMode           ConvertFillMode(ERHIRasterFillMode inMode);
    VkCullModeFlags         ConvertCullMode(ERHIRasterCullMode inMode);
    VkCompareOp             ConvertCompareOp(ERHIComparisonFunc inOp);
    VkStencilOp             ConvertStencilOp(ERHIStencilOp inOp);
    VkBlendFactor           ConvertBlendValue(ERHIBlendFactor inFactor);
    VkBlendOp               ConvertBlendOp(ERHIBlendOp inOp);
    VkColorComponentFlags   ConvertColorMask(ERHIColorWriteMask inMask);
    void                    TranslateDepthStencilState(const RHIDepthStencilDesc& depthStencilState, VkPipelineDepthStencilStateCreateInfo& outState);
    void                    TranslateBlendState(const RHIBlendStateDesc& inState, VkPipelineColorBlendStateCreateInfo& outState);
    void                    TranslateRasterizerState(const RHIRasterizerDesc& inState, VkPipelineRasterizationStateCreateInfo& outState);
    
}
