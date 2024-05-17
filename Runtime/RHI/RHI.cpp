#include "RHI.h"
#include <mutex>

#include "RHIResources.h"
#include "../Core/Log.h"
#include "../Core/Templates.h"
#include "D3D12/D3D12Device.h"
#include "D3D12/D3D12SwapChain.h"
#if HAS_VULKAN
#include "Vulkan/VulkanSwapChain.h"
#include "Vulkan/VulkanDevice.h"
#endif

namespace RHI
{
    static RHIDevice* s_Device = nullptr;
    static std::mutex s_Mtx;
    static bool s_UseVulkan = false;
    
    bool Init(bool useVulkan)
    {
        if(s_Device == nullptr)
        {
            std::lock_guard locker(s_Mtx);
#if HAS_VULKAN
            if(useVulkan)
            {
                s_Device = new VulkanDevice();
            }
            else
#endif
            {
                s_Device = new D3D12Device();
            }
            s_UseVulkan = useVulkan;
            return s_Device->Init();
        }
        return true;
    }

    void Shutdown()
    {
        if(s_Device != nullptr)
        {
            std::lock_guard locker(s_Mtx);
            s_Device->Shutdown();
            delete s_Device;
            s_Device = nullptr;
        }
    }

    RHIDevice* GetDevice()
    {
        if(!Init() || !s_Device->IsValid())
        {
            throw std::runtime_error("[RHI] RHI not validated!");
        }
        return s_Device;
    }

    std::shared_ptr<RHISwapChain> CreateSwapChain(const RHISwapChainDesc& inDesc)
    {
#if HAS_VULKAN
        if(s_UseVulkan)
        {
            VulkanDevice* device = CheckCast<VulkanDevice*>(GetDevice());
            if(!device || !device->IsValid())
            {
                Log::Error("[Vulkan] Failed to create swap chain, the device is not valid");
                return nullptr;
            }
            std::shared_ptr<RHISwapChain> swapChain(new VulkanSwapChain(*device, inDesc));
            if(!swapChain->Init())
            {
                Log::Error("[Vulkan] Failed to create swap chain");
            }
            return swapChain;
        }
        else
#endif
        {
            D3D12Device* device = CheckCast<D3D12Device*>(GetDevice());
            if(!device || !device->IsValid())
            {
                Log::Error("[D3D12] Failed to create swap chain, the device is not valid");
                return nullptr;
            }
            std::shared_ptr<RHISwapChain> swapChain(new D3D12SwapChain(*device, inDesc));
            if(!swapChain->Init())
            {
                Log::Error("[D3D12] Failed to create swap chain");
            }
            return swapChain;
        }
    }

