#include "D3D12SwapChain.h"
#include "D3D12Device.h"
#include "../../Core/Log.h"

D3D12SwapChain::D3D12SwapChain(D3D12Device& inDevice, const RHISwapChainDesc& inDesc)
    : m_Device(inDevice)
    , m_Desc(inDesc)
    , m_SwapChainFormat(DXGI_FORMAT_R8G8B8A8_UNORM)
    , m_SwapChainHandle(nullptr)
    , m_CurrentBackBufferIndex(0)
{
    
}

D3D12SwapChain::~D3D12SwapChain()
{
    ShutdownInternal();
}

bool D3D12SwapChain::Init()
{
    if(IsValid())
    {
        Log::Warning("[D3D12] SwapChain already initialized.");
        return true;
    }
    
    m_BackBuffers.clear();
    m_SwapChainFormat = RHI::D3D12::ConvertFormat(m_Desc.Format);

    DXGI_SWAP_CHAIN_DESC1 swapChainDes{};
    swapChainDes.Width = m_Desc.Width;
    swapChainDes.Height = m_Desc.Height;
    swapChainDes.Format = m_SwapChainFormat;
    swapChainDes.SampleDesc.Count = m_Desc.SampleCount;
    swapChainDes.SampleDesc.Quality = 0;
    swapChainDes.BufferUsage = DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDes.BufferCount = m_Desc.BufferCount;
    swapChainDes.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    swapChainDes.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    if(!m_Desc.IsVSync)
    {
        swapChainDes.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    }
    swapChainDes.Stereo = false;
    
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullScreenDesc{};
    fullScreenDesc.RefreshRate.Numerator = 0;
    fullScreenDesc.RefreshRate.Denominator = 1;
    fullScreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
    fullScreenDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    fullScreenDesc.Windowed = !m_Desc.IsFullScreen;

#if WIN32 || _WIN32
    if(!(m_Desc.hwnd && m_Desc.hInstance))
    {
        Log::Error("HWND or HINSTANCE is nullptr");
        return false;
    }
    
    ID3D12CommandQueue* Queue = m_Device.GetCommandQueue(ERHICommandQueueType::Direct);
    HRESULT hr = m_Device.GetFactory()->CreateSwapChainForHwnd(Queue, m_Desc.hwnd, &swapChainDes, &fullScreenDesc, nullptr, &m_SwapChainHandle);
    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        return false;
    }
#endif

   
    InitBackBuffers();
    
    return true;
}

void D3D12SwapChain::InitBackBuffers()
{
    m_BackBuffers.resize(GetBackBufferCount());
    for(uint32_t i = 0; i < GetBackBufferCount(); i++)
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> renderTexture;

        if(m_SwapChainHandle != nullptr)
        {
            HRESULT hr = m_SwapChainHandle->GetBuffer(i, IID_PPV_ARGS(&renderTexture));
            if(FAILED(hr))
            {
                OUTPUT_D3D12_FAILED_RESULT(hr)
            }

            RHITextureDesc backBufferDesc{};
            backBufferDesc.Width = m_Desc.Width;
            backBufferDesc.Height = m_Desc.Height;
            backBufferDesc.Format = m_Desc.Format;
            backBufferDesc.Usages = ERHITextureUsage::ShaderResource | ERHITextureUsage::RenderTarget;
            backBufferDesc.SampleCount = m_Desc.SampleCount;
            m_BackBuffers[i] = m_Device.CreateTexture(backBufferDesc, renderTexture);
        }
    }
}

void D3D12SwapChain::Shutdown()
{
    ShutdownInternal();
}

void D3D12SwapChain::ShutdownInternal()
{
    m_BackBuffers.clear();
    m_SwapChainHandle.Reset();
}

bool D3D12SwapChain::IsValid() const
{
    return m_SwapChainHandle != nullptr;
}

void D3D12SwapChain::Present()
{
    if(IsValid())
    {
        m_SwapChainHandle->Present(0, 0);
        m_CurrentBackBufferIndex = (m_CurrentBackBufferIndex + 1) % GetBackBufferCount();
    }
    else
    {
        Log::Error("[D3D12] Failed to present back buffer to screen, swapChain is not valid");
    }
}

void D3D12SwapChain::Resize(uint32_t inWidth, uint32_t inHeight)
{
    m_Device.FlushDirectCommandQueue();
    m_Desc.Width = inWidth;
    m_Desc.Height = inHeight;
    if(IsValid())
    {
        m_BackBuffers.clear();
        
        DXGI_SWAP_CHAIN_DESC swapChainDesc;
        m_SwapChainHandle->GetDesc(&swapChainDesc);
        HRESULT hr = m_SwapChainHandle->ResizeBuffers(m_Desc.BufferCount, inWidth, inHeight, m_SwapChainFormat, swapChainDesc.Flags);
        if(FAILED(hr))
        {
            OUTPUT_D3D12_FAILED_RESULT(hr)
            return;
        }
        InitBackBuffers();
    }
}

RHITexture* D3D12SwapChain::GetBackBuffer(uint32_t index)
{
    if(index < m_BackBuffers.size())
    {
        return m_BackBuffers[index].GetReference();
    }
    return nullptr;
}

RHITexture* D3D12SwapChain::GetCurrentBackBuffer()
{
    return GetBackBuffer(m_CurrentBackBufferIndex);
}
