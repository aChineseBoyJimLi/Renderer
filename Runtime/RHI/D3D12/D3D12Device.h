#pragma once
#include "../RHIDevice.h"
#include "D3D12Definitions.h"

class D3D12Texture;
class D3D12DescriptorManager;
class D3D12Fence : public RHIFence
{
public:
    ~D3D12Fence() override;
    bool Init() override;
    void Shutdown() override;
    bool IsValid() const override;
    void Reset() override;
    void CpuWait() override;
    
    ID3D12Fence* GetFence() const { return m_FenceHandle.Get(); }

protected:
    void SetNameInternal() override;
    
private:
    friend class D3D12Device;
    D3D12Fence(D3D12Device& inDevice);
    void ShutdownInternal();
    
    D3D12Device& m_Device;
    Microsoft::WRL::ComPtr<ID3D12Fence> m_FenceHandle;
    HANDLE m_SignaledEvent;
    uint8_t m_CurrentFenceValue;
};

class D3D12Semaphore : public RHISemaphore
{
public:
    ~D3D12Semaphore() override;
    bool Init() override;
    void Shutdown() override;
    bool IsValid() const override;
    void Reset() override;

    ID3D12Fence* GetSemaphore() const { return m_FenceHandle.Get(); }

protected:
    void SetNameInternal() override;
    
private:
    friend class D3D12Device;
    D3D12Semaphore(D3D12Device& inDevice);
    void ShutdownInternal();
    
    D3D12Device& m_Device;
    Microsoft::WRL::ComPtr<ID3D12Fence> m_FenceHandle;
    uint8_t m_CurrentFenceValue;
};

class D3D12Device : public RHIDevice
{
public:
    ~D3D12Device() override;
    bool Init() override;
    void Shutdown() override;
    bool IsValid() const override;
    
    std::shared_ptr<RHIFence>       CreateRhiFence() override;
    std::shared_ptr<RHISemaphore>   CreateRhiSemaphore() override;
    std::shared_ptr<RHICommandList> CreateCommandList(ERHICommandQueueType inType = ERHICommandQueueType::Direct) override;
    std::shared_ptr<RHIPipelineBindingLayout> CreatePipelineBindingLayout(const RHIPipelineBindingLayoutDesc& inBindingItems) override;
    std::shared_ptr<RHIShader> CreateShader(ERHIShaderType inType) override;
    std::shared_ptr<RHIComputePipeline> CreateComputePipeline(const RHIComputePipelineDesc& inDesc) override;
    std::shared_ptr<RHIResourceHeap> CreateResourceHeap(const RHIResourceHeapDesc& inDesc) override;
    std::shared_ptr<RHIBuffer> CreateBuffer(const RHIBufferDesc& inDesc, bool isVirtual = false) override;
    std::shared_ptr<RHITexture> CreateTexture(const RHITextureDesc& inDesc, bool isVirtual = false) override;
    std::shared_ptr<D3D12Texture> CreateTexture(const RHITextureDesc& inDesc, const Microsoft::WRL::ComPtr<ID3D12Resource>& inResource);
    std::shared_ptr<RHIFrameBuffer> CreateFrameBuffer(const RHIFrameBufferDesc& inDesc) override;
    void ExecuteCommandList(const std::shared_ptr<RHICommandList>& inCommandList, const std::shared_ptr<RHIFence>& inSignalFence = nullptr,
                            const std::vector<std::shared_ptr<RHISemaphore>>* inWaitForSemaphores = nullptr, 
                            const std::vector<std::shared_ptr<RHISemaphore>>* inSignalSemaphores = nullptr) override;
    
    ERHIBackend GetBackend() const override { return ERHIBackend::D3D12; }
    IDXGIFactory2* GetFactory() const { return m_FactoryHandle.Get(); }
    IDXGIAdapter1* GetAdapter() const { return m_AdapterHandle.Get(); }
    ID3D12Device5* GetDevice() const { return m_DeviceHandle.Get(); }
    ID3D12CommandQueue* GetCommandQueue(ERHICommandQueueType inQueueType) const { return m_QueueHandles[static_cast<uint8_t>(inQueueType)].Get(); }
    D3D12DescriptorManager& GetDescriptorManager() { return *m_DescriptorManager; }
    
    // no mGPU support so far
    static uint32_t GetNodeMask() { return 0; }
    static uint32_t GetCreationNodeMask() { return 1; }
    static uint32_t GetVisibleNodeMask() { return 1; }

    static constexpr D3D_FEATURE_LEVEL  s_FeatureLevel = D3D_FEATURE_LEVEL_12_1;
    static void LogAdapterDesc(const DXGI_ADAPTER_DESC1& inDesc);

private:
    friend bool RHI::Init(bool useVulkan);
    D3D12Device();
    void ShutdownInternal();
    
    Microsoft::WRL::ComPtr<IDXGIFactory2>               m_FactoryHandle;
    Microsoft::WRL::ComPtr<IDXGIAdapter1>               m_AdapterHandle;
    Microsoft::WRL::ComPtr<ID3D12Device5>               m_DeviceHandle;
    std::array<Microsoft::WRL::ComPtr<ID3D12CommandQueue>, COMMAND_QUEUES_COUNT> m_QueueHandles;
    std::unique_ptr<D3D12DescriptorManager>             m_DescriptorManager;

    D3D12_FEATURE_DATA_D3D12_OPTIONS5 m_Feature5Data{}; // RayTracing, RenderPass
    D3D12_FEATURE_DATA_D3D12_OPTIONS6 m_Feature6Data{}; // VariableShadingRate
    D3D12_FEATURE_DATA_D3D12_OPTIONS7 m_Feature7Data{}; // MeshShader, SamplerFeedback
};
