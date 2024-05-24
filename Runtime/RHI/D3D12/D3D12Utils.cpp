#include "D3D12Definitions.h"
#include "D3D12PipelineState.h"
#include "../../Core/Log.h"
#include <cassert>

namespace RHI::D3D12
{

    struct FormatMapping
    {
        ERHIFormat RhiFormat;
        DXGI_FORMAT DxgiFormat;
    };

    static const std::array<FormatMapping, size_t(ERHIFormat::Count)> s_FormatMap = { {
        {ERHIFormat::Unknown,           DXGI_FORMAT_UNKNOWN},
        { ERHIFormat::R8_UINT,           DXGI_FORMAT_R8_UINT                  },
        { ERHIFormat::R8_SINT,           DXGI_FORMAT_R8_SINT                  },
        { ERHIFormat::R8_UNORM,          DXGI_FORMAT_R8_UNORM                 },
        { ERHIFormat::R8_SNORM,          DXGI_FORMAT_R8_SNORM                 },
        { ERHIFormat::RG8_UINT,          DXGI_FORMAT_R8G8_UINT                },
        { ERHIFormat::RG8_SINT,          DXGI_FORMAT_R8G8_SINT                },
        { ERHIFormat::RG8_UNORM,         DXGI_FORMAT_R8G8_UNORM               },
        { ERHIFormat::RG8_SNORM,         DXGI_FORMAT_R8G8_SNORM               },
        { ERHIFormat::R16_UINT,          DXGI_FORMAT_R16_UINT                 },
        { ERHIFormat::R16_SINT,          DXGI_FORMAT_R16_SINT                 },
        { ERHIFormat::R16_UNORM,         DXGI_FORMAT_R16_UNORM                },
        { ERHIFormat::R16_SNORM,         DXGI_FORMAT_R16_SNORM                },
        { ERHIFormat::R16_FLOAT,         DXGI_FORMAT_R16_FLOAT               },
        { ERHIFormat::BGRA4_UNORM,       DXGI_FORMAT_B4G4R4A4_UNORM    },
        { ERHIFormat::B5G6R5_UNORM,      DXGI_FORMAT_B5G6R5_UNORM      },
        { ERHIFormat::B5G5R5A1_UNORM,    DXGI_FORMAT_B5G5R5A1_UNORM    },
        { ERHIFormat::RGBA8_UINT,        DXGI_FORMAT_R8G8B8A8_UINT            },
        { ERHIFormat::RGBA8_SINT,        DXGI_FORMAT_R8G8B8A8_SINT            },
        { ERHIFormat::RGBA8_UNORM,       DXGI_FORMAT_R8G8B8A8_UNORM           },
        { ERHIFormat::RGBA8_SNORM,       DXGI_FORMAT_R8G8B8A8_SNORM           },
        { ERHIFormat::BGRA8_UNORM,       DXGI_FORMAT_B8G8R8A8_UNORM           },
        { ERHIFormat::SRGBA8_UNORM,      DXGI_FORMAT_R8G8B8A8_UNORM_SRGB            },
        { ERHIFormat::SBGRA8_UNORM,      DXGI_FORMAT_B8G8R8A8_UNORM_SRGB            },
        { ERHIFormat::R10G10B10A2_UNORM, DXGI_FORMAT_R10G10B10A2_UNORM },
        { ERHIFormat::R11G11B10_FLOAT,   DXGI_FORMAT_R11G11B10_FLOAT  },
        { ERHIFormat::RG16_UINT,         DXGI_FORMAT_R16G16_UINT              },
        { ERHIFormat::RG16_SINT,         DXGI_FORMAT_R16G16_SINT              },
        { ERHIFormat::RG16_UNORM,        DXGI_FORMAT_R16G16_UNORM             },
        { ERHIFormat::RG16_SNORM,        DXGI_FORMAT_R16G16_SNORM             },
        { ERHIFormat::RG16_FLOAT,        DXGI_FORMAT_R16G16_FLOAT            },
        { ERHIFormat::R32_UINT,          DXGI_FORMAT_R32_UINT                 },
        { ERHIFormat::R32_SINT,          DXGI_FORMAT_R32_SINT                 },
        { ERHIFormat::R32_FLOAT,         DXGI_FORMAT_R32_FLOAT               },
        { ERHIFormat::RGBA16_UINT,       DXGI_FORMAT_R16G16B16A16_UINT        },
        { ERHIFormat::RGBA16_SINT,       DXGI_FORMAT_R16G16B16A16_SINT        },
        { ERHIFormat::RGBA16_FLOAT,      DXGI_FORMAT_R16G16B16A16_FLOAT      },
        { ERHIFormat::RGBA16_UNORM,      DXGI_FORMAT_R16G16B16A16_UNORM       },
        { ERHIFormat::RGBA16_SNORM,      DXGI_FORMAT_R16G16B16A16_SNORM       },
        { ERHIFormat::RG32_UINT,         DXGI_FORMAT_R32G32_UINT              },
        { ERHIFormat::RG32_SINT,         DXGI_FORMAT_R32G32_SINT              },
        { ERHIFormat::RG32_FLOAT,        DXGI_FORMAT_R32G32_FLOAT            },
        { ERHIFormat::RGB32_UINT,        DXGI_FORMAT_R32G32B32_UINT           },
        { ERHIFormat::RGB32_SINT,        DXGI_FORMAT_R32G32B32_SINT           },
        { ERHIFormat::RGB32_FLOAT,       DXGI_FORMAT_R32G32B32_FLOAT         },
        { ERHIFormat::RGBA32_UINT,       DXGI_FORMAT_R32G32B32A32_UINT        },
        { ERHIFormat::RGBA32_SINT,       DXGI_FORMAT_R32G32B32A32_SINT        },
        { ERHIFormat::RGBA32_FLOAT,      DXGI_FORMAT_R32G32B32A32_FLOAT      },
        { ERHIFormat::D16,               DXGI_FORMAT_D16_UNORM                },
        { ERHIFormat::D24S8,             DXGI_FORMAT_D24_UNORM_S8_UINT        },
        { ERHIFormat::X24G8_UINT,        DXGI_FORMAT_D24_UNORM_S8_UINT        },
        { ERHIFormat::D32,               DXGI_FORMAT_D32_FLOAT               },
        { ERHIFormat::D32S8,             DXGI_FORMAT_D32_FLOAT_S8X24_UINT       },
        { ERHIFormat::X32G8_UINT,        DXGI_FORMAT_D32_FLOAT_S8X24_UINT       },
        { ERHIFormat::BC1_UNORM,         DXGI_FORMAT_BC1_UNORM      },
        { ERHIFormat::BC1_UNORM_SRGB,    DXGI_FORMAT_BC1_UNORM_SRGB       },
        { ERHIFormat::BC2_UNORM,         DXGI_FORMAT_BC2_UNORM          },
        { ERHIFormat::BC2_UNORM_SRGB,    DXGI_FORMAT_BC2_UNORM_SRGB           },
        { ERHIFormat::BC3_UNORM,         DXGI_FORMAT_BC3_UNORM          },
        { ERHIFormat::BC3_UNORM_SRGB,    DXGI_FORMAT_BC3_UNORM_SRGB           },
        { ERHIFormat::BC4_UNORM,         DXGI_FORMAT_BC4_UNORM          },
        { ERHIFormat::BC4_SNORM,         DXGI_FORMAT_BC4_SNORM          },
        { ERHIFormat::BC5_UNORM,         DXGI_FORMAT_BC5_UNORM          },
        { ERHIFormat::BC5_SNORM,         DXGI_FORMAT_BC5_SNORM          },
        { ERHIFormat::BC6H_UFLOAT,       DXGI_FORMAT_BC6H_UF16        },
        { ERHIFormat::BC6H_SFLOAT,       DXGI_FORMAT_BC6H_SF16        },
        { ERHIFormat::BC7_UNORM,         DXGI_FORMAT_BC7_UNORM          },
        { ERHIFormat::BC7_UNORM_SRGB,    DXGI_FORMAT_BC7_UNORM_SRGB           },
    }};
    
