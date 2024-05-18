#include "D3D12Device.h"

#include "D3D12CommandList.h"
#include "D3D12DescriptorManager.h"
#include "../RHICommandList.h"
#include "../../Core/Log.h"
#include "../../Core/Templates.h"

void D3D12Device::LogAdapterDesc(const DXGI_ADAPTER_DESC1& inDesc)
{
    std::wstring adapterDesc(inDesc.Description);
    std::string name(adapterDesc.begin(), adapterDesc.end());
    Log::Info("----------------------------------------------------------------", name.c_str());
    Log::Info("[D3D12] Adapter Description: %s", name.c_str());
    Log::Info("[D3D12] Adapter Dedicated Video Memory: %d MB", inDesc.DedicatedVideoMemory >> 20);
    Log::Info("[D3D12] Adapter Dedicated System Memory: %d MB", inDesc.DedicatedSystemMemory >> 20);
    Log::Info("[D3D12] Adapter Shared System Memory: %d MB", inDesc.SharedSystemMemory >> 20);
    Log::Info("----------------------------------------------------------------", name.c_str());
}

D3D12Device::D3D12Device()
    : m_FactoryHandle(nullptr)
    , m_AdapterHandle(nullptr)
    , m_DeviceHandle(nullptr)
    , m_QueueHandles {nullptr, nullptr, nullptr}
    , m_DescriptorManager(nullptr)
{
    
}

D3D12Device::~D3D12Device()
{
    ShutdownInternal();
}

bool D3D12Device::Init()
{
    if(IsValid())
    {
        Log::Warning("[D3D12] Device already initialized");
        return true;    
    }
    
    bool allowDebugging = false;
    
#if _DEBUG || DEBUG
    allowDebugging = true;
    // Enable the D3D12 debug layer
    Microsoft::WRL::ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        debugController->EnableDebugLayer();
    else
        Log::Warning("[D3D12] Failed to enable the D3D12 debug layer");
#endif
    
    const uint32_t factoryFlags = allowDebugging ? DXGI_CREATE_FACTORY_DEBUG : 0;
    HRESULT hr = CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&m_FactoryHandle));
    if(FAILED(hr))
    {
        OUTPUT_D3D12_FAILED_RESULT(hr)
        return false;
    }

    // Enumerate the adapters and create the device
    int adapterIndex = -1;
    size_t memorySizeMax = 0;
    Microsoft::WRL::ComPtr<IDXGIAdapter1> tempAdapter;
    for (int i = 0; m_FactoryHandle->EnumAdapters1(i, &tempAdapter) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        DXGI_ADAPTER_DESC1 desc;
        if(SUCCEEDED(tempAdapter->GetDesc1(&desc)))
        {
            LogAdapterDesc(desc);
            // Skip software and remote adapters
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
            if (desc.Flags & DXGI_ADAPTER_FLAG_REMOTE) continue;
            
            if(desc.DedicatedVideoMemory > memorySizeMax)
            {
                memorySizeMax = desc.DedicatedVideoMemory;
                adapterIndex = i;
            }
        }
    }

    if(adapterIndex >= 0)
    {
        m_FactoryHandle->EnumAdapters1(adapterIndex, &tempAdapter);
        Microsoft::WRL::ComPtr<ID3D12Device5> tempDevice;
        if (SUCCEEDED(D3D12CreateDevice(tempAdapter.Get(), s_FeatureLevel, IID_PPV_ARGS(&tempDevice))))
        {
            tempDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &m_Feature5Data, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));
            if(m_Feature5Data.RaytracingTier > D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
            {
                Log::Info("[D3D12] GPU supports ray tracing");
            }
            tempDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &m_Feature6Data, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS6));
            if(m_Feature6Data.VariableShadingRateTier > D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED)
            {
                Log::Info("[D3D12] GPU supports variable shading rate");
            }
            tempDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &m_Feature7Data, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS7));
            if(m_Feature7Data.MeshShaderTier > D3D12_MESH_SHADER_TIER_NOT_SUPPORTED)
            {
                Log::Info("[D3D12] GPU supports mesh shading");
            }
            m_AdapterHandle = tempAdapter;
            m_DeviceHandle = tempDevice;
            Log::Info("[D3D12] Using adapter: %d", adapterIndex);
        }
    }

    if(m_DeviceHandle.Get() == nullptr)
    {
        Log::Error("[D3D12] Failed to create the D3D12 device");
        return false;
    }

    for(uint32_t i = 0; i < COMMAND_QUEUES_COUNT; ++i)
    {
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> tempQueue;
        D3D12_COMMAND_QUEUE_DESC Desc = {};
        Desc.Type = RHI::D3D12::ConvertCommandQueueType(static_cast<ERHICommandQueueType>(i));
        Desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        Desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        Desc.NodeMask = GetNodeMask();
        OUTPUT_D3D12_FAILED_RESULT(m_DeviceHandle->CreateCommandQueue(&Desc, IID_PPV_ARGS(&tempQueue)));
        m_QueueHandles[i] = tempQueue;
    }

    m_DescriptorManager = std::make_unique<D3D12DescriptorManager>(*this);
    if(!m_DescriptorManager->Init())
    {
        Log::Error("[D3D12] Failed to create the descriptor manager");
        return false;
    }
    
    return true;
}

