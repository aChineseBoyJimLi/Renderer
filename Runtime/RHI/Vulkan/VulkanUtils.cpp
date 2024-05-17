#include "VulkanDefinitions.h"
#include "VulkanPipelineState.h"
#include "../../Core/Log.h"
#include <cassert>

namespace RHI::Vulkan
{
    struct FormatMapping
    {
        ERHIFormat RhiFormat;
        VkFormat VulkanFormat;
    };

    static const std::array<FormatMapping, size_t(ERHIFormat::Count)> s_FormatMap = { {
        { ERHIFormat::Unknown,           VK_FORMAT_UNDEFINED                },
        { ERHIFormat::R8_UINT,           VK_FORMAT_R8_UINT                  },
        { ERHIFormat::R8_SINT,           VK_FORMAT_R8_SINT                  },
        { ERHIFormat::R8_UNORM,          VK_FORMAT_R8_UNORM                 },
        { ERHIFormat::R8_SNORM,          VK_FORMAT_R8_SNORM                 },
        { ERHIFormat::RG8_UINT,          VK_FORMAT_R8G8_UINT                },
        { ERHIFormat::RG8_SINT,          VK_FORMAT_R8G8_SINT                },
        { ERHIFormat::RG8_UNORM,         VK_FORMAT_R8G8_UNORM               },
        { ERHIFormat::RG8_SNORM,         VK_FORMAT_R8G8_SNORM               },
        { ERHIFormat::R16_UINT,          VK_FORMAT_R16_UINT                 },
        { ERHIFormat::R16_SINT,          VK_FORMAT_R16_SINT                 },
        { ERHIFormat::R16_UNORM,         VK_FORMAT_R16_UNORM                },
        { ERHIFormat::R16_SNORM,         VK_FORMAT_R16_SNORM                },
        { ERHIFormat::R16_FLOAT,         VK_FORMAT_R16_SFLOAT               },
        { ERHIFormat::BGRA4_UNORM,       VK_FORMAT_B4G4R4A4_UNORM_PACK16    },
        { ERHIFormat::B5G6R5_UNORM,      VK_FORMAT_B5G6R5_UNORM_PACK16      },
        { ERHIFormat::B5G5R5A1_UNORM,    VK_FORMAT_B5G5R5A1_UNORM_PACK16    },
        { ERHIFormat::RGBA8_UINT,        VK_FORMAT_R8G8B8A8_UINT            },
        { ERHIFormat::RGBA8_SINT,        VK_FORMAT_R8G8B8A8_SINT            },
        { ERHIFormat::RGBA8_UNORM,       VK_FORMAT_R8G8B8A8_UNORM           },
        { ERHIFormat::RGBA8_SNORM,       VK_FORMAT_R8G8B8A8_SNORM           },
        { ERHIFormat::BGRA8_UNORM,       VK_FORMAT_B8G8R8A8_UNORM           },
        { ERHIFormat::SRGBA8_UNORM,      VK_FORMAT_R8G8B8A8_SRGB            },
        { ERHIFormat::SBGRA8_UNORM,      VK_FORMAT_B8G8R8A8_SRGB            },
        { ERHIFormat::R10G10B10A2_UNORM, VK_FORMAT_A2B10G10R10_UNORM_PACK32 },
        { ERHIFormat::R11G11B10_FLOAT,   VK_FORMAT_B10G11R11_UFLOAT_PACK32  },
        { ERHIFormat::RG16_UINT,         VK_FORMAT_R16G16_UINT              },
        { ERHIFormat::RG16_SINT,         VK_FORMAT_R16G16_SINT              },
        { ERHIFormat::RG16_UNORM,        VK_FORMAT_R16G16_UNORM             },
        { ERHIFormat::RG16_SNORM,        VK_FORMAT_R16G16_SNORM             },
        { ERHIFormat::RG16_FLOAT,        VK_FORMAT_R16G16_SFLOAT            },
        { ERHIFormat::R32_UINT,          VK_FORMAT_R32_UINT                 },
        { ERHIFormat::R32_SINT,          VK_FORMAT_R32_SINT                 },
        { ERHIFormat::R32_FLOAT,         VK_FORMAT_R32_SFLOAT               },
        { ERHIFormat::RGBA16_UINT,       VK_FORMAT_R16G16B16A16_UINT        },
        { ERHIFormat::RGBA16_SINT,       VK_FORMAT_R16G16B16A16_SINT        },
        { ERHIFormat::RGBA16_FLOAT,      VK_FORMAT_R16G16B16A16_SFLOAT      },
        { ERHIFormat::RGBA16_UNORM,      VK_FORMAT_R16G16B16A16_UNORM       },
        { ERHIFormat::RGBA16_SNORM,      VK_FORMAT_R16G16B16A16_SNORM       },
        { ERHIFormat::RG32_UINT,         VK_FORMAT_R32G32_UINT              },
        { ERHIFormat::RG32_SINT,         VK_FORMAT_R32G32_SINT              },
        { ERHIFormat::RG32_FLOAT,        VK_FORMAT_R32G32_SFLOAT            },
        { ERHIFormat::RGB32_UINT,        VK_FORMAT_R32G32B32_UINT           },
        { ERHIFormat::RGB32_SINT,        VK_FORMAT_R32G32B32_SINT           },
        { ERHIFormat::RGB32_FLOAT,       VK_FORMAT_R32G32B32_SFLOAT         },
        { ERHIFormat::RGBA32_UINT,       VK_FORMAT_R32G32B32A32_UINT        },
        { ERHIFormat::RGBA32_SINT,       VK_FORMAT_R32G32B32A32_SINT        },
        { ERHIFormat::RGBA32_FLOAT,      VK_FORMAT_R32G32B32A32_SFLOAT      },
        { ERHIFormat::D16,               VK_FORMAT_D16_UNORM                },
        { ERHIFormat::D24S8,             VK_FORMAT_D24_UNORM_S8_UINT        },
        { ERHIFormat::X24G8_UINT,        VK_FORMAT_D24_UNORM_S8_UINT        },
        { ERHIFormat::D32,               VK_FORMAT_D32_SFLOAT               },
        { ERHIFormat::D32S8,             VK_FORMAT_D32_SFLOAT_S8_UINT       },
        { ERHIFormat::X32G8_UINT,        VK_FORMAT_D32_SFLOAT_S8_UINT       },
        { ERHIFormat::BC1_UNORM,         VK_FORMAT_BC1_RGB_UNORM_BLOCK      },
        { ERHIFormat::BC1_UNORM_SRGB,    VK_FORMAT_BC1_RGB_SRGB_BLOCK       },
        { ERHIFormat::BC2_UNORM,         VK_FORMAT_BC2_UNORM_BLOCK          },
        { ERHIFormat::BC2_UNORM_SRGB,    VK_FORMAT_BC2_SRGB_BLOCK           },
        { ERHIFormat::BC3_UNORM,         VK_FORMAT_BC3_UNORM_BLOCK          },
        { ERHIFormat::BC3_UNORM_SRGB,    VK_FORMAT_BC3_SRGB_BLOCK           },
        { ERHIFormat::BC4_UNORM,         VK_FORMAT_BC4_UNORM_BLOCK          },
        { ERHIFormat::BC4_SNORM,         VK_FORMAT_BC4_SNORM_BLOCK          },
        { ERHIFormat::BC5_UNORM,         VK_FORMAT_BC5_UNORM_BLOCK          },
        { ERHIFormat::BC5_SNORM,         VK_FORMAT_BC5_SNORM_BLOCK          },
        { ERHIFormat::BC6H_UFLOAT,       VK_FORMAT_BC6H_UFLOAT_BLOCK        },
        { ERHIFormat::BC6H_SFLOAT,       VK_FORMAT_BC6H_SFLOAT_BLOCK        },
        { ERHIFormat::BC7_UNORM,         VK_FORMAT_BC7_UNORM_BLOCK          },
        { ERHIFormat::BC7_UNORM_SRGB,    VK_FORMAT_BC7_SRGB_BLOCK           },
    } };
    
