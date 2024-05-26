#pragma once

#include "RHIDefinitions.h"

class RHIPipelineBindingLayout;
class RHIResourceHeap;

///////////////////////////////////////////////////////////////////////////////////
/// RHIResourceHeap
///////////////////////////////////////////////////////////////////////////////////
struct RHIResourceHeapDesc
{
    ERHIResourceHeapType Type = ERHIResourceHeapType::DeviceLocal;
    ERHIHeapUsage Usage = ERHIHeapUsage::Buffer;
    uint64_t Size = 0;
    uint64_t Alignment = 65536;
    uint32_t TypeFilter = ~0u; // using for Vulkan
};

class RHIResourceHeap : public RHIObject
{
public:
    virtual const RHIResourceHeapDesc& GetDesc() const = 0;
    virtual bool IsEmpty() const = 0;
    virtual bool TryAllocate(size_t inSize, size_t& outOffset) = 0;
    virtual void Free(size_t inOffset, size_t inSize) = 0;
    virtual uint32_t GetTotalChunks() const = 0;
};

///////////////////////////////////////////////////////////////////////////////////
/// RHIBuffer
///////////////////////////////////////////////////////////////////////////////////
struct RHIBufferDesc
{
    uint64_t Size = 0;
    uint64_t Stride = 0;
    ERHIFormat Format = ERHIFormat::Unknown; 
    ERHICpuAccessMode CpuAccess = ERHICpuAccessMode::None;
    ERHIBufferUsage Usages = ERHIBufferUsage::None;
    
    static RHIBufferDesc ConstantsBuffer(uint64_t inSize)
    {
        RHIBufferDesc desc;
        desc.Size = inSize;
        desc.CpuAccess = ERHICpuAccessMode::Write;
        desc.Usages = ERHIBufferUsage::ConstantBuffer;
        return desc;
    }

    static RHIBufferDesc StructuredBuffer(uint64_t inSize, uint64_t inStride, ERHIBufferUsage inUsage)
    {
        RHIBufferDesc desc;
        desc.Size = inSize;
        desc.Stride = inStride;
        desc.CpuAccess = ERHICpuAccessMode::None;
        desc.Usages = inUsage;
        return desc;
    }
    
    static RHIBufferDesc VertexBuffer(uint64_t inSize, uint64_t inStride, bool isAccelerationStructureBuildInput = false)
    {
        RHIBufferDesc desc;
        desc.Size = inSize;
        desc.Stride = inStride;
        desc.CpuAccess = ERHICpuAccessMode::None;
        desc.Usages = isAccelerationStructureBuildInput ? ERHIBufferUsage::AccelerationStructureBuildInput : ERHIBufferUsage::VertexBuffer;
        return desc;
    }

    static RHIBufferDesc IndexBuffer(uint64_t inSize, ERHIFormat inFormat, bool isAccelerationStructureBuildInput = false)
    {
        const RHIFormatInfo& formatInfo = RHI::GetFormatInfo(inFormat);
        RHIBufferDesc desc;
        desc.Size = inSize;
        desc.Format = inFormat;
        desc.CpuAccess = ERHICpuAccessMode::None;
        desc.Usages = isAccelerationStructureBuildInput ? ERHIBufferUsage::AccelerationStructureBuildInput : ERHIBufferUsage::IndexBuffer;
        desc.Stride = formatInfo.BytesPerBlock;
        return desc;
    }

    static RHIBufferDesc StagingBuffer(uint64_t inSize)
    {
        RHIBufferDesc desc;
        desc.CpuAccess = ERHICpuAccessMode::Write;
        desc.Usages = ERHIBufferUsage::None;
        desc.Size = inSize;
        return desc;
    }

    static RHIBufferDesc AccelerationStructure(uint64_t inSize)
    {
        RHIBufferDesc desc;
        desc.Size = inSize;
        desc.CpuAccess = ERHICpuAccessMode::None;
        desc.Usages = ERHIBufferUsage::AccelerationStructureStorage;
        return desc;
    }

