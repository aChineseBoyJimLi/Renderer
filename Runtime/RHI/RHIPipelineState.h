#pragma once

#include <vector>
#include <memory>
#include "RHIDefinitions.h"
#include "../Core/Blob.h"

class RHIFrameBuffer;

///////////////////////////////////////////////////////////////////////////////////
/// RHIShader
///////////////////////////////////////////////////////////////////////////////////
class RHIShader : public RHIObject
{
public:
    virtual ERHIShaderType GetType() const = 0;
    virtual void SetEntryName(const char* inName) = 0;
    virtual const char* GetEntryName() const = 0;
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
/// RHIVertexInputLayout
///////////////////////////////////////////////////////////////////////////////////
struct RHIVertexInputItem
{
    const char* SemanticName = nullptr;
    uint32_t SemanticIndex = 0;
    ERHIFormat Format = ERHIFormat::Unknown;
    uint32_t AlignedByteOffset;

    RHIVertexInputItem(const char* SemanticName, uint32_t SemanticIndex, ERHIFormat Format, uint32_t AlignedByteOffset)
        : SemanticName(SemanticName), SemanticIndex(SemanticIndex), Format(Format), AlignedByteOffset(AlignedByteOffset) {}

    bool operator ==(const RHIVertexInputItem& b) const {
        return SemanticName == b.SemanticName
            && SemanticIndex == b.SemanticIndex
            && Format == b.Format
            && AlignedByteOffset == b.AlignedByteOffset;
    }

    bool operator !=(const RHIVertexInputItem& b) const { return !(*this == b); }
};

class RHIVertexInputLayout : public RHIObject
{
public:
    virtual const RHIVertexInputItem& GetVertexInputItem(uint32_t index) const = 0;
    virtual void GetVertexInputItemItems(std::vector<RHIVertexInputItem>& OutItems) const = 0;
};

///////////////////////////////////////////////////////////////////////////////////
/// RHIComputePipeline
///////////////////////////////////////////////////////////////////////////////////
struct RHIComputePipelineDesc
{
    std::shared_ptr<RHIShader> ComputeShader;
    std::shared_ptr<RHIPipelineBindingLayout> BindingLayout;
};

class RHIComputePipeline : public RHIObject
{
public:
    virtual const RHIComputePipelineDesc& GetDesc() const = 0;
};

///////////////////////////////////////////////////////////////////////////////////
/// RHIGraphicsPipeline
///////////////////////////////////////////////////////////////////////////////////
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
    uint32_t RenderTargetCount = 1;
    RenderTarget Targets[RHIRenderTargetsMaxCount];
    bool AlphaToCoverageEnable = true;
};

struct RHIGraphicsPipelineDesc
{
    std::shared_ptr<RHIShader> VertexShader;
    std::shared_ptr<RHIShader> HullShader;
    std::shared_ptr<RHIShader> DomainShader;
    std::shared_ptr<RHIShader> GeometryShader;
    std::shared_ptr<RHIShader> PixelShader;
    std::shared_ptr<RHIPipelineBindingLayout> BindingLayout;
    std::shared_ptr<RHIVertexInputLayout> VertexInputLayout;
    
    ERHIPrimitiveType PrimitiveType = ERHIPrimitiveType::TriangleList;
    RHIRasterizerDesc RasterizerState;
    RHIDepthStencilDesc DepthStencilState;
    RHIBlendStateDesc BlendState;

    uint32_t NumRenderTargets = 0;
    ERHIFormat RTVFormats[RHIRenderTargetsMaxCount];
    ERHIFormat DSVFormat = ERHIFormat::Unknown;

    uint32_t SampleCount = 1;
    uint32_t SampleQuality = 0;
};

class RHIGraphicsPipeline : public RHIObject
{
public:
    virtual const RHIGraphicsPipelineDesc& GetDesc() const = 0;
};

///////////////////////////////////////////////////////////////////////////////////
/// RHIMeshPipeline
///////////////////////////////////////////////////////////////////////////////////
struct RHIMeshPipelineDesc
{
    std::shared_ptr<RHIShader> AmplificationShader;
    std::shared_ptr<RHIShader> MeshShader;
    std::shared_ptr<RHIShader> PixelShader;
    
    std::shared_ptr<RHIPipelineBindingLayout> BindingLayout;
    
    uint32_t NumRenderTargets = 0;
    ERHIFormat RTVFormats[RHIRenderTargetsMaxCount];
    ERHIFormat DSVFormat = ERHIFormat::Unknown;

    ERHIPrimitiveType PrimitiveType = ERHIPrimitiveType::TriangleList;
    RHIRasterizerDesc RasterizerState;
    RHIDepthStencilDesc DepthStencilState;
    RHIBlendStateDesc BlendState;

    uint32_t SampleCount = 1;
    uint32_t SampleQuality = 0;
};

class RHIMeshPipeline : public RHIObject
{
public:
    virtual const RHIMeshPipelineDesc& GetDesc() = 0;
};

///////////////////////////////////////////////////////////////////////////////////
/// RHIRayTracingPipeline
///////////////////////////////////////////////////////////////////////////////////
struct RHIRayGenShaderGroup
{
    std::shared_ptr<RHIShader> RayGenShader;
    std::shared_ptr<RHIPipelineBindingLayout> LocalBindingLayout;
};

struct RHIMissShaderGroup
{
    std::shared_ptr<RHIShader> MissShader;
    std::shared_ptr<RHIPipelineBindingLayout> LocalBindingLayout;
};

struct RHIHitGroup
{
    std::shared_ptr<RHIShader> ClosestHitShader;
    std::shared_ptr<RHIShader> AnyHitShader;
    std::shared_ptr<RHIShader> IntersectionShader;
    std::shared_ptr<RHIPipelineBindingLayout> LocalBindingLayout;
    bool isProceduralPrimitive = false;
};

struct ShaderTableEntry
{
    RHIResourceGpuAddress StartAddress;
    uint64_t SizeInBytes;
    uint64_t StrideInBytes;
};

class RHIShaderTable
{
public:
    virtual ~RHIShaderTable() = default;
    virtual const ShaderTableEntry& GetRayGenShaderEntry() const = 0;
    virtual const ShaderTableEntry& GetMissShaderEntry() const = 0;
    virtual const ShaderTableEntry& GetHitGroupEntry() const = 0;
    virtual const ShaderTableEntry& GetCallableShaderEntry() const = 0;
};

struct RHIRayTracingPipelineDesc
{
    RHIRayGenShaderGroup RayGenShaderGroup;
    std::vector<RHIMissShaderGroup> MissShaderGroups;
    std::vector<RHIHitGroup> HitGroups;
    uint32_t MaxPayloadSize = 0;
    uint32_t MaxAttributeSize = sizeof(float) * 2; // typical case: float2 uv;
    uint32_t MaxRecursionDepth = 1;
};

class RHIRayTracingPipeline : public RHIObject
{
public:
    virtual const RHIRayTracingPipelineDesc& GetDesc() const = 0;
    virtual const RHIShaderTable& GetShaderTable() = 0;
};