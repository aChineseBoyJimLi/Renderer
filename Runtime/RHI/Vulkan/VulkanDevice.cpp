#include "VulkanDevice.h"
#include <vector>
#include "../../Core/Log.h"

static const std::vector<const char*> s_ValidationLayerNames = {
    "VK_LAYER_KHRONOS_validation",
};

static const std::vector<const char*> s_InstanceExtensions = {
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
#if _DEBUG || DEBUG
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif
};

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity
    , VkDebugUtilsMessageTypeFlagsEXT messageType
    , const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData
    , void* pUserData)
{
    if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        Log::Error(TEXT("Validation layer: %s"), pCallbackData->pMessage);
        return VK_FALSE;
    }
    else if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        Log::Warning(TEXT("Validation layer: %s"), pCallbackData->pMessage);
        return VK_SUCCESS;
    }
    else
    {
        Log::Info(TEXT("Validation layer: %s"), pCallbackData->pMessage);
        return VK_SUCCESS;
    }
}

static void LogPhysicalDeviceProperties(const VkPhysicalDeviceProperties& inProperties)
{
    Log::Info("----------------------------------------------------------------");
    Log::Info("GPU Name: %s", inProperties.deviceName);
    Log::Info("API Version: %d.%d.%d", VK_VERSION_MAJOR(inProperties.apiVersion), VK_VERSION_MINOR(inProperties.apiVersion), VK_VERSION_PATCH(inProperties.apiVersion));
    Log::Info("----------------------------------------------------------------");
}

VulkanDevice::VulkanDevice()
    : m_InstanceHandle(VK_NULL_HANDLE)
    , m_DebugMessenger(VK_NULL_HANDLE)
    , m_PhysicalDeviceHandle(VK_NULL_HANDLE)
    , m_DeviceHandle(VK_NULL_HANDLE)
    , m_QueueIndex {-1, -1, -1}
    , m_QueueHandles {VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE}
{
    
}

VulkanDevice::~VulkanDevice()
{
    ShutdownInternal();
}