    static RHIBufferDesc ShaderTable(uint64_t inSize)
    {
        RHIBufferDesc desc;
        desc.Size = inSize;
        desc.CpuAccess = ERHICpuAccessMode::Write;
        desc.Usages = ERHIBufferUsage::ShaderTable;
        return desc;
    }
};

struct RHIBufferSubRange
{
    uint64_t Offset = 0;
    uint64_t Size = 0;
    // Structured Buffer
    uint32_t FirstElement = 0;
    uint32_t NumElements = 0;
    uint32_t StructureByteStride = 0;

    const static RHIBufferSubRange All;

    bool operator==(const RHIBufferSubRange& Other) const
    {
        return Offset == Other.Offset && Size == Other.Size &&
            FirstElement == Other.FirstElement && NumElements == Other.NumElements && StructureByteStride == Other.StructureByteStride;
    }

    bool operator!=(const RHIBufferSubRange& Other) const
    {
        return !(*this == Other);
    }
};

template<>
struct std::hash<RHIBufferSubRange>
{
    size_t operator()(const RHIBufferSubRange& subRange) const noexcept
    {
        std::hash<uint64_t> hasher64;
        std::hash<uint32_t> hasher32;

        size_t h1 = hasher64(subRange.Offset);
        size_t h2 = hasher64(subRange.Size);
        size_t h3 = hasher32(subRange.FirstElement);
        size_t h4 = hasher32(subRange.NumElements);
        size_t h5 = hasher32(subRange.StructureByteStride);
        
        size_t combinedHash = h1 ^ (h2 << 1) ^ (h3 << 3) ^ (h4 << 4) ^ (h5 << 5);
        return combinedHash;
    }
};

class RHIBuffer : public RHIObject
{
public:
    virtual bool IsVirtual() const = 0;
    virtual bool IsManaged() const = 0;
    virtual bool BindMemory(RefCountPtr<RHIResourceHeap> inHeap) = 0;
    virtual size_t GetOffsetInHeap() const = 0;
    virtual void* Map(uint64_t inSize, uint64_t inOffset = 0) = 0;
    virtual void  Unmap() = 0;
    virtual void  WriteData(const void* inData, uint64_t inSize, uint64_t inOffset = 0) = 0;
    virtual void  ReadData(void* outData, uint64_t inSize, uint64_t inOffset = 0) = 0;
    virtual const RHIBufferDesc& GetDesc() const = 0;
    virtual uint32_t GetMemTypeFilter() const = 0; // Using for vulkan buffer memory allocation, d3d12 return UINT32_MAX
    virtual size_t GetAllocSizeInByte() const = 0;
    virtual size_t GetAllocAlignment() const = 0;
    virtual RHIResourceGpuAddress GetGpuAddress() const = 0;
};

///////////////////////////////////////////////////////////////////////////////////
/// RHITexture
///////////////////////////////////////////////////////////////////////////////////
struct RHITextureDesc
{
    ERHIFormat Format = ERHIFormat::RGBA8_UNORM;
    ERHITextureDimension Dimension = ERHITextureDimension::Texture2D;
    uint32_t Width = 0;
    uint32_t Height = 0;
    uint32_t Depth = 1;
    uint32_t ArraySize = 1;
    uint32_t MipLevels = 1;
    uint32_t SampleCount = 1;
    ERHITextureUsage Usages = ERHITextureUsage::ShaderResource;
    RHIClearValue ClearValue = RHIClearValue::Transparent;
};

struct RHITextureSubResource
{
    uint32_t FirstMipSlice = 0;
    uint32_t NumMipSlices = 1;
    // Texture Array
    uint32_t FirstArraySlice = 0;
    uint32_t NumArraySlices = 1;

    const static RHITextureSubResource All;

    bool operator==(const RHITextureSubResource& Other) const
    {
        return FirstMipSlice == Other.FirstMipSlice && NumMipSlices == Other.NumMipSlices &&
            FirstArraySlice == Other.FirstArraySlice && NumArraySlices == Other.NumArraySlices;
    }

    bool operator!=(const RHITextureSubResource& Other) const
    {
        return !(*this == Other);
    }
};