    // ERHIFormat mapping table. The rows must be in the exactly same order as ERHIFormat enum members are defined.
    static const RHIFormatInfo s_FormatInfo[] = {
    //        format                   name             bytes blk         kind               red   green   blue  alpha  depth  stencl signed  srgb
        { ERHIFormat::Unknown,           "UNKNOWN",           0,   0, ERHIFormatKind::Integer,      false, false, false, false, false, false, false, false },
        { ERHIFormat::R8_UINT,           "R8_UINT",           1,   1, ERHIFormatKind::Integer,      true,  false, false, false, false, false, false, false },
        { ERHIFormat::R8_SINT,           "R8_SINT",           1,   1, ERHIFormatKind::Integer,      true,  false, false, false, false, false, true,  false },
        { ERHIFormat::R8_UNORM,          "R8_UNORM",          1,   1, ERHIFormatKind::Normalized,   true,  false, false, false, false, false, false, false },
        { ERHIFormat::R8_SNORM,          "R8_SNORM",          1,   1, ERHIFormatKind::Normalized,   true,  false, false, false, false, false, false, false },
        { ERHIFormat::RG8_UINT,          "RG8_UINT",          2,   1, ERHIFormatKind::Integer,      true,  true,  false, false, false, false, false, false },
        { ERHIFormat::RG8_SINT,          "RG8_SINT",          2,   1, ERHIFormatKind::Integer,      true,  true,  false, false, false, false, true,  false },
        { ERHIFormat::RG8_UNORM,         "RG8_UNORM",         2,   1, ERHIFormatKind::Normalized,   true,  true,  false, false, false, false, false, false },
        { ERHIFormat::RG8_SNORM,         "RG8_SNORM",         2,   1, ERHIFormatKind::Normalized,   true,  true,  false, false, false, false, false, false },
        { ERHIFormat::R16_UINT,          "R16_UINT",          2,   1, ERHIFormatKind::Integer,      true,  false, false, false, false, false, false, false },
        { ERHIFormat::R16_SINT,          "R16_SINT",          2,   1, ERHIFormatKind::Integer,      true,  false, false, false, false, false, true,  false },
        { ERHIFormat::R16_UNORM,         "R16_UNORM",         2,   1, ERHIFormatKind::Normalized,   true,  false, false, false, false, false, false, false },
        { ERHIFormat::R16_SNORM,         "R16_SNORM",         2,   1, ERHIFormatKind::Normalized,   true,  false, false, false, false, false, false, false },
        { ERHIFormat::R16_FLOAT,         "R16_FLOAT",         2,   1, ERHIFormatKind::Float,        true,  false, false, false, false, false, true,  false },
        { ERHIFormat::BGRA4_UNORM,       "BGRA4_UNORM",       2,   1, ERHIFormatKind::Normalized,   true,  true,  true,  true,  false, false, false, false },
        { ERHIFormat::B5G6R5_UNORM,      "B5G6R5_UNORM",      2,   1, ERHIFormatKind::Normalized,   true,  true,  true,  false, false, false, false, false },
        { ERHIFormat::B5G5R5A1_UNORM,    "B5G5R5A1_UNORM",    2,   1, ERHIFormatKind::Normalized,   true,  true,  true,  true,  false, false, false, false },
        { ERHIFormat::RGBA8_UINT,        "RGBA8_UINT",        4,   1, ERHIFormatKind::Integer,      true,  true,  true,  true,  false, false, false, false },
        { ERHIFormat::RGBA8_SINT,        "RGBA8_SINT",        4,   1, ERHIFormatKind::Integer,      true,  true,  true,  true,  false, false, true,  false },
        { ERHIFormat::RGBA8_UNORM,       "RGBA8_UNORM",       4,   1, ERHIFormatKind::Normalized,   true,  true,  true,  true,  false, false, false, false },
        { ERHIFormat::RGBA8_SNORM,       "RGBA8_SNORM",       4,   1, ERHIFormatKind::Normalized,   true,  true,  true,  true,  false, false, false, false },
        { ERHIFormat::BGRA8_UNORM,       "BGRA8_UNORM",       4,   1, ERHIFormatKind::Normalized,   true,  true,  true,  true,  false, false, false, false },
        { ERHIFormat::SRGBA8_UNORM,      "SRGBA8_UNORM",      4,   1, ERHIFormatKind::Normalized,   true,  true,  true,  true,  false, false, false, true  },
        { ERHIFormat::SBGRA8_UNORM,      "SBGRA8_UNORM",      4,   1, ERHIFormatKind::Normalized,   true,  true,  true,  true,  false, false, false, false },
        { ERHIFormat::R10G10B10A2_UNORM, "R10G10B10A2_UNORM", 4,   1, ERHIFormatKind::Normalized,   true,  true,  true,  true,  false, false, false, false },
        { ERHIFormat::R11G11B10_FLOAT,   "R11G11B10_FLOAT",   4,   1, ERHIFormatKind::Float,        true,  true,  true,  false, false, false, false, false },
        { ERHIFormat::RG16_UINT,         "RG16_UINT",         4,   1, ERHIFormatKind::Integer,      true,  true,  false, false, false, false, false, false },
        { ERHIFormat::RG16_SINT,         "RG16_SINT",         4,   1, ERHIFormatKind::Integer,      true,  true,  false, false, false, false, true,  false },
        { ERHIFormat::RG16_UNORM,        "RG16_UNORM",        4,   1, ERHIFormatKind::Normalized,   true,  true,  false, false, false, false, false, false },
        { ERHIFormat::RG16_SNORM,        "RG16_SNORM",        4,   1, ERHIFormatKind::Normalized,   true,  true,  false, false, false, false, false, false },
        { ERHIFormat::RG16_FLOAT,        "RG16_FLOAT",        4,   1, ERHIFormatKind::Float,        true,  true,  false, false, false, false, true,  false },
        { ERHIFormat::R32_UINT,          "R32_UINT",          4,   1, ERHIFormatKind::Integer,      true,  false, false, false, false, false, false, false },
        { ERHIFormat::R32_SINT,          "R32_SINT",          4,   1, ERHIFormatKind::Integer,      true,  false, false, false, false, false, true,  false },
        { ERHIFormat::R32_FLOAT,         "R32_FLOAT",         4,   1, ERHIFormatKind::Float,        true,  false, false, false, false, false, true,  false },
        { ERHIFormat::RGBA16_UINT,       "RGBA16_UINT",       8,   1, ERHIFormatKind::Integer,      true,  true,  true,  true,  false, false, false, false },
        { ERHIFormat::RGBA16_SINT,       "RGBA16_SINT",       8,   1, ERHIFormatKind::Integer,      true,  true,  true,  true,  false, false, true,  false },
        { ERHIFormat::RGBA16_FLOAT,      "RGBA16_FLOAT",      8,   1, ERHIFormatKind::Float,        true,  true,  true,  true,  false, false, true,  false },
        { ERHIFormat::RGBA16_UNORM,      "RGBA16_UNORM",      8,   1, ERHIFormatKind::Normalized,   true,  true,  true,  true,  false, false, false, false },
        { ERHIFormat::RGBA16_SNORM,      "RGBA16_SNORM",      8,   1, ERHIFormatKind::Normalized,   true,  true,  true,  true,  false, false, false, false },
        { ERHIFormat::RG32_UINT,         "RG32_UINT",         8,   1, ERHIFormatKind::Integer,      true,  true,  false, false, false, false, false, false },
        { ERHIFormat::RG32_SINT,         "RG32_SINT",         8,   1, ERHIFormatKind::Integer,      true,  true,  false, false, false, false, true,  false },
        { ERHIFormat::RG32_FLOAT,        "RG32_FLOAT",        8,   1, ERHIFormatKind::Float,        true,  true,  false, false, false, false, true,  false },
        { ERHIFormat::RGB32_UINT,        "RGB32_UINT",        12,  1, ERHIFormatKind::Integer,      true,  true,  true,  false, false, false, false, false },
        { ERHIFormat::RGB32_SINT,        "RGB32_SINT",        12,  1, ERHIFormatKind::Integer,      true,  true,  true,  false, false, false, true,  false },
        { ERHIFormat::RGB32_FLOAT,       "RGB32_FLOAT",       12,  1, ERHIFormatKind::Float,        true,  true,  true,  false, false, false, true,  false },
        { ERHIFormat::RGBA32_UINT,       "RGBA32_UINT",       16,  1, ERHIFormatKind::Integer,      true,  true,  true,  true,  false, false, false, false },
        { ERHIFormat::RGBA32_SINT,       "RGBA32_SINT",       16,  1, ERHIFormatKind::Integer,      true,  true,  true,  true,  false, false, true,  false },
        { ERHIFormat::RGBA32_FLOAT,      "RGBA32_FLOAT",      16,  1, ERHIFormatKind::Float,        true,  true,  true,  true,  false, false, true,  false },
        { ERHIFormat::D16,               "D16",               2,   1, ERHIFormatKind::DepthStencil, false, false, false, false, true,  false, false, false },
        { ERHIFormat::D24S8,             "D24S8",             4,   1, ERHIFormatKind::DepthStencil, false, false, false, false, true,  true,  false, false },
        { ERHIFormat::X24G8_UINT,        "X24G8_UINT",        4,   1, ERHIFormatKind::Integer,      false, false, false, false, false, true,  false, false },
        { ERHIFormat::D32,               "D32",               4,   1, ERHIFormatKind::DepthStencil, false, false, false, false, true,  false, false, false },
        { ERHIFormat::D32S8,             "D32S8",             8,   1, ERHIFormatKind::DepthStencil, false, false, false, false, true,  true,  false, false },
        { ERHIFormat::X32G8_UINT,        "X32G8_UINT",        8,   1, ERHIFormatKind::Integer,      false, false, false, false, false, true,  false, false },
        { ERHIFormat::BC1_UNORM,         "BC1_UNORM",         8,   4, ERHIFormatKind::Normalized,   true,  true,  true,  true,  false, false, false, false },
        { ERHIFormat::BC1_UNORM_SRGB,    "BC1_UNORM_SRGB",    8,   4, ERHIFormatKind::Normalized,   true,  true,  true,  true,  false, false, false, true  },
        { ERHIFormat::BC2_UNORM,         "BC2_UNORM",         16,  4, ERHIFormatKind::Normalized,   true,  true,  true,  true,  false, false, false, false },
        { ERHIFormat::BC2_UNORM_SRGB,    "BC2_UNORM_SRGB",    16,  4, ERHIFormatKind::Normalized,   true,  true,  true,  true,  false, false, false, true  },
        { ERHIFormat::BC3_UNORM,         "BC3_UNORM",         16,  4, ERHIFormatKind::Normalized,   true,  true,  true,  true,  false, false, false, false },
        { ERHIFormat::BC3_UNORM_SRGB,    "BC3_UNORM_SRGB",    16,  4, ERHIFormatKind::Normalized,   true,  true,  true,  true,  false, false, false, true  },
        { ERHIFormat::BC4_UNORM,         "BC4_UNORM",         8,   4, ERHIFormatKind::Normalized,   true,  false, false, false, false, false, false, false },
        { ERHIFormat::BC4_SNORM,         "BC4_SNORM",         8,   4, ERHIFormatKind::Normalized,   true,  false, false, false, false, false, false, false },
        { ERHIFormat::BC5_UNORM,         "BC5_UNORM",         16,  4, ERHIFormatKind::Normalized,   true,  true,  false, false, false, false, false, false },
        { ERHIFormat::BC5_SNORM,         "BC5_SNORM",         16,  4, ERHIFormatKind::Normalized,   true,  true,  false, false, false, false, false, false },
        { ERHIFormat::BC6H_UFLOAT,       "BC6H_UFLOAT",       16,  4, ERHIFormatKind::Float,        true,  true,  true,  false, false, false, false, false },
        { ERHIFormat::BC6H_SFLOAT,       "BC6H_SFLOAT",       16,  4, ERHIFormatKind::Float,        true,  true,  true,  false, false, false, true,  false },
        { ERHIFormat::BC7_UNORM,         "BC7_UNORM",         16,  4, ERHIFormatKind::Normalized,   true,  true,  true,  true,  false, false, false, false },
        { ERHIFormat::BC7_UNORM_SRGB,    "BC7_UNORM_SRGB",    16,  4, ERHIFormatKind::Normalized,   true,  true,  true,  true,  false, false, false, true  },
    };