bool VulkanDevice::Init()
{
    if(IsValid())
    {
        Log::Warning("[Vulkan] Device already initialized");
        return true;
    }
    
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan App";
    appInfo.pEngineName = "No Engine";
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo insCreateInfo{};
    insCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    insCreateInfo.pApplicationInfo = &appInfo;
    insCreateInfo.enabledExtensionCount = static_cast<uint32_t>(s_InstanceExtensions.size());
    insCreateInfo.ppEnabledExtensionNames = s_InstanceExtensions.data();

#if _DEBUG || DEBUG
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCreateInfo.pfnUserCallback = DebugCallback;
    
    insCreateInfo.enabledLayerCount = static_cast<uint32_t>(s_ValidationLayerNames.size());
    insCreateInfo.ppEnabledLayerNames = s_ValidationLayerNames.data();
    insCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
#else
    instanceCreateInfo.enabledLayerCount = 0;
    instanceCreateInfo.pNext = nullptr;
#endif

    VkResult result = vkCreateInstance(&insCreateInfo, nullptr, &m_InstanceHandle);
    if(result != VK_SUCCESS)
    {
        Log::Error("[Vulkan] Failed to create Vulkan instance");
        OUTPUT_VULKAN_FAILED_RESULT(result);
        return false;
    }

#if _DEBUG || DEBUG
    auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(m_InstanceHandle, "vkCreateDebugUtilsMessengerEXT");
    if(vkCreateDebugUtilsMessengerEXT)
    {
        result = vkCreateDebugUtilsMessengerEXT(m_InstanceHandle, &debugCreateInfo, nullptr, &m_DebugMessenger);
        if(result != VK_SUCCESS)
        {
            Log::Warning("[Vulkan] Failed to create debug messenger");
        }
    }
#endif

    // Enumerate all gpu devices
    uint32_t gpuCount = 0;
    vkEnumeratePhysicalDevices(m_InstanceHandle, &gpuCount, nullptr);
    if( gpuCount == 0)
    {
        Log::Error("[Vulkan] No GPU found");
        return false;
    }
    
    std::vector<VkPhysicalDevice> tempPhysicalDevices(gpuCount);
    vkEnumeratePhysicalDevices(m_InstanceHandle, &gpuCount, tempPhysicalDevices.data());
    for(uint32_t i = 0; i < gpuCount; ++i)
    {
        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(tempPhysicalDevices[i], &physicalDeviceProperties);
        LogPhysicalDeviceProperties(physicalDeviceProperties);

        if(physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU || physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU)
            continue;

        if(physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            m_PhysicalDeviceHandle = tempPhysicalDevices[i];
            m_PhysicalDeviceProperties = physicalDeviceProperties;
            break;
        }

        if(physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
        {
            m_PhysicalDeviceHandle = tempPhysicalDevices[i];
            m_PhysicalDeviceProperties = physicalDeviceProperties;
        }
    }

    if(m_PhysicalDeviceHandle != VK_NULL_HANDLE)
    {
        Log::Info("[Vulkan] Using gpu: %s", m_PhysicalDeviceProperties.deviceName);
    }
    else
    {
        Log::Error("[Vulkan] Failed to find a discrete GPU");
        return false;
    }

    vkGetPhysicalDeviceFeatures(m_PhysicalDeviceHandle, &m_PhysicalDeviceFeatures);
    vkGetPhysicalDeviceMemoryProperties(m_PhysicalDeviceHandle, &m_PhysicalDeviceMemoryProperties);

    // Enumerate all extensions
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(m_PhysicalDeviceHandle, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(m_PhysicalDeviceHandle, nullptr, &extensionCount, extensions.data());

    std::vector<const char*> deviceExtensions;
    deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    bool supportRayTracingPipeline = false, supportAccelerationStructure = false, supportMeshShader = false;
    bool supportDeferredHostOperations = false, supportRayQuery = false;
    for(const auto& extension : extensions)
    {
        if(strcmp(extension.extensionName, VK_KHR_SPIRV_1_4_EXTENSION_NAME) == 0)
        {
            Log::Info("[Vulkan] GPU supports SPIR-V 1.4");
            m_SupportSpirv14 = true;
            deviceExtensions.push_back(VK_KHR_SPIRV_1_4_EXTENSION_NAME);
        }
    
        if(strcmp(extension.extensionName, VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME) == 0)
        {
            Log::Info("[Vulkan] GPU supports descriptor indexing");
            m_SupportDescriptorIndexing = true;
            deviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
        }
    
        if(strcmp(extension.extensionName, VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME) == 0)
        {
            Log::Info("[Vulkan] GPU supports shader float controls");
            m_SupportShaderFloatControls = true;
            deviceExtensions.push_back(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
        }
        
        if(strcmp(extension.extensionName, VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME) == 0)
        {
            Log::Info("[Vulkan] GPU supports pipeline library");
            m_SupportPipelineLibrary = true;
            deviceExtensions.push_back(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);
        }

        if(strcmp(extension.extensionName, VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) == 0)
        {
            Log::Info("[Vulkan] GPU supports buffer device address");
            m_SupportBufferDeviceAddress = true;
            deviceExtensions.push_back(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);
        }

        if(strcmp(extension.extensionName, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME) == 0)
        {
            supportRayTracingPipeline = true;
        }

        if(strcmp(extension.extensionName, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) == 0)
        {
            supportAccelerationStructure = true;
        }

        if(strcmp(extension.extensionName, VK_EXT_MESH_SHADER_EXTENSION_NAME) == 0)
        {
            supportMeshShader = true;
        }

        if(strcmp(extension.extensionName, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME) == 0)
        {
            supportDeferredHostOperations = true;
        }

        if(strcmp(extension.extensionName, VK_KHR_RAY_QUERY_EXTENSION_NAME) == 0)
        {
            supportRayQuery = true;
        }
    }
    
    if(m_SupportSpirv14 && m_SupportShaderFloatControls)
    {
        if(supportMeshShader)
        {
            m_SupportMeshShading = true;
            Log::Info("[Vulkan] GPU supports mesh shading");
            deviceExtensions.push_back(VK_EXT_MESH_SHADER_EXTENSION_NAME);
        }

        if(supportRayTracingPipeline
            && supportAccelerationStructure
            && supportDeferredHostOperations
            && supportRayQuery
            && m_SupportBufferDeviceAddress
            && m_SupportDescriptorIndexing
            && m_SupportPipelineLibrary)
        {
            m_SupportRayTracing = true;
            Log::Info("[Vulkan] GPU supports ray tracing");
            deviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
            deviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
            deviceExtensions.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
            deviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
        }
    }

    // Enumerate all queue families
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDeviceHandle, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDeviceHandle, &queueFamilyCount, queueFamilies.data());

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    
    for(uint32_t i = 0; i < queueFamilyCount; i++)
    {
        if((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && m_QueueIndex[static_cast<uint8_t>(ERHICommandQueueType::Direct)] == -1)
        {
            m_QueueIndex[static_cast<uint8_t>(ERHICommandQueueType::Direct)] = i;
            queueCreateInfo.queueFamilyIndex = i;
            queueCreateInfos.push_back(queueCreateInfo);
        }
        else if((queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && m_QueueIndex[static_cast<uint8_t>(ERHICommandQueueType::Async)] == -1)
        {
            m_QueueIndex[static_cast<uint8_t>(ERHICommandQueueType::Async)] = i;
            queueCreateInfo.queueFamilyIndex = i;
            queueCreateInfos.push_back(queueCreateInfo);
        }
        else if((queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT) && m_QueueIndex[static_cast<uint8_t>(ERHICommandQueueType::Copy)] == -1)
        {
            m_QueueIndex[static_cast<uint8_t>(ERHICommandQueueType::Copy)] = i;
            queueCreateInfo.queueFamilyIndex = i;
            queueCreateInfos.push_back(queueCreateInfo);
        }
    }

    // Create logical m_DeviceHandle
    vkGetPhysicalDeviceFeatures(m_PhysicalDeviceHandle, &m_PhysicalDeviceFeatures);
    VkPhysicalDeviceFeatures2 deviceFeatures2{};
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.features = m_PhysicalDeviceFeatures;
    deviceFeatures2.pNext = nullptr;

    VkPhysicalDeviceDescriptorIndexingFeaturesEXT physicalDeviceDescriptorIndexingFeatures{};
    if(m_SupportDescriptorIndexing)
    {
        // Enable descriptor indexing feature
        physicalDeviceDescriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
        physicalDeviceDescriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
        physicalDeviceDescriptorIndexingFeatures.shaderStorageImageArrayNonUniformIndexing = VK_TRUE;
        physicalDeviceDescriptorIndexingFeatures.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
        physicalDeviceDescriptorIndexingFeatures.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;
        physicalDeviceDescriptorIndexingFeatures.runtimeDescriptorArray = VK_TRUE;
        physicalDeviceDescriptorIndexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;
        physicalDeviceDescriptorIndexingFeatures.pNext = deviceFeatures2.pNext; // Chain the feature to the top of existing features
        deviceFeatures2.pNext = &physicalDeviceDescriptorIndexingFeatures;
    }

    // Enable features required for mesh shader
    VkPhysicalDeviceMeshShaderFeaturesEXT enableMeshShaderFeature{};
    if(m_SupportMeshShading)
    {
        enableMeshShaderFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
        enableMeshShaderFeature.taskShader = VK_TRUE;
        enableMeshShaderFeature.meshShader = VK_TRUE;
        enableMeshShaderFeature.pNext = deviceFeatures2.pNext; // Chain the feature to the top of existing features
        deviceFeatures2.pNext = &enableMeshShaderFeature;
    }

    // Enable features required for buffer device address
    VkPhysicalDeviceBufferDeviceAddressFeatures enableBufferDeviceAddressFeature{};
    if(m_SupportBufferDeviceAddress)
    {
        enableBufferDeviceAddressFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
        enableBufferDeviceAddressFeature.bufferDeviceAddress = VK_TRUE;
        enableBufferDeviceAddressFeature.pNext = deviceFeatures2.pNext; // Chain the feature to the top of existing features
        deviceFeatures2.pNext = &enableBufferDeviceAddressFeature;
    }

    // Enable features required for ray tracing
    VkPhysicalDeviceRayQueryFeaturesKHR enabledRayQueryFeatures{};
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR enabledRayTracingPipelineFeatures{};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR enabledAccelerationStructureFeatures{};
    if(m_SupportRayTracing)
    {
        enabledRayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
        enabledRayQueryFeatures.rayQuery = VK_TRUE;
        enabledRayQueryFeatures.pNext = deviceFeatures2.pNext; // Chain the feature to the top of existing features
        
        enabledRayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
        enabledRayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
        enabledRayTracingPipelineFeatures.pNext = &enabledRayQueryFeatures;
        
        enabledAccelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
        enabledAccelerationStructureFeatures.accelerationStructure = VK_TRUE;
        enabledAccelerationStructureFeatures.pNext = &enabledRayTracingPipelineFeatures;
        deviceFeatures2.pNext = &enabledAccelerationStructureFeatures;
    }
    
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pEnabledFeatures = nullptr;
    deviceCreateInfo.pNext = &deviceFeatures2;
    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
    deviceCreateInfo.enabledLayerCount = 0;

#if _DEBUG || DEBUG
    deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(s_ValidationLayerNames.size());
    deviceCreateInfo.ppEnabledLayerNames = s_ValidationLayerNames.data();
#endif

    result = vkCreateDevice(m_PhysicalDeviceHandle, &deviceCreateInfo, nullptr, &m_DeviceHandle);
    if(result != VK_SUCCESS)
    {
        Log::Error("[Vulkan] Failed to create logical device");
        OUTPUT_VULKAN_FAILED_RESULT(result);
        return false;
    }

    for(uint32_t i = 0; i < COMMAND_QUEUES_COUNT; i++)
    {
        VkQueue queue;
        vkGetDeviceQueue(m_DeviceHandle, m_QueueIndex[i], 0, &queue);
        m_QueueHandles[i] = queue;
    }

    if(m_SupportMeshShading)
    {
        vkCmdDrawMeshTasksEXT = reinterpret_cast<PFN_vkCmdDrawMeshTasksEXT>(vkGetDeviceProcAddr(m_DeviceHandle, "vkCmdDrawMeshTasksEXT"));
    }

    if(m_SupportBufferDeviceAddress)
    {
        vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(m_DeviceHandle, "vkGetBufferDeviceAddressKHR"));
    }

    if(m_SupportRayTracing)
    {
        vkCmdBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkCmdBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(m_DeviceHandle, "vkCmdBuildAccelerationStructuresKHR"));
        vkBuildAccelerationStructuresKHR = reinterpret_cast<PFN_vkBuildAccelerationStructuresKHR>(vkGetDeviceProcAddr(m_DeviceHandle, "vkBuildAccelerationStructuresKHR"));
        vkCreateAccelerationStructureKHR = reinterpret_cast<PFN_vkCreateAccelerationStructureKHR>(vkGetDeviceProcAddr(m_DeviceHandle, "vkCreateAccelerationStructureKHR"));
        vkDestroyAccelerationStructureKHR = reinterpret_cast<PFN_vkDestroyAccelerationStructureKHR>(vkGetDeviceProcAddr(m_DeviceHandle, "vkDestroyAccelerationStructureKHR"));
        vkGetAccelerationStructureBuildSizesKHR = reinterpret_cast<PFN_vkGetAccelerationStructureBuildSizesKHR>(vkGetDeviceProcAddr(m_DeviceHandle, "vkGetAccelerationStructureBuildSizesKHR"));
        vkGetAccelerationStructureDeviceAddressKHR = reinterpret_cast<PFN_vkGetAccelerationStructureDeviceAddressKHR>(vkGetDeviceProcAddr(m_DeviceHandle, "vkGetAccelerationStructureDeviceAddressKHR"));
        vkCmdTraceRaysKHR = reinterpret_cast<PFN_vkCmdTraceRaysKHR>(vkGetDeviceProcAddr(m_DeviceHandle, "vkCmdTraceRaysKHR"));
        vkGetRayTracingShaderGroupHandlesKHR = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesKHR>(vkGetDeviceProcAddr(m_DeviceHandle, "vkGetRayTracingShaderGroupHandlesKHR"));
        vkCreateRayTracingPipelinesKHR = reinterpret_cast<PFN_vkCreateRayTracingPipelinesKHR>(vkGetDeviceProcAddr(m_DeviceHandle, "vkCreateRayTracingPipelinesKHR"));
    }
#if _DEBUG || DEBUG
    vkSetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetDeviceProcAddr(m_DeviceHandle, "vkSetDebugUtilsObjectNameEXT"));
#endif
    
    return true;
}

void VulkanDevice::Shutdown()
{
    ShutdownInternal();
}

void VulkanDevice::ShutdownInternal()
{
    if(m_DeviceHandle != VK_NULL_HANDLE) vkDestroyDevice(m_DeviceHandle, nullptr);
#if _DEBUG || DEBUG
    if(m_DebugMessenger != VK_NULL_HANDLE)
    {
        auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_InstanceHandle, "vkDestroyDebugUtilsMessengerEXT");
        vkDestroyDebugUtilsMessengerEXT(m_InstanceHandle, m_DebugMessenger, nullptr);
    }
#endif
    if(m_InstanceHandle != VK_NULL_HANDLE) vkDestroyInstance(m_InstanceHandle, nullptr);
    
    m_DeviceHandle = VK_NULL_HANDLE;
    m_PhysicalDeviceHandle = VK_NULL_HANDLE;
    m_DebugMessenger = VK_NULL_HANDLE;
    m_InstanceHandle = VK_NULL_HANDLE;
}

