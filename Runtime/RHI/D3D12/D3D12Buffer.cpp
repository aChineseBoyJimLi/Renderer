#include <cassert>

#include "D3D12Resources.h"
#include "D3D12Device.h"
#include "../../Core/Log.h"
#include "../../Core/Templates.h"

RefCountPtr<RHIBuffer> D3D12Device::CreateBuffer(const RHIBufferDesc& inDesc, bool isVirtual)
{
    RefCountPtr<RHIBuffer> buffer(new D3D12Buffer(*this, inDesc, isVirtual));
    if(!buffer->Init())
    {
        Log::Error("[D3D12] Failed to create buffer");
    }
    return buffer;
}

RefCountPtr<D3D12Buffer> D3D12Device::CreateD3D12Buffer(const RHIBufferDesc& inDesc)
{
    RefCountPtr<D3D12Buffer> buffer(new D3D12Buffer(*this, inDesc, false));
    if(!buffer->Init())
    {
        Log::Error("[D3D12] Failed to create buffer");
    }
    return buffer;
}

D3D12Buffer::D3D12Buffer(D3D12Device& inDevice, const RHIBufferDesc& inDesc, bool isVirtual)
    : IsVirtualBuffer(isVirtual)
    , IsManagedBuffer(true)
    , m_Device(inDevice)
    , m_Desc(inDesc)
    , m_InitialStates(D3D12_RESOURCE_STATE_COMMON)
    , m_BufferHandle(nullptr)
    , m_BufferDescD3D()
    , m_AllocationInfo{m_Desc.Size, 0}
    , m_ResourceHeap(nullptr)
    , m_OffsetInHeap(0)
    , m_NumMapCalls(0)
    , m_ResourceBaseAddress(nullptr)
{
    
}

D3D12Buffer::~D3D12Buffer()
{
    ShutdownInternal();
}

