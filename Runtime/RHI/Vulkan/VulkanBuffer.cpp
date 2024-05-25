#include "VulkanDevice.h"
#include "VulkanResources.h"
#include "../../Core/Log.h"
#include "../../Core/Templates.h"
#include <cassert>

RefCountPtr<RHIBuffer> VulkanDevice::CreateBuffer(const RHIBufferDesc& inDesc, bool isVirtual)
{
    RefCountPtr<RHIBuffer> buffer(new VulkanBuffer(*this, inDesc, isVirtual));
    if(!buffer->Init())
    {
        Log::Error("[Vulkan] Failed to create buffer");
    }
    return buffer;
}

VulkanBuffer::VulkanBuffer(VulkanDevice& inDevice, const RHIBufferDesc& inDesc, bool inIsVirtual)
    : IsVirtualBuffer(inIsVirtual)
    , IsManagedBuffer(true)
    , m_Device(inDevice)
    , m_Desc(inDesc)
    , m_BufferHandle(VK_NULL_HANDLE)
    , m_InitialAccessFlags(VK_ACCESS_NONE)
    , m_MemRequirements{m_Desc.Size, 0, UINT32_MAX}
    , m_ResourceHeap(nullptr)
    , m_OffsetInHeap(0)
    , m_NumMapCalls(0)
    , m_ResourceBaseAddress(nullptr)
{
    
}

VulkanBuffer::~VulkanBuffer()
{
    ShutdownInternal();
}

bool VulkanBuffer::Init()
{
    if(!IsManaged())
    {
        Log::Warning("[Vulkan] Buffer is not managed, initialization is not allowed.");
        return true;
    }
    
    if(IsValid())
    {
        Log::Warning("[Vulkan] Buffer already initialized.");
        return true;
    }

    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.size = m_Desc.Size;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBufferUsageFlags usages = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if(m_Device.SupportBufferDeviceAddress())
    {
        usages |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    }
    if((m_Desc.Usages & ERHIBufferUsage::VertexBuffer) == ERHIBufferUsage::VertexBuffer) usages |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if((m_Desc.Usages & ERHIBufferUsage::IndexBuffer) == ERHIBufferUsage::IndexBuffer) usages |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if((m_Desc.Usages & ERHIBufferUsage::ConstantBuffer) == ERHIBufferUsage::ConstantBuffer) usages |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if((m_Desc.Usages & ERHIBufferUsage::IndirectCommands) == ERHIBufferUsage::IndirectCommands) usages |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    if((m_Desc.Usages & ERHIBufferUsage::UnorderedAccess) == ERHIBufferUsage::UnorderedAccess) usages |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    if((m_Desc.Usages & ERHIBufferUsage::ShaderResource) == ERHIBufferUsage::ShaderResource) usages |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    if((m_Desc.Usages & ERHIBufferUsage::AccelerationStructureStorage) == ERHIBufferUsage::AccelerationStructureStorage) usages |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR;
    if((m_Desc.Usages & ERHIBufferUsage::AccelerationStructureBuildInput) == ERHIBufferUsage::AccelerationStructureBuildInput) usages |= VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
    if((m_Desc.Usages & ERHIBufferUsage::ShaderTable) == ERHIBufferUsage::ShaderTable) usages |= VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR;
    bufferCreateInfo.usage = usages;

    VkResult re = vkCreateBuffer(m_Device.GetDevice(), &bufferCreateInfo, nullptr, &m_BufferHandle);
    if(re != VK_SUCCESS)
    {
        OUTPUT_VULKAN_FAILED_RESULT(re)
        return false;
    }

    vkGetBufferMemoryRequirements(m_Device.GetDevice(), m_BufferHandle, &m_MemRequirements);
    m_ResourceHeap = nullptr;

    if(!IsVirtual() && IsManaged())
    {
        RHIResourceHeapDesc heapDesc{};
        heapDesc.Alignment = GetAllocAlignment();
        heapDesc.Size = GetAllocSizeInByte();
        heapDesc.TypeFilter = GetMemTypeFilter();
        heapDesc.Usage = ERHIHeapUsage::Buffer;

        switch (m_Desc.CpuAccess)
        {
        case ERHICpuAccessMode::None:
            heapDesc.Type = ERHIResourceHeapType::DeviceLocal;
            break;

        case ERHICpuAccessMode::Read:
            heapDesc.Type = ERHIResourceHeapType::Readback;
            break;

        case ERHICpuAccessMode::Write:
            heapDesc.Type = ERHIResourceHeapType::Upload;
            break;
        }

        m_ResourceHeap = m_Device.CreateResourceHeap(heapDesc);
        if(!m_ResourceHeap->IsValid())
        {
            Log::Error("[Vulkan] Failed to create resource heap for buffer");
            return false;
        }
        VulkanResourceHeap* heap = CheckCast<VulkanResourceHeap*>(m_ResourceHeap.GetReference());
        re = vkBindBufferMemory(m_Device.GetDevice(), m_BufferHandle, heap->GetHeap(), 0);
        if(re != VK_SUCCESS)
        {
            OUTPUT_VULKAN_FAILED_RESULT(re)
            return false;
        }
    }
    
    return true;
}