    VkFormat ConvertFormat(ERHIFormat inFormat)
    {
        assert(inFormat < ERHIFormat::Count);
        assert(s_FormatMap[static_cast<size_t>(inFormat)].RhiFormat == inFormat);
        return s_FormatMap[static_cast<size_t>(inFormat)].VulkanFormat;
    }

    VkSampleCountFlagBits ConvertSampleBits(uint32_t inSampleCount)
    {
        switch (inSampleCount)
        {
        case 1:
            return VK_SAMPLE_COUNT_1_BIT;
        case 2:
            return VK_SAMPLE_COUNT_2_BIT;
        case 4:
            return VK_SAMPLE_COUNT_4_BIT;
        case 8:
            return VK_SAMPLE_COUNT_8_BIT;
        case 16:
            return VK_SAMPLE_COUNT_16_BIT;
        case 32:
            return VK_SAMPLE_COUNT_32_BIT;
        case 64:
            return VK_SAMPLE_COUNT_64_BIT;
        default:
            return VK_SAMPLE_COUNT_1_BIT;
        }
    }
    
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

    VkMemoryPropertyFlags ConvertHeapType(ERHIResourceHeapType inType)
    {
        switch (inType)
        {
        case ERHIResourceHeapType::DeviceLocal:
            return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        case ERHIResourceHeapType::Upload:
            return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        case ERHIResourceHeapType::Readback:
            return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
        }
        return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }

