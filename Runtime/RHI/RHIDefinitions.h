#pragma once
#include "RHIConstants.h"
#include <string>
#include <vector>
#include <array>
#include <memory>
#include <unordered_map>

#define COMMAND_QUEUES_COUNT static_cast<uint32_t>(ERHICommandQueueType::Count)

typedef uint64_t RHIResourceGpuAddress;
typedef float RHIAffineTransform[12];

struct RHIFormatInfo;
struct RHISwapChainDesc;
class RHISwapChain;
class RHIDevice;

namespace RHI
{
    bool                    Init(bool useVulkan = false);
    void                    Shutdown();
    RHIDevice*              GetDevice();
    const RHIFormatInfo&    GetFormatInfo(ERHIFormat inFormat);
    std::shared_ptr<RHISwapChain> CreateSwapChain(const RHISwapChainDesc& inDesc);
}

class RHIObject
{
public:
    RHIObject() : m_Name("Unnamed") {}
    RHIObject(const RHIObject&) = delete;
    RHIObject& operator=(const RHIObject&)  = delete;
    RHIObject(RHIObject&&)  = delete;
    RHIObject& operator=(const RHIObject&&)  = delete;
    virtual ~RHIObject() = default;

    virtual bool Init() { return true; }
    virtual void Shutdown() {}
    virtual bool IsValid() const { return true; }
    
    void SetName(const std::string& name)
    {
        m_Name = name;
        SetNameInternal();
    }
    
    const std::string& GetName() const { return m_Name; }

protected:
    std::string m_Name;
    virtual void SetNameInternal() {}
};

struct RHIViewport
{
    float X;
    float Y;
    float Width;
    float Height;
    float MinDepth;
    float MaxDepth;

    static RHIViewport Create(float Width, float Height);
    static RHIViewport Create(float X, float Y, float Width, float Height);
    static RHIViewport Create(float X, float Y, float Width, float Height, float MinDepth, float MaxDepth);
};

struct RHIRect
{
    int32_t MinX;
    int32_t MinY;
    uint32_t Width;
    uint32_t Height;

    int32_t Left() const { return MinX; }
    int32_t Right() const { return MinX + Width; }
    int32_t Bottom() const { return MinY; }
    int32_t Top() const { return MinY + Height; }

    static RHIRect Create(uint32_t Width, uint32_t Height);
    static RHIRect Create(int32_t MinX, int32_t MinY, uint32_t Width, uint32_t Height);
};

struct RHIAABB
{
    float MinX;
    float MinY;
    float MinZ;
    float MaxX;
    float MaxY;
    float MaxZ;
};

struct RHIFormatInfo
{
    ERHIFormat Format;
    const char* Name;
    uint8_t BytesPerBlock;
    uint8_t BlockSize;
    ERHIFormatKind Kind;
    bool HasRed : 1;
    bool HasGreen : 1;
    bool HasBlue : 1;
    bool HasAlpha : 1;
    bool HasDepth : 1;
    bool HasStencil : 1;
    bool IsSigned : 1;
    bool IsSRGB : 1;
};

struct RHIClearValue
{
    struct DSValue
    {
        float Depth;
        uint8_t Stencil;
    };

    union
    {
        float Color[4];
        DSValue DepthStencil;
    };

    RHIClearValue()
        : Color { 0.0f, 0.0f, 0.0f, 0.0f }
    {
        
    }

    explicit RHIClearValue(float R, float G, float B, float A = 1.f)
    {
        Color[0] = R;
        Color[1] = G;
        Color[2] = B;
        Color[3] = A;
    }

    explicit RHIClearValue(float depth, uint32_t stencil = 0)
    {
        DepthStencil.Depth = depth;
        DepthStencil.Stencil = stencil;
    }

    static const RHIClearValue Black;
    static const RHIClearValue White;
    static const RHIClearValue Red;
    static const RHIClearValue Green;
    static const RHIClearValue Transparent;
    static const RHIClearValue DepthOne;
    static const RHIClearValue DepthZero;
};