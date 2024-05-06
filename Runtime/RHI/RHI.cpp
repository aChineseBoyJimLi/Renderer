#include "RHI.h"
#include <mutex>

#include "RHIResources.h"
#include "D3D12/D3D12Device.h"
#if HAS_VULKAN
#include "Vulkan/VulkanDevice.h"
#endif

namespace RHI
{
    static RHIDevice* s_Device = nullptr;
    static std::mutex s_Mtx;
    
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
}

const RHITextureSubResource RHITextureSubResource::All {0, UINT32_MAX, 0, UINT32_MAX};

const RHIBufferSubRange RHIBufferSubRange::All{0, UINT64_MAX, 0, UINT32_MAX, UINT32_MAX};

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