// a single texture of a single mip level in texture array
struct RHITextureSlice
{
    uint32_t X = 0;
    uint32_t Y = 0;
    uint32_t Z = 0;

    uint32_t Width = UINT32_MAX;
    uint32_t Height = UINT32_MAX;
    uint32_t Depth = UINT32_MAX;

    uint32_t MipLevel = 0;
    uint32_t ArraySlice = 0;

    RHITextureSlice() = delete;
    
    RHITextureSlice(const RHITextureDesc& inDesc, uint32_t inMipLevel = 0, uint32_t inArraySlice = 0)
    {
        assert(inArraySlice < inDesc.ArraySize);
        assert(inMipLevel < inDesc.MipLevels);

        MipLevel = inMipLevel;
        ArraySlice = inArraySlice;
        Width = inDesc.Width >> inMipLevel;
        Height = inDesc.Height >> inMipLevel;

        if(inDesc.Dimension == ERHITextureDimension::Texture3D)
        {
            Depth = inDesc.Depth >> inMipLevel;
        }
        else
        {
            Depth = 1;
        }
    }
};

template<>
struct std::hash<RHITextureSubResource>
{
    size_t operator()(const RHITextureSubResource& subResource) const noexcept
    {
        std::hash<uint32_t> hasher32;

        size_t h1 = hasher32(subResource.FirstMipSlice);
        size_t h2 = hasher32(subResource.NumMipSlices);
        size_t h3 = hasher32(subResource.FirstArraySlice);
        size_t h4 = hasher32(subResource.NumArraySlices);

        size_t combinedHash = h1 ^ (h2 << 1) ^ (h3 << 3) ^ (h4 << 4);
        return combinedHash;
    }
};

class RHITexture : public RHIObject
{
public:
    virtual bool IsVirtual() const = 0;
    virtual bool IsManaged() const = 0;
    virtual bool BindMemory(RefCountPtr<RHIResourceHeap> inHeap) = 0;
    virtual size_t GetOffsetInHeap() const = 0;
    virtual const RHITextureDesc& GetDesc() const = 0;
    virtual uint32_t GetMemTypeFilter() const = 0; // Using for vulkan texture memory allocation, d3d12 return UINT32_MAX
    virtual size_t GetAllocSizeInByte() const = 0;
    virtual size_t GetAllocAlignment() const = 0;
    virtual const RHIClearValue& GetClearValue() const = 0;
};

///////////////////////////////////////////////////////////////////////////////////
/// RHISampler
///////////////////////////////////////////////////////////////////////////////////
struct RHISamplerDesc
{
    float BorderColor[4] {1.f, 1.f, 1.f, 1.f};
    float MaxAnisotropy = 1.f;
    float MipBias = 0.f;
    bool MinFilter = true;
    bool MagFilter = true;
    bool MipFilter = true;
    ERHISamplerAddressMode AddressU = ERHISamplerAddressMode::Clamp;
    ERHISamplerAddressMode AddressV = ERHISamplerAddressMode::Clamp;
    ERHISamplerAddressMode AddressW = ERHISamplerAddressMode::Clamp;
    ERHISamplerReductionType ReductionType = ERHISamplerReductionType::Standard;
};

class RHISampler : public RHIObject
{
public:
    virtual const RHISamplerDesc& GetDesc() const = 0;
};

///////////////////////////////////////////////////////////////////////////////////
/// RHIAccelerationStructure
///////////////////////////////////////////////////////////////////////////////////

struct RHIRayTracingGeometryTriangleDesc
{
    // RefCountPtr<RHIBuffer> VertexBuffer;
    // RefCountPtr<RHIBuffer> IndexBuffer;
    RHIBuffer* VertexBuffer  = nullptr;
    RHIBuffer* IndexBuffer  = nullptr;
    uint32_t VertexOffsetCount;
    uint32_t IndexOffsetCount;
    uint32_t VertexCount;
    uint32_t IndexCount;
    uint32_t PrimCount;
};

