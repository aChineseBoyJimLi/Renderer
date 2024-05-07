#pragma once

#include <unordered_map>

#include "D3D12Definitions.h"
#include "../RHIResources.h"

class D3D12DescriptorHeap;

class D3D12ResourceHeap : public RHIResourceHeap
{
public:
    ~D3D12ResourceHeap() override;
    bool Init() override;
    void Shutdown() override;
    bool IsValid() const override;
    const RHIResourceHeapDesc& GetDesc() const override {return m_Desc;}
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
};

template<typename TDesc>
struct D3D12ResourceView
{
    const D3D12DescriptorHeap* const DescriptorHeap;
    const uint32_t Slot;
    const ERHIResourceViewType ViewType;
    const TDesc ViewDesc;
    ERHIResourceState CurrentState;

    D3D12ResourceView(const D3D12DescriptorHeap* inHeap
        , uint32_t inSlot
        , ERHIResourceViewType inType
        , const TDesc& inDesc
        , ERHIResourceState inState)
        : DescriptorHeap(inHeap)
        , Slot(inSlot)
        , ViewType(inType)
        , ViewDesc(inDesc)
        , CurrentState(inState) {}
};

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

    const RHIBufferDesc& GetDesc() const override { return m_Desc; }
    ID3D12Resource* GetBuffer() const { return m_BufferHandle.Get(); }
    RHIResourceGpuAddress GetGpuAddress() const override { return IsValid() ? m_BufferHandle->GetGPUVirtualAddress() : 0; }
    
protected:
    void SetNameInternal() override;

private:
    friend class D3D12Device;
    D3D12Buffer(D3D12Device& inDevice, const RHIBufferDesc& inDesc);
    void ShutdownInternal();
    
    D3D12Device& m_Device;
    RHIBufferDesc m_Desc;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_BufferHandle;
    std::unordered_map<RHIBufferSubRange, D3D12ResourceView<D3D12_CONSTANT_BUFFER_VIEW_DESC>> m_ConstantBufferViews;
    std::unordered_map<RHIBufferSubRange, D3D12ResourceView<D3D12_SHADER_RESOURCE_VIEW_DESC>> m_ShaderResourceViews;
    std::unordered_map<RHIBufferSubRange, D3D12ResourceView<D3D12_UNORDERED_ACCESS_VIEW_DESC>> m_UnorderedAccessViews;
};

class D3D12Texture : public RHITexture
{
public:
    ~D3D12Texture() override;
    bool Init() override;
    void Shutdown() override;
    bool IsValid() const override;
    
    const RHITextureDesc& GetDesc() const override { return m_Desc; }
    ID3D12Resource* GetTexture() const { return m_TextureHandle.Get(); }
    RHIResourceGpuAddress GetGpuAddress() const override { return IsValid() ? m_TextureHandle->GetGPUVirtualAddress() : 0; }

protected:
    void SetNameInternal() override;
    
private:
    friend class D3D12Device;
    D3D12Texture(D3D12Device& inDevice, const RHITextureDesc& inDesc);
    void ShutdownInternal();
    
    D3D12Device& m_Device;
    RHITextureDesc m_Desc;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_TextureHandle;
    std::unordered_map<RHITextureSubResource, D3D12ResourceView<D3D12_RENDER_TARGET_VIEW_DESC>> m_RenderTargetViews;
    std::unordered_map<RHITextureSubResource, D3D12ResourceView<D3D12_DEPTH_STENCIL_VIEW_DESC>> m_DepthStencilViews;
    std::unordered_map<RHITextureSubResource, D3D12ResourceView<D3D12_SHADER_RESOURCE_VIEW_DESC>> m_ShaderResourceViews;
    std::unordered_map<RHITextureSubResource, D3D12ResourceView<D3D12_UNORDERED_ACCESS_VIEW_DESC>> m_UnorderedAccessViews;
};