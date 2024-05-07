#pragma once
#include "D3D12Definitions.h"

class D3D12Device;

class D3D12DescriptorHeap
{
public:
    ~D3D12DescriptorHeap();
    bool Init();
    bool IsValid() const;
    void Shutdown();
    // Try to copy a number of descriptors to this heap
    void CopyDescriptors(uint32_t inNumDescriptors, uint32_t inDestSlot, const D3D12_CPU_DESCRIPTOR_HANDLE& srcDescriptorRangeStart);
    // Try to allocate a number of descriptors from the heap, returns true if successful
    // TODO: Try to use Buddy Allocator to reduce fragmentation
    bool TryAllocate(uint32_t inNumDescriptors, uint32_t& outSlot);
    // Free a number of descriptors from the heap
    void Free(uint32_t inSlot, uint32_t inNumDescriptors);
    // Reset the heap to its initial state
    void ResetHeap();

    D3D12_CPU_DESCRIPTOR_HANDLE GetCpuSlotHandle(uint32_t inSlot) const { return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_CpuBase, static_cast<int>(inSlot), DescriptorSize); }
    D3D12_GPU_DESCRIPTOR_HANDLE GetGpuSlotHandle(uint32_t inSlot) const { return CD3DX12_GPU_DESCRIPTOR_HANDLE(m_GpuBase, static_cast<int>(inSlot), DescriptorSize); }
    
    const D3D12_DESCRIPTOR_HEAP_TYPE HeapType;
    const uint32_t NumDescriptors;
    const uint32_t DescriptorSize;
    const bool IsShaderVisible;
    
private:
    friend class D3D12DescriptorManager;
    D3D12DescriptorHeap(D3D12Device& inDevice, D3D12_DESCRIPTOR_HEAP_TYPE inType, uint32_t inNum, bool inIsShaderVisible = false);
    
    D3D12Device& m_Device;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_HeapHandle;
    CD3DX12_CPU_DESCRIPTOR_HANDLE m_CpuBase;
    CD3DX12_GPU_DESCRIPTOR_HANDLE m_GpuBase;

    struct DescriptorAllocatorRange
    {
        uint32_t First;
        uint32_t Last;

        DescriptorAllocatorRange(uint32_t InFirst, uint32_t InLast)
            : First(InFirst), Last(InLast) {}
    };

    std::list<DescriptorAllocatorRange> m_FreeList;
};


class D3D12DescriptorManager
{
public:
    D3D12DescriptorManager(D3D12Device& inDevice);
    ~D3D12DescriptorManager();

    bool Init();
    bool IsValid() const;
    void Shutdown();
    
    const D3D12DescriptorHeap* Allocate(D3D12_DESCRIPTOR_HEAP_TYPE inType, uint32_t inNumDescriptors, uint32_t& outSlot);
    void Free(const D3D12DescriptorHeap* inHeap, uint32_t inSlot, uint32_t inNumDescriptors);
    
private:
    D3D12Device& m_Device;
    std::vector<D3D12DescriptorHeap*> m_ManagedDescriptorHeaps;
    std::unique_ptr<D3D12DescriptorHeap> m_ShaderVisibleHeap;
    std::unique_ptr<D3D12DescriptorHeap> m_ShaderVisibleSamplersHeap;
};