struct RHIRayTracingGeometryAABBDesc
{
    // RefCountPtr<RHIBuffer> Buffer;
    RHIBuffer* Buffer = nullptr;
    uint32_t OffsetCount = 0;
    uint32_t Count = 0;
    uint32_t Stride = 0;
};

struct RHIRayTracingGeometryDesc
{
    ERHIRayTracingGeometryType Type = ERHIRayTracingGeometryType::Triangles;
    ERHIRayTracingGeometryFlags Flags = ERHIRayTracingGeometryFlags::None;
    union
    {
        RHIRayTracingGeometryTriangleDesc Triangles;
        RHIRayTracingGeometryAABBDesc AABBs;
    };
};

struct RHIRayTracingInstanceDesc
{
    RHIAffineTransform Transform{};
    uint32_t InstanceID : 24;
    uint32_t InstanceMask : 8;
    uint32_t InstanceContributionToHitGroupIndex	: 24;
    ERHIRayTracingInstanceFlags Flags : 8;
    RHIResourceGpuAddress* BLAS = nullptr;
};

class RHIAccelerationStructure : public RHIObject
{
public:
    virtual const RHIRayTracingGeometryDesc* GetGeometryDesc() const = 0;
    virtual const RHIRayTracingInstanceDesc* GetInstanceDesc() const = 0;
    virtual RefCountPtr<RHIBuffer> GetScratchBuffer() const = 0;
    virtual size_t GetGeometryDescCount() const = 0;
    virtual size_t GetInstanceDescCount() const = 0;
    virtual bool IsTopLevel() const = 0;
    virtual bool IsBuilt() const = 0;
};

///////////////////////////////////////////////////////////////////////////////////
/// RHIResourcesSet
///////////////////////////////////////////////////////////////////////////////////
class RHIResourceSet : public RHIObject
{
public:
    virtual const RHIPipelineBindingLayout* GetLayout() const = 0;
    virtual void BindBufferSRV(uint32_t inRegister, uint32_t inSpace, const RefCountPtr<RHIBuffer>& inBuffer) = 0;
    virtual void BindBufferUAV(uint32_t inRegister, uint32_t inSpace, const RefCountPtr<RHIBuffer>& inBuffer) = 0;
    virtual void BindBufferCBV(uint32_t inRegister, uint32_t inSpace, const RefCountPtr<RHIBuffer>& inBuffer) = 0;
    virtual void BindTextureSRV(uint32_t inRegister, uint32_t inSpace, const RefCountPtr<RHITexture>& inTexture) = 0;
    virtual void BindTextureUAV(uint32_t inRegister, uint32_t inSpace, const RefCountPtr<RHITexture>& inTexture) = 0;
    virtual void BindSampler(uint32_t inRegister, uint32_t inSpace, const RefCountPtr<RHISampler>& inSampler) = 0;
    virtual void BindBufferSRVArray(uint32_t inBaseRegister, uint32_t inSpace, const std::vector<RefCountPtr<RHIBuffer>>& inBuffer) = 0;
    virtual void BindBufferUAVArray(uint32_t inBaseRegister, uint32_t inSpace, const std::vector<RefCountPtr<RHIBuffer>>& inBuffer) = 0;
    virtual void BindBufferCBVArray(uint32_t inBaseRegister, uint32_t inSpace, const std::vector<RefCountPtr<RHIBuffer>>& inBuffer) = 0;
    virtual void BindTextureSRVArray(uint32_t inBaseRegister, uint32_t inSpace, const std::vector<RefCountPtr<RHITexture>>& inTextures) = 0;
    virtual void BindTextureUAVArray(uint32_t inBaseRegister, uint32_t inSpace, const std::vector<RefCountPtr<RHITexture>>& inTextures) = 0;
    virtual void BindSamplerArray(uint32_t inRegister, uint32_t inSpace, const std::vector<RefCountPtr<RHISampler>>& inSampler) = 0;
    virtual void BindAccelerationStructure(uint32_t inRegister, uint32_t inSpace, const RefCountPtr<RHIAccelerationStructure>& inAccelerationStructure) = 0;
};