bool D3D12Buffer::Init()
{
    if(!IsManaged())
    {
        Log::Warning("[D3D12] Buffer is not managed, initialization is not allowed.");
        return true;
    }
    
    if(IsValid())
    {
        Log::Warning("[D3D12] Buffer already initialized.");
        return true;
    }

    m_InitialStates = D3D12_RESOURCE_STATE_COMMON;

    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

    const ERHIBufferUsage allowUnorderedAccess = ERHIBufferUsage::UnorderedAccess
       | ERHIBufferUsage::AccelerationStructureStorage
       | ERHIBufferUsage::AccelerationStructureBuildInput
       | ERHIBufferUsage::IndirectCommands;

    if((m_Desc.Usages & allowUnorderedAccess) != ERHIBufferUsage::None)
    {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    if((m_Desc.Usages & ERHIBufferUsage::AccelerationStructureStorage) == ERHIBufferUsage::AccelerationStructureStorage)
    {
        // flags |= D3D12_RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE;
        flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    if((m_Desc.Usages & ERHIBufferUsage::ConstantBuffer) == ERHIBufferUsage::ConstantBuffer) 
    {
        m_Desc.Size = Align(m_Desc.Size, 256ull); // Constant buffer size must be a multiple of 256 bytes
    }

    m_BufferDescD3D = CD3DX12_RESOURCE_DESC::Buffer(m_Desc.Size, flags);
    m_ResourceHeap.SafeRelease();
    m_ConstantBufferViews.clear();
    m_ShaderResourceViews.clear();
    m_UnorderedAccessViews.clear();
    
    if(!IsVirtual() && IsManaged())
    {
        D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;
        switch(m_Desc.CpuAccess)
        {
        case ERHICpuAccessMode::None:
            heapType = D3D12_HEAP_TYPE_DEFAULT;
            break;

        case ERHICpuAccessMode::Read:
            heapType = D3D12_HEAP_TYPE_READBACK;
            m_InitialStates |= D3D12_RESOURCE_STATE_COPY_DEST;
            break;

        case ERHICpuAccessMode::Write:
            heapType = D3D12_HEAP_TYPE_UPLOAD;
            m_InitialStates |= D3D12_RESOURCE_STATE_GENERIC_READ;
            break;
        }
        
        const CD3DX12_HEAP_PROPERTIES heapProp(heapType);
        HRESULT hr = m_Device.GetDevice()->CreateCommittedResource(&heapProp
            , D3D12_HEAP_FLAG_NONE
            , &m_BufferDescD3D
            , m_InitialStates
            , nullptr
            , IID_PPV_ARGS(&m_BufferHandle));
        
        if(FAILED(hr))
        {
            OUTPUT_D3D12_FAILED_RESULT(hr)
            return false;
        }
    }

    m_AllocationInfo = m_Device.GetDevice()->GetResourceAllocationInfo(D3D12Device::GetNodeMask(), 1, &m_BufferDescD3D);
    
    return true;
}

void D3D12Buffer::Shutdown()
{
    ShutdownInternal();
}

void D3D12Buffer::ShutdownInternal()
{
    for(auto cbv : m_ConstantBufferViews)
    {
        m_Device.GetDescriptorManager().Free(cbv.second.DescriptorHeap, cbv.second.Slot, 1);
    }
    for(auto srv : m_ShaderResourceViews)
    {
        m_Device.GetDescriptorManager().Free(srv.second.DescriptorHeap, srv.second.Slot, 1);
    }
    for(auto uav : m_UnorderedAccessViews)
    {
        m_Device.GetDescriptorManager().Free(uav.second.DescriptorHeap, uav.second.Slot, 1);
    }
    m_ConstantBufferViews.clear();
    m_ShaderResourceViews.clear();
    m_UnorderedAccessViews.clear();
    
    if(m_ResourceHeap != nullptr)
    {
        m_ResourceHeap->Free(m_OffsetInHeap, GetSizeInByte());
        m_ResourceHeap.SafeRelease();
        m_OffsetInHeap = 0;
    }

    m_BufferHandle.Reset();
}

bool D3D12Buffer::IsValid() const
{
    bool valid = m_BufferHandle != nullptr;
    if(IsManaged() && IsVirtual())
    {
        valid = valid && m_ResourceHeap != nullptr && m_ResourceHeap->IsValid();
    }
    return valid;
}

void D3D12Buffer::SetNameInternal()
{
    if(IsValid())
    {
        std::wstring name(m_Name.begin(), m_Name.end());
        m_BufferHandle->SetName(name.c_str());
    }
}

void* D3D12Buffer::Map(uint64_t inSize, uint64_t inOffset)
{
    if(m_NumMapCalls == 0)
    {
        assert(IsValid());
        assert(m_ResourceBaseAddress == nullptr);
        D3D12_RANGE Range = {inOffset, inOffset + inSize};
        m_BufferHandle->Map(0, &Range, &m_ResourceBaseAddress);
    }
    else
    {
        assert(m_ResourceBaseAddress != nullptr);
    }
    ++m_NumMapCalls;
    return m_ResourceBaseAddress;
}

void D3D12Buffer::Unmap()
{
    assert(m_BufferHandle.Get());
    assert(m_ResourceBaseAddress);
    assert(m_NumMapCalls > 0);
    
    --m_NumMapCalls;
    if(m_NumMapCalls == 0)
    {
        m_BufferHandle->Unmap(0, nullptr);
        m_ResourceBaseAddress = nullptr;
    }
}

void D3D12Buffer::WriteData(const void* inData, uint64_t inSize, uint64_t inOffset)
{
    void* data = Map(inSize, inOffset);
    memcpy(data, inData, inSize);
    Unmap();
}

void D3D12Buffer::ReadData(void* outData, uint64_t inSize, uint64_t inOffset)
{
    const void* data = Map(inSize, inOffset);
    memcpy(outData, data, inSize);
    Unmap();
}

bool D3D12Buffer::BindMemory(RefCountPtr<RHIResourceHeap> inHeap)
{
    if(!IsManaged())
    {
        Log::Warning("[D3D12] Buffer is not managed, memory binding is not allowed.");
        return true;    
    }
    
    if(!IsVirtual())
    {
        Log::Warning("[D3D12] Buffer is not virtual, memory binding is not allowed.");
        return true;
    }

    if(m_ResourceHeap != nullptr)
    {
        Log::Warning("[D3D12] Buffer already bound to a heap.");
        return true;
    }

    D3D12ResourceHeap* heap = CheckCast<D3D12ResourceHeap*>(inHeap.GetReference());

    if(heap == nullptr || !heap->IsValid())
    {
        Log::Error("[D3D12] Failed to bind buffer memory, the heap is invalid");
        return false;
    }

    const RHIResourceHeapDesc& heapDesc = heap->GetDesc();

    if(heapDesc.Usage != ERHIHeapUsage::Buffer)
    {
        Log::Error("[D3D12] Failed to bind buffer memory, the heap is not a buffer heap");
        return false;
    }

    if(m_Desc.CpuAccess == ERHICpuAccessMode::Write)
    {
        if(heapDesc.Type != ERHIResourceHeapType::Upload)
        {
            Log::Error("[D3D12] Failed to bind buffer memory, the heap is not upload heap");
            return false;
        }
        m_InitialStates = D3D12_RESOURCE_STATE_GENERIC_READ;
    }

    else if(m_Desc.CpuAccess == ERHICpuAccessMode::Read)
    {
        if(heapDesc.Type != ERHIResourceHeapType::Readback)
        {
            Log::Error("[D3D12] Failed to bind buffer memory, the heap is not read back heap");
            return false;
        }
        m_InitialStates = D3D12_RESOURCE_STATE_COPY_DEST;
    }
    
    if(!inHeap->TryAllocate(GetSizeInByte(), m_OffsetInHeap))
    {
        Log::Error("[D3D12] Failed to bind buffer memory, the heap size is not enough");
        return false;
    }

    HRESULT hr = m_Device.GetDevice()->CreatePlacedResource(heap->GetHeap()
        , m_OffsetInHeap
        , &m_BufferDescD3D
        , m_InitialStates
        , nullptr
        , IID_PPV_ARGS(&m_BufferHandle));

    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        inHeap->Free(m_OffsetInHeap, GetSizeInByte());
        m_OffsetInHeap = 0;
        return false;
    }

    m_ResourceHeap = inHeap;
    return true;
}

bool D3D12Buffer::CreateCBV(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle, const RHIBufferSubRange& inSubResource)
{
    if(!IsValid())
    {
        Log::Error("[D3D12] Buffer is not valid.");
        return false;
    }

    if(TryGetCBVHandle(outHandle, inSubResource))
    {
        Log::Warning("[D3D12] Constants buffer view already created on this subresource.");
        return true;
    }
    
    if((m_Desc.Usages & ERHIBufferUsage::ConstantBuffer) != ERHIBufferUsage::ConstantBuffer)
    {
        Log::Error("[D3D12] Buffer is not a constant buffer.");
        return false;
    }
    
    size_t offset = inSubResource.Offset;
    uint32_t size = (uint32_t)inSubResource.Size;
    if(inSubResource == RHIBufferSubRange::All)
    {
        offset = 0;
        size = (uint32_t)GetSizeInByte();
    }

    uint32_t slot;
    const D3D12DescriptorHeap* descriptorHeap = m_Device.GetDescriptorManager().Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, slot);
    if(descriptorHeap == nullptr)
    {
        Log::Error("[D3D12] Failed to allocate a descriptor for the cbv");
        return false;
    }

    outHandle = descriptorHeap->GetCpuSlotHandle(slot);

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
    cbvDesc.BufferLocation = m_BufferHandle->GetGPUVirtualAddress() + offset;
    cbvDesc.SizeInBytes = size;

    m_Device.GetDevice()->CreateConstantBufferView(&cbvDesc, outHandle);

    m_ConstantBufferViews.emplace(inSubResource, D3D12ResourceView(descriptorHeap
        , slot
        , ERHIResourceViewType::CBV
        , cbvDesc));
    
    return true;
}

