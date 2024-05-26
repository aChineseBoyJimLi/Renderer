#include "VulkanResources.h"
#include "VulkanDevice.h"
#include "../../Core/Log.h"
#include "../../Core/Templates.h"

RefCountPtr<RHIAccelerationStructure> VulkanDevice::CreateBottomLevelAccelerationStructure(const std::vector<RHIRayTracingGeometryDesc>& inDesc)
{
    RefCountPtr<RHIAccelerationStructure> accelerationStructure(new VulkanAccelerationStructure(*this, inDesc));
    if(!accelerationStructure->Init())
    {
        Log::Error("[Vulkan] failed to create acceleration structure");
    }
    return accelerationStructure;
}

RefCountPtr<RHIAccelerationStructure> VulkanDevice::CreateTopLevelAccelerationStructure(const std::vector<RHIRayTracingInstanceDesc>& inDesc)
{
    RefCountPtr<RHIAccelerationStructure> accelerationStructure(new VulkanAccelerationStructure(*this, inDesc));
    if(!accelerationStructure->Init())
    {
        Log::Error("[Vulkan] failed to create acceleration structure");
    }
    return accelerationStructure;
}

VulkanAccelerationStructure::VulkanAccelerationStructure(VulkanDevice& inDevice, const std::vector<RHIRayTracingGeometryDesc>& inDesc)
    : m_Device(inDevice)
    , m_GeometryDesc(inDesc)
    , m_IsTopLevel(false)
    , m_IsBuilt(false)
    , m_InstanceBuffer(nullptr)
    , m_AccelerationStructureBuffer(nullptr)
    , m_AccelerationStructure(VK_NULL_HANDLE)
    , m_BuildInfo{}
{
    
}

VulkanAccelerationStructure::VulkanAccelerationStructure(VulkanDevice& inDevice, const std::vector<RHIRayTracingInstanceDesc>& inDesc)
    : m_Device(inDevice)
    , m_InstanceDesc(inDesc)
    , m_IsTopLevel(true)
    , m_IsBuilt(false)
    , m_InstanceBuffer(nullptr)
    , m_AccelerationStructureBuffer(nullptr)
    , m_AccelerationStructure(VK_NULL_HANDLE)
    , m_BuildInfo{}
{
    
}

VulkanAccelerationStructure::~VulkanAccelerationStructure()
{
    ShutdownInternal();
}

bool VulkanAccelerationStructure::Init()
{
    if(IsValid())
    {
        Log::Warning("[Vulkan] Acceleration structure already initialized.");
        return true;
    }

    m_GeometryDescVulkan.clear();
    m_GeometryPrimCount.clear();
    
    return m_IsTopLevel ? InitTopLevel() : InitBottomLevel();
}

