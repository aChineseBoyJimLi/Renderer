#pragma once

#include "../RHIResources.h"
#include "../RHIDevice.h"
#include "VulkanDefinitions.h"

class VulkanTexture;

class VulkanFence : public RHIFence
{
public:
    ~VulkanFence() override;
    bool Init() override;
    void Shutdown() override;
    bool IsValid() const override;
    void Reset() override;
    void CpuWait() override;

    VkFence GetFence() const { return m_FenceHandle; }

protected:
    void SetNameInternal() override;

private:
    friend class VulkanDevice;
    VulkanFence(VulkanDevice& inDevice);
    void ShutdownInternal();
    
    VulkanDevice& m_Device;
    VkFence m_FenceHandle;
};

class VulkanSemaphore : public RHISemaphore
{
public:
    ~VulkanSemaphore() override;
    bool Init() override;
    void Shutdown() override;
    bool IsValid() const override;
    void Reset() override;
    
    VkSemaphore GetSemaphore() const { return m_SemaphoreHandle; }

protected:
    void SetNameInternal() override;
    
private:
    friend class VulkanDevice;
    VulkanSemaphore(VulkanDevice& inDevice);
    void ShutdownInternal();
    
    VulkanDevice& m_Device;
    VkSemaphore m_SemaphoreHandle;
};

class VulkanDevice : public RHIDevice
{
public:
    ~VulkanDevice() override;
    bool Init() override;
    void Shutdown() override;
    bool IsValid() const override;
    
    RefCountPtr<RHIFence>       CreateRhiFence() override;
    RefCountPtr<RHISemaphore>   CreateRhiSemaphore() override;
    RefCountPtr<RHICommandList> CreateCommandList(ERHICommandQueueType inType = ERHICommandQueueType::Direct) override;
    RefCountPtr<RHIPipelineBindingLayout> CreatePipelineBindingLayout(const RHIPipelineBindingLayoutDesc& inBindingItems) override;
    RefCountPtr<RHIShader> CreateShader(ERHIShaderType inType) override;
    RefCountPtr<RHIComputePipeline> CreatePipeline(const RHIComputePipelineDesc& inDesc) override;
    RefCountPtr<RHIGraphicsPipeline> CreatePipeline(const RHIGraphicsPipelineDesc& inDesc) override;
    RefCountPtr<RHIResourceHeap> CreateResourceHeap(const RHIResourceHeapDesc& inDesc) override;
    RefCountPtr<RHIBuffer> CreateBuffer(const RHIBufferDesc& inDesc, bool isVirtual = false) override;
    RefCountPtr<RHITexture> CreateTexture(const RHITextureDesc& inDesc, bool isVirtual = false) override;
    RefCountPtr<VulkanTexture> CreateTexture(const RHITextureDesc& inDesc, VkImage inImage);
    RefCountPtr<RHIFrameBuffer> CreateFrameBuffer(const RHIFrameBufferDesc& inDesc) override;
    void ExecuteCommandList(const RefCountPtr<RHICommandList>& inCommandList, const RefCountPtr<RHIFence>& inSignalFence = nullptr,
                            const std::vector<RefCountPtr<RHISemaphore>>* inWaitForSemaphores = nullptr, 
                            const std::vector<RefCountPtr<RHISemaphore>>* inSignalSemaphores = nullptr) override;

    RefCountPtr<VulkanSemaphore> CreateVulkanSemaphore();
    void SetDebugName(VkObjectType objectType, uint64_t objectHandle, const std::string& name) const;
    ERHIBackend GetBackend() const override { return ERHIBackend::Vulkan; }
    VkInstance GetInstance() const { return m_InstanceHandle; }
    VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDeviceHandle; }
    VkDevice GetDevice() const { return m_DeviceHandle; }
    uint32_t GetQueueFamilyIndex(ERHICommandQueueType type) const {return m_QueueIndex[static_cast<uint32_t>(type)]; }
    VkQueue GetCommandQueue(ERHICommandQueueType type) const {return m_QueueHandles[static_cast<uint32_t>(type)]; }
    bool GetMemoryTypeIndex(uint32_t inTypeFilter, VkMemoryPropertyFlags inProperties, uint32_t& outMemTypeIndex) const;
    bool SupportBufferDeviceAddress() const {return m_SupportBufferDeviceAddress;}
    bool SupportVariableRateShading() const {return m_SupportVariableRateShading; }

    PFN_vkSetDebugUtilsObjectNameEXT                vkSetDebugUtilsObjectNameEXT;
    PFN_vkGetBufferDeviceAddressKHR                 vkGetBufferDeviceAddressKHR;
    PFN_vkCmdDrawMeshTasksEXT                       vkCmdDrawMeshTasksEXT;
    PFN_vkCreateAccelerationStructureKHR            vkCreateAccelerationStructureKHR;
    PFN_vkDestroyAccelerationStructureKHR           vkDestroyAccelerationStructureKHR;
    PFN_vkGetAccelerationStructureBuildSizesKHR     vkGetAccelerationStructureBuildSizesKHR;
    PFN_vkGetAccelerationStructureDeviceAddressKHR  vkGetAccelerationStructureDeviceAddressKHR;
    PFN_vkCmdBuildAccelerationStructuresKHR         vkCmdBuildAccelerationStructuresKHR;
    PFN_vkBuildAccelerationStructuresKHR            vkBuildAccelerationStructuresKHR;
    PFN_vkCmdTraceRaysKHR                           vkCmdTraceRaysKHR;
    PFN_vkGetRayTracingShaderGroupHandlesKHR        vkGetRayTracingShaderGroupHandlesKHR;
    PFN_vkCreateRayTracingPipelinesKHR              vkCreateRayTracingPipelinesKHR;
    
private:
    friend bool RHI::Init(bool useVulkan);
    VulkanDevice();
    void ShutdownInternal();
    void EnableDeviceExtensions(VkPhysicalDeviceFeatures2& deviceFeatures2);

    VkInstance                  m_InstanceHandle;
    VkDebugUtilsMessengerEXT    m_DebugMessenger;
    VkPhysicalDevice            m_PhysicalDeviceHandle;
    VkDevice                    m_DeviceHandle;
    std::array<int, COMMAND_QUEUES_COUNT>       m_QueueIndex;
    std::array<VkQueue, COMMAND_QUEUES_COUNT>   m_QueueHandles;
    
    VkPhysicalDeviceFeatures            m_PhysicalDeviceFeatures {};

    // PhysicalDevice Properties
    VkPhysicalDeviceMemoryProperties    m_PhysicalDeviceMemoryProperties {};
    VkPhysicalDeviceProperties          m_PhysicalDeviceProperties {};
    VkPhysicalDeviceFragmentShadingRatePropertiesKHR m_PhysicalDeviceShadingRateProperties{};
    
    bool m_SupportDescriptorIndexing {false};
    bool m_SupportSpirv14 {false};
    bool m_SupportPipelineLibrary {false};
    bool m_SupportShaderFloatControls {false};
    bool m_SupportBufferDeviceAddress {false};
    bool m_SupportRayTracing {false};
    bool m_SupportMeshShading {false};
    bool m_SupportVariableRateShading {false};
};