bool VulkanBuffer::BindMemory(RefCountPtr<RHIResourceHeap> inHeap)
{
    if(!IsManaged())
    {
        Log::Warning("[Vulkan] Buffer is not managed, memory binding is not allowed.");
        return true;    
    }
    
    if(!IsVirtual())
    {
        Log::Warning("[Vulkan] Buffer is not virtual, memory binding is not allowed.");
        return true;
    }

    if(m_ResourceHeap != nullptr)
    {
        Log::Warning("[Vulkan] Buffer already bound to a heap.");
        return true;
    }

    VulkanResourceHeap* heap = CheckCast<VulkanResourceHeap*>(inHeap.GetReference());
    if(heap == nullptr || !heap->IsValid())
    {
        Log::Error("[Vulkan] Failed to bind buffer memory, the heap is invalid");
        return false;
    }

    const RHIResourceHeapDesc& heapDesc = heap->GetDesc();

    if(heapDesc.Usage != ERHIHeapUsage::Buffer)
    {
        Log::Error("[Vulkan] Failed to bind buffer memory, the heap is not a buffer heap");
        return false;
    }

    if(m_Desc.CpuAccess == ERHICpuAccessMode::Write)
    {
        if(heapDesc.Type != ERHIResourceHeapType::Upload)
        {
            Log::Error("[Vulkan] Failed to bind buffer memory, the heap is not upload heap");
            return false;
        }
    }

    else if(m_Desc.CpuAccess == ERHICpuAccessMode::Read)
    {
        if(heapDesc.Type != ERHIResourceHeapType::Readback)
        {
            Log::Error("[Vulkan] Failed to bind buffer memory, the heap is not read back heap");
            return false;
        }
    }

    if(!inHeap->TryAllocate(GetAllocSizeInByte(), m_OffsetInHeap))
    {
        Log::Error("[Vulkan] Failed to bind buffer memory, the heap size is not enough");
        return false;
    }

    VkResult re = vkBindBufferMemory(m_Device.GetDevice(), m_BufferHandle, heap->GetHeap(), m_OffsetInHeap);
    if(re != VK_SUCCESS)
    {
        OUTPUT_VULKAN_FAILED_RESULT(re)
        inHeap->Free(m_OffsetInHeap, GetAllocSizeInByte());
        m_OffsetInHeap = 0;
        return false;
    }

    m_ResourceHeap = inHeap;
    return true;
}

void VulkanBuffer::Shutdown()
{
    ShutdownInternal();
}

