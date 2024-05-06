#include "D3D12CommandList.h"
#include "D3D12Device.h"
#include "../../Core/Log.h"

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
        m_CmdListHandle->Close();
        m_IsClosed = true;
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
