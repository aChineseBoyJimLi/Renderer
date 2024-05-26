#pragma once

#include "D3D12DescriptorManager.h"
#include "D3D12Definitions.h"
#include "../RHIResources.h"
#include "../../Core/FreeListAllocator.h"

class D3D12PipelineBindingLayout;

///////////////////////////////////////////////////////////////////////////////////
/// D3D12ResourceHeap
///////////////////////////////////////////////////////////////////////////////////
class D3D12ResourceHeap : public RHIResourceHeap
{
public:
    ~D3D12ResourceHeap() override;
    bool Init() override;
    void Shutdown() override;
    bool IsValid() const override;
    const RHIResourceHeapDesc& GetDesc() const override {return m_Desc;}
    bool TryAllocate(size_t inSize, size_t& outOffset) override;
    void Free(size_t inOffset, size_t inSize) override;
    bool IsEmpty() const override;
    uint32_t GetTotalChunks() const override { return m_TotalChunkNum; }
    ID3D12Heap* GetHeap() const { return m_HeapHandle.Get(); }
    
protected:
    void SetNameInternal() override;
    
private:
    friend class D3D12Device;
    D3D12ResourceHeap(D3D12Device& inDevice, const RHIResourceHeapDesc& inDesc);
    void ShutdownInternal();
    
    D3D12Device& m_Device;
    RHIResourceHeapDesc m_Desc;
    Microsoft::WRL::ComPtr<ID3D12Heap> m_HeapHandle;
    uint32_t m_TotalChunkNum;
    FreeListAllocator m_MemAllocator;
};

///////////////////////////////////////////////////////////////////////////////////
/// D3D12ResourceView
///////////////////////////////////////////////////////////////////////////////////
template<typename TDesc>
struct D3D12ResourceView
{
    const D3D12DescriptorHeap* DescriptorHeap = nullptr;
    uint32_t Slot = 0;
    ERHIResourceViewType ViewType = ERHIResourceViewType::RTV;
    TDesc ViewDesc;
    
    D3D12ResourceView() = default;
    
    D3D12ResourceView(const D3D12DescriptorHeap* inHeap
        , uint32_t inSlot
        , ERHIResourceViewType inType
        , const TDesc& inDesc)
        : DescriptorHeap(inHeap)
        , Slot(inSlot)
        , ViewType(inType)
        , ViewDesc(inDesc) {}
    
    D3D12ResourceView(const D3D12ResourceView& inOther)
        : DescriptorHeap(inOther.DescriptorHeap)
        , Slot(inOther.Slot)
        , ViewType(inOther.ViewType)
        , ViewDesc(inOther.ViewDesc){}
    
    D3D12ResourceView(D3D12ResourceView&& inOther) noexcept
        : DescriptorHeap(inOther.DescriptorHeap)
        , Slot(inOther.Slot)
        , ViewType(inOther.ViewType)
        , ViewDesc(inOther.ViewDesc) {}

    D3D12_CPU_DESCRIPTOR_HANDLE GetCpuHande() const
    {
        return DescriptorHeap->GetCpuSlotHandle(Slot);
    }
    
    D3D12ResourceView& operator=(const D3D12ResourceView& inOther)
    {
        if (this != &inOther)
        {
            this->DescriptorHeap = inOther.DescriptorHeap;
            this->Slot = inOther.Slot;
            this->ViewType = inOther.ViewType;
            this->ViewDesc = inOther.ViewDesc;
        }
        return *this;
    }
    
    D3D12ResourceView& operator=(D3D12ResourceView&& inOther) noexcept
    {
        if (this != &inOther)
        {
            DescriptorHeap = inOther.DescriptorHeap;
            Slot = inOther.Slot;
            ViewType = inOther.ViewType;
            ViewDesc = inOther.ViewDesc;
        }
        return *this;
    }
};

///////////////////////////////////////////////////////////////////////////////////
/// D3D12Buffer
///////////////////////////////////////////////////////////////////////////////////
class D3D12Buffer : public RHIBuffer
{
public:
    ~D3D12Buffer() override;
    bool Init() override;
    void Shutdown() override;
    bool IsValid() const override;

    void* Map(uint64_t inSize, uint64_t inOffset = 0) override;
    void  Unmap() override;
    void  WriteData(const void* inData, uint64_t inSize, uint64_t inOffset = 0) override;
    void  ReadData(void* outData, uint64_t inSize, uint64_t inOffset = 0) override;
    bool IsVirtual() const override { return IsVirtualBuffer; }
    bool IsManaged() const override { return IsManagedBuffer; }
    bool BindMemory(RefCountPtr<RHIResourceHeap> inHeap) override;
    size_t GetOffsetInHeap() const override { return m_OffsetInHeap; }
    