    DXGI_FORMAT ConvertFormat(ERHIFormat inFormat)
    {
        assert(inFormat < ERHIFormat::Count);
        assert(s_FormatMap[static_cast<size_t>(inFormat)].RhiFormat == inFormat);
        return s_FormatMap[static_cast<size_t>(inFormat)].DxgiFormat;
    }
    
    D3D12_COMMAND_LIST_TYPE ConvertCommandQueueType(ERHICommandQueueType inType)
    {
        switch (inType)
        {
        case ERHICommandQueueType::Direct: return D3D12_COMMAND_LIST_TYPE_DIRECT;
        case ERHICommandQueueType::Copy  : return D3D12_COMMAND_LIST_TYPE_COPY;
        case ERHICommandQueueType::Async : return D3D12_COMMAND_LIST_TYPE_COMPUTE;
        default:
            Log::Warning("Invalid command queue type. Defaulting to D3D12_COMMAND_LIST_TYPE_DIRECT.");
            return D3D12_COMMAND_LIST_TYPE_DIRECT;
        }
    }

    D3D12_HEAP_TYPE ConvertHeapType(ERHIResourceHeapType inType)
    {
        switch (inType)
        {
        case ERHIResourceHeapType::DeviceLocal:
            return D3D12_HEAP_TYPE_DEFAULT;
        case ERHIResourceHeapType::Upload:
            return D3D12_HEAP_TYPE_UPLOAD;
        case ERHIResourceHeapType::Readback:
            return D3D12_HEAP_TYPE_READBACK;
        }
        return D3D12_HEAP_TYPE_DEFAULT;
    }