bool D3D12Buffer::CreateSRV(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle, const RHIBufferSubRange& inSubResource)
{
    if(!IsValid())
    {
        Log::Error("[D3D12] Buffer is not valid.");
        return false;
    }
    
    if(TryGetSRVHandle(outHandle, inSubResource))
    {
        Log::Warning("[D3D12] Shader resource view already created on this subresource.");
        return true;
    }

    size_t offset = inSubResource.Offset;
    uint32_t size = (uint32_t)inSubResource.Size;
    if(inSubResource == RHIBufferSubRange::All)
    {
        offset = 0;
        size = (uint32_t)GetSizeInByte();
    }

    uint32_t slot;
    const D3D12DescriptorHeap* descriptorHeap = m_Device.GetDescriptorManager().Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, slot);
    if(descriptorHeap == nullptr)
    {
        Log::Error("[D3D12] Failed to allocate a descriptor for the srv");
        return false;
    }
    
    outHandle = descriptorHeap->GetCpuSlotHandle(slot);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    if(inSubResource.StructureByteStride > 0)
    {
        srvDesc.Buffer.StructureByteStride = inSubResource.StructureByteStride;
        srvDesc.Buffer.FirstElement = inSubResource.FirstElement;
        srvDesc.Buffer.NumElements = inSubResource.NumElements;
    }
    else
    {
        srvDesc.Buffer.FirstElement = offset;
        srvDesc.Buffer.NumElements = size;
        srvDesc.Buffer.StructureByteStride = 0;
    }
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    m_Device.GetDevice()->CreateShaderResourceView(m_BufferHandle.Get(), &srvDesc, outHandle);
    
    m_ShaderResourceViews.emplace(inSubResource, D3D12ResourceView(descriptorHeap
        , slot
        , ERHIResourceViewType::SRV
        , srvDesc));
    
    return true;
}

