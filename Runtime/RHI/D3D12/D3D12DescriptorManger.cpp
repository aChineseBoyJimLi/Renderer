#include "D3D12DescriptorManager.h"
#include "D3D12Device.h"
#include "D3D12Definitions.h"
#include "../../Core/Log.h"

D3D12DescriptorHeap::D3D12DescriptorHeap(D3D12Device& inDevice, D3D12_DESCRIPTOR_HEAP_TYPE inType, uint32_t inNum, bool inIsShaderVisible)
    : HeapType(inType)
    , NumDescriptors(inNum)
    , DescriptorSize(inDevice.GetDevice()->GetDescriptorHandleIncrementSize(HeapType))
    , IsShaderVisible(inIsShaderVisible)
    , m_Device(inDevice)
    , m_HeapHandle(nullptr)
    , m_CpuBase()
    , m_GpuBase()
{
    
}

D3D12DescriptorHeap::~D3D12DescriptorHeap()
{
    Shutdown();
}

bool D3D12DescriptorHeap::Init()
{
    if(IsValid())
    {
        Log::Warning("[D3D12] Descriptor heap is already initialized");
        return true;
    }
    
    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
    descriptorHeapDesc.Type = HeapType;
    descriptorHeapDesc.NumDescriptors = NumDescriptors;
    descriptorHeapDesc.Flags = IsShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    descriptorHeapDesc.NodeMask = D3D12Device::GetNodeMask();
    
    HRESULT hr = m_Device.GetDevice()->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&m_HeapHandle));
    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        return false;
    }
    
    m_CpuBase = m_HeapHandle->GetCPUDescriptorHandleForHeapStart();
    if (IsShaderVisible)
    {
        m_GpuBase = m_HeapHandle->GetGPUDescriptorHandleForHeapStart();
    }

    m_DescriptorAllocator.SetTotalCount(NumDescriptors);

    return true;
}

bool D3D12DescriptorHeap::IsValid() const
{
    return m_HeapHandle != nullptr;
}

void D3D12DescriptorHeap::Shutdown()
{
    m_HeapHandle.Reset();
    m_DescriptorAllocator.Reset();
}

bool D3D12DescriptorHeap::TryAllocate(uint32_t inNumDescriptors, uint32_t& outSlot)
{
    if(!IsValid())
    {
        outSlot = UINT_MAX;
        return false;
    }
    
    return m_DescriptorAllocator.TryAllocate(inNumDescriptors, outSlot);
}

void D3D12DescriptorHeap::Free(uint32_t inSlot, uint32_t inNumDescriptors)
{
    m_DescriptorAllocator.Free(inSlot, inNumDescriptors);
}

void D3D12DescriptorHeap::CopyDescriptors(uint32_t inNumDescriptors, uint32_t inDestSlot, const D3D12_CPU_DESCRIPTOR_HANDLE& srcDescriptorRangeStart)
{
    if(!IsValid())
    {
        Log::Error("[D3D12] The descriptor heap is not initialized");
        return;
    }
    const D3D12_CPU_DESCRIPTOR_HANDLE DstHandle = GetCpuSlotHandle(static_cast<int>(inDestSlot));
    m_Device.GetDevice()->CopyDescriptorsSimple(inNumDescriptors, DstHandle, srcDescriptorRangeStart, HeapType);
}

void D3D12DescriptorHeap::ResetHeap()
{
    m_DescriptorAllocator.Reset();
}

D3D12DescriptorManager::D3D12DescriptorManager(D3D12Device& inDevice)
    : m_Device(inDevice)
    , m_ShaderVisibleHeap(new D3D12DescriptorHeap(inDevice, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_1, true))
    , m_ShaderVisibleSamplersHeap(new D3D12DescriptorHeap(inDevice, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE, true))
{
    
}

D3D12DescriptorManager::~D3D12DescriptorManager()
{
    Shutdown();
}

bool D3D12DescriptorManager::Init()
{
    if(IsValid())
    {
        Log::Warning("[D3D12] Descriptor manager is already initialized");
        return true;
    }
    return m_ShaderVisibleHeap->Init() && m_ShaderVisibleSamplersHeap->Init();
}

bool D3D12DescriptorManager::IsValid() const
{
    return m_ShaderVisibleHeap->IsValid() && m_ShaderVisibleSamplersHeap->IsValid();
}

void D3D12DescriptorManager::Shutdown()
{
    for(auto i : m_ManagedDescriptorHeaps)
    {
        delete i;
    }
    m_ManagedDescriptorHeaps.clear();
}

const D3D12DescriptorHeap* D3D12DescriptorManager::Allocate(D3D12_DESCRIPTOR_HEAP_TYPE inType, uint32_t inNumDescriptors, uint32_t& outSlot)
{
    for(auto i : m_ManagedDescriptorHeaps)
    {
        if (i->HeapType == inType && i->TryAllocate(inNumDescriptors, outSlot))
        {
            return i;
        }
    }
    
    // Round up to the next multiple of 16
    const uint32_t allocatedNumDescriptors = ((inNumDescriptors >> 4) + 1) << 4;
    auto* newHeap = new D3D12DescriptorHeap(m_Device, inType, allocatedNumDescriptors, false);

    if(!newHeap->Init())
    {
        delete newHeap;
        outSlot = UINT_MAX;
        return nullptr;
    }
    
    m_ManagedDescriptorHeaps.push_back(newHeap);
    if(newHeap->TryAllocate(inNumDescriptors, outSlot))
    {
        return newHeap;
    }
    
    outSlot = UINT_MAX;
    return nullptr;
}

void D3D12DescriptorManager::Free(const D3D12DescriptorHeap* inHeap, uint32_t inSlot, uint32_t inNumDescriptors)
{
    // It's inefficient when we have a large number of descriptor heaps
    for(auto i : m_ManagedDescriptorHeaps)
    {
        if (i == inHeap)
        {
            i->Free(inSlot, inNumDescriptors);
            return;
        }
    }
}