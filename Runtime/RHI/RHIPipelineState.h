#pragma once

#include "RHIDefinitions.h"
#include "../Core/Blob.h"

class RHITexture;

///////////////////////////////////////////////////////////////////////////////////
/// RHIShader
///////////////////////////////////////////////////////////////////////////////////
class RHIShader : public RHIObject
{
public:
    virtual ERHIShaderType GetType() const = 0;
    virtual void SetEntryName(const char* inName) = 0;
    virtual const std::string& GetEntryName() const = 0;
    virtual const uint8_t* GetData() const = 0;
    virtual size_t GetSize() const = 0;
    virtual std::shared_ptr<Blob> GetByteCode() const = 0;
    virtual void SetByteCode(std::shared_ptr<Blob> inByteCode) = 0;
};

///////////////////////////////////////////////////////////////////////////////////
/// RHIPipelineBindingLayout
///////////////////////////////////////////////////////////////////////////////////
struct RHIPipelineBindingItem
{
    ERHIBindingResourceType Type = ERHIBindingResourceType::Texture_SRV;
    uint32_t BaseRegister = 0;
    uint32_t Space = 0;
    uint32_t NumResources = 1;
    // uint16_t PushConstantsSize = 0; 
    bool IsBindless = false;

    RHIPipelineBindingItem(ERHIBindingResourceType Type, uint32_t BaseRegister, uint32_t Space = 0, uint32_t NumResources = 1, bool isBindless = false)
        : Type(Type), BaseRegister(BaseRegister), Space(Space), NumResources(NumResources), IsBindless(isBindless) {}

    bool operator ==(const RHIPipelineBindingItem& b) const {
        return BaseRegister == b.BaseRegister
            && Type == b.Type
            && Space == b.Space
            && NumResources == b.NumResources
            && IsBindless == b.IsBindless;
    }
    bool operator !=(const RHIPipelineBindingItem& b) const { return !(*this == b); }
};

template<>
struct std::hash<RHIPipelineBindingItem>
{
    size_t operator()(const RHIPipelineBindingItem& subResource) const noexcept
    {
        std::hash<uint32_t> hasher32;

        size_t h1 = hasher32((uint32_t)subResource.Type);
        size_t h2 = hasher32(subResource.BaseRegister);
        size_t h3 = hasher32(subResource.Space);
        size_t h4 = hasher32(subResource.NumResources);
        size_t h5 = hasher32(subResource.IsBindless);

        size_t combinedHash = h1 ^ (h2 << 1) ^ (h3 << 3) ^ (h4 << 4) ^ h5 << 5;
        return combinedHash;
    }
};

struct RHIPipelineBindingLayoutDesc
{
    std::vector<RHIPipelineBindingItem> Items;
    bool IsRayTracingLocalLayout = false;   // Using for d3d12
    bool AllowInputLayout = false;          // Using for d3d12
};

class RHIPipelineBindingLayout : public RHIObject
{
public:
    virtual const RHIPipelineBindingLayoutDesc& GetDesc() const = 0;
};

///////////////////////////////////////////////////////////////////////////////////
/// RHIComputePipeline
///////////////////////////////////////////////////////////////////////////////////
struct RHIComputePipelineDesc
{
    RHIShader* ComputeShader = nullptr;
    RHIPipelineBindingLayout* BindingLayout = nullptr;
};

class RHIComputePipeline : public RHIObject
{
public:
    virtual const RHIComputePipelineDesc& GetDesc() const = 0;
};

///////////////////////////////////////////////////////////////////////////////////
/// RHIGraphicsPipeline
///////////////////////////////////////////////////////////////////////////////////
struct RHIVertexInputItem
{
    const char* SemanticName = nullptr;
    uint32_t SemanticIndex = 0;
    ERHIFormat Format = ERHIFormat::Unknown;
    // uint32_t AlignedByteOffset;

    RHIVertexInputItem(const char* SemanticName, ERHIFormat Format, uint32_t SemanticIndex = 0)
        : SemanticName(SemanticName), SemanticIndex(SemanticIndex), Format(Format) {}

    bool operator ==(const RHIVertexInputItem& b) const {
        return SemanticName == b.SemanticName
            && SemanticIndex == b.SemanticIndex
            && Format == b.Format;
        // && AlignedByteOffset == b.AlignedByteOffset;
    }

    bool operator !=(const RHIVertexInputItem& b) const { return !(*this == b); }
};

struct RHIVertexInputLayoutDesc
{
    std::vector<RHIVertexInputItem> Items;
};