void VulkanBuffer::ShutdownInternal()
{
    if(m_ResourceHeap != nullptr)
    {
        m_ResourceHeap->Free(m_OffsetInHeap, GetAllocSizeInByte());
        m_ResourceHeap.SafeRelease();
        m_OffsetInHeap = 0;
    }
    
    if(IsManaged())
    {
        if(m_BufferHandle != VK_NULL_HANDLE)
        {
            vkDestroyBuffer(m_Device.GetDevice(), m_BufferHandle, nullptr);
        }
    }
    m_BufferHandle = VK_NULL_HANDLE;
}

bool VulkanBuffer::IsValid() const
{
    bool valid = m_BufferHandle != VK_NULL_HANDLE;
    if(IsManaged())
    {
        valid = valid && m_ResourceHeap != nullptr && m_ResourceHeap->IsValid();
    }
    
    return valid;
}

void* VulkanBuffer::Map(uint64_t inSize, uint64_t inOffset)
{
    if(m_NumMapCalls == 0)
    {
        assert(IsValid());
        assert(m_ResourceHeap != nullptr);
        assert(m_ResourceHeap->IsValid());
        assert(m_BufferHandle != VK_NULL_HANDLE);
        assert(m_ResourceBaseAddress == nullptr);
        VulkanResourceHeap* heap = CheckCast<VulkanResourceHeap*>(m_ResourceHeap.GetReference());
        assert(heap != nullptr);
        vkMapMemory(m_Device.GetDevice(), heap->GetHeap(), inOffset, inSize, 0, &m_ResourceBaseAddress);
    }
    else
    {
        assert(m_ResourceBaseAddress);
    }
    ++m_NumMapCalls;

    return m_ResourceBaseAddress;
}

void VulkanBuffer::Unmap()
{
    assert(m_BufferHandle != VK_NULL_HANDLE);
    assert(m_ResourceBaseAddress);
    assert(m_NumMapCalls > 0);
    
    --m_NumMapCalls;
    if(m_NumMapCalls == 0)
    {
        VulkanResourceHeap* heap = CheckCast<VulkanResourceHeap*>(m_ResourceHeap.GetReference());
        assert(heap != nullptr);
        vkUnmapMemory(m_Device.GetDevice(), heap->GetHeap());
        m_ResourceBaseAddress = nullptr;
    }
}

void VulkanBuffer::WriteData(const void* inData, uint64_t inSize, uint64_t inOffset)
{
    void* data = Map(inSize, inOffset);
    memcpy(data, inData, inSize);
    Unmap();
}

void VulkanBuffer::ReadData(void* outData, uint64_t inSize, uint64_t inOffset)
{
    const void* data = Map(inSize, inOffset);
    memcpy(outData, data, inSize);
    Unmap();
}

RHIResourceGpuAddress VulkanBuffer::GetGpuAddress() const
{
    if(!IsValid())
    {
        Log::Error("[Vulkan] Trying to get GPU address of an invalid buffer");
        return UINT64_MAX;
    }

    if(!m_Device.SupportBufferDeviceAddress())
    {
        Log::Error("[Vulkan] Trying to get GPU address of a buffer, but the device does not support it");
        return UINT64_MAX;
    }

    VkBufferDeviceAddressInfoKHR bufferDeviceAI{};
    bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    bufferDeviceAI.buffer = m_BufferHandle;
    return m_Device.vkGetBufferDeviceAddressKHR(m_Device.GetDevice(), &bufferDeviceAI);
}

void VulkanBuffer::SetNameInternal()
{
    if(IsValid())
    {
        m_Device.SetDebugName(VK_OBJECT_TYPE_BUFFER, reinterpret_cast<uint64_t>(m_BufferHandle), m_Name);
    }
}

VkDescriptorBufferInfo VulkanBuffer::GetDescriptorBufferInfo() const
{
    VkDescriptorBufferInfo bufferInfo;

    bufferInfo.buffer = m_BufferHandle;
    bufferInfo.offset = 0;
    bufferInfo.range = m_Desc.Size;
    
    return bufferInfo;
}