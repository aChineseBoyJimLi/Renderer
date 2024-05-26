#pragma once

#include "../RHIResources.h"
#include "../RHIDevice.h"
#include "VulkanDefinitions.h"

class VulkanBuffer;
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
    RefCountPtr<VulkanBuffer> CreateVulkanBuffer(const RHIBufferDesc& inDesc, bool isVirtual = false);
    RefCountPtr<RHITexture> CreateTexture(const RHITextureDesc& inDesc, bool isVirtual = false) override;
    RefCountPtr<VulkanTexture> CreateTexture(const RHITextureDesc& inDesc, VkImage inImage);
    RefCountPtr<RHISampler> CreateSampler(const RHISamplerDesc& inDesc) override;
    RefCountPtr<RHIAccelerationStructure> CreateBottomLevelAccelerationStructure(const std::vector<RHIRayTracingGeometryDesc>& inDesc) override;
    RefCountPtr<RHIAccelerationStructure> CreateTopLevelAccelerationStructure(const std::vector<RHIRayTracingInstanceDesc>& inDesc) override;
    RefCountPtr<RHIFrameBuffer> CreateFrameBuffer(const RHIFrameBufferDesc& inDesc) override;
    RefCountPtr<RHIResourceSet> CreateResourceSet(const RHIPipelineBindingLayout* inLayout) override;

    void AddQueueWaitForSemaphore(ERHICommandQueueType inType, RefCountPtr<RHISemaphore>& inSemaphore) override;
    void AddQueueSignalSemaphore(ERHICommandQueueType inType, RefCountPtr<RHISemaphore>& inSemaphore) override;
    void AddQueueWaitForSemaphore(ERHICommandQueueType inType, RefCountPtr<VulkanSemaphore>& inSemaphore);
    void AddQueueSignalSemaphore(ERHICommandQueueType inType, RefCountPtr<VulkanSemaphore>& inSemaphore);
    void ExecuteCommandList(const RefCountPtr<RHICommandList>& inCommandList, const RefCountPtr<RHIFence>& inSignalFence = nullptr) override;

    RefCountPtr<VulkanFence> CreateVulkanFence();
    RefCountPtr<VulkanSemaphore> CreateVulkanSemaphore();
    void SetDebugName(VkObjectType objectType, uint64_t objectHandle, const std::string& name) const;
    void BeginDebugMarker(const char* name, VkCommandBuffer cmdBuffer) const;
    void EndDebugMarker(VkCommandBuffer cmdBuffer) const;
    ERHIBackend GetBackend() const override { return ERHIBackend::Vulkan; }
    VkInstance GetInstance() const { return m_InstanceHandle; }
    VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDeviceHandle; }
    VkDevice GetDevice() const { return m_DeviceHandle; }
    VkDescriptorPool GetDescriptorPool() const { return m_DescriptorPoolHandle; }
    uint32_t GetQueueFamilyIndex(ERHICommandQueueType type) const {return m_QueueIndex[static_cast<uint32_t>(type)]; }
    VkQueue GetCommandQueue(ERHICommandQueueType type) const {return m_QueueHandles[static_cast<uint32_t>(type)]; }
    bool GetMemoryTypeIndex(uint32_t inTypeFilter, VkMemoryPropertyFlags inProperties, uint32_t& outMemTypeIndex) const;
    bool SupportBufferDeviceAddress() const {return m_SupportBufferDeviceAddress;}
    bool SupportVariableRateShading() const {return m_SupportVariableRateShading; }

    PFN_vkSetDebugUtilsObjectNameEXT                vkSetDebugUtilsObjectNameEXT;
    PFN_vkGetBufferDeviceAddressKHR                 vkGetBufferDeviceAddressKHR;
    PFN_vkCmdDrawMeshTasksEXT                       vkCmdDrawMeshTasksEXT;
    PFN_vkCmdDrawMeshTasksIndirectEXT               vkCmdDrawMeshTasksIndirectEXT;
    PFN_vkCreateAccelerationStructureKHR            vkCreateAccelerationStructureKHR;
    PFN_vkDestroyAccelerationStructureKHR           vkDestroyAccelerationStructureKHR;
    PFN_vkGetAccelerationStructureBuildSizesKHR     vkGetAccelerationStructureBuildSizesKHR;
    PFN_vkGetAccelerationStructureDeviceAddressKHR  vkGetAccelerationStructureDeviceAddressKHR;
    PFN_vkCmdBuildAccelerationStructuresKHR         vkCmdBuildAccelerationStructuresKHR;
    PFN_vkBuildAccelerationStructuresKHR            vkBuildAccelerationStructuresKHR;
    PFN_vkCmdTraceRaysKHR                           vkCmdTraceRaysKHR;
    PFN_vkGetRayTracingShaderGroupHandlesKHR        vkGetRayTracingShaderGroupHandlesKHR;
    PFN_vkCreateRayTracingPipelinesKHR              vkCreateRayTracingPipelinesKHR;

    PFN_vkCmdDebugMarkerBeginEXT                    vkCmdDebugMarkerBeginEXT;
    PFN_vkCmdDebugMarkerEndEXT                      vkCmdDebugMarkerEndEXT;
    
private:
    friend bool RHI::Init(bool useVulkan);
    VulkanDevice();
    void ShutdownInternal();
    void EnableDeviceExtensions(VkPhysicalDeviceFeatures2& deviceFeatures2);
    bool InitDescriptorPool();
    void AddQueueWaitForSemaphore(ERHICommandQueueType inType, VkSemaphore inSemaphore);
    void AddQueueSignalSemaphore(ERHICommandQueueType inType, VkSemaphore inSemaphore);

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

    bool m_SupportDebugMarker {false};
    bool m_SupportDescriptorIndexing {false};
    bool m_SupportSpirv14 {false};
    bool m_SupportPipelineLibrary {false};
    bool m_SupportShaderFloatControls {false};
    bool m_SupportBufferDeviceAddress {false};
    bool m_SupportRayTracing {false};
    bool m_SupportMeshShading {false};
    bool m_SupportVariableRateShading {false};

    VkDescriptorPool    m_DescriptorPoolHandle;

    std::array<std::vector<VkSemaphore>, COMMAND_QUEUES_COUNT> m_WaitForSemaphores;
    std::array<std::vector<VkSemaphore>, COMMAND_QUEUES_COUNT> m_SignalSemaphores;
};