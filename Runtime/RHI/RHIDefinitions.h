#pragma once
#include "RHIConstants.h"
#include <string>

#define COMMAND_QUEUES_COUNT static_cast<uint32_t>(ERHICommandQueueType::Count)

typedef uint64_t RHIResourceGpuAddress;
typedef float RHIAffineTransform[12];

struct RHISwapChainDesc;
class RHISwapChain;
class RHIDevice;

namespace RHI
{
    bool        Init(bool useVulkan = false);
    void        Shutdown();
    RHIDevice*  GetDevice();
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