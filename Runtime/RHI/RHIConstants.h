#pragma once

#include <cstdint>

#define ENUM_CLASS_FLAG_OPERATORS(T) \
    inline T operator | (T a, T b) { return T(uint32_t(a) | uint32_t(b)); } \
    inline T operator & (T a, T b) { return T(uint32_t(a) & uint32_t(b)); } /* NOLINT(bugprone-macro-parentheses) */ \
    inline T operator ~ (T a) { return T(~uint32_t(a)); } /* NOLINT(bugprone-macro-parentheses) */ \
    inline bool operator !(T a) { return uint32_t(a) == 0; } \
    inline bool operator ==(T a, uint32_t b) { return uint32_t(a) == b; } \
    inline bool operator !=(T a, uint32_t b) { return uint32_t(a) != b; }

constexpr uint32_t RHIRenderTargetsMaxCount = 8;

enum class ERHIBackend  : uint8_t
{
    D3D12,
    Vulkan
};

enum class ERHICommandQueueType : uint8_t
{
    Direct = 0,
    Copy,
    Async,
    Count
};

enum class ERHIFormat : uint8_t
{
    Unknown = 0,

    R8_UINT,
    R8_SINT,
    R8_UNORM,
    R8_SNORM,
    RG8_UINT,
    RG8_SINT,
    RG8_UNORM,
    RG8_SNORM,
    R16_UINT,
    R16_SINT,
    R16_UNORM,
    R16_SNORM,
    R16_FLOAT,
    BGRA4_UNORM,
    B5G6R5_UNORM,
    B5G5R5A1_UNORM,
    RGBA8_UINT,
    RGBA8_SINT,
    RGBA8_UNORM,
    RGBA8_SNORM,
    BGRA8_UNORM,
    SRGBA8_UNORM,
    SBGRA8_UNORM,
    R10G10B10A2_UNORM,
    R11G11B10_FLOAT,
    RG16_UINT,
    RG16_SINT,
    RG16_UNORM,
    RG16_SNORM,
    RG16_FLOAT,
    R32_UINT,
    R32_SINT,
    R32_FLOAT,
    RGBA16_UINT,
    RGBA16_SINT,
    RGBA16_FLOAT,
    RGBA16_UNORM,
    RGBA16_SNORM,
    RG32_UINT,
    RG32_SINT,
    RG32_FLOAT,
    RGB32_UINT,
    RGB32_SINT,
    RGB32_FLOAT,
    RGBA32_UINT,
    RGBA32_SINT,
    RGBA32_FLOAT,

    D16,
    D24S8,
    X24G8_UINT,
    D32,
    D32S8,
    X32G8_UINT,

    BC1_UNORM,
    BC1_UNORM_SRGB,
    BC2_UNORM,
    BC2_UNORM_SRGB,
    BC3_UNORM,
    BC3_UNORM_SRGB,
    BC4_UNORM,
    BC4_SNORM,
    BC5_UNORM,
    BC5_SNORM,
    BC6H_UFLOAT,
    BC6H_SFLOAT,
    BC7_UNORM,
    BC7_UNORM_SRGB,

    Count,
};

enum class ERHIFormatKind : uint8_t
{
    Integer,
    Normalized,
    Float,
    DepthStencil
};

enum class ERHITextureDimension : uint8_t
{
    // Texture1D and Texture1D Array are not supported
    Texture2D,
    Texture3D,
    TextureCube,
    Texture2DArray,
    // TextureCubeArray is not supported
};

enum class ERHIBindingResourceType : uint8_t
{
    Texture_SRV,
    Texture_UAV,
    Buffer_SRV,
    Buffer_UAV,
    Buffer_CBV,
    Sampler,
    AccelerationStructure,
    // PushConstants
};

enum class ERHIRegisterType : uint8_t
{
    ShaderResource = 0, // t
    UnorderedAccess,    // u
    ConstantBuffer,     // b
    Sampler,            // s
    Count
};

enum class ERHIResourceViewType : uint8_t
{
    RTV,
    DSV,
    SRV,
    UAV,
    CBV,
    Sampler,
};

enum class ERHIShaderType : uint8_t
{
    Compute,
    
    Vertex,
    Hull,
    Domain,
    Geometry,
    Pixel,
    
    RayGen,
    RayMiss,
    RayClosestHit,
    RayAnyHit,
    RayIntersection,
    
    Amplification,
    Mesh
};

enum class ERHIRasterFillMode : uint8_t
{
    Wireframe,
    Solid
};

enum class ERHIRasterCullMode : uint8_t
{
    None,
    Front,
    Back
};

enum class ERHIDepthWriteMask : uint8_t
{
    Zero,
    All
};

