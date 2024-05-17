#include "D3D12CommandList.h"
#include "D3D12Device.h"
#include "D3D12Resources.h"
#include "../../Core/Log.h"
#include "../../Core/Templates.h"

std::shared_ptr<RHICommandList> D3D12Device::CreateCommandList(ERHICommandQueueType inType)
{
    std::shared_ptr<D3D12CommandList> cmdList(new D3D12CommandList(*this, inType));
    if(!cmdList->Init())
    {
        Log::Error("[D3D12] Failed to create a command list");
    }
    return cmdList;
}

D3D12CommandList::D3D12CommandList(D3D12Device& inDevice, ERHICommandQueueType inType)
    : m_Device(inDevice)
    , m_QueueType(inType)
    , m_CmdAllocatorHandle(nullptr)
    , m_CmdListHandle(nullptr)
    , m_IsClosed(false)
{
    
}

D3D12CommandList::~D3D12CommandList()
{
    ShutdownInternal();
}

bool D3D12CommandList::Init()
{
    if(IsValid())
    {
        Log::Warning("[D3D12] Command list is already initialized");
        return true;
    }

    const D3D12_COMMAND_LIST_TYPE d3dQueueType = RHI::D3D12::ConvertCommandQueueType(m_QueueType);

    HRESULT hr = m_Device.GetDevice()->CreateCommandAllocator(d3dQueueType, IID_PPV_ARGS(&m_CmdAllocatorHandle));
    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        return false;
    }
    
    hr = m_Device.GetDevice()->CreateCommandList(D3D12Device::GetNodeMask(), d3dQueueType, m_CmdAllocatorHandle.Get(), nullptr, IID_PPV_ARGS(&m_CmdListHandle));
    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        return false;
    }

    return true;
}

void D3D12CommandList::Shutdown()
{
    ShutdownInternal();
}

bool D3D12CommandList::IsValid() const
{
    bool valid = m_CmdAllocatorHandle != nullptr && m_CmdListHandle != nullptr;
    return valid;
}

void D3D12CommandList::Begin()
{
    if(IsValid() && m_IsClosed)
    {
        m_CmdAllocatorHandle->Reset();
        m_CmdListHandle->Reset(m_CmdAllocatorHandle.Get(), nullptr);
        m_IsClosed = false;
    }
}

void D3D12CommandList::End()
{
    if(IsValid() && !m_IsClosed)
    {
        FlushBarriers();
        m_CmdListHandle->Close();
        m_IsClosed = true;
    }
}

void D3D12CommandList::BeginRenderPass(const std::shared_ptr<RHIFrameBuffer>& inFrameBuffer)
{
    if(IsValid())
    {
        uint32_t numRenderTargets = inFrameBuffer->GetNumRenderTargets();
        std::vector<RHIClearValue> clearColors(numRenderTargets);
        for(uint32_t i = 0; i < numRenderTargets; i++)
        {
            clearColors[i] = inFrameBuffer->GetRenderTarget(i)->GetClearValue();
        }
        BeginRenderPass(inFrameBuffer, clearColors.data(), numRenderTargets);
    }
}

void D3D12CommandList::BeginRenderPass(const std::shared_ptr<RHIFrameBuffer>& inFrameBuffer
    , const RHIClearValue* inColor , uint32_t inNumRenderTargets)
{
    if(IsValid())
    {
        float depth = 1;
        uint8_t stencil = 0;
        if(inFrameBuffer->HasDepthStencil())
        {
            const RHIClearValue& clearValue = inFrameBuffer->GetDepthStencil()->GetClearValue();
            depth = clearValue.DepthStencil.Depth;
            stencil = clearValue.DepthStencil.Stencil;
        }
        BeginRenderPass(inFrameBuffer, inColor, inNumRenderTargets, depth, stencil);
    }
}

void D3D12CommandList::BeginRenderPass(const std::shared_ptr<RHIFrameBuffer>& inFrameBuffer
        , const RHIClearValue* inColor , uint32_t inNumRenderTargets
        , float inDepth, uint8_t inStencil)
{
    if (IsValid())
    {
        FlushBarriers();
        const D3D12FrameBuffer* frameBuffer = CheckCast<D3D12FrameBuffer*>(inFrameBuffer.get());
        if(frameBuffer && frameBuffer->IsValid() && inNumRenderTargets == frameBuffer->GetNumRenderTargets())
        {
            m_CmdListHandle->OMSetRenderTargets(frameBuffer->GetNumRenderTargets()
                , frameBuffer->GetRenderTargetViews()
                , false
                , frameBuffer->GetDepthStencilViews());
            for(uint32_t i = 0; i < inNumRenderTargets; ++i)
            {
                m_CmdListHandle->ClearRenderTargetView(frameBuffer->GetRenderTargetView(i), inColor[i].Color, 0, nullptr);
            }
            if(frameBuffer->HasDepthStencil())
            {
                D3D12_CLEAR_FLAGS flags = D3D12_CLEAR_FLAG_DEPTH;
                const RHIFormatInfo& formatInfo = RHI::GetFormatInfo(frameBuffer->GetDepthStencilFormat());
                if(formatInfo.HasStencil) flags |= D3D12_CLEAR_FLAG_STENCIL;
                m_CmdListHandle->ClearDepthStencilView(frameBuffer->GetDepthStencilView(), flags, inDepth, inStencil, 0, nullptr);
            }
        }
    }
}

void D3D12CommandList::EndRenderPass()
{
    FlushBarriers();
}

void D3D12CommandList::ResourceBarrier(std::shared_ptr<RHITexture>& inResource , ERHIResourceStates inAfterState)
{
    if(IsValid())
    {
        D3D12Texture* texture = CheckCast<D3D12Texture*>(inResource.get());
        if(texture && texture->IsValid())
        {
            D3D12_RESOURCE_STATES beforeState = texture->GetCurrentState();
            D3D12_RESOURCE_STATES afterState = RHI::D3D12::ConvertResourceStates(inAfterState);
            m_CachedBarriers.push_back(CD3DX12_RESOURCE_BARRIER::Transition(texture->GetTexture(), beforeState, afterState));
            texture->ChangeState(afterState);
        }
    }
}

void D3D12CommandList::FlushBarriers()
{
    if(IsValid() && !m_CachedBarriers.empty())
    {
        m_CmdListHandle->ResourceBarrier((uint32_t)m_CachedBarriers.size(), m_CachedBarriers.data());
        m_CachedBarriers.clear();
    }
}

void D3D12CommandList::ShutdownInternal()
{
    m_CmdAllocatorHandle.Reset();
    m_CmdListHandle.Reset();
}

void D3D12CommandList::SetNameInternal()
{
    if(IsValid())
    {
        std::wstring wname(m_Name.begin(), m_Name.end());
        m_CmdListHandle->SetName(wname.c_str());
    }
}
