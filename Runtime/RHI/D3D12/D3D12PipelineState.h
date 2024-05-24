#pragma once

#include "D3D12Definitions.h"
#include "../RHIPipelineState.h"

class D3D12Device;
class D3D12Texture;

///////////////////////////////////////////////////////////////////////////////////
/// D3D12PipelineBindingLayout
///////////////////////////////////////////////////////////////////////////////////
class D3D12PipelineBindingLayout : public RHIPipelineBindingLayout
{
public:
    ~D3D12PipelineBindingLayout() override;
    bool Init() override;
    void Shutdown() override;
    bool IsValid() const override;
    const RHIPipelineBindingLayoutDesc& GetDesc() const override { return m_Desc; }
    ID3D12RootSignature* GetRootSignature() const {return m_RootSignature.Get(); }
    const std::vector<CD3DX12_ROOT_PARAMETER>& GetRootParameters() const { return m_RootParameters; }

    // The maximum size of a root signature is 64 DWORDs.
    // - Descriptor tables cost 1 DWORD each.
    // - Root constants cost 1 DWORD each, since they are 32-bit values.
    // - Root descriptors cost 2 DWORDs each, since they are 64-bit GPU virtual addresses.
    // https://learn.microsoft.com/en-us/windows/win32/direct3d12/root-signature-limits
    constexpr static uint8_t s_RootSignatureMaxSize = 64;
    constexpr static uint8_t s_CbvRootDescriptorsMaxCount = 16; // only 16 CBV descriptors are allowed in a single root signature

protected:
    void SetNameInternal() override;
    
private:
    friend D3D12Device;
    D3D12PipelineBindingLayout(D3D12Device& inDevice, const RHIPipelineBindingLayoutDesc& inBindingItems);
    void ShutdownInternal();

    D3D12Device& m_Device;
    RHIPipelineBindingLayoutDesc m_Desc;
    std::array<CD3DX12_DESCRIPTOR_RANGE, s_RootSignatureMaxSize> m_DescriptorRanges;
    std::vector<CD3DX12_ROOT_PARAMETER> m_RootParameters;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;
};

///////////////////////////////////////////////////////////////////////////////////
/// D3D12Shader
///////////////////////////////////////////////////////////////////////////////////
class D3D12Shader : public RHIShader
{
public:
    ~D3D12Shader() override = default;
    bool Init() override { return true; }
    void Shutdown() override {}
    bool IsValid() const override {return m_ShaderBlob != nullptr && !m_ShaderBlob->IsEmpty();}

    ERHIShaderType GetType() const override {return m_Type;}
    void SetEntryName(const char* inName) override { m_EntryName = inName; }
    const std::string& GetEntryName() const override { return m_EntryName; }
    std::shared_ptr<Blob> GetByteCode() const override {return m_ShaderBlob;}
    void SetByteCode(std::shared_ptr<Blob> inByteCode) override;
    const uint8_t* GetData() const override { return m_ShaderBlob->GetData(); }
    size_t GetSize() const override { return m_ShaderBlob->GetSize(); }
    const D3D12_SHADER_BYTECODE* GetByteCodeD3D() const { return &m_ShaderByteCodeD3D; }

private:
    friend D3D12Device;
    D3D12Shader(ERHIShaderType inType)
        : m_Type(inType)
        , m_EntryName("main")
        , m_ShaderBlob(nullptr)
        , m_ShaderByteCodeD3D() {}

    ERHIShaderType m_Type;
    std::string m_EntryName;
    std::shared_ptr<Blob> m_ShaderBlob;
    D3D12_SHADER_BYTECODE m_ShaderByteCodeD3D;
};

///////////////////////////////////////////////////////////////////////////////////
/// D3D12ComputePipeline
///////////////////////////////////////////////////////////////////////////////////
class D3D12ComputePipeline : public RHIComputePipeline
{
public:
    ~D3D12ComputePipeline() override;
    bool Init() override;
    void Shutdown() override;
    bool IsValid() const override;
    const RHIComputePipelineDesc& GetDesc() const override { return m_Desc; }
    ID3D12PipelineState* GetPipelineState() const { return m_PipelineState.Get(); }
    
protected:
    void SetNameInternal() override;
    
private:
    friend D3D12Device;
    D3D12ComputePipeline(D3D12Device& inDevice, const RHIComputePipelineDesc& inDesc);
    void ShutdownInternal();

    D3D12Device& m_Device;
    RHIComputePipelineDesc m_Desc;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;
};

///////////////////////////////////////////////////////////////////////////////////
/// D3D12GraphicsPipeline
///////////////////////////////////////////////////////////////////////////////////
class D3D12GraphicsPipeline : public RHIGraphicsPipeline
{
public:
    ~D3D12GraphicsPipeline() override;
    bool Init() override;
    void Shutdown() override;
    bool IsValid() const override;
    const RHIGraphicsPipelineDesc& GetDesc() const override { return m_Desc; }
    ID3D12PipelineState* GetPipelineState() const { return m_PipelineState.Get(); }
    
protected:
    void SetNameInternal() override;
    
private:
    friend D3D12Device;
    D3D12GraphicsPipeline(D3D12Device& inDevice, const RHIGraphicsPipelineDesc& inDesc);
    void ShutdownInternal();
    void InitInputElements(std::vector<D3D12_INPUT_ELEMENT_DESC>& outInputElements) const;
    