struct RHIRasterizerDesc
{
    ERHIRasterFillMode FillMode = ERHIRasterFillMode::Solid;
    ERHIRasterCullMode CullMode = ERHIRasterCullMode::Back;
    bool FrontCounterClockwise = false;
    int32_t DepthBias = 0;
    float DepthBiasClamp = 0.0f;
    float SlopeScaledDepthBias = 0.0f;
    bool DepthClipEnable = true;
    bool MultisampleEnable = false;
    bool AntialiasedLineEnable = false;
    bool ConservativeRasterEnable = false;
    uint32_t ForcedSampleCount = 0;
};

struct RHIDepthStencilDesc
{
    // Depth State
    bool DepthTestEnable = true;
    bool DepthWriteEnable = true;
    ERHIComparisonFunc DepthFunc = ERHIComparisonFunc::Less;

    // Stencil
    bool StencilEnable = false;
    
    uint8_t StencilReadMask = 0xFF;
    uint8_t StencilWriteMask = 0xFF;
    uint8_t StencilRef = 0;
    
    ERHIStencilOp FrontFaceStencilFailOp = ERHIStencilOp::Keep;
    ERHIStencilOp FrontFaceDepthFailOp = ERHIStencilOp::Keep;
    ERHIStencilOp FrontFacePassOp = ERHIStencilOp::Keep;
    ERHIComparisonFunc FrontFaceFunc = ERHIComparisonFunc::Always;
    
    ERHIStencilOp BackFaceStencilFailOp = ERHIStencilOp::Keep;
    ERHIStencilOp BackFaceDepthFailOp = ERHIStencilOp::Keep;
    ERHIStencilOp BackFacePassOp = ERHIStencilOp::Keep;
    ERHIComparisonFunc BackFaceFunc = ERHIComparisonFunc::Always;
};

struct RHIBlendStateDesc
{
    struct RenderTarget
    {
        bool BlendEnable = false;
        
        ERHIBlendFactor ColorSrcBlend= ERHIBlendFactor::One;
        ERHIBlendFactor ColorDstBlend = ERHIBlendFactor::Zero;
        ERHIBlendOp     ColorBlendOp = ERHIBlendOp::Add;
        
        ERHIBlendFactor AlphaSrcBlend = ERHIBlendFactor::One;
        ERHIBlendFactor AlphaDstBlend = ERHIBlendFactor::Zero;
        ERHIBlendOp     AlphaBlendOp = ERHIBlendOp::Add;
        
        ERHIColorWriteMask   ColorWriteMask = ERHIColorWriteMask::All;

        RenderTarget() = default;

        RenderTarget( ERHIBlendFactor InColorSrcBlend, ERHIBlendFactor InColorDstBlend, ERHIBlendOp InColorBlendOp,
                        ERHIBlendFactor InAlphaSrcBlend, ERHIBlendFactor InAlphaDstBlend, ERHIBlendOp InAlphaBlendOp,
                        ERHIColorWriteMask InColorWriteMask)
            : ColorSrcBlend(InColorSrcBlend)
            , ColorDstBlend(InColorDstBlend)
            , ColorBlendOp(InColorBlendOp)
            , AlphaSrcBlend(InAlphaSrcBlend)
            , AlphaDstBlend(InAlphaDstBlend)
            , AlphaBlendOp(InAlphaBlendOp)
            , ColorWriteMask(InColorWriteMask)
        {}

        constexpr bool operator == (const RenderTarget& Other) const
        {
            return BlendEnable == Other.BlendEnable &&
                   ColorSrcBlend == Other.ColorSrcBlend &&
                   ColorDstBlend == Other.ColorDstBlend &&
                   ColorBlendOp == Other.ColorBlendOp &&
                   AlphaSrcBlend == Other.AlphaSrcBlend &&
                   AlphaDstBlend == Other.AlphaDstBlend &&
                   AlphaBlendOp == Other.AlphaBlendOp &&
                   ColorWriteMask == Other.ColorWriteMask;
        }

        constexpr bool operator != (const RenderTarget& Other) const
        {
            return !(*this == Other);
        }
    };
    uint32_t NumRenderTarget = 1;
    RenderTarget Targets[RHIRenderTargetsMaxCount];
    bool AlphaToCoverageEnable = true;
};

