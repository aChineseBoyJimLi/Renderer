#pragma once
#include "D3D12Definitions.h"
#include "../../Core/FreeListAllocator.h"

class D3D12Device;

class D3D12DescriptorHeap
{
public:
    ~D3D12DescriptorHeap();
    bool Init();
    bool IsValid() const;
    void Shutdown();
    
    D3D12_CPU_DESCRIPTOR_HANDLE GetCpuSlotHandle(uint32_t inSlot) const { return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_CpuBase, static_cast<int>(inSlot), DescriptorSize); }
    D3D12_GPU_DESCRIPTOR_HANDLE GetGpuSlotHandle(uint32_t inSlot) const { return CD3DX12_GPU_DESCRIPTOR_HANDLE(m_GpuBase, static_cast<int>(inSlot), DescriptorSize); }
    ID3D12DescriptorHeap* GetHeap() const { return m_HeapHandle.Get(); }
    
    const D3D12_DESCRIPTOR_HEAP_TYPE HeapType;
    const uint32_t NumDescriptors;
    const uint32_t DescriptorSize;
    const bool IsShaderVisible;
    
private:
    friend class D3D12DescriptorManager;
    D3D12DescriptorHeap(D3D12Device& inDevice, D3D12_DESCRIPTOR_HEAP_TYPE inType, uint32_t inNum, bool inIsShaderVisible = false);

    // The srcDescriptorRangeStart parameter must be in a non shader-visible descriptor heap.
    // https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device-copydescriptorssimple
    void CopyDescriptors(uint32_t inNumDescriptors, uint32_t inDestSlot, const D3D12_CPU_DESCRIPTOR_HANDLE& srcDescriptorRangeStart);
    // Try to allocate a number of descriptors from the heap, returns true if successful
    bool TryAllocate(uint32_t inNumDescriptors, uint32_t& outSlot);
    void Free(uint32_t inSlot, uint32_t inNumDescriptors);
    // Reset the heap to its initial state
    void ResetHeap();
    
    D3D12Device& m_Device;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_HeapHandle;
    CD3DX12_CPU_DESCRIPTOR_HANDLE m_CpuBase;
    CD3DX12_GPU_DESCRIPTOR_HANDLE m_GpuBase;

    FreeListAllocator m_DescriptorAllocator;
};


class D3D12DescriptorManager
{
public:
    D3D12DescriptorManager(D3D12Device& inDevice);
    ~D3D12DescriptorManager();

    bool Init();
    bool IsValid() const;
    void Shutdown();
    
    // Try to allocate a number of descriptors
    const D3D12DescriptorHeap* Allocate(D3D12_DESCRIPTOR_HEAP_TYPE inType, uint32_t inNumDescriptors, uint32_t& outSlot);
    const D3D12DescriptorHeap* AllocateShaderVisibleDescriptors(uint32_t inNumDescriptors, uint32_t& outSlot);
    const D3D12DescriptorHeap* AllocateShaderVisibleSamplers(uint32_t inNumDescriptors, uint32_t& outSlot);
    
    // Free a number of descriptors from the heap
    void Free(const D3D12DescriptorHeap* inHeap, uint32_t inSlot, uint32_t inNumDescriptors);
    
    // Copy a number of descriptors to this heap
    void CopyDescriptors(const D3D12DescriptorHeap* inHeap, uint32_t inNumDescriptors, uint32_t inDestSlot, const D3D12_CPU_DESCRIPTOR_HANDLE& srcDescriptorRangeStart);

    void BindShaderVisibleHeaps(ID3D12GraphicsCommandList* inCmdList) const;
    
private:
    D3D12Device& m_Device;
    std::vector<D3D12DescriptorHeap*> m_ManagedDescriptorHeaps;
    D3D12DescriptorHeap* m_ShaderVisibleHeap;
    D3D12DescriptorHeap* m_ShaderVisibleSamplersHeap;
};