    D3D12Device& m_Device;
    RHIGraphicsPipelineDesc m_Desc;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;
};

///////////////////////////////////////////////////////////////////////////////////
/// D3D12FrameBuffer
///////////////////////////////////////////////////////////////////////////////////
class D3D12FrameBuffer : public RHIFrameBuffer
{
public:
    ~D3D12FrameBuffer() override;
    bool Init() override;
    void Shutdown() override;
    bool IsValid() const override;
    const RHIFrameBufferDesc& GetDesc() const override { return m_Desc; }
    uint32_t GetFrameBufferWidth() const override { return m_FrameBufferWidth; }
    uint32_t GetFrameBufferHeight() const override { return m_FrameBufferHeight; }
    uint32_t GetNumRenderTargets() const override { return m_NumRenderTargets; }
    const RHITexture* GetRenderTarget(uint32_t inIndex) const override { return m_Desc.RenderTargets[inIndex]; }
    const RHITexture* GetDepthStencil() const override { return m_Desc.DepthStencil; }
    bool HasDepthStencil() const override { return m_Desc.DepthStencil != nullptr; }
    ERHIFormat GetRenderTargetFormat(uint32_t inIndex) const override;
    ERHIFormat GetDepthStencilFormat() const override;
    const D3D12_CPU_DESCRIPTOR_HANDLE* GetRenderTargetViews() const { return IsValid() ? m_RTVHandles : nullptr; }
    const D3D12_CPU_DESCRIPTOR_HANDLE* GetDepthStencilViews() const { return HasDepthStencil() ? &m_DSVHandle : nullptr; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetView(uint32_t inIndex) const { return m_RTVHandles[inIndex]; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetDepthStencilView() const { return m_DSVHandle; }
    
private:
    friend D3D12Device;
    D3D12FrameBuffer(D3D12Device& inDevice, const RHIFrameBufferDesc& inDesc);
    void ShutdownInternal();

    D3D12Device& m_Device;
    RHIFrameBufferDesc m_Desc;
    uint32_t m_FrameBufferWidth;
    uint32_t m_FrameBufferHeight;
    D3D12Texture* m_RenderTargets[RHIRenderTargetsMaxCount];
    D3D12Texture* m_DepthStencil;
    uint32_t m_NumRenderTargets;
    D3D12_CPU_DESCRIPTOR_HANDLE m_RTVHandles[RHIRenderTargetsMaxCount];
    D3D12_CPU_DESCRIPTOR_HANDLE m_DSVHandle;
};


///////////////////////////////////////////////////////////////////////////////////
/// D3D12RayTracingPipeline
///////////////////////////////////////////////////////////////////////////////////
class D3D12ShaderTable : public RHIShaderTable
{
public:
    const RHIShaderTableEntry& GetRayGenShaderEntry() const { return m_RayGenerationShaderRecord; }
    const RHIShaderTableEntry& GetMissShaderEntry() const { return m_MissShaderRecord; }
    const RHIShaderTableEntry& GetHitGroupEntry() const { return m_HitGroupRecord; }
    const RHIShaderTableEntry& GetCallableShaderEntry() const { return m_CallableShaderRecord; }
    
private:
    friend class D3D12RayTracingPipeline;
    Microsoft::WRL::ComPtr<ID3D12Resource>              m_ShaderTableBuffer;
    RHIShaderTableEntry                                    m_RayGenerationShaderRecord;
    RHIShaderTableEntry                                    m_MissShaderRecord;
    RHIShaderTableEntry                                    m_HitGroupRecord;
    RHIShaderTableEntry                                    m_CallableShaderRecord;
};

class D3D12RayTracingPipeline : public RHIRayTracingPipeline
{
public:
    ~D3D12RayTracingPipeline() override;
    bool Init() override;
    void Shutdown() override;
    bool IsValid() const override;
    const RHIRayTracingPipelineDesc& GetDesc() const override { return m_Desc; }
    const RHIShaderTable& GetShaderTable() const override { return *m_ShaderTable; }
    ID3D12PipelineState* GetPipelineState() const { return m_PipelineState.Get(); }
    
protected:
    void SetNameInternal() override;

private:
    friend D3D12Device;
    D3D12RayTracingPipeline(D3D12Device& inDevice, const RHIRayTracingPipelineDesc& inDesc);
    void ShutdownInternal();

    D3D12Device& m_Device;
    RHIRayTracingPipelineDesc m_Desc;
    std::unique_ptr<D3D12ShaderTable> m_ShaderTable;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;
    Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> m_PipelineProperties;
    std::vector<std::wstring> m_ShaderGroupNames;
};