#pragma once

#include "../RHIResources.h"
#include "VulkanDefinitions.h"
#include "../../Core/FreeListAllocator.h"


class VulkanDevice;

///////////////////////////////////////////////////////////////////////////////////
/// VulkanResourceHeap
///////////////////////////////////////////////////////////////////////////////////
class VulkanResourceHeap : public RHIResourceHeap
{
public:
    ~VulkanResourceHeap() override;
    bool Init() override;
    void Shutdown() override;
    bool IsValid() const override;
    const RHIResourceHeapDesc& GetDesc() const override {return m_Desc;}
    bool TryAllocate(size_t inSize, size_t& outOffset) override;
    void Free(size_t inOffset, size_t inSize) override;
    VkDeviceMemory GetHeap() const { return m_HeapHandle; }
    uint32_t GetMemoryTypeIndex() const { return m_MemoryTypeIndex; }
    bool IsEmpty() const override;
    uint32_t GetTotalChunks() const { return m_TotalChunkNum; }
    
protected:
    void SetNameInternal() override;
    
private:
    friend VulkanDevice;
    VulkanResourceHeap(VulkanDevice& inDevice, const RHIResourceHeapDesc& inDesc, uint32_t inMemoryTypeIndex);
    void ShutdownInternal();
    
    VulkanDevice& m_Device;
    RHIResourceHeapDesc m_Desc;
    VkDeviceMemory m_HeapHandle;
    uint32_t m_MemoryTypeIndex;
    uint32_t m_TotalChunkNum;
    FreeListAllocator m_MemAllocator;
};

///////////////////////////////////////////////////////////////////////////////////
/// VulkanBuffer
///////////////////////////////////////////////////////////////////////////////////
class VulkanBuffer : public RHIBuffer
{
public:
    ~VulkanBuffer() override;
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
    const RHIBufferDesc& GetDesc() const override { return m_Desc; }
    RHIResourceGpuAddress GetGpuAddress() const override;
    uint32_t GetMemTypeFilter()  const override { return m_MemRequirements.memoryTypeBits; }
    size_t GetSizeInByte() const override { return m_MemRequirements.size; }
    size_t GetAlignment() const override { return m_MemRequirements.alignment; }
    
    const bool IsVirtualBuffer;
    const bool IsManagedBuffer;
    
protected:
    void SetNameInternal() override;

private:
    friend VulkanDevice;
    VulkanBuffer(VulkanDevice& inDevice, const RHIBufferDesc& inDesc, bool inIsVirtual = false);
    void ShutdownInternal();

    VulkanDevice& m_Device;
    RHIBufferDesc m_Desc;
    VkBuffer m_BufferHandle;
    VkAccessFlags m_InitialAccessFlags;
    VkMemoryRequirements m_MemRequirements;

    RefCountPtr<RHIResourceHeap> m_ResourceHeap;
    size_t m_OffsetInHeap;

    int32_t m_NumMapCalls;
    void* m_ResourceBaseAddress;
};

///////////////////////////////////////////////////////////////////////////////////
/// VulkanTexture
///////////////////////////////////////////////////////////////////////////////////

struct VulkanTextureState
{
    VkAccessFlags AccessFlags;
    VkImageLayout Layout;

    VulkanTextureState(VkAccessFlags inFlags, VkImageLayout inLayout)
        : AccessFlags(inFlags)
        , Layout(inLayout)
    {}
};