void D3D12Device::Shutdown()
{
    ShutdownInternal();
}

void D3D12Device::ShutdownInternal()
{
    for(auto i : m_QueueHandles)
    {
        i.Reset();
    }
    m_DeviceHandle.Reset();
    m_AdapterHandle.Reset();
    m_FactoryHandle.Reset();
}

bool D3D12Device::IsValid() const
{
    bool valid = m_FactoryHandle != nullptr && m_AdapterHandle != nullptr && m_DeviceHandle != nullptr && m_DescriptorManager != nullptr;
    for(auto i : m_QueueHandles)
    {
        valid = valid && i != nullptr;
    }
    return valid;
}

void D3D12Device::ExecuteCommandList(const RefCountPtr<RHICommandList>& inCommandList, const RefCountPtr<RHIFence>& inSignalFence,
                            const std::vector<RefCountPtr<RHISemaphore>>* inWaitForSemaphores, 
                            const std::vector<RefCountPtr<RHISemaphore>>* inSignalSemaphores)
{
    D3D12CommandList* commandList = CheckCast<D3D12CommandList*>(inCommandList.GetReference());
    if(commandList && commandList->IsValid())
    {
        if(!commandList->IsClosed())
        {
            commandList->End();
        }

        ID3D12CommandQueue* queue = GetCommandQueue(inCommandList->GetQueueType());
        ID3D12CommandList* cmdListHandle = commandList->GetCommandList();
        queue->ExecuteCommandLists(1, &cmdListHandle);

        if(inSignalFence != nullptr && inSignalFence->IsValid())
        {
            inSignalFence->Reset();
            ID3D12Fence* fence = CheckCast<D3D12Fence*>(inSignalFence.GetReference())->GetFence();
            queue->Signal(fence, FENCE_COMPLETED_VALUE);
        }

        if(inWaitForSemaphores && !inWaitForSemaphores->empty())
        {
            for(const auto& waitSemaphore : *inWaitForSemaphores)
            {
                D3D12Semaphore* d3dSemaphore = CheckCast<D3D12Semaphore*>(waitSemaphore.GetReference());
                if(d3dSemaphore && d3dSemaphore->IsValid())
                {
                    queue->Wait(d3dSemaphore->GetSemaphore(), FENCE_COMPLETED_VALUE);
                }
            }
        }

        if(inSignalSemaphores && !inSignalSemaphores->empty())
        {
            for(const auto& signalSemaphore : *inSignalSemaphores)
            {
                D3D12Semaphore* d3dSemaphore = CheckCast<D3D12Semaphore*>(signalSemaphore.GetReference());
                if(d3dSemaphore && d3dSemaphore->IsValid())
                {
                    d3dSemaphore->Reset();
                    queue->Signal(d3dSemaphore->GetSemaphore(), FENCE_COMPLETED_VALUE);
                }
            }
        }
    }
}

void D3D12Device::FlushDirectCommandQueue()
{
    RefCountPtr<D3D12Fence> tmpFence = CreateD3D12Fence();
    tmpFence->Reset();
    ID3D12CommandQueue* queue = GetCommandQueue(ERHICommandQueueType::Direct);
    queue->Signal(tmpFence->GetFence(), FENCE_COMPLETED_VALUE);
    tmpFence->CpuWait();
}

RefCountPtr<D3D12Fence> D3D12Device::CreateD3D12Fence()
{
    RefCountPtr<D3D12Fence> fence(new D3D12Fence(*this));
    if(!fence->Init())
    {
        Log::Error("[D3D12] Failed to create a fence");
    }
    return fence;
}
    