    bool CreateCBV(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle, const RHIBufferSubRange& inSubResource = RHIBufferSubRange::All);
    bool CreateSRV(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle, const RHIBufferSubRange& inSubResource = RHIBufferSubRange::All);
    bool CreateUAV(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle, const RHIBufferSubRange& inSubResource = RHIBufferSubRange::All);
    
    bool TryGetCBVHandle(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle, const RHIBufferSubRange& inSubResource = RHIBufferSubRange::All);
    bool TryGetSRVHandle(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle, const RHIBufferSubRange& inSubResource = RHIBufferSubRange::All);
    bool TryGetUAVHandle(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle, const RHIBufferSubRange& inSubResource = RHIBufferSubRange::All);
    
    D3D12_RESOURCE_STATES GetCurrentState(const RHIBufferSubRange& inSubResource = RHIBufferSubRange::All);
    void ChangeState(D3D12_RESOURCE_STATES inAfterState, const RHIBufferSubRange& inSubResource = RHIBufferSubRange::All);
    
    const RHIBufferDesc& GetDesc() const override { return m_Desc; }
    uint32_t GetMemTypeFilter()  const override { return UINT32_MAX; }
    size_t GetAllocSizeInByte() const override { return m_AllocationInfo.SizeInBytes; }
    size_t GetAllocAlignment() const override { return m_AllocationInfo.Alignment; }
    ID3D12Resource* GetBuffer() const { return m_BufferHandle.Get(); }
    RHIResourceGpuAddress GetGpuAddress() const override { return IsValid() ? m_BufferHandle->GetGPUVirtualAddress() : 0; }

    const bool IsVirtualBuffer;
    const bool IsManagedBuffer;
    
protected:
    void SetNameInternal() override;

private:
    friend class D3D12Device;
    D3D12Buffer(D3D12Device& inDevice, const RHIBufferDesc& inDesc, bool isVirtual = false);
    void ShutdownInternal();
    
    D3D12Device& m_Device;
    RHIBufferDesc m_Desc;
    D3D12_RESOURCE_STATES m_InitialStates;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_BufferHandle;
    CD3DX12_RESOURCE_DESC m_BufferDescD3D;
    D3D12_RESOURCE_ALLOCATION_INFO m_AllocationInfo;
    RefCountPtr<RHIResourceHeap> m_ResourceHeap;
    size_t m_OffsetInHeap;
    
    int32_t m_NumMapCalls;
    void* m_ResourceBaseAddress;
    
    std::unordered_map<RHIBufferSubRange, D3D12ResourceView<D3D12_CONSTANT_BUFFER_VIEW_DESC>> m_ConstantBufferViews;
    std::unordered_map<RHIBufferSubRange, D3D12ResourceView<D3D12_SHADER_RESOURCE_VIEW_DESC>> m_ShaderResourceViews;
    std::unordered_map<RHIBufferSubRange, D3D12ResourceView<D3D12_UNORDERED_ACCESS_VIEW_DESC>> m_UnorderedAccessViews;
    std::unordered_map<RHIBufferSubRange, D3D12_RESOURCE_STATES> m_SubResourceStates;
};

///////////////////////////////////////////////////////////////////////////////////
/// D3D12Texture
///////////////////////////////////////////////////////////////////////////////////
class D3D12Texture : public RHITexture
{
public:
    ~D3D12Texture() override;
    bool Init() override;
    void Shutdown() override;
    bool IsValid() const override;
    bool IsVirtual() const override { return IsVirtualTexture; }
    bool IsManaged() const override { return IsManagedTexture; }
    const RHIClearValue& GetClearValue() const override { return m_Desc.ClearValue; }
    bool BindMemory(RefCountPtr<RHIResourceHeap> inHeap) override;
    size_t GetOffsetInHeap() const override { return m_OffsetInHeap; }
    const RHITextureDesc& GetDesc() const override { return m_Desc; }
    uint32_t GetMemTypeFilter()  const override { return UINT32_MAX; }
    size_t GetAllocSizeInByte() const override { return m_AllocationInfo.SizeInBytes; }
    size_t GetAllocAlignment() const override { return m_AllocationInfo.Alignment; }
    ID3D12Resource* GetTexture() const { return m_TextureHandle.Get(); }

    bool CreateRTV(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle, const RHITextureSubResource& inSubResource = RHITextureSubResource::All);
    bool CreateDSV(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle, const RHITextureSubResource& inSubResource = RHITextureSubResource::All);
    bool CreateSRV(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle, const RHITextureSubResource& inSubResource = RHITextureSubResource::All);
    bool CreateUAV(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle, const RHITextureSubResource& inSubResource = RHITextureSubResource::All);