enum class ERHIStencilOp : uint8_t
{
    Keep,
    Zero,
    Replace,
    IncrementAndClamp,
    DecrementAndClamp,
    Invert,
    Increment,
    Decrement
};

enum class ERHIComparisonFunc : uint8_t
{
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always
};

enum class ERHIBlendFactor : uint8_t
{
    Zero,
    One,
    SrcColor,
    SrcAlpha,
    OneMinusSrcColor,
    OneMinusSrcAlpha,
    DstAlpha,
    DstColor,
    OneMinusDstAlpha,
    OneMinusDstColor,
    SrcAlphaSaturate,
    ConstantColor,
    OneMinusConstantColor,
    Src1Color,
    OneMinusSrc1Color,
    Src1Alpha,
    OneMinusSrc1Alpha
};

enum class ERHIBlendOp : uint8_t
{
    Add,
    Subtract,
    RevSubtract,
    Min,
    Max
};

enum class ERHIColorWriteMask : uint8_t
{
    // These values are equal to their counterparts in DX11, DX12, and Vulkan.
    Red = 1,
    Green = 2,
    Blue = 4,
    Alpha = 8,
    All = 0xF
};

enum class ERHIPrimitiveType : uint8_t
{
    PointList,
    LineList,
    TriangleList,
    TriangleStrip,
    TriangleFan,
    TriangleListWithAdjacency,
    TriangleStripWithAdjacency,
    PatchList
};

enum class ERHIResourceHeapType : uint8_t
{
    DeviceLocal,
    Upload,
    Readback
};

enum class ERHIHeapUsage : uint8_t
{
    Buffer,
    Texture
};

enum class ERHICpuAccessMode : uint8_t
{
    None,
    Read,
    Write
};

enum class ERHIResourceStates : uint32_t
{
    None = 0,
    Present = 1,
    // ConstantBuffer = 1,
    // VertexBuffer = ConstantBuffer << 1,
    // IndexBuffer = VertexBuffer << 1,
    GpuReadOnly = Present << 1,
    IndirectCommands = GpuReadOnly << 1,
    UnorderedAccess = IndirectCommands << 1,
    RenderTarget = UnorderedAccess << 1,
    ShadingRateSource = RenderTarget << 1,
    CopySrc = ShadingRateSource << 1,
    CopyDst = CopySrc << 1,
    DepthStencilWrite = CopyDst << 1,
    DepthStencilRead = DepthStencilWrite << 1,
    AccelerationStructure = DepthStencilRead << 1,
    OcclusionPrediction = AccelerationStructure << 1
};
ENUM_CLASS_FLAG_OPERATORS(ERHIResourceStates)

enum class ERHIBufferUsage : uint32_t
{
    None = 0,
    VertexBuffer = 1,
    IndexBuffer = VertexBuffer << 1,
    ConstantBuffer = IndexBuffer << 1,
    UnorderedAccess = ConstantBuffer << 1,
    ShaderResource = UnorderedAccess << 1,
    IndirectCommands = ShaderResource << 1,
    ShaderTable = IndirectCommands << 1,
    AccelerationStructureStorage = ShaderTable << 1,
    AccelerationStructureBuildInput = AccelerationStructureStorage << 1,
};
ENUM_CLASS_FLAG_OPERATORS(ERHIBufferUsage)

enum class ERHITextureUsage : uint32_t
{
    None           = 0,
    ShaderResource = 1,
    RenderTarget   = ShaderResource << 1,
    DepthStencil   = RenderTarget << 1,
    UnorderedAccess = DepthStencil << 1,
    ShadingRateSource = UnorderedAccess
};
ENUM_CLASS_FLAG_OPERATORS(ERHITextureUsage)

enum class ERHISamplerAddressMode : uint8_t
{
    Clamp,
    Wrap,
    Border,
    Mirror,
    MirrorOnce,
};

enum class ERHISamplerReductionType : uint8_t
{
    Standard,
    Comparison,
    Minimum,
    Maximum
};

enum class ERHIRayTracingGeometryFlags : uint8_t
{
    None = 0,
    Opaque = 1,
    NoDuplicateAnyHitInvocation = 2
};

ENUM_CLASS_FLAG_OPERATORS(ERHIRayTracingGeometryFlags)

enum class ERHIRayTracingGeometryType : uint8_t
{
	Triangles,
	AABBs
};

enum class ERHIRayTracingInstanceFlags : uint8_t
{
    None = 0,
    TriangleCullDisable = 1,
    TriangleFrontCounterclockwise = 2,
    ForceOpaque = 4,
    ForceNonOpaque = 8,
};

ENUM_CLASS_FLAG_OPERATORS(ERHIRayTracingInstanceFlags)