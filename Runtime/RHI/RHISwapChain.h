#pragma once
#include "RHIDefinitions.h"
#if WIN32 || _WIN32
#include <Windows.h>
#endif

class RHITexture;

struct RHISwapChainDesc
{
    uint32_t Width = 0;
    uint32_t Height = 0;
    ERHIFormat Format = ERHIFormat::RGBA8_UNORM;
    uint32_t BufferCount = 2;
    bool IsFullScreen = false;
    bool IsVSync = false;
    uint32_t SampleCount = 1;
    
#if WIN32 || _WIN32
    HWND hwnd;
    HINSTANCE hInstance;
#endif
};

class RHISwapChain : public RHIObject
{
public:
    virtual const RHISwapChainDesc& GetDesc() const = 0;
    virtual void Present() = 0;
    virtual void Resize(uint32_t inWidth, uint32_t inHeight) = 0;
    virtual RHITexture* GetBackBuffer(uint32_t index) = 0;
    virtual RHITexture* GetCurrentBackBuffer() = 0;
    virtual uint32_t GetBackBufferCount() = 0;
    virtual uint32_t GetCurrentBackBufferIndex() = 0;
};