struct RHIGraphicsPipelineDesc
{
    bool UsingMeshShader = false;
    RHIShader* VertexShader = nullptr;
    RHIShader* HullShader = nullptr;
    RHIShader* DomainShader = nullptr;
    RHIShader* GeometryShader = nullptr;
    RHIShader* PixelShader = nullptr;
    RHIShader* AmplificationShader = nullptr;
    RHIShader* MeshShader = nullptr;
    RHIPipelineBindingLayout* BindingLayout = nullptr;
    RHIVertexInputLayoutDesc* VertexInputLayout = nullptr;
    ERHIPrimitiveType PrimitiveType = ERHIPrimitiveType::TriangleList;
    RHIRasterizerDesc RasterizerState;
    RHIDepthStencilDesc DepthStencilState;
    RHIBlendStateDesc BlendState;
    ERHIFormat RTVFormats[RHIRenderTargetsMaxCount];
    ERHIFormat DSVFormat;
    uint32_t NumRenderTarget = 1;
    uint32_t SampleCount = 1;
    uint32_t SampleQuality = 0;
};

class RHIGraphicsPipeline : public RHIObject
{
public:
    virtual const RHIGraphicsPipelineDesc& GetDesc() const = 0;
};

///////////////////////////////////////////////////////////////////////////////////
/// RHIFrameBuffer
///////////////////////////////////////////////////////////////////////////////////
struct RHIFrameBufferDesc
{
    // uint32_t NumRenderTargets = 0;
    RHITexture* RenderTargets[RHIRenderTargetsMaxCount];
    RHITexture* DepthStencil;
    RHIGraphicsPipeline* PipelineState;
};

class RHIFrameBuffer : public RHIObject
{
public:
    virtual uint32_t GetFrameBufferWidth() const = 0;
    virtual uint32_t GetFrameBufferHeight() const = 0;
    virtual ERHIFormat GetRenderTargetFormat(uint32_t inIndex) const = 0;
    virtual ERHIFormat GetDepthStencilFormat() const = 0;
    virtual const RHITexture* GetRenderTarget(uint32_t inIndex) const = 0;
    virtual const RHITexture* GetDepthStencil() const = 0;
    virtual const RHIFrameBufferDesc& GetDesc() const = 0;
    virtual uint32_t GetNumRenderTargets() const = 0;
    virtual bool HasDepthStencil() const = 0;
};

///////////////////////////////////////////////////////////////////////////////////
/// RHIRayTracingPipeline
///////////////////////////////////////////////////////////////////////////////////
struct RHIRayGenShaderGroup
{
    RHIShader* RayGenShader = nullptr;
    RHIPipelineBindingLayout* LocalBindingLayout = nullptr; // Not support yet
};

struct RHIMissShaderGroup
{
    RHIShader* MissShader = nullptr;
    RHIPipelineBindingLayout* LocalBindingLayout = nullptr; // Not support yet
};

struct RHIHitGroup
{
    RHIShader* ClosestHitShader = nullptr;
    RHIShader* AnyHitShader = nullptr;
    RHIShader* IntersectionShader = nullptr;
    RHIPipelineBindingLayout* LocalBindingLayout = nullptr; // Not support yet
    bool isProceduralPrimitive = false;
};

struct RHIShaderTableEntry
{
    RHIResourceGpuAddress StartAddress = 0;
    uint64_t SizeInBytes = 0;
    uint64_t StrideInBytes = 0;
};

class RHIShaderTable
{
public:
    virtual ~RHIShaderTable() = default;
    RHIShaderTable() = default;
    RHIShaderTable(const RHIShaderTable&) = delete;
    RHIShaderTable(RHIShaderTable&&) = delete;
    RHIShaderTable& operator=(const RHIShaderTable&) = delete;
    RHIShaderTable& operator=(const RHIShaderTable&&) = delete;
    virtual const RHIShaderTableEntry& GetRayGenShaderEntry() const = 0;
    virtual const RHIShaderTableEntry& GetMissShaderEntry() const = 0;
    virtual const RHIShaderTableEntry& GetHitGroupEntry() const = 0;
    virtual const RHIShaderTableEntry& GetCallableShaderEntry() const = 0;
};

struct RHIRayTracingPipelineDesc
{
    RHIRayGenShaderGroup RayGenShaderGroup;
    std::vector<RHIMissShaderGroup> MissShaderGroups;
    std::vector<RHIHitGroup> HitGroups;
    RHIPipelineBindingLayout* GlobalBindingLayout = nullptr;
    uint32_t MaxPayloadSize = 0;
    uint32_t MaxAttributeSize = sizeof(float) * 2; // typical case: float2 uv;
    uint32_t MaxRecursionDepth = 1;
};

class RHIRayTracingPipeline : public RHIObject
{
public:
    virtual const RHIRayTracingPipelineDesc& GetDesc() const = 0;
    virtual const RHIShaderTable& GetShaderTable() const = 0;
};