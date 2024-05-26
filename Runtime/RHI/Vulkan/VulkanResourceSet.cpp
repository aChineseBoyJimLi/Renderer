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

    const RHIPipelineBindingLayoutDesc& rhiLayoutDesc = m_LayoutVulkan->GetDesc();

    std::unordered_map<uint32_t, std::vector<uint32_t>> bindlessDescriptorsCount;
    
    
    for(const auto& bindingItem : rhiLayoutDesc.Items)
    {
        if(bindingItem.IsBindless)
        {
            bindlessDescriptorsCount[bindingItem.Space].emplace_back(bindingItem.NumResources);
        }
    }

    m_DescriptorSet.resize(m_LayoutVulkan->GetDescriptorSetLayoutCount());

    const VkDescriptorSetLayout* layouts = m_LayoutVulkan->GetDescriptorSetLayouts();

    for(uint32_t i = 0; i < m_LayoutVulkan->GetDescriptorSetLayoutCount(); ++i)
    {
        bool hasBindless = false;
        VkDescriptorSetVariableDescriptorCountAllocateInfoEXT variableDescriptorCountAllocInfo{};
        if(bindlessDescriptorsCount.find(i) != bindlessDescriptorsCount.end())
        {
            variableDescriptorCountAllocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
            variableDescriptorCountAllocInfo.descriptorSetCount = (uint32_t)bindlessDescriptorsCount[i].size();
            variableDescriptorCountAllocInfo.pDescriptorCounts  = bindlessDescriptorsCount[i].data();
            hasBindless = true;
        }
        
        VkDescriptorSetAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.descriptorPool = m_Device.GetDescriptorPool();
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &layouts[i];
        allocateInfo.pNext = hasBindless ? &variableDescriptorCountAllocInfo : nullptr;

        VkResult result = vkAllocateDescriptorSets(m_Device.GetDevice(), &allocateInfo, &m_DescriptorSet[i]);
        if(result != VK_SUCCESS)
        {
            Log::Error("[Vulkan] Failed to allocate descriptor sets");
            return false;
        }
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

static ERHIRegisterType ConvertRegisterType(ERHIBindingResourceType inViewType)
{
    switch (inViewType)
    {
    case ERHIBindingResourceType::Texture_SRV:
    case ERHIBindingResourceType::Buffer_SRV:
        return ERHIRegisterType::ShaderResource;
    case ERHIBindingResourceType::AccelerationStructure:
    case ERHIBindingResourceType::Texture_UAV:
    case ERHIBindingResourceType::Buffer_UAV:
        return ERHIRegisterType::UnorderedAccess;
    case ERHIBindingResourceType::Buffer_CBV:
        return ERHIRegisterType::ConstantBuffer;
    case ERHIBindingResourceType::Sampler:
        return ERHIRegisterType::Sampler;
    }
    return ERHIRegisterType::ShaderResource;
}

void VulkanResourceSet::BindBuffer(ERHIBindingResourceType inViewType, uint32_t inRegister, uint32_t inSpace, const RefCountPtr<RHIBuffer>& inBuffer)
{
    ERHIRegisterType registerType = ConvertRegisterType(inViewType);
    VulkanBuffer* buffer = CheckCast<VulkanBuffer*>(inBuffer.GetReference());
    VkDescriptorBufferInfo bufferInfo = buffer->GetDescriptorBufferInfo();
    VkWriteDescriptorSet descriptorSetWriter{};
    descriptorSetWriter.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorSetWriter.dstSet = m_DescriptorSet[inSpace];
    descriptorSetWriter.dstBinding = RHI::Vulkan::GetBindingSlot(registerType, inRegister);
    descriptorSetWriter.descriptorType = RHI::Vulkan::ConvertDescriptorType(inViewType);
    descriptorSetWriter.pBufferInfo = &bufferInfo;
    descriptorSetWriter.descriptorCount = 1;
    vkUpdateDescriptorSets(m_Device.GetDevice(), 1, &descriptorSetWriter, 0, nullptr);
}

void VulkanResourceSet::BindBufferArray(ERHIBindingResourceType inViewType, uint32_t inRegister, uint32_t inSpace, const std::vector<RefCountPtr<RHIBuffer>>& inBuffer)
{
    if(inBuffer.empty())
        return;
    
    ERHIRegisterType registerType = ConvertRegisterType(inViewType);
    std::vector<VkDescriptorBufferInfo> bufferInfos(inBuffer.size());
    
    for(uint32_t i = 0; i < inBuffer.size(); ++i)
    {
        VulkanBuffer* buffer = CheckCast<VulkanBuffer*>(inBuffer[i].GetReference());
        bufferInfos[i] = buffer->GetDescriptorBufferInfo();
    }

    VkWriteDescriptorSet descriptorSetWriter{};
    descriptorSetWriter.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorSetWriter.dstSet = m_DescriptorSet[inSpace];
    descriptorSetWriter.dstBinding = RHI::Vulkan::GetBindingSlot(registerType, inRegister);
    descriptorSetWriter.descriptorType  = RHI::Vulkan::ConvertDescriptorType(inViewType);
    descriptorSetWriter.pBufferInfo = bufferInfos.data();
    descriptorSetWriter.descriptorCount = (uint32_t)bufferInfos.size();
    vkUpdateDescriptorSets(m_Device.GetDevice(), 1, &descriptorSetWriter, 0, nullptr);
}

void VulkanResourceSet::BindTexture(ERHIBindingResourceType inViewType, uint32_t inRegister, uint32_t inSpace, const RefCountPtr<RHITexture>& inTexture)
{
    ERHIRegisterType registerType = ConvertRegisterType(inViewType);
    VulkanTexture* texture = CheckCast<VulkanTexture*>(inTexture.GetReference());
    VkDescriptorImageInfo imageInfo = texture->GetDescriptorImageInfo(inViewType);
    VkWriteDescriptorSet descriptorSetWriter{};
    descriptorSetWriter.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorSetWriter.dstSet = m_DescriptorSet[inSpace];
    descriptorSetWriter.dstBinding = RHI::Vulkan::GetBindingSlot(registerType, inRegister);
    descriptorSetWriter.descriptorType  = RHI::Vulkan::ConvertDescriptorType(inViewType);
    descriptorSetWriter.pImageInfo = &imageInfo;
    descriptorSetWriter.descriptorCount = 1;
    vkUpdateDescriptorSets(m_Device.GetDevice(), 1, &descriptorSetWriter, 0, nullptr);
}

void VulkanResourceSet::BindTextureArray(ERHIBindingResourceType inViewType, uint32_t inRegister, uint32_t inSpace, const std::vector<RefCountPtr<RHITexture>>& inTexture)
{
    if(inTexture.empty())
        return;
    
    ERHIRegisterType registerType = ConvertRegisterType(inViewType);
    std::vector<VkDescriptorImageInfo> imageInfos(inTexture.size());
    
    for(uint32_t i = 0; i < inTexture.size(); ++i)
    {
        VulkanTexture* texture = CheckCast<VulkanTexture*>(inTexture[i].GetReference());
        imageInfos[i] = texture->GetDescriptorImageInfo(inViewType);
    }

    VkWriteDescriptorSet descriptorSetWriter{};
    descriptorSetWriter.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorSetWriter.dstSet = m_DescriptorSet[inSpace];
    descriptorSetWriter.dstBinding = RHI::Vulkan::GetBindingSlot(registerType, inRegister);
    descriptorSetWriter.descriptorType  = RHI::Vulkan::ConvertDescriptorType(inViewType);
    descriptorSetWriter.pImageInfo = imageInfos.data();
    descriptorSetWriter.descriptorCount = (uint32_t)imageInfos.size();
    vkUpdateDescriptorSets(m_Device.GetDevice(), 1, &descriptorSetWriter, 0, nullptr);
}

void VulkanResourceSet::BindBufferSRV(uint32_t inRegister, uint32_t inSpace, const RefCountPtr<RHIBuffer>& inBuffer)
{
    BindBuffer(ERHIBindingResourceType::Buffer_SRV, inRegister, inSpace, inBuffer);
}

void VulkanResourceSet::BindBufferUAV(uint32_t inRegister, uint32_t inSpace, const RefCountPtr<RHIBuffer>& inBuffer)
{
    BindBuffer(ERHIBindingResourceType::Buffer_UAV, inRegister, inSpace, inBuffer);
}

void VulkanResourceSet::BindBufferCBV(uint32_t inRegister, uint32_t inSpace, const RefCountPtr<RHIBuffer>& inBuffer)
{
    BindBuffer(ERHIBindingResourceType::Buffer_CBV, inRegister, inSpace, inBuffer);
}

void VulkanResourceSet::BindTextureSRV(uint32_t inRegister, uint32_t inSpace, const RefCountPtr<RHITexture>& inTexture)
{
    BindTexture(ERHIBindingResourceType::Texture_SRV, inRegister, inSpace, inTexture);
}

void VulkanResourceSet::BindTextureSRVArray(uint32_t inBaseRegister, uint32_t inSpace, const std::vector<RefCountPtr<RHITexture>>& inTextures)
{
    BindTextureArray(ERHIBindingResourceType::Texture_SRV, inBaseRegister, inSpace, inTextures);
}

void VulkanResourceSet::BindTextureUAVArray(uint32_t inBaseRegister, uint32_t inSpace, const std::vector<RefCountPtr<RHITexture>>& inTextures)
{
    BindTextureArray(ERHIBindingResourceType::Texture_UAV, inBaseRegister, inSpace, inTextures);
}

void VulkanResourceSet::BindTextureUAV(uint32_t inRegister, uint32_t inSpace, const RefCountPtr<RHITexture>& inTexture)
{
    BindTexture(ERHIBindingResourceType::Texture_UAV, inRegister, inSpace, inTexture);
}

void VulkanResourceSet::BindBufferSRVArray(uint32_t inBaseRegister, uint32_t inSpace, const std::vector<RefCountPtr<RHIBuffer>>& inBuffer)
{
    BindBufferArray(ERHIBindingResourceType::Buffer_SRV, inBaseRegister, inSpace, inBuffer);
}

void VulkanResourceSet::BindBufferUAVArray(uint32_t inBaseRegister, uint32_t inSpace, const std::vector<RefCountPtr<RHIBuffer>>& inBuffer)
{
    BindBufferArray(ERHIBindingResourceType::Buffer_UAV, inBaseRegister, inSpace, inBuffer);
}

void VulkanResourceSet::BindBufferCBVArray(uint32_t inBaseRegister, uint32_t inSpace, const std::vector<RefCountPtr<RHIBuffer>>& inBuffer)
{
    BindBufferArray(ERHIBindingResourceType::Buffer_CBV, inBaseRegister, inSpace, inBuffer);
}

void VulkanResourceSet::BindSampler(uint32_t inRegister, uint32_t inSpace, const RefCountPtr<RHISampler>& inSampler)
{
    VulkanSampler* sampler = CheckCast<VulkanSampler*>(inSampler.GetReference());
    VkDescriptorImageInfo samplerInfo{};
    samplerInfo.sampler = sampler->GetSampler();
    samplerInfo.imageView = VK_NULL_HANDLE;
    VkWriteDescriptorSet descriptorSetWriter{};
    descriptorSetWriter.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorSetWriter.dstSet = m_DescriptorSet[inSpace];
    descriptorSetWriter.dstBinding = RHI::Vulkan::GetBindingSlot(ERHIRegisterType::Sampler, inRegister);
    descriptorSetWriter.descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER;
    descriptorSetWriter.pImageInfo = &samplerInfo;
    descriptorSetWriter.descriptorCount = 1;
    vkUpdateDescriptorSets(m_Device.GetDevice(), 1, &descriptorSetWriter, 0, nullptr);
}

void VulkanResourceSet::BindSamplerArray(uint32_t inBaseRegister, uint32_t inSpace, const std::vector<RefCountPtr<RHISampler>>& inSampler)
{
    if(inSampler.empty())
        return;

    std::vector<VkDescriptorImageInfo> samplerInfos(inSampler.size());
    
    for(uint32_t i = 0; i < inSampler.size(); ++i)
    {
        VulkanSampler* sampler = CheckCast<VulkanSampler*>(inSampler[i].GetReference());
        samplerInfos[i].sampler = sampler->GetSampler();
        samplerInfos[i].imageView = VK_NULL_HANDLE;
    }

    VkWriteDescriptorSet descriptorSetWriter{};
    descriptorSetWriter.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorSetWriter.dstSet = m_DescriptorSet[inSpace];
    descriptorSetWriter.dstBinding = RHI::Vulkan::GetBindingSlot(ERHIRegisterType::Sampler, inBaseRegister);
    descriptorSetWriter.descriptorType  = VK_DESCRIPTOR_TYPE_SAMPLER;
    descriptorSetWriter.pImageInfo = samplerInfos.data();
    descriptorSetWriter.descriptorCount = (uint32_t)samplerInfos.size();
    vkUpdateDescriptorSets(m_Device.GetDevice(), 1, &descriptorSetWriter, 0, nullptr);
}

void VulkanResourceSet::BindAccelerationStructure(uint32_t inRegister, uint32_t inSpace, const RefCountPtr<RHIAccelerationStructure>& inAccelerationStructure)
{
    VulkanAccelerationStructure* accelerationStructure = CheckCast<VulkanAccelerationStructure*>(inAccelerationStructure.GetReference());
    if(IsValid() && accelerationStructure && accelerationStructure->IsValid() && accelerationStructure->IsTopLevel())
    {
        VkAccelerationStructureKHR tlas = accelerationStructure->GetAccelerationStructure();
        VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo{};
        descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
        descriptorAccelerationStructureInfo.pAccelerationStructures = &tlas;
        descriptorAccelerationStructureInfo.pNext = nullptr;

        VkWriteDescriptorSet descriptorSetWriter{};
        descriptorSetWriter.pNext = &descriptorAccelerationStructureInfo;
        descriptorSetWriter.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorSetWriter.dstSet = m_DescriptorSet[inSpace];
        descriptorSetWriter.dstBinding =  RHI::Vulkan::GetBindingSlot(ERHIRegisterType::ShaderResource, inRegister); 
        descriptorSetWriter.descriptorCount = 1;
        descriptorSetWriter.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        vkUpdateDescriptorSets(m_Device.GetDevice(), 1, &descriptorSetWriter, 0, nullptr);
    }
}