bool VulkanAccelerationStructure::InitBottomLevel()
{
    VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
    for(uint32_t i = 0; i < m_GeometryDesc.size(); ++i)
    {
        const auto& geometryDesc = m_GeometryDesc[i];
        accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        accelerationStructureGeometry.flags = RHI::Vulkan::ConvertGeometryFlags(geometryDesc.Flags);
        if(geometryDesc.Type == ERHIRayTracingGeometryType::Triangles)
        {
            // VulkanBuffer* vertexBuffer = CheckCast<VulkanBuffer*>(geometryDesc.Triangles.VertexBuffer);
            // VulkanBuffer* indexBuffer = CheckCast<VulkanBuffer*>(geometryDesc.Triangles.IndexBuffer);
            RHIBuffer* vertexBuffer = geometryDesc.Triangles.VertexBuffer;
            RHIBuffer* indexBuffer = geometryDesc.Triangles.IndexBuffer;
            const RHIBufferDesc& vbDesc = vertexBuffer->GetDesc();
            if(vertexBuffer == nullptr || !vertexBuffer->IsValid() || indexBuffer == nullptr || !indexBuffer->IsValid())
            {
                continue;
            }
            accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
            accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
            accelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
            accelerationStructureGeometry.geometry.triangles.vertexData.deviceAddress = vertexBuffer->GetGpuAddress();
            accelerationStructureGeometry.geometry.triangles.maxVertex = static_cast<uint32_t>(vbDesc.Size / vbDesc.Stride);
            accelerationStructureGeometry.geometry.triangles.vertexStride = vbDesc.Stride;
            accelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
            accelerationStructureGeometry.geometry.triangles.indexData.deviceAddress = indexBuffer->GetGpuAddress();
            m_GeometryPrimCount.push_back(geometryDesc.Triangles.PrimCount);
        }
        else // ERHIRayTracingGeometryType::AABBs
        {
            RHIBuffer* aabbBuffer = geometryDesc.AABBs.Buffer;
            const RHIBufferDesc& aabbDesc = aabbBuffer->GetDesc();
            if(aabbBuffer == nullptr || !aabbBuffer->IsValid())
            {
                continue;
            }
            accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
            accelerationStructureGeometry.geometry.aabbs.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
            accelerationStructureGeometry.geometry.aabbs.data.deviceAddress = aabbBuffer->GetGpuAddress();
            accelerationStructureGeometry.geometry.aabbs.stride = aabbDesc.Stride != 0 ? aabbDesc.Stride : sizeof(RHIAABB);
            constexpr uint32_t numAABB = 1;
            m_GeometryPrimCount.push_back(numAABB);
        }
        m_GeometryDescVulkan.push_back(accelerationStructureGeometry);
    }

    uint32_t geometryCount = (uint32_t) m_GeometryDescVulkan.size();
    VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
    accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelerationStructureBuildGeometryInfo.geometryCount = geometryCount;
    accelerationStructureBuildGeometryInfo.pGeometries = m_GeometryDescVulkan.data();
    
    m_BuildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

    uint32_t maxPrimitiveCounts[1] {geometryCount};
    
    m_Device.vkGetAccelerationStructureBuildSizesKHR(
        m_Device.GetDevice(),
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &accelerationStructureBuildGeometryInfo,
        maxPrimitiveCounts,
        &m_BuildInfo);

    RHIBufferDesc asDesc = RHIBufferDesc::AccelerationStructure(m_BuildInfo.accelerationStructureSize);
    m_AccelerationStructureBuffer = m_Device.CreateVulkanBuffer(asDesc);
    if(m_AccelerationStructureBuffer.GetReference() == nullptr || !m_AccelerationStructureBuffer->IsValid())
    {
        Log::Error("[Vulkan] Failed to create acceleration structure buffer");
        return false;
    }

    VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
    accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    accelerationStructureCreateInfo.buffer = m_AccelerationStructureBuffer->GetBuffer();
    accelerationStructureCreateInfo.size = m_BuildInfo.accelerationStructureSize;
    accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
    VkResult result = m_Device.vkCreateAccelerationStructureKHR(m_Device.GetDevice(), &accelerationStructureCreateInfo, nullptr, &m_AccelerationStructure);
    if(result != VK_SUCCESS)
    {
        Log::Error("Failed to create BLAS");
        return false;
    }
    
    return true;
}

bool VulkanAccelerationStructure::InitTopLevel()
{
    RHIBufferDesc instanceBufferDesc;
    instanceBufferDesc.CpuAccess = ERHICpuAccessMode::Write;
    instanceBufferDesc.Usages = ERHIBufferUsage::AccelerationStructureBuildInput;
    instanceBufferDesc.Size = sizeof(RHIRayTracingInstanceDesc) * m_InstanceDesc.size();
    instanceBufferDesc.Stride = sizeof(RHIRayTracingInstanceDesc);

    m_InstanceBuffer = m_Device.CreateVulkanBuffer(instanceBufferDesc);

    if(m_InstanceBuffer.GetReference() == nullptr || !m_InstanceBuffer.IsValid())
    {
        Log::Error("[Vulkan] failed to create top level instance buffer");
        return false;
    }

    m_InstanceBuffer->WriteData(m_InstanceDesc.data(), instanceBufferDesc.Size);

    VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
    accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
    accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
    accelerationStructureGeometry.geometry.instances.data.deviceAddress = m_InstanceBuffer->GetGpuAddress();

    m_GeometryDescVulkan.push_back(accelerationStructureGeometry);
    uint32_t instanceCount = (uint32_t) m_InstanceDesc.size();
    m_GeometryPrimCount.push_back(instanceCount);

    uint32_t geometryCount = (uint32_t) m_GeometryDescVulkan.size();
    VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
    accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
    accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    accelerationStructureBuildGeometryInfo.geometryCount = geometryCount;
    accelerationStructureBuildGeometryInfo.pGeometries = m_GeometryDescVulkan.data();

    uint32_t maxPrimitiveCounts[1] {geometryCount};
    
    m_BuildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
    m_Device.vkGetAccelerationStructureBuildSizesKHR(
        m_Device.GetDevice(),
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &accelerationStructureBuildGeometryInfo,
        maxPrimitiveCounts,
        &m_BuildInfo);

    RHIBufferDesc asDesc = RHIBufferDesc::AccelerationStructure(m_BuildInfo.accelerationStructureSize);
    m_AccelerationStructureBuffer = m_Device.CreateVulkanBuffer(asDesc);
    if(m_AccelerationStructureBuffer.GetReference() == nullptr || !m_AccelerationStructureBuffer->IsValid())
    {
        Log::Error("[Vulkan] Failed to create acceleration structure buffer");
        return false;
    }

    VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
    accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
    accelerationStructureCreateInfo.buffer = m_AccelerationStructureBuffer->GetBuffer();
    accelerationStructureCreateInfo.size = m_BuildInfo.accelerationStructureSize;
    accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
    VkResult result = m_Device.vkCreateAccelerationStructureKHR(m_Device.GetDevice(), &accelerationStructureCreateInfo, nullptr, &m_AccelerationStructure);
    if(result != VK_SUCCESS)
    {
        Log::Error("Failed to create TLAS");
        return false;
    }
    
    return true;
}