    D3D12_RESOURCE_STATES ConvertResourceStates(ERHIResourceStates inState)
    {
        D3D12_RESOURCE_STATES outState = D3D12_RESOURCE_STATE_COMMON;
        if((inState & ERHIResourceStates::Present) == ERHIResourceStates::Present)
            outState |= D3D12_RESOURCE_STATE_PRESENT;

        if((inState & ERHIResourceStates::GpuReadOnly) == ERHIResourceStates::GpuReadOnly)
            outState |= D3D12_RESOURCE_STATE_GENERIC_READ;

        if((inState & ERHIResourceStates::IndirectCommands) == ERHIResourceStates::IndirectCommands)
            outState |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;

        if((inState & ERHIResourceStates::UnorderedAccess) == ERHIResourceStates::UnorderedAccess)
            outState |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

        if((inState & ERHIResourceStates::RenderTarget) == ERHIResourceStates::RenderTarget)
            outState |= D3D12_RESOURCE_STATE_RENDER_TARGET;

        if((inState & ERHIResourceStates::ShadingRateSource) == ERHIResourceStates::ShadingRateSource)
            outState |= D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;

        if((inState & ERHIResourceStates::CopySrc) == ERHIResourceStates::CopySrc)
            outState |= D3D12_RESOURCE_STATE_COPY_SOURCE;

        if((inState & ERHIResourceStates::CopyDst) == ERHIResourceStates::CopyDst)
            outState |= D3D12_RESOURCE_STATE_COPY_DEST;

        if((inState & ERHIResourceStates::DepthStencilWrite) == ERHIResourceStates::DepthStencilWrite)
            outState |= D3D12_RESOURCE_STATE_DEPTH_WRITE;

        if((inState & ERHIResourceStates::DepthStencilRead) == ERHIResourceStates::DepthStencilRead)
            outState |= D3D12_RESOURCE_STATE_DEPTH_READ;

        if((inState & ERHIResourceStates::AccelerationStructure) == ERHIResourceStates::AccelerationStructure)
            outState |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

        if((inState & ERHIResourceStates::OcclusionPrediction) == ERHIResourceStates::OcclusionPrediction)
            outState |= D3D12_RESOURCE_STATE_PREDICATION;
        
        return outState;
    }