bool D3D12Buffer::CreateUAV(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle, const RHIBufferSubRange& inSubResource)
{
    if(!IsValid())
    {
        Log::Error("[D3D12] Buffer is not valid.");
        return false;
    }

    if(TryGetUAVHandle(outHandle, inSubResource))
    {
        Log::Warning("[D3D12] Unordered access view already created on this subresource.");
        return true;
    }

    size_t offset = inSubResource.Offset;
    uint32_t size = (uint32_t)inSubResource.Size;
    if(inSubResource == RHIBufferSubRange::All)
    {
        offset = 0;
        size = (uint32_t)GetSizeInByte();
    }

    uint32_t slot;
    const D3D12DescriptorHeap* descriptorHeap = m_Device.GetDescriptorManager().Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, slot);
    if(descriptorHeap == nullptr)
    {
        Log::Error("[D3D12] Failed to allocate a descriptor for the uav");
        return false;
    }

    outHandle = descriptorHeap->GetCpuSlotHandle(slot);

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    if(inSubResource.StructureByteStride > 0)
    {
        uavDesc.Buffer.StructureByteStride = inSubResource.StructureByteStride;
        uavDesc.Buffer.FirstElement = inSubResource.FirstElement;
        uavDesc.Buffer.NumElements = inSubResource.NumElements;
    }
    else
    {
        uavDesc.Buffer.StructureByteStride = 0;
        uavDesc.Buffer.FirstElement = offset;
        uavDesc.Buffer.NumElements = size;
    }
    uavDesc.Buffer.CounterOffsetInBytes = 0; // TODO: Add support for uav counters
    uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

    m_Device.GetDevice()->CreateUnorderedAccessView(m_BufferHandle.Get(), nullptr, &uavDesc, outHandle);

    m_UnorderedAccessViews.emplace(inSubResource, D3D12ResourceView(descriptorHeap
        , slot
        , ERHIResourceViewType::UAV
        , uavDesc));
    
    return true;
}

bool D3D12Buffer::TryGetCBVHandle(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle, const RHIBufferSubRange& inSubResource)
{
    if(m_ConstantBufferViews.find(inSubResource) == m_ConstantBufferViews.end())
    {
        return false;
    }

    outHandle = m_ConstantBufferViews[inSubResource].GetCpuHande();
    return true;
}

bool D3D12Buffer::TryGetSRVHandle(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle, const RHIBufferSubRange& inSubResource)
{
    if(m_ShaderResourceViews.find(inSubResource) == m_ShaderResourceViews.end())
    {
        return false;
    }

    outHandle = m_ShaderResourceViews[inSubResource].GetCpuHande();
    return true;
}

bool D3D12Buffer::TryGetUAVHandle(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle, const RHIBufferSubRange& inSubResource)
{
    if(m_UnorderedAccessViews.find(inSubResource) == m_UnorderedAccessViews.end())
    {
        return false;
    }

    outHandle = m_UnorderedAccessViews[inSubResource].GetCpuHande();
    return true;
}