bool VulkanDevice::IsValid() const
{
    bool valid = m_InstanceHandle != VK_NULL_HANDLE
        && m_PhysicalDeviceHandle != VK_NULL_HANDLE
        && m_DeviceHandle != VK_NULL_HANDLE;

    for(auto i : m_QueueHandles)
    {
        valid = valid && i != VK_NULL_HANDLE;
    }
    
    return valid;
}

void VulkanDevice::SetDebugName(VkObjectType objectType, uint64_t objectHandle, const std::string& name) const
{
    if(vkSetDebugUtilsObjectNameEXT != VK_NULL_HANDLE)
    {
        VkDebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectHandle = objectHandle;
        nameInfo.objectType = objectType;
        nameInfo.pObjectName = name.c_str();
        VkResult result = vkSetDebugUtilsObjectNameEXT(m_DeviceHandle, &nameInfo);
        if(result != VK_SUCCESS) Log::Warning("Failed to set name: %s on object", name.c_str());
    }
}
    
std::shared_ptr<RHIFence> VulkanDevice::CreateRhiFence()
{
    std::shared_ptr<RHIFence> fence(new VulkanFence(*this));
    if(!fence->Init())
    {
        Log::Error("[Vulkan] Failed to create fence");
    }
    return fence;
}

VulkanFence::VulkanFence(VulkanDevice& inDevice)
    : m_Device(inDevice)
    , m_FenceHandle(VK_NULL_HANDLE)
{
    
}