    VkAccessFlags ConvertAccessFlags(ERHIResourceStates inState)
    {
        VkAccessFlags result = VK_ACCESS_NONE;
        if((inState & ERHIResourceStates::DepthStencilWrite) == ERHIResourceStates::DepthStencilWrite)
            result |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        if((inState & ERHIResourceStates::DepthStencilRead) == ERHIResourceStates::DepthStencilRead)
            result |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        if((inState & ERHIResourceStates::RenderTarget) == ERHIResourceStates::RenderTarget)
            result |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        if((inState & ERHIResourceStates::GpuReadOnly) == ERHIResourceStates::GpuReadOnly)
            result |= VK_ACCESS_SHADER_READ_BIT;
        if((inState & ERHIResourceStates::CopyDst) == ERHIResourceStates::CopyDst)
            result |= VK_ACCESS_TRANSFER_WRITE_BIT;
        if((inState & ERHIResourceStates::CopyDst) == ERHIResourceStates::CopySrc)
            result |= VK_ACCESS_TRANSFER_READ_BIT;
        if((inState & ERHIResourceStates::IndirectCommands) == ERHIResourceStates::IndirectCommands)
            result |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        if((inState & ERHIResourceStates::UnorderedAccess) == ERHIResourceStates::UnorderedAccess)
            result |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        if((inState & ERHIResourceStates::AccelerationStructure) == ERHIResourceStates::AccelerationStructure)
            result |= VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
        if((inState & ERHIResourceStates::ShadingRateSource) == ERHIResourceStates::ShadingRateSource)
            result |= VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
        if((inState & ERHIResourceStates::OcclusionPrediction) == ERHIResourceStates::OcclusionPrediction)
            result |= VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT;
        return result;
    }

    VkImageLayout ConvertImageLayout(ERHIResourceStates inState)
    {
        VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
        if((inState & ERHIResourceStates::GpuReadOnly) == ERHIResourceStates::GpuReadOnly)
            layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        if((inState & ERHIResourceStates::DepthStencilWrite) == ERHIResourceStates::DepthStencilWrite)
            layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        if((inState & ERHIResourceStates::DepthStencilRead) == ERHIResourceStates::DepthStencilRead)
            layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        if((inState & ERHIResourceStates::RenderTarget) == ERHIResourceStates::RenderTarget)
            layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        if((inState & ERHIResourceStates::CopyDst) == ERHIResourceStates::CopyDst)
            layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        if((inState & ERHIResourceStates::CopySrc) == ERHIResourceStates::CopySrc)
            layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        if((inState & ERHIResourceStates::IndirectCommands) == ERHIResourceStates::IndirectCommands)
            layout = VK_IMAGE_LAYOUT_GENERAL;
        if((inState & ERHIResourceStates::UnorderedAccess) == ERHIResourceStates::UnorderedAccess)
            layout = VK_IMAGE_LAYOUT_GENERAL;
        if((inState & ERHIResourceStates::AccelerationStructure) == ERHIResourceStates::AccelerationStructure)
            layout = VK_IMAGE_LAYOUT_GENERAL;
        if((inState & ERHIResourceStates::ShadingRateSource) == ERHIResourceStates::ShadingRateSource)
            layout = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
        if((inState & ERHIResourceStates::OcclusionPrediction) == ERHIResourceStates::OcclusionPrediction)
            layout = VK_IMAGE_LAYOUT_GENERAL;
        if((inState & ERHIResourceStates::Present) == ERHIResourceStates::Present)
            layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        
        return layout;
    }

