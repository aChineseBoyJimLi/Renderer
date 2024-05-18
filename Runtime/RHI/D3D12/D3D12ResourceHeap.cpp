#include "D3D12Resources.h"
#include "D3D12Device.h"
#include "../../Core/Log.h"
#include "../../Core/Templates.h"

RefCountPtr<RHIResourceHeap> D3D12Device::CreateResourceHeap(const RHIResourceHeapDesc& inDesc)
{
    RefCountPtr<RHIResourceHeap> heap(new D3D12ResourceHeap(*this, inDesc));
    if(!heap->Init())
    {
        Log::Error("[D3D12] Failed to create resource heap");
    }
    return heap;
}

D3D12ResourceHeap::D3D12ResourceHeap(D3D12Device& inDevice, const RHIResourceHeapDesc& inDesc)
    : m_Device(inDevice), m_Desc(inDesc), m_HeapHandle(nullptr), m_TotalChunkNum(0)
{
    
}

D3D12ResourceHeap::~D3D12ResourceHeap()
{
    ShutdownInternal();
}

bool D3D12ResourceHeap::Init()
{
    if(IsValid())
    {
        Log::Warning("[D3D12] Resource Heap already initialized.");
        return true;
    }

    m_Desc.Size = Align(m_Desc.Size, m_Desc.Alignment);

    D3D12_HEAP_DESC heapDesc{};
    heapDesc.SizeInBytes = m_Desc.Size;
    heapDesc.Alignment = m_Desc.Alignment;
    heapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES;
    heapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapDesc.Properties.CreationNodeMask = D3D12Device::GetCreationNodeMask(); 
    heapDesc.Properties.VisibleNodeMask = D3D12Device::GetVisibleNodeMask();
    heapDesc.Properties.Type = RHI::D3D12::ConvertHeapType(m_Desc.Type);

    HRESULT hr = m_Device.GetDevice()->CreateHeap(&heapDesc, IID_PPV_ARGS(&m_HeapHandle));
    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        return false;
    }

    m_TotalChunkNum = static_cast<uint32_t>(m_Desc.Size / m_Desc.Alignment);
    m_MemAllocator.SetTotalCount(m_TotalChunkNum);
    
    return true;
}

void D3D12ResourceHeap::Shutdown()
{
    ShutdownInternal();
}

void D3D12ResourceHeap::ShutdownInternal()
{
    m_HeapHandle.Reset();
}

bool D3D12ResourceHeap::IsValid() const
{
    return m_HeapHandle.Get() != nullptr;
}

void D3D12ResourceHeap::SetNameInternal()
{
    if(IsValid())
    {
        std::wstring name(m_Name.begin(), m_Name.end());
        m_HeapHandle->SetName(name.c_str());
    }
}

bool D3D12ResourceHeap::TryAllocate(size_t inSize, size_t& outOffset)
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

void D3D12ResourceHeap::Free(size_t inOffset, size_t inSize)
{
    if(inOffset % m_Desc.Alignment != 0)
    {
        Log::Error("[D3D12] Free offset is not aligned to the heap's alignment: %d", m_Desc.Alignment);
        return;
    }
    
    uint32_t offsetChunks = static_cast<uint32_t>(inOffset / m_Desc.Alignment);
    uint32_t chunks = static_cast<uint32_t>(Align(inSize, m_Desc.Alignment) / m_Desc.Alignment);

    m_MemAllocator.Free(offsetChunks, chunks);
}

bool D3D12ResourceHeap::IsEmpty() const
{
    return m_MemAllocator.IsEmpty();
}