VulkanFence::~VulkanFence()
{
    ShutdownInternal();
}

bool VulkanFence::Init()
{
    if(IsValid())
    {
        Log::Warning("[Vulkan] Fence already initialized");
        return true;
    }

    VkFenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    VkResult result = vkCreateFence(m_Device.GetDevice(), &fenceCreateInfo, nullptr, &m_FenceHandle);
    if(result != VK_SUCCESS)
    {
        OUTPUT_VULKAN_FAILED_RESULT(result);
        return false;
    }

    Reset();
    
    return true;
}

void VulkanFence::Shutdown()
{
    ShutdownInternal();
}

bool VulkanFence::IsValid() const
{
    return m_FenceHandle != VK_NULL_HANDLE;
}

void VulkanFence::Reset()
{
    if(IsValid())
    {
        vkResetFences(m_Device.GetDevice(), 1, &m_FenceHandle);
    }
}

void VulkanFence::CpuWait()
{
    if(IsValid())
    {
        vkWaitForFences(m_Device.GetDevice(), 1, &m_FenceHandle, VK_TRUE, UINT64_MAX);
    }
}

void VulkanFence::ShutdownInternal()
{
    if(m_FenceHandle != VK_NULL_HANDLE)
    {
        vkDestroyFence(m_Device.GetDevice(), m_FenceHandle, nullptr);
        m_FenceHandle = VK_NULL_HANDLE;
    }
}