    bool CreateShaderShage(VkDevice inDevice
        , const std::shared_ptr<RHIShader>& inShader
        , VkShaderStageFlagBits inShaderType
        , VkShaderModule& outShaderModule
        , VkPipelineShaderStageCreateInfo& outShaderStage)
    {

        if(inShader == nullptr || !inShader->IsValid())
        {
            Log::Error("[Vulkan] The shader is invalid");
            return false;
        }

        if(!CreateShaderModule(inShader->GetByteCode(), inDevice, outShaderModule))
        {
            return false;
        }

        outShaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        outShaderStage.stage = inShaderType;
        outShaderStage.module = outShaderModule;
        if(inShader->GetEntryName().empty())
        {
            outShaderStage.pName = "main";
        }
        else
        {
            outShaderStage.pName = inShader->GetEntryName().c_str();
        }
        outShaderStage.flags = 0;
        outShaderStage.pSpecializationInfo = nullptr;
        outShaderStage.pNext = nullptr;
        
        return true;
    }

    VkSamplerAddressMode ConvertSamplerAddressMode(ERHISamplerAddressMode inMode)
    {
        switch(inMode)
        {
        case ERHISamplerAddressMode::Clamp:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case ERHISamplerAddressMode::Wrap:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case ERHISamplerAddressMode::Border:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        case ERHISamplerAddressMode::Mirror:
            return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case ERHISamplerAddressMode::MirrorOnce:
            return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
        default:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        }
    }

    VkPrimitiveTopology ConvertPrimitiveTopology(ERHIPrimitiveType inType)
    {
        switch(inType)
        {
            case ERHIPrimitiveType::PointList:
                return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;

            case ERHIPrimitiveType::LineList:
                return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

            case ERHIPrimitiveType::TriangleList:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

            case ERHIPrimitiveType::TriangleStrip:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

            case ERHIPrimitiveType::TriangleFan:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;

            case ERHIPrimitiveType::TriangleListWithAdjacency:
                return VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY;

            case ERHIPrimitiveType::TriangleStripWithAdjacency:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY;

            case ERHIPrimitiveType::PatchList:
                return VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;

            default:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        }
    }

    VkPolygonMode ConvertFillMode(ERHIRasterFillMode inMode)
    {
        switch(inMode)
        {
            case ERHIRasterFillMode::Solid:
                return VK_POLYGON_MODE_FILL;

            case ERHIRasterFillMode::Wireframe:
                return VK_POLYGON_MODE_LINE;

            default:
                return VK_POLYGON_MODE_FILL;
        }
    }

    VkCullModeFlags ConvertCullMode(ERHIRasterCullMode inMode)
    {
        switch(inMode)
        {
            case ERHIRasterCullMode::Back:
                return VK_CULL_MODE_BACK_BIT;

            case ERHIRasterCullMode::Front:
                return VK_CULL_MODE_FRONT_BIT;

            case ERHIRasterCullMode::None:
                return VK_CULL_MODE_NONE;

            default:
                return VK_CULL_MODE_NONE;
        }
    }

    VkCompareOp ConvertCompareOp(ERHIComparisonFunc inOp)
    {
        switch(inOp)
        {
            case ERHIComparisonFunc::Never:
                return VK_COMPARE_OP_NEVER;

            case ERHIComparisonFunc::Less:
                return VK_COMPARE_OP_LESS;

            case ERHIComparisonFunc::Equal:
                return VK_COMPARE_OP_EQUAL;

            case ERHIComparisonFunc::LessEqual:
                return VK_COMPARE_OP_LESS_OR_EQUAL;

            case ERHIComparisonFunc::Greater:
                return VK_COMPARE_OP_GREATER;

            case ERHIComparisonFunc::NotEqual:
                return VK_COMPARE_OP_NOT_EQUAL;

            case ERHIComparisonFunc::GreaterEqual:
                return VK_COMPARE_OP_GREATER_OR_EQUAL;

            case ERHIComparisonFunc::Always:
                return VK_COMPARE_OP_ALWAYS;

            default:
                return VK_COMPARE_OP_ALWAYS;
        }
    }