    bool TryGetRTVHandle(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle, const RHITextureSubResource& inSubResource = RHITextureSubResource::All);
    bool TryGetDSVHandle(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle, const RHITextureSubResource& inSubResource = RHITextureSubResource::All);
    bool TryGetSRVHandle(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle, const RHITextureSubResource& inSubResource = RHITextureSubResource::All);
    bool TryGetUAVHandle(D3D12_CPU_DESCRIPTOR_HANDLE& outHandle, const RHITextureSubResource& inSubResource = RHITextureSubResource::All);

    D3D12_RESOURCE_STATES GetCurrentState(const RHITextureSubResource& inSubResource = RHITextureSubResource::All);
    void ChangeState(D3D12_RESOURCE_STATES inAfterState, const RHITextureSubResource& inSubResource = RHITextureSubResource::All);

    const bool IsVirtualTexture;
    const bool IsManagedTexture;
    
protected:
    void SetNameInternal() override;
    
private:
    friend class D3D12Device;
    D3D12Texture(D3D12Device& inDevice, const RHITextureDesc& inDesc, bool inIsVirtual);
    D3D12Texture(D3D12Device& inDevice, const RHITextureDesc& inDesc, const Microsoft::WRL::ComPtr<ID3D12Resource>& inTexture);
    void ShutdownInternal();
    
    D3D12Device& m_Device;
    RHITextureDesc m_Desc;
    D3D12_RESOURCE_STATES m_InitialStates;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_TextureHandle;
    D3D12_RESOURCE_DESC m_TextureDescD3D;
    D3D12_RESOURCE_ALLOCATION_INFO m_AllocationInfo;
    RefCountPtr<RHIResourceHeap> m_ResourceHeap;
    size_t m_OffsetInHeap;
    
    std::unordered_map<RHITextureSubResource, D3D12ResourceView<D3D12_RENDER_TARGET_VIEW_DESC>> m_RenderTargetViews;
    std::unordered_map<RHITextureSubResource, D3D12ResourceView<D3D12_DEPTH_STENCIL_VIEW_DESC>> m_DepthStencilViews;
    std::unordered_map<RHITextureSubResource, D3D12ResourceView<D3D12_SHADER_RESOURCE_VIEW_DESC>> m_ShaderResourceViews;
    std::unordered_map<RHITextureSubResource, D3D12ResourceView<D3D12_UNORDERED_ACCESS_VIEW_DESC>> m_UnorderedAccessViews;
    std::unordered_map<RHITextureSubResource, D3D12_RESOURCE_STATES> m_SubResourceStates;
};

///////////////////////////////////////////////////////////////////////////////////
/// D3D12Sampler
///////////////////////////////////////////////////////////////////////////////////
class D3D12Sampler : public RHISampler
{
public:
    ~D3D12Sampler() override;
    bool Init() override;
    void Shutdown() override;
    bool IsValid() const override;
    const RHISamplerDesc& GetDesc() const override { return m_Desc; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetHandle() const { return m_SamplerView.GetCpuHande(); }
    
private:
    friend D3D12Device;
    D3D12Sampler(D3D12Device& inDevice, const RHISamplerDesc& inDesc);
    void ShutdownInternal();
    
    D3D12Device& m_Device;
    RHISamplerDesc m_Desc;
    D3D12ResourceView<D3D12_SAMPLER_DESC> m_SamplerView;
};

///////////////////////////////////////////////////////////////////////////////////
/// D3D12AccelerationStructure
///////////////////////////////////////////////////////////////////////////////////
class D3D12AccelerationStructure : public RHIAccelerationStructure
{
public:
    ~D3D12AccelerationStructure() override;
    bool Init() override;
    void Shutdown() override;
    bool IsValid() const override;
    const RHIRayTracingGeometryDesc* GetGeometryDesc() const override { return m_GeometryDesc.data(); }
    const RHIRayTracingInstanceDesc* GetInstanceDesc() const override { return m_InstanceDesc.data(); }
    size_t GetGeometryDescCount() const override { return m_GeometryDesc.size(); }
    size_t GetInstanceDescCount() const override { return m_GeometryDesc.size(); }
    bool IsTopLevel() const override { return m_IsTopLevel; }
    bool IsBuilt() const override { return m_IsBuilt; }
    RefCountPtr<D3D12Buffer> GetAccelerationStructure() { return m_AccelerationStructureBuffer; }
    RefCountPtr<RHIBuffer> GetScratchBuffer() const override;
    void Build(ID3D12GraphicsCommandList* inCmdList, RefCountPtr<RHIBuffer>& inScratchBuffer);

protected:
    void SetNameInternal() override;
    
private:
    friend D3D12Device;
    D3D12AccelerationStructure(D3D12Device& inDevice, const std::vector<RHIRayTracingGeometryDesc>& inDesc);
    D3D12AccelerationStructure(D3D12Device& inDevice, const std::vector<RHIRayTracingInstanceDesc>& inDesc);
    void ShutdownInternal();
    bool InitBottomLevel();
    bool InitTopLevel();
    