    D3D12_BLEND ConvertBlendValue(ERHIBlendFactor value)
    {
        switch (value)
        {
        case ERHIBlendFactor::Zero:
            return D3D12_BLEND_ZERO;
        case ERHIBlendFactor::One:
            return D3D12_BLEND_ONE;
        case ERHIBlendFactor::SrcColor:
            return D3D12_BLEND_SRC_COLOR;
        case ERHIBlendFactor::SrcAlpha:
            return D3D12_BLEND_SRC_ALPHA;
        case ERHIBlendFactor::OneMinusSrcColor:
            return D3D12_BLEND_INV_SRC_COLOR;
        case ERHIBlendFactor::OneMinusSrcAlpha:
            return D3D12_BLEND_INV_SRC_ALPHA;
        case ERHIBlendFactor::DstAlpha:
            return D3D12_BLEND_DEST_ALPHA;
        case ERHIBlendFactor::OneMinusDstAlpha:
            return D3D12_BLEND_INV_DEST_ALPHA;
        case ERHIBlendFactor::DstColor:
            return D3D12_BLEND_DEST_COLOR;
        case ERHIBlendFactor::OneMinusDstColor:
            return D3D12_BLEND_INV_DEST_COLOR;
        case ERHIBlendFactor::SrcAlphaSaturate:
            return D3D12_BLEND_SRC_ALPHA_SAT;
        case ERHIBlendFactor::ConstantColor:
            return D3D12_BLEND_BLEND_FACTOR;
        case ERHIBlendFactor::OneMinusConstantColor:
            return D3D12_BLEND_INV_BLEND_FACTOR;
        case ERHIBlendFactor::Src1Color:
            return D3D12_BLEND_SRC1_COLOR;
        case ERHIBlendFactor::OneMinusSrc1Color:
            return D3D12_BLEND_INV_SRC1_COLOR;
        case ERHIBlendFactor::Src1Alpha:
            return D3D12_BLEND_SRC1_ALPHA;
        case ERHIBlendFactor::OneMinusSrc1Alpha:
            return D3D12_BLEND_INV_SRC1_ALPHA;
        default:
            return D3D12_BLEND_ZERO;
        }
    }

    D3D12_BLEND_OP ConvertBlendOp(ERHIBlendOp value)
    {
        switch (value)
        {
        case ERHIBlendOp::Add:
            return D3D12_BLEND_OP_ADD;
        case ERHIBlendOp::Subtract:
            return D3D12_BLEND_OP_SUBTRACT;
        case ERHIBlendOp::RevSubtract:
            return D3D12_BLEND_OP_REV_SUBTRACT;
        case ERHIBlendOp::Min:
            return D3D12_BLEND_OP_MIN;
        case ERHIBlendOp::Max:
            return D3D12_BLEND_OP_MAX;
        default:
            return D3D12_BLEND_OP_ADD;
        }
    }

    D3D12_STENCIL_OP ConvertStencilOp(ERHIStencilOp value)
    {
        switch (value)
        {
        case ERHIStencilOp::Keep:
            return D3D12_STENCIL_OP_KEEP;
        case ERHIStencilOp::Zero:
            return D3D12_STENCIL_OP_ZERO;
        case ERHIStencilOp::Replace:
            return D3D12_STENCIL_OP_REPLACE;
        case ERHIStencilOp::IncrementAndClamp:
            return D3D12_STENCIL_OP_INCR_SAT;
        case ERHIStencilOp::DecrementAndClamp:
            return D3D12_STENCIL_OP_DECR_SAT;
        case ERHIStencilOp::Invert:
            return D3D12_STENCIL_OP_INVERT;
        case ERHIStencilOp::Increment:
            return D3D12_STENCIL_OP_INCR;
        case ERHIStencilOp::Decrement:
            return D3D12_STENCIL_OP_DECR;
        default:
            return D3D12_STENCIL_OP_KEEP;
        }
    }

