#include "VulkanDevice.h"
#include "VulkanResources.h"
#include "../../Core/Log.h"
#include "../../Core/Templates.h"

RefCountPtr<RHIResourceHeap> VulkanDevice::CreateResourceHeap(const RHIResourceHeapDesc& inDesc)
{
    const VkMemoryPropertyFlags memoryPropertyFlags = RHI::Vulkan::ConvertHeapType(inDesc.Type);
    
    uint32_t memoryTypeIndex;
    if(GetMemoryTypeIndex(inDesc.TypeFilter, memoryPropertyFlags, memoryTypeIndex))
    {
        RefCountPtr<RHIResourceHeap> heap(new VulkanResourceHeap(*this, inDesc, memoryTypeIndex));
        if(!heap->Init())
        {
            Log::Error("[Vulkan] Failed to create resource heap!");
        }
        return heap;
    }
    return nullptr;
}

bool VulkanDevice::GetMemoryTypeIndex(uint32_t inTypeFilter, VkMemoryPropertyFlags inProperties, uint32_t& outMemTypeIndex) const
{
    for (uint32_t i = 0; i < m_PhysicalDeviceMemoryProperties.memoryTypeCount; i++) {
        if ((inTypeFilter & (1 << i)) && (m_PhysicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & inProperties) == inProperties) {
            outMemTypeIndex = i;
            return true;
        }
    }
    Log::Warning("[Vulkan] Failed to find suitable memory type!");
    outMemTypeIndex = m_PhysicalDeviceMemoryProperties.memoryTypeCount;
    return false;
}

VulkanResourceHeap::VulkanResourceHeap(VulkanDevice& inDevice, const RHIResourceHeapDesc& inDesc, uint32_t inMemoryTypeIndex)
    : m_Device(inDevice), m_Desc(inDesc), m_HeapHandle(VK_NULL_HANDLE), m_MemoryTypeIndex(inMemoryTypeIndex), m_TotalChunkNum(0)
{
    
}

VulkanResourceHeap::~VulkanResourceHeap()
{
    ShutdownInternal();
}

bool VulkanResourceHeap::Init()
{
    if(IsValid())
    {
        Log::Warning("[Vulkan] Resource Heap already initialized.");
        return true;
    }

    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = m_Desc.Size;
    allocateInfo.memoryTypeIndex = m_MemoryTypeIndex;

    if(m_Device.SupportBufferDeviceAddress()
        && m_Desc.Usage == ERHIHeapUsage::Buffer)
    {
        VkMemoryAllocateFlagsInfoKHR allocFlagsInfo{};
        allocFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO_KHR;
        allocFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
        allocateInfo.pNext = &allocFlagsInfo;
    }

    VkResult re = vkAllocateMemory(m_Device.GetDevice(), &allocateInfo, nullptr, &m_HeapHandle);
    if(re != VK_SUCCESS)
    {
        OUTPUT_VULKAN_FAILED_RESULT(re)
        return false;
    }

    m_TotalChunkNum = static_cast<uint32_t>(m_Desc.Size / m_Desc.Alignment);
    m_MemAllocator.SetTotalCount(m_TotalChunkNum);

    return true;
}

void VulkanResourceHeap::Shutdown()
{
    ShutdownInternal();
}

void VulkanResourceHeap::ShutdownInternal()
{
    if(m_HeapHandle != VK_NULL_HANDLE)
    {
        vkFreeMemory(m_Device.GetDevice(), m_HeapHandle, nullptr);
        m_HeapHandle = VK_NULL_HANDLE;
    }
}

bool VulkanResourceHeap::IsValid() const
{
    return m_HeapHandle != VK_NULL_HANDLE;
}

void VulkanResourceHeap::SetNameInternal()
{
    if(IsValid())
    {
        m_Device.SetDebugName(VK_OBJECT_TYPE_DEVICE_MEMORY, reinterpret_cast<uint64_t>(m_HeapHandle), m_Name);
    }
}

bool VulkanResourceHeap::TryAllocate(size_t inSize, size_t& outOffset)
{
    if(!IsValid())
    {
        outOffset = UINT64_MAX;
        return false;
    }

    uint32_t chunks = static_cast<uint32_t>(Align(inSize, m_Desc.Alignment) / m_Desc.Alignment);
    uint32_t offsetChunks = 0;

    if(m_MemAllocator.TryAllocate(chunks, offsetChunks))
    {
        outOffset = m_Desc.Alignment * offsetChunks;
        return true;
    }

    outOffset = UINT64_MAX;
    return false;   
}

void VulkanResourceHeap::Free(size_t inOffset, size_t inSize)
{
    if(inOffset % m_Desc.Alignment != 0)
    {
        Log::Error("[Vulkan] Free offset is not aligned to the heap's alignment: %d", m_Desc.Alignment);
        return;
    }
    
    uint32_t offsetChunks = static_cast<uint32_t>(inOffset / m_Desc.Alignment);
    uint32_t chunks = static_cast<uint32_t>(Align(inSize, m_Desc.Alignment) / m_Desc.Alignment);

    m_MemAllocator.Free(offsetChunks, chunks);
}

bool VulkanResourceHeap::IsEmpty() const
{
    return m_MemAllocator.IsEmpty();
}