    VkStencilOp ConvertStencilOp(ERHIStencilOp inOp)
    {
        switch(inOp)
        {
            case ERHIStencilOp::Keep:
                return VK_STENCIL_OP_KEEP;

            case ERHIStencilOp::Zero:
                return VK_STENCIL_OP_ZERO;

            case ERHIStencilOp::Replace:
                return VK_STENCIL_OP_REPLACE;

            case ERHIStencilOp::IncrementAndClamp:
                return VK_STENCIL_OP_INCREMENT_AND_CLAMP;

            case ERHIStencilOp::DecrementAndClamp:
                return VK_STENCIL_OP_DECREMENT_AND_CLAMP;

            case ERHIStencilOp::Invert:
                return VK_STENCIL_OP_INVERT;

            case ERHIStencilOp::Increment:
                return VK_STENCIL_OP_INCREMENT_AND_WRAP;

            case ERHIStencilOp::Decrement:
                return VK_STENCIL_OP_DECREMENT_AND_WRAP;

            default:
                return VK_STENCIL_OP_KEEP;
        }
    }
    
    VkBlendFactor ConvertBlendValue(ERHIBlendFactor inFactor)
    {
        switch(inFactor)
        {
            case ERHIBlendFactor::Zero:
                return VK_BLEND_FACTOR_ZERO;

            case ERHIBlendFactor::One:
                return VK_BLEND_FACTOR_ONE;

            case ERHIBlendFactor::SrcColor:
                return VK_BLEND_FACTOR_SRC_COLOR;

            case ERHIBlendFactor::OneMinusSrcColor:
                return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;

            case ERHIBlendFactor::SrcAlpha:
                return VK_BLEND_FACTOR_SRC_ALPHA;

            case ERHIBlendFactor::OneMinusSrcAlpha:
                return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

            case ERHIBlendFactor::DstAlpha:
                return VK_BLEND_FACTOR_DST_ALPHA;

            case ERHIBlendFactor::OneMinusDstAlpha:
                return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;

            case ERHIBlendFactor::DstColor:
                return VK_BLEND_FACTOR_DST_COLOR;

            case ERHIBlendFactor::OneMinusDstColor:
                return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;

            case ERHIBlendFactor::SrcAlphaSaturate:
                return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;

            case ERHIBlendFactor::ConstantColor:
                return VK_BLEND_FACTOR_CONSTANT_COLOR;

            case ERHIBlendFactor::OneMinusConstantColor:
                return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;

            case ERHIBlendFactor::Src1Color:
                return VK_BLEND_FACTOR_SRC1_COLOR;

            case ERHIBlendFactor::OneMinusSrc1Color:
                return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;

            case ERHIBlendFactor::Src1Alpha:
                return VK_BLEND_FACTOR_SRC1_ALPHA;

            case ERHIBlendFactor::OneMinusSrc1Alpha:
                return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;

            default:
                return VK_BLEND_FACTOR_ZERO;
        }
    }

    VkBlendOp ConvertBlendOp(ERHIBlendOp inOp)
    {
        switch(inOp)
        {
            case ERHIBlendOp::Add:
                return VK_BLEND_OP_ADD;

            case ERHIBlendOp::Subtract:
                return VK_BLEND_OP_SUBTRACT;

            case ERHIBlendOp::RevSubtract:
                return VK_BLEND_OP_REVERSE_SUBTRACT;

            case ERHIBlendOp::Min:
                return VK_BLEND_OP_MIN;

            case ERHIBlendOp::Max:
                return VK_BLEND_OP_MAX;

            default:
                return VK_BLEND_OP_ADD;
        }
    }

    VkColorComponentFlags ConvertColorMask(ERHIColorWriteMask inMask)
    {
        // return vk::ColorComponentFlags(uint8_t(mask));
        switch (inMask)
        {
            case ERHIColorWriteMask::Red:
                return VK_COLOR_COMPONENT_R_BIT;
            case ERHIColorWriteMask::Green:
                return VK_COLOR_COMPONENT_G_BIT;
            case ERHIColorWriteMask::Blue:
                return VK_COLOR_COMPONENT_B_BIT;
            case ERHIColorWriteMask::Alpha:
                return VK_COLOR_COMPONENT_A_BIT;
            case ERHIColorWriteMask::All:
                return VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            default:
                return VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        }
    }