    D3D12_COMPARISON_FUNC ConvertComparisonFunc(ERHIComparisonFunc value)
    {
        switch (value)
        {
        case ERHIComparisonFunc::Never:
            return D3D12_COMPARISON_FUNC_NEVER;
        case ERHIComparisonFunc::Less:
            return D3D12_COMPARISON_FUNC_LESS;
        case ERHIComparisonFunc::Equal:
            return D3D12_COMPARISON_FUNC_EQUAL;
        case ERHIComparisonFunc::LessEqual:
            return D3D12_COMPARISON_FUNC_LESS_EQUAL;
        case ERHIComparisonFunc::Greater:
            return D3D12_COMPARISON_FUNC_GREATER;
        case ERHIComparisonFunc::NotEqual:
            return D3D12_COMPARISON_FUNC_NOT_EQUAL;
        case ERHIComparisonFunc::GreaterEqual:
            return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        case ERHIComparisonFunc::Always:
            return D3D12_COMPARISON_FUNC_ALWAYS;
        default:
            return D3D12_COMPARISON_FUNC_NEVER;
        }
    }
    D3D_PRIMITIVE_TOPOLOGY ConvertPrimitiveType(ERHIPrimitiveType pt, uint32_t controlPoints)
    {
        switch (pt)
        {
        case ERHIPrimitiveType::PointList:
            return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
        case ERHIPrimitiveType::LineList:
            return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
        case ERHIPrimitiveType::TriangleList:
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case ERHIPrimitiveType::TriangleStrip:
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        case ERHIPrimitiveType::TriangleFan:
            return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
        case ERHIPrimitiveType::TriangleListWithAdjacency:
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ;
        case ERHIPrimitiveType::TriangleStripWithAdjacency:
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ;
        case ERHIPrimitiveType::PatchList:
            if (controlPoints == 0 || controlPoints > 32)
            {
                return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
            }
            return D3D_PRIMITIVE_TOPOLOGY(D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST + (controlPoints - 1));
        default:
            return D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
        }
    }

    D3D12_TEXTURE_ADDRESS_MODE ConvertSamplerAddressMode(ERHISamplerAddressMode mode)
    {
        switch (mode)
        {
        case ERHISamplerAddressMode::Clamp:
            return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        case ERHISamplerAddressMode::Wrap:
            return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        case ERHISamplerAddressMode::Border:
            return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        case ERHISamplerAddressMode::Mirror:
            return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        case ERHISamplerAddressMode::MirrorOnce:
            return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
        default:
            return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        }
    }
    
    UINT ConvertSamplerReductionType(ERHISamplerReductionType reductionType)
    {
        switch (reductionType)
        {
        case ERHISamplerReductionType::Standard:
            return D3D12_FILTER_REDUCTION_TYPE_STANDARD;
        case ERHISamplerReductionType::Comparison:
            return D3D12_FILTER_REDUCTION_TYPE_COMPARISON;
        case ERHISamplerReductionType::Minimum:
            return D3D12_FILTER_REDUCTION_TYPE_MINIMUM;
        case ERHISamplerReductionType::Maximum:
            return D3D12_FILTER_REDUCTION_TYPE_MAXIMUM;
        default:
            return D3D12_FILTER_REDUCTION_TYPE_STANDARD;
        }
    }
    
    void TranslateBlendState(const RHIBlendStateDesc& inState, D3D12_BLEND_DESC& outState)
    {
        outState.AlphaToCoverageEnable = inState.AlphaToCoverageEnable;
        outState.IndependentBlendEnable = false;

        for (uint32_t i = 0; i < inState.NumRenderTarget; i++)
        {
            const RHIBlendStateDesc::RenderTarget& src = inState.Targets[i];
            D3D12_RENDER_TARGET_BLEND_DESC& dst = outState.RenderTarget[i];

            dst.BlendEnable = src.BlendEnable ? TRUE : FALSE;
            dst.SrcBlend = ConvertBlendValue(src.ColorSrcBlend);
            dst.DestBlend = ConvertBlendValue(src.ColorDstBlend);
            dst.BlendOp = ConvertBlendOp(src.ColorBlendOp);
            dst.SrcBlendAlpha = ConvertBlendValue(src.AlphaSrcBlend);
            dst.DestBlendAlpha = ConvertBlendValue(src.AlphaDstBlend);
            dst.BlendOpAlpha = ConvertBlendOp(src.AlphaBlendOp);
            dst.RenderTargetWriteMask = (D3D12_COLOR_WRITE_ENABLE)src.ColorWriteMask;
        }
    }