    const RHIFormatInfo& GetFormatInfo(ERHIFormat inFormat)
    {
        const RHIFormatInfo& info = s_FormatInfo[uint32_t(inFormat)];
        return info;
    }
}

const RHITextureSubResource RHITextureSubResource::All {0, UINT32_MAX, 0, UINT32_MAX};
const RHIBufferSubRange RHIBufferSubRange::All{0, UINT64_MAX, 0, UINT32_MAX, UINT32_MAX};
const RHIClearValue RHIClearValue::Black(0, 0, 0);
const RHIClearValue RHIClearValue::White(1.0f, 1.0f, 1.0f);
const RHIClearValue RHIClearValue::Green(0.0f, 1.0f, 0.0f);
const RHIClearValue RHIClearValue::Red(1.0f, 0.0f, 0.0f);
const RHIClearValue RHIClearValue::Transparent(0.0f, 0.0f, 0.0f, 0.0f);
const RHIClearValue RHIClearValue::DepthOne(1.0f, 0);
const RHIClearValue RHIClearValue::DepthZero(0.0f, 0);

RHIViewport RHIViewport::Create(float Width, float Height)
{
    return RHIViewport{0, 0, Width, Height, 0, 1};
}

RHIViewport RHIViewport::Create(float X, float Y, float Width, float Height)
{
    return RHIViewport{X, Y, Width, Height, 0, 1};
}

RHIViewport RHIViewport::Create(float X, float Y, float Width, float Height, float MinDepth, float MaxDepth)
{
    return RHIViewport{X, Y, Width, Height, MinDepth, MaxDepth};
}

RHIRect RHIRect::Create(uint32_t Width, uint32_t Height)
{
    return RHIRect{0, 0, Width, Height};
}

RHIRect RHIRect::Create(int32_t MinX, int32_t MinY, uint32_t Width, uint32_t Height)
{
    return RHIRect{MinX, MinY, Width, Height};
}