class VulkanTexture : public RHITexture
{
public:
    ~VulkanTexture() override;
    bool Init() override;
    void Shutdown() override;
    bool IsValid() const override;
    bool IsVirtual() const override { return IsVirtualTexture; }
    bool IsManaged() const override { return IsManagedTexture; }
    const RHIClearValue& GetClearValue() const override { return m_Desc.ClearValue; }
    bool BindMemory(RefCountPtr<RHIResourceHeap> inHeap) override;
    const RHITextureDesc& GetDesc() const override { return m_Desc; }
    size_t GetOffsetInHeap() const override { return m_OffsetInHeap; }
    uint32_t GetMemTypeFilter()  const override { return m_MemRequirements.memoryTypeBits; }
    size_t GetSizeInByte() const override { return m_MemRequirements.size; }
    size_t GetAlignment() const override { return m_MemRequirements.alignment; }
    VkImage GetTextureHandle() const { return m_TextureHandle; }
    bool CreateRTV(VkImageView& outHandle, const RHITextureSubResource& inSubResource = RHITextureSubResource::All);
    bool CreateDSV(VkImageView& outHandle, const RHITextureSubResource& inSubResource = RHITextureSubResource::All);
    bool CreateSRV(VkImageView& outHandle, const RHITextureSubResource& inSubResource = RHITextureSubResource::All);
    bool CreateUAV(VkImageView& outHandle, const RHITextureSubResource& inSubResource = RHITextureSubResource::All);

    bool TryGetRTVHandle(VkImageView& outHandle, const RHITextureSubResource& inSubResource = RHITextureSubResource::All);
    bool TryGetDSVHandle(VkImageView& outHandle, const RHITextureSubResource& inSubResource = RHITextureSubResource::All);
    bool TryGetSRVHandle(VkImageView& outHandle, const RHITextureSubResource& inSubResource = RHITextureSubResource::All);
    bool TryGetUAVHandle(VkImageView& outHandle, const RHITextureSubResource& inSubResource = RHITextureSubResource::All);

    VulkanTextureState GetCurrentState(const RHITextureSubResource& inSubResource = RHITextureSubResource::All);
    void ChangeState(const VulkanTextureState& inAfterState, const RHITextureSubResource& inSubResource = RHITextureSubResource::All);

    
    const bool IsVirtualTexture;
    const bool IsManagedTexture;
    
protected:
    void SetNameInternal() override;

private:
    friend VulkanDevice;
    VulkanTexture(VulkanDevice& inDevice, const RHITextureDesc& inDesc, bool inIsVirtual = false);
    VulkanTexture(VulkanDevice& inDevice, const RHITextureDesc& inDesc, VkImage inImage);
    void ShutdownInternal();
    bool CreateImageView(VkImageView& outImageView, const RHITextureSubResource& inSubResource, bool depth = false, bool stencil = false) const;
    
    VulkanDevice& m_Device;
    RHITextureDesc m_Desc;
    VkImage m_TextureHandle;
    VkAccessFlags m_InitialAccessFlags;
    VkImageLayout m_InitialLayout;
    VkMemoryRequirements m_MemRequirements;

    RefCountPtr<RHIResourceHeap> m_ResourceHeap;
    size_t m_OffsetInHeap;

    std::unordered_map<RHITextureSubResource, VkImageView> m_RenderTargetViews;
    std::unordered_map<RHITextureSubResource, VkImageView> m_DepthStencilViews;
    std::unordered_map<RHITextureSubResource, VkImageView> m_ShaderResourceViews;
    std::unordered_map<RHITextureSubResource, VkImageView> m_UnorderedAccessViews;
    std::unordered_map<RHITextureSubResource, VulkanTextureState> m_SubResourceStates;
};

///////////////////////////////////////////////////////////////////////////////////
/// VulkanSampler
///////////////////////////////////////////////////////////////////////////////////
class VulkanSampler : public RHISampler
{
public:
    ~VulkanSampler() override;
    bool Init() override;
    void Shutdown() override;
    bool IsValid() const override;
    const RHISamplerDesc& GetDesc() const override { return m_Desc; }
    VkSampler GetSamplerHandle() const { return m_SamplerHandle; }
    
protected:
    void SetNameInternal() override;
    
private:
    friend VulkanDevice;
    VulkanSampler(VulkanDevice& inDevice, const RHISamplerDesc& inDesc);
    void ShutdownInternal();
    VulkanDevice& m_Device;
    RHISamplerDesc m_Desc;
    VkSampler m_SamplerHandle;
};

