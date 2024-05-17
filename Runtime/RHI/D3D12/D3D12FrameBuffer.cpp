#include "D3D12Resources.h"
#include "D3D12Device.h"
#include "../../Core/Log.h"
#include "../../Core/Templates.h"

std::shared_ptr<RHIFrameBuffer> D3D12Device::CreateFrameBuffer(const RHIFrameBufferDesc& inDesc)
{
    std::shared_ptr<RHIFrameBuffer> frameBuffer(new D3D12FrameBuffer(*this, inDesc));
    if(!frameBuffer->Init())
    {
        Log::Error("[D3D12] Failed to create frame buffer");
    }
    return frameBuffer;
}

D3D12FrameBuffer::D3D12FrameBuffer(D3D12Device& inDevice, const RHIFrameBufferDesc& inDesc)
    : m_Device(inDevice)
    , m_Desc(inDesc)
{
    ShutdownInternal();
}

D3D12FrameBuffer::~D3D12FrameBuffer()
{
    ShutdownInternal();
}

bool D3D12FrameBuffer::Init()
{
    if(IsValid())
    {
        Log::Warning("[D3D12] FrameBuffer already initialized.");
        return true;
    }

    for(uint32_t i = 0; i < m_Desc.NumRenderTargets; ++i)
    {
        m_RenderTargets[i] = CheckCast<D3D12Texture*>(GetRenderTarget(i).get());
        
        if(!m_RenderTargets[i] || !m_RenderTargets[i]->IsValid())
        {
            Log::Error("[D3D12] Render target %u is invalid", i);
            return false;
        }

        m_FrameBufferWidth = m_RenderTargets[i]->GetDesc().Width;
        m_FrameBufferHeight = m_RenderTargets[i]->GetDesc().Height;
        if(!m_RenderTargets[i]->TryGetRTVHandle(m_RTVHandles[i]))
        {
            if(!m_RenderTargets[i]->CreateRTV(m_RTVHandles[i]))
            {
                Log::Error("[D3D12] Failed to create RTV for render target %u", i);
                return false;       
            }
        }
    }

    m_DepthStencil = CheckCast<D3D12Texture*>(GetDepthStencil().get());
    if(m_DepthStencil && m_DepthStencil->IsValid())
    {
        if(!m_DepthStencil->TryGetDSVHandle(m_DSVHandle))
        {
            if(!m_DepthStencil->CreateDSV(m_DSVHandle))
            {
                Log::Error("[D3D12] Failed to create DSV for depth stencil");
                return false;
            }
        }
        m_FrameBufferWidth = m_DepthStencil->GetDesc().Width;
        m_FrameBufferHeight = m_DepthStencil->GetDesc().Height;
    }
    
    return true;
}

void D3D12FrameBuffer::Shutdown()
{
    ShutdownInternal();
}

bool D3D12FrameBuffer::IsValid() const
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle;
    for(uint32_t i = 0; i < m_Desc.NumRenderTargets; ++i)
    {
        if(!m_RenderTargets[i] || !m_RenderTargets[i]->IsValid() || !m_RenderTargets[i]->TryGetRTVHandle(handle))
            return false;
    }

    if(HasDepthStencil())
    {
        if(!m_DepthStencil || !m_DepthStencil->IsValid() || !m_DepthStencil->TryGetDSVHandle(handle))
            return false;
    }
    return true;
}

bool D3D12FrameBuffer::Resize(std::shared_ptr<RHITexture>* inRenderTargets, std::shared_ptr<RHITexture> inDepthStencil)
{
    ShutdownInternal();
    for(uint32_t i = 0; i < GetNumRenderTargets(); i++)
    {
        if(!inRenderTargets[i] || !inRenderTargets[i]->IsValid())
        {
            Log::Error("[D3D12] Failed to resize framebuffer, new render targets %d is invalid", i);
            return false;
        }
        m_Desc.RenderTargets[i] = inRenderTargets[i];
    }
    if(HasDepthStencil())
    {
        if(!inDepthStencil || !inDepthStencil->IsValid())
        {
            Log::Error("[D3D12] Failed to resize framebuffer, new depth stencil texture is invalid");
            return false;
        }
        m_Desc.DepthStencil = inDepthStencil;
    }
    return Init();
}

void D3D12FrameBuffer::ShutdownInternal()
{
    for(uint32_t i = 0; i < RHIRenderTargetsMaxCount; ++i)
    {
        m_RenderTargets[i] = nullptr;
    }
    m_DepthStencil = nullptr;
}

ERHIFormat D3D12FrameBuffer::GetRenderTargetFormat(uint32_t inIndex) const
{
    if(inIndex < m_Desc.NumRenderTargets && m_Desc.RenderTargets[inIndex] != nullptr)
    {
        return m_Desc.RenderTargets[inIndex]->GetDesc().Format;
    }
    else
    {
        return ERHIFormat::Unknown;
    }
}
ERHIFormat D3D12FrameBuffer::GetDepthStencilFormat() const
{
    if(HasDepthStencil())
    {
        return m_Desc.DepthStencil->GetDesc().Format;
    }
    else
    {
        return ERHIFormat::Unknown;
    }
}
    
    