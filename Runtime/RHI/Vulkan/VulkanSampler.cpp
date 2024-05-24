#include "VulkanResources.h"
#include "VulkanDevice.h"
#include "../../Core/Log.h"
#include "../../Core/Templates.h"
#include <limits>

#ifdef max
#undef max
#endif

namespace RHI::Vulkan
{
    static VkBorderColor PickSamplerBorderColor(const RHISamplerDesc& d)
    {
        if (d.BorderColor[0] == 0.f && d.BorderColor[1] == 0.f && d.BorderColor[2] == 0.f)
        {
            if (d.BorderColor[3] == 0.f)
            {
                return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
            }

            if (d.BorderColor[3] == 1.f)
            {
                return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
            }
        }

        if (d.BorderColor[0] == 1.f && d.BorderColor[1] == 1.f && d.BorderColor[2] == 1.f)
        {
            if (d.BorderColor[3] == 1.f)
            {
                return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
            }
        }
        Log::Warning("No suitable border color found, using opaque black as default");
        return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    }
}

RefCountPtr<RHISampler> VulkanDevice::CreateSampler(const RHISamplerDesc& inDesc)
{
    RefCountPtr<RHISampler> sampler(new VulkanSampler(*this, inDesc));
    if(!sampler->Init())
    {
        Log::Error("[Vulkan] Failed to create sampler");
    }
    return sampler;
}

VulkanSampler::VulkanSampler(VulkanDevice& inDevice, const RHISamplerDesc& inDesc)
    : m_Device(inDevice)
    , m_Desc(inDesc)
    , m_SamplerHandle(VK_NULL_HANDLE)
{
    
}

VulkanSampler::~VulkanSampler()
{
    ShutdownInternal();
}

bool VulkanSampler::Init()
{
    if(IsValid())
    {
        Log::Warning("[Vulkan] Sampler has already initialized");
        return false;
    }

    const bool anisotropyEnable = m_Desc.MaxAnisotropy > 1.0f;

    VkSamplerCreateInfo samplerCreateInfos{};
    samplerCreateInfos.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfos.magFilter = m_Desc.MagFilter ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    samplerCreateInfos.minFilter = m_Desc.MinFilter ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
    samplerCreateInfos.mipmapMode = m_Desc.MinFilter ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerCreateInfos.addressModeU = RHI::Vulkan::ConvertSamplerAddressMode(m_Desc.AddressU);
    samplerCreateInfos.addressModeV = RHI::Vulkan::ConvertSamplerAddressMode(m_Desc.AddressV);
    samplerCreateInfos.addressModeW = RHI::Vulkan::ConvertSamplerAddressMode(m_Desc.AddressW);
    samplerCreateInfos.mipLodBias = m_Desc.MipBias;
    samplerCreateInfos.anisotropyEnable = anisotropyEnable;
    samplerCreateInfos.maxAnisotropy = anisotropyEnable ? m_Desc.MaxAnisotropy : 1.f;
    samplerCreateInfos.compareEnable = m_Desc.ReductionType == ERHISamplerReductionType::Comparison;
    samplerCreateInfos.compareOp = VK_COMPARE_OP_LESS;
    samplerCreateInfos.minLod = 0.f;
    samplerCreateInfos.maxLod = std::numeric_limits<float>::max();
    samplerCreateInfos.borderColor = RHI::Vulkan::PickSamplerBorderColor(m_Desc);
    
    VkSamplerReductionModeCreateInfoEXT samplerReductionCreateInfo{};
    if (m_Desc.ReductionType == ERHISamplerReductionType::Minimum || m_Desc.ReductionType == ERHISamplerReductionType::Maximum)
    {
        samplerReductionCreateInfo.reductionMode = m_Desc.ReductionType == ERHISamplerReductionType::Maximum ? VK_SAMPLER_REDUCTION_MODE_MAX : VK_SAMPLER_REDUCTION_MODE_MIN;
        samplerCreateInfos.pNext = &samplerReductionCreateInfo;
    }

    VkResult res = vkCreateSampler(m_Device.GetDevice(), &samplerCreateInfos, nullptr, &m_SamplerHandle);
    if(res != VK_SUCCESS)
    {
        OUTPUT_VULKAN_FAILED_RESULT(res)
        return false;
    }
    
    return true;
}

void VulkanSampler::Shutdown()
{
    ShutdownInternal();
}

bool VulkanSampler::IsValid() const
{
    return m_SamplerHandle != VK_NULL_HANDLE;
}

void VulkanSampler::ShutdownInternal()
{
    if(m_SamplerHandle != VK_NULL_HANDLE)
    {
        vkDestroySampler(m_Device.GetDevice(), m_SamplerHandle, nullptr);
        m_SamplerHandle = VK_NULL_HANDLE;
    }
}

void VulkanSampler::SetNameInternal()
{
    if(IsValid())
    {
        m_Device.SetDebugName(VK_OBJECT_TYPE_SAMPLER, reinterpret_cast<uint64_t>(m_SamplerHandle), m_Name);
    }
}
