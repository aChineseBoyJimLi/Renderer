#pragma once

#include "RHIDefinitions.h"

///////////////////////////////////////////////////////////////////////////////////
/// RHIResourceHeap
///////////////////////////////////////////////////////////////////////////////////
struct RHIResourceHeapDesc
{
    ERHIResourceHeapType Type = ERHIResourceHeapType::DeviceLocal;
    ERHIHeapUsage Usage = ERHIHeapUsage::Unknown;
    uint64_t Size = 0;
    uint64_t Alignment = 0;
    uint32_t TypeFilter = ~0u; // using for Vulkan
};

class RHIResourceHeap : public RHIObject
{
public:
    virtual const RHIResourceHeapDesc& GetDesc() const = 0;
};

struct RHIResourceAllocation
{
    std::shared_ptr<RHIResourceHeap> Heap;
    size_t Size;
    size_t Offset;
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
    ERHIResourceState InitialState;
    ERHIBufferUsage Usages;
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
    virtual bool BindMemory(std::shared_ptr<RHIResourceHeap> inHeap, uint64_t inOffset = 0) = 0;
    virtual void* Map(uint64_t inSize, uint64_t inOffset = 0) = 0;
    virtual void  Unmap() = 0;
    virtual void  WriteData(const void* inData, uint64_t inSize, uint64_t inOffset = 0) = 0;
    virtual void  ReadData(void* outData, uint64_t inSize, uint64_t inOffset = 0) = 0;
    virtual const RHIBufferDesc& GetDesc() const = 0;
    virtual RHIResourceGpuAddress GetGpuAddress() const = 0;
    virtual void ChangeState(const RHICommandList& inCmdList, ERHIResourceState inState, const RHIBufferSubRange& subRange = RHIBufferSubRange::All) = 0;
    virtual ERHIResourceState GetState(const RHIBufferSubRange& subRange = RHIBufferSubRange::All) const = 0;
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
    ERHIResourceState InitialState;
    ERHITextureUsage Usages;
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
    virtual bool BindMemory(std::shared_ptr<RHIResourceHeap> inHeap, uint64_t inOffset = 0) = 0;
    virtual const RHITextureDesc& GetDesc() const = 0;
    virtual RHIResourceGpuAddress GetGpuAddress() const = 0;
    virtual void ChangeState(const RHICommandList& inCmdList, ERHIResourceState inState, const RHITextureSubResource& subResource = RHITextureSubResource::All) = 0;
    virtual ERHIResourceState GetState(const RHITextureSubResource& subResource = RHITextureSubResource::All) const = 0;
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
    std::shared_ptr<RHIBuffer> VertexBuffer;
    std::shared_ptr<RHIBuffer> IndexBuffer;
    uint32_t VertexOffsetCount;
    uint32_t IndexOffsetCount;
    uint32_t VertexCount;
    uint32_t IndexCount;
};

struct RHIRayTracingGeometryAABBDesc
{
    std::shared_ptr<RHIBuffer> Buffer;
    uint32_t OffsetCount;
    uint32_t Count;
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

struct RHIRayTracingDesc
{
    std::vector<RHIRayTracingGeometryTriangleDesc> Triangles;
    std::vector<RHIRayTracingGeometryAABBDesc> AABBs;
    std::vector<RHIRayTracingInstanceDesc> Instances;
    
    ERHIRayTracingGeometryFlags Flags = ERHIRayTracingGeometryFlags::None;
    ERHIRayTracingGeometryType Type = ERHIRayTracingGeometryType::Triangles;
};

class RHIAccelerationStructure : public RHIObject
{
public:
    virtual const RHIRayTracingDesc& GetDesc() const = 0;
    virtual void Build(const RHICommandList& inCmdList) = 0;
};

///////////////////////////////////////////////////////////////////////////////////
/// RHIResourcesSet
///////////////////////////////////////////////////////////////////////////////////

class RHIResourceSet : public RHIObject
{
public:
    virtual std::shared_ptr<RHIPipelineBindingLayout> GetLayout() const = 0;
    
    virtual void BindBuffer(ERHIBindingResourceType Type, uint32_t BaseRegister, uint32_t Space, const std::shared_ptr<RHIBuffer>& inBuffer) = 0;
    virtual void BindBuffers(ERHIBindingResourceType Type, uint32_t BaseRegister, uint32_t Space, const std::vector<std::shared_ptr<RHIBuffer>>& inBuffers) = 0;
    virtual void BindBuffer(uint32_t inIndex, const std::shared_ptr<RHIBuffer>& inBuffer) = 0;
    virtual void BindBuffers(uint32_t inIndex, const std::vector<std::shared_ptr<RHIBuffer>>& inBuffers) = 0;
    
    virtual void BindTexture(uint32_t inIndex, const std::shared_ptr<RHITexture>& inTexture) = 0;
    virtual void BindSampler(uint32_t inIndex, const std::shared_ptr<RHISampler>& inSampler) = 0;
    virtual void BindAccelerationStructure(uint32_t inIndex, const std::shared_ptr<RHIAccelerationStructure>& inAccelerationStructure) = 0;
};

///////////////////////////////////////////////////////////////////////////////////
/// RHIFrameBuffer
///////////////////////////////////////////////////////////////////////////////////
struct RHIFrameBufferDesc
{
    uint32_t Width = 0;
    uint32_t Height = 0;
    uint32_t NumRenderTargets = 0;
    std::shared_ptr<RHITexture> RenderTargets[RHIRenderTargetsMaxCount];
    std::shared_ptr<RHITexture> DepthStencil;
};

class RHIFrameBuffer : public RHIObject
{
public:
    virtual const RHIFrameBufferDesc& GetDesc() = 0;
};