bool VulkanAccelerationStructure::IsValid() const
{
    return m_AccelerationStructure != VK_NULL_HANDLE && m_AccelerationStructureBuffer.GetReference() != nullptr && m_AccelerationStructureBuffer->IsValid();
}

void VulkanAccelerationStructure::Shutdown()
{
    ShutdownInternal();
}

void VulkanAccelerationStructure::ShutdownInternal()
{
    if(m_AccelerationStructure != VK_NULL_HANDLE)
    {
        m_Device.vkDestroyAccelerationStructureKHR(m_Device.GetDevice(), m_AccelerationStructure, nullptr);
        m_AccelerationStructure = VK_NULL_HANDLE;
    }
    m_InstanceBuffer.SafeRelease();
    m_AccelerationStructureBuffer.SafeRelease();
}

RefCountPtr<RHIBuffer> VulkanAccelerationStructure::GetScratchBuffer() const
{
    if(IsValid())
    {
        RHIBufferDesc scratchBufferDesc;
        scratchBufferDesc.Size = m_BuildInfo.buildScratchSize;
        scratchBufferDesc.Usages = ERHIBufferUsage::UnorderedAccess;
        scratchBufferDesc.CpuAccess = ERHICpuAccessMode::None;
        return m_Device.CreateBuffer(scratchBufferDesc);
    }
    return nullptr;
}

void VulkanAccelerationStructure::Build(VkCommandBuffer inCmdBuffer, RefCountPtr<RHIBuffer>& inScratchBuffer)
{
    if(IsValid() && !IsBuilt() && inScratchBuffer.GetReference() && inScratchBuffer->IsValid())
    {
        uint32_t geometryDescCount = (uint32_t) m_GeometryDescVulkan.size();
        VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
        accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        accelerationBuildGeometryInfo.type = IsTopLevel() ? VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR : VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        accelerationBuildGeometryInfo.dstAccelerationStructure = m_AccelerationStructure;
        accelerationBuildGeometryInfo.geometryCount = (uint32_t) m_GeometryDescVulkan.size();
        accelerationBuildGeometryInfo.pGeometries = m_GeometryDescVulkan.data();
        accelerationBuildGeometryInfo.scratchData.deviceAddress = inScratchBuffer->GetGpuAddress();
        
        std::vector<VkAccelerationStructureBuildRangeInfoKHR> oneGeometryBuildRangeInfos(geometryDescCount);
        for(uint32_t i = 0; i < geometryDescCount; i++)
        {
            oneGeometryBuildRangeInfos[i].primitiveCount = m_GeometryPrimCount[i];
            oneGeometryBuildRangeInfos[i].primitiveOffset = 0;
            oneGeometryBuildRangeInfos[i].firstVertex = 0;
            oneGeometryBuildRangeInfos[i].transformOffset = 0;
        }

        std::vector<const VkAccelerationStructureBuildRangeInfoKHR*> allGeometryBuildRangeInfos;
        allGeometryBuildRangeInfos.push_back(oneGeometryBuildRangeInfos.data());

        m_Device.vkCmdBuildAccelerationStructuresKHR(inCmdBuffer
            , 1
            , &accelerationBuildGeometryInfo
            , allGeometryBuildRangeInfos.data());
    }
}

void VulkanAccelerationStructure::SetNameInternal()
{
    if(IsValid())
    {
        m_Device.SetDebugName(VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR, reinterpret_cast<uint64_t>(m_AccelerationStructure), m_Name);
    }
}