    D3D12Device & m_Device;
    std::vector<RHIRayTracingGeometryDesc> m_GeometryDesc;
    std::vector<RHIRayTracingInstanceDesc> m_InstanceDesc;
    const bool m_IsTopLevel;
    bool m_IsBuilt;
    RefCountPtr<D3D12Buffer> m_InstanceBuffer;
    RefCountPtr<D3D12Buffer> m_AccelerationStructureBuffer;
    std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> m_GeometryDescD3D;
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS m_BuildInfos;
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO m_PrebuildInfo;
};

///////////////////////////////////////////////////////////////////////////////////
/// D3D12ResourceSet
///////////////////////////////////////////////////////////////////////////////////
struct D3D12ResourceArgument
{
    ERHIResourceViewType ViewType;
    D3D12_ROOT_PARAMETER_TYPE ParameterType;
    uint32_t Space;
    uint32_t BaseRegister;
    uint32_t NumDescriptors;
    D3D12_GPU_DESCRIPTOR_HANDLE DescriptorTableStartHandle;
    size_t ShaderVisibleDescriptorRangeIndex;
    RHIResourceGpuAddress GpuAddress;
};

struct ShaderVisibleDescriptorRange
{
    const D3D12DescriptorHeap* Heap;
    uint32_t Slot;
    uint32_t NumDescriptors;
};

class D3D12ResourceSet : public RHIResourceSet
{
public:
    ~D3D12ResourceSet() override;
    bool Init() override;
    void Shutdown() override;
    bool IsValid() const override;

    void BindBufferSRV(uint32_t inRegister, uint32_t inSpace, const RefCountPtr<RHIBuffer>& inBuffer) override;
    void BindBufferUAV(uint32_t inRegister, uint32_t inSpace, const RefCountPtr<RHIBuffer>& inBuffer) override;
    void BindBufferCBV(uint32_t inRegister, uint32_t inSpace, const RefCountPtr<RHIBuffer>& inBuffer) override;
    void BindTextureSRV(uint32_t inRegister, uint32_t inSpace, const RefCountPtr<RHITexture>& inTexture) override;
    void BindTextureUAV(uint32_t inRegister, uint32_t inSpace, const RefCountPtr<RHITexture>& inTexture) override;
    void BindSampler(uint32_t inRegister, uint32_t inSpace, const RefCountPtr<RHISampler>& inSampler) override;
    void BindBufferSRVArray(uint32_t inBaseRegister, uint32_t inSpace, const std::vector<RefCountPtr<RHIBuffer>>& inBuffer) override;
    void BindBufferUAVArray(uint32_t inBaseRegister, uint32_t inSpace, const std::vector<RefCountPtr<RHIBuffer>>& inBuffer) override;
    void BindBufferCBVArray(uint32_t inBaseRegister, uint32_t inSpace, const std::vector<RefCountPtr<RHIBuffer>>& inBuffer) override;
    void BindTextureSRVArray(uint32_t inBaseRegister, uint32_t inSpace, const std::vector<RefCountPtr<RHITexture>>& inTextures) override;
    void BindTextureUAVArray(uint32_t inBaseRegister, uint32_t inSpace, const std::vector<RefCountPtr<RHITexture>>& inTextures) override;
    void BindSamplerArray(uint32_t inBaseRegister, uint32_t inSpace, const std::vector<RefCountPtr<RHISampler>>& inSampler) override;
    void BindAccelerationStructure(uint32_t inRegister, uint32_t inSpace, const RefCountPtr<RHIAccelerationStructure>& inAccelerationStructure) override;
    
    const RHIPipelineBindingLayout* GetLayout() const override { return m_Layout; }
    void SetGraphicsRootArguments(ID3D12GraphicsCommandList* inCmdList) const;
    void SetComputeRootArguments(ID3D12GraphicsCommandList* inCmdList) const;
    
private:
    friend D3D12Device;
    D3D12ResourceSet(D3D12Device& inDevice, const RHIPipelineBindingLayout* inLayout);
    void ShutdownInternal();
    void BindResource(ERHIResourceViewType inViewType, uint32_t inRegister, uint32_t inSpace, D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle, RHIResourceGpuAddress address);
    void BindResourceArray(ERHIResourceViewType inViewType, uint32_t inBaseRegister, uint32_t inSpace, const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& cpuDescriptorHandles);
    D3D12Device& m_Device;
    const RHIPipelineBindingLayout* m_Layout;
    const D3D12PipelineBindingLayout* m_LayoutD3D;
    std::vector<ShaderVisibleDescriptorRange> m_AllocatedDescriptorRanges;
    std::vector<D3D12ResourceArgument> m_RootArguments;
};
