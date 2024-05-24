#include "VulkanDevice.h"
#include "VulkanResources.h"
#include "VulkanPipelineState.h"
#include "../../Core/Log.h"
#include "../../Core/Templates.h"

RefCountPtr<RHIResourceSet> VulkanDevice::CreateResourceSet(const RHIPipelineBindingLayout* inLayout)
{
    RefCountPtr<RHIResourceSet> resourceSet(new VulkanResourceSet(*this, inLayout));
    if(!resourceSet->Init())
    {
        Log::Error("[Vulkan] Failed to create resource set");
    }
    return resourceSet;
}

VulkanResourceSet::VulkanResourceSet(VulkanDevice& inDevice, const RHIPipelineBindingLayout* inLayout)
    : m_Device(inDevice)
    , m_Layout(inLayout)
    , m_LayoutVulkan(nullptr)
{
    
}

VulkanResourceSet::~VulkanResourceSet()
{
    ShutdownInternal();
}

bool VulkanResourceSet::Init()
{
    m_LayoutVulkan = CheckCast<const VulkanPipelineBindingLayout*>(m_Layout);
    if(m_LayoutVulkan == nullptr || !m_LayoutVulkan->IsValid())
    {
        Log::Error("[Vulkan] D3D12PipelineBindingLayout is invalid");
        return false;
    }

    m_DescriptorSet.resize(m_LayoutVulkan->GetDescriptorSetLayoutCount());

    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = m_Device.GetDescriptorPool();
    allocateInfo.descriptorSetCount = static_cast<uint32_t>(m_DescriptorSet.size());
    allocateInfo.pSetLayouts = m_LayoutVulkan->GetDescriptorSetLayouts();
    
    VkResult result = vkAllocateDescriptorSets(m_Device.GetDevice(), &allocateInfo, m_DescriptorSet.data());
    if(result != VK_SUCCESS)
    {
        Log::Error("[Vulkan] Failed to allocate descriptor sets");
        return false;
    }
    
    return true;
}

void VulkanResourceSet::Shutdown()
{
    ShutdownInternal();
}

bool VulkanResourceSet::IsValid() const
{
    return m_LayoutVulkan && !m_DescriptorSet.empty();
}

void VulkanResourceSet::ShutdownInternal()
{
    if(!m_DescriptorSet.empty())
    {
        vkFreeDescriptorSets(m_Device.GetDevice(), m_Device.GetDescriptorPool(), static_cast<uint32_t>(m_DescriptorSet.size()), m_DescriptorSet.data());
        m_DescriptorSet.clear();
    }
}

void VulkanResourceSet::BindBufferSRV(uint32_t inRegister, uint32_t inSpace, const RefCountPtr<RHIBuffer>& inBuffer)
{
    
}

void VulkanResourceSet::BindBufferUAV(uint32_t inRegister, uint32_t inSpace, const RefCountPtr<RHIBuffer>& inBuffer)
{
    
}

void VulkanResourceSet::BindBufferCBV(uint32_t inRegister, uint32_t inSpace, const RefCountPtr<RHIBuffer>& inBuffer)
{
    
}

void VulkanResourceSet::BindTextureSRV(uint32_t inRegister, uint32_t inSpace, const RefCountPtr<RHITexture>& inTexture)
{
    
}

void VulkanResourceSet::BindTextureUAV(uint32_t inRegister, uint32_t inSpace, const RefCountPtr<RHITexture>& inTexture)
{
    
}

void VulkanResourceSet::BindSampler(uint32_t inRegister, uint32_t inSpace, const RefCountPtr<RHISampler>& inSampler)
{
    
}