void VulkanFence::SetNameInternal()
{
    if(IsValid())
    {
        m_Device.SetDebugName(VK_OBJECT_TYPE_FENCE, reinterpret_cast<uint64_t>(m_FenceHandle), m_Name);
    }
}

std::shared_ptr<RHISemaphore> VulkanDevice::CreateRhiSemaphore()
{
    std::shared_ptr<RHISemaphore> semaphore(new VulkanSemaphore(*this));
    if(!semaphore->Init())
    {
        Log::Error("[Vulkan] Failed to create semaphore");
    }
    return semaphore;
}

VulkanSemaphore::VulkanSemaphore(VulkanDevice& inDevice)
    : m_Device(inDevice)
    , m_SemaphoreHandle(VK_NULL_HANDLE)
{
    
}

VulkanSemaphore::~VulkanSemaphore()
{
    ShutdownInternal();
}

void VulkanSemaphore::Shutdown()
{
    ShutdownInternal();
}

bool VulkanSemaphore::Init()
{
    if(IsValid())
    {
        Log::Warning("[Vulkan] Semaphore already initialized");
        return true;
    }

    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkResult result = vkCreateSemaphore(m_Device.GetDevice(), &semaphoreCreateInfo, nullptr, &m_SemaphoreHandle);
    if(result != VK_SUCCESS)
    {
        OUTPUT_VULKAN_FAILED_RESULT(result);
        return false;
    }
    
    return true;
}

bool VulkanSemaphore::IsValid() const
{
    return m_SemaphoreHandle != VK_NULL_HANDLE;
}

void VulkanSemaphore::Reset()
{
    
}

void VulkanSemaphore::ShutdownInternal()
{
    if(m_SemaphoreHandle != VK_NULL_HANDLE)
    {
        vkDestroySemaphore(m_Device.GetDevice(), m_SemaphoreHandle, nullptr);
        m_SemaphoreHandle = VK_NULL_HANDLE;
    }
}

void VulkanSemaphore::SetNameInternal()
{
    if(IsValid())
    {
        m_Device.SetDebugName(VK_OBJECT_TYPE_SEMAPHORE, reinterpret_cast<uint64_t>(m_SemaphoreHandle), m_Name);
    }
}