    void TranslateDepthStencilState(const RHIDepthStencilDesc& depthStencilState, VkPipelineDepthStencilStateCreateInfo& outState)
    {
        outState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        // Depth State
        outState.depthTestEnable = depthStencilState.DepthTestEnable;
        outState.depthWriteEnable = depthStencilState.DepthWriteEnable;
        outState.depthCompareOp = ConvertCompareOp(depthStencilState.DepthFunc);
        outState.depthBoundsTestEnable = false;

        // Stencil State
        outState.stencilTestEnable = depthStencilState.StencilEnable;
        outState.front.failOp = ConvertStencilOp(depthStencilState.FrontFaceStencilFailOp);
        outState.front.passOp = ConvertStencilOp(depthStencilState.FrontFacePassOp);
        outState.front.depthFailOp = ConvertStencilOp(depthStencilState.FrontFaceDepthFailOp);
        outState.front.compareOp = ConvertCompareOp(depthStencilState.FrontFaceFunc);
        outState.front.compareMask = depthStencilState.StencilReadMask;
        outState.front.writeMask = depthStencilState.StencilWriteMask;
        outState.front.reference = depthStencilState.StencilRef;

        outState.back.failOp = ConvertStencilOp(depthStencilState.BackFaceStencilFailOp);
        outState.back.passOp = ConvertStencilOp(depthStencilState.BackFacePassOp);
        outState.back.depthFailOp = ConvertStencilOp(depthStencilState.BackFaceDepthFailOp);
        outState.back.compareOp = ConvertCompareOp(depthStencilState.BackFaceFunc);
        outState.back.compareMask = depthStencilState.StencilReadMask;
        outState.back.writeMask = depthStencilState.StencilWriteMask;
        outState.back.reference = depthStencilState.StencilRef;
    }

    void TranslateBlendState(const RHIBlendStateDesc& inState, VkPipelineColorBlendStateCreateInfo& outState)
    {
        VkPipelineColorBlendAttachmentState attachment[RHIRenderTargetsMaxCount];

        for(uint32_t i = 0; i < inState.RenderTargetCount; ++i)
        {
            attachment[i].blendEnable = true;
            attachment[i].srcColorBlendFactor = ConvertBlendValue(inState.Targets[i].ColorSrcBlend);
            attachment[i].dstColorBlendFactor = ConvertBlendValue(inState.Targets[i].ColorDstBlend);
            attachment[i].colorBlendOp = ConvertBlendOp(inState.Targets[i].ColorBlendOp);
            attachment[i].srcAlphaBlendFactor = ConvertBlendValue(inState.Targets[i].AlphaSrcBlend);
            attachment[i].dstAlphaBlendFactor = ConvertBlendValue(inState.Targets[i].AlphaDstBlend);
            attachment[i].alphaBlendOp = ConvertBlendOp(inState.Targets[i].AlphaBlendOp);
            attachment[i].colorWriteMask = ConvertColorMask(inState.Targets[i].ColorWriteMask);
        }
        
        outState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        outState.logicOpEnable = VK_FALSE;
        outState.logicOp = VK_LOGIC_OP_COPY;
        outState.attachmentCount = inState.RenderTargetCount;
        outState.pAttachments = attachment;
        outState.blendConstants[0] = 1.0f;
        outState.blendConstants[1] = 1.0f;
        outState.blendConstants[2] = 1.0f;
        outState.blendConstants[3] = 1.0f;
    }

    void TranslateRasterizerState(const RHIRasterizerDesc& inState, VkPipelineRasterizationStateCreateInfo& outState)
    {
        outState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        outState.cullMode = ConvertCullMode(inState.CullMode);
        outState.frontFace = inState.FrontCounterClockwise ? VK_FRONT_FACE_COUNTER_CLOCKWISE : VK_FRONT_FACE_CLOCKWISE;
        outState.polygonMode = ConvertFillMode(inState.FillMode);
        outState.lineWidth = 1.0f;
        outState.depthBiasEnable = inState.DepthBias != 0;
        outState.depthBiasConstantFactor = inState.DepthBiasClamp;
        outState.depthBiasSlopeFactor = inState.SlopeScaledDepthBias;
        outState.depthBiasClamp = inState.DepthBiasClamp;
        outState.depthClampEnable = inState.DepthClipEnable;
        outState.rasterizerDiscardEnable = false;
    }
}