    void TranslateDepthStencilState(const RHIDepthStencilDesc& inState, D3D12_DEPTH_STENCIL_DESC& outState)
    {
        outState.DepthEnable = inState.DepthTestEnable ? TRUE : FALSE;
        outState.DepthWriteMask = inState.DepthWriteEnable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
        outState.DepthFunc = ConvertComparisonFunc(inState.DepthFunc);
        outState.StencilEnable = inState.StencilEnable ? TRUE : FALSE;
        outState.StencilReadMask = (UINT8)inState.StencilReadMask;
        outState.StencilWriteMask = (UINT8)inState.StencilWriteMask;
        outState.FrontFace.StencilFailOp = ConvertStencilOp(inState.FrontFaceStencilFailOp);
        outState.FrontFace.StencilDepthFailOp = ConvertStencilOp(inState.FrontFaceDepthFailOp);
        outState.FrontFace.StencilPassOp = ConvertStencilOp(inState.FrontFacePassOp);
        outState.FrontFace.StencilFunc = ConvertComparisonFunc(inState.FrontFaceFunc);
        outState.BackFace.StencilFailOp = ConvertStencilOp(inState.BackFaceStencilFailOp);
        outState.BackFace.StencilDepthFailOp = ConvertStencilOp(inState.BackFaceDepthFailOp);
        outState.BackFace.StencilPassOp = ConvertStencilOp(inState.BackFacePassOp);
        outState.BackFace.StencilFunc = ConvertComparisonFunc(inState.BackFaceFunc);
    }

    void TranslateRasterizerState(const RHIRasterizerDesc& inState, D3D12_RASTERIZER_DESC& outState)
    {
        switch (inState.FillMode)
        {
        case ERHIRasterFillMode::Solid:
            outState.FillMode = D3D12_FILL_MODE_SOLID;
            break;
        case ERHIRasterFillMode::Wireframe:
            outState.FillMode = D3D12_FILL_MODE_WIREFRAME;
            break;
        default:
            break;
        }

        switch (inState.CullMode)
        {
        case ERHIRasterCullMode::Back:
            outState.CullMode = D3D12_CULL_MODE_BACK;
            break;
        case ERHIRasterCullMode::Front:
            outState.CullMode = D3D12_CULL_MODE_FRONT;
            break;
        case ERHIRasterCullMode::None:
            outState.CullMode = D3D12_CULL_MODE_NONE;
            break;
        default:
            break;
        }

        outState.FrontCounterClockwise = inState.FrontCounterClockwise ? TRUE : FALSE;
        outState.DepthBias = inState.DepthBias;
        outState.DepthBiasClamp = inState.DepthBiasClamp;
        outState.SlopeScaledDepthBias = inState.SlopeScaledDepthBias;
        outState.DepthClipEnable = inState.DepthClipEnable ? TRUE : FALSE;
        outState.MultisampleEnable = inState.MultisampleEnable ? TRUE : FALSE;
        outState.AntialiasedLineEnable = inState.AntialiasedLineEnable ? TRUE : FALSE;
        outState.ConservativeRaster = inState.ConservativeRasterEnable ? D3D12_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
        outState.ForcedSampleCount = inState.ForcedSampleCount;
    }

    ERHIResourceViewType ConvertRHIResourceViewType(D3D12_DESCRIPTOR_RANGE_TYPE inRangeType)
    {
        switch(inRangeType)
        {
        case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
            return ERHIResourceViewType::SRV;
        case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
            return ERHIResourceViewType::CBV;
        case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
            return ERHIResourceViewType::UAV;
        case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
            return ERHIResourceViewType::Sampler;
        }
        assert(!"Enclosing block should never be called");
        return ERHIResourceViewType::SRV;
    }

    ERHIResourceViewType ConvertRHIResourceViewType(D3D12_ROOT_PARAMETER_TYPE inParameterType)
    {
        switch(inParameterType)
        {
        case D3D12_ROOT_PARAMETER_TYPE_CBV:
        case D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS:
            return ERHIResourceViewType::CBV;
        case D3D12_ROOT_PARAMETER_TYPE_SRV:
            return ERHIResourceViewType::SRV;
        case D3D12_ROOT_PARAMETER_TYPE_UAV:
            return ERHIResourceViewType::UAV;
        case D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE:
            assert(!"Enclosing block should never be called");
            break;
        }
        return ERHIResourceViewType::SRV;    
    }
}