RefCountPtr<RHIFence> D3D12Device::CreateRhiFence()
{
    RefCountPtr<RHIFence> fence(new D3D12Fence(*this));
    if(!fence->Init())
    {
        Log::Error("[D3D12] Failed to create a fence");
    }
    return fence;
}

RefCountPtr<RHISemaphore> D3D12Device::CreateRhiSemaphore()
{
    RefCountPtr<RHISemaphore> semaphore(new D3D12Semaphore(*this));
    if(!semaphore->Init())
    {
        Log::Error("[D3D12] Failed to create a semaphore");
    }
    return semaphore;
}

D3D12Fence::D3D12Fence(D3D12Device& inDevice)
    : m_Device(inDevice)
    , m_FenceHandle(nullptr)
    , m_SignaledEvent(nullptr)
    , m_CurrentFenceValue(FENCE_INITIAL_VALUE)
{
    
}

D3D12Fence::~D3D12Fence()
{
    ShutdownInternal();
}

bool D3D12Fence::Init()
{
    if(IsValid())
    {
        Log::Warning("[D3D12] Fence already initialized");
        return true;    
    }

    HRESULT result = m_Device.GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_FenceHandle));
    if(FAILED(result))
    {
        OUTPUT_D3D12_FAILED_RESULT(result);
        return false;
    }
    
    Reset();
    
    return true;
}

void D3D12Fence::Shutdown()
{
    ShutdownInternal();
}

bool D3D12Fence::IsValid() const
{
    return m_FenceHandle != nullptr;
}

void D3D12Fence::ShutdownInternal()
{
    m_FenceHandle.Reset();
    if(m_SignaledEvent != nullptr)
    {
        CloseHandle(m_SignaledEvent);
        m_SignaledEvent = nullptr;
    }
}

void D3D12Fence::Reset()
{
    if(IsValid())
    {
        m_CurrentFenceValue = FENCE_INITIAL_VALUE;
        m_FenceHandle->Signal(m_CurrentFenceValue);
    }
}

void D3D12Fence::CpuWait()
{
    if(m_CurrentFenceValue != FENCE_INITIAL_VALUE)
    {
        Log::Error(TEXT("Fence has not been reset"));
        return;
    }
    
    if(IsValid())
    {
        if(m_FenceHandle->GetCompletedValue() < FENCE_COMPLETED_VALUE)
        {
            if(m_SignaledEvent == nullptr)
            {
                m_SignaledEvent = CreateEventEx(nullptr, TEXT("Wait For GPU"), false, EVENT_ALL_ACCESS);
            }
            ResetEvent(m_SignaledEvent);
            m_FenceHandle->SetEventOnCompletion(FENCE_COMPLETED_VALUE, m_SignaledEvent);
            WaitForSingleObject(m_SignaledEvent, INFINITE);
            m_CurrentFenceValue = FENCE_COMPLETED_VALUE;
        }
    }
}

void D3D12Fence::SetNameInternal()
{
    if(IsValid())
    {
        std::wstring name(m_Name.begin(), m_Name.end());
        m_FenceHandle->SetName(name.c_str());
    }
}

D3D12Semaphore::D3D12Semaphore(D3D12Device& inDevice)
    : m_Device(inDevice)
    , m_FenceHandle(nullptr)
    , m_CurrentFenceValue(FENCE_INITIAL_VALUE)
{
    
}

D3D12Semaphore::~D3D12Semaphore()
{
    ShutdownInternal();
}

bool D3D12Semaphore::Init()
{
    if(IsValid())
    {
        Log::Warning("[D3D12] Semaphore already initialized");
        return true;    
    }

    HRESULT result = m_Device.GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_FenceHandle));
    if(FAILED(result))
    {
        OUTPUT_D3D12_FAILED_RESULT(result);
        return false;
    }
    
    Reset();
    
    return true;
}

void D3D12Semaphore::Shutdown()
{
    ShutdownInternal();
}

bool D3D12Semaphore::IsValid() const
{
    return m_FenceHandle != nullptr;
}

void D3D12Semaphore::ShutdownInternal()
{
    m_FenceHandle.Reset();
}

void D3D12Semaphore::SetNameInternal()
{
    if(IsValid())
    {
        std::wstring name(m_Name.begin(), m_Name.end());
        m_FenceHandle->SetName(name.c_str());
    }
}

void D3D12Semaphore::Reset()
{
    if(IsValid())
    {
        m_CurrentFenceValue = FENCE_INITIAL_VALUE;
        m_FenceHandle->Signal(m_CurrentFenceValue);
    }
}

