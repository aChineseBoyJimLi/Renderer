#pragma once

#include "D3D12Definitions.h"
#include "../RHIPipelineState.h"

class D3D12PipelineBindingLayout : public RHIPipelineBindingLayout
{
public:
    ~D3D12PipelineBindingLayout() override;
    bool Init() override;
    void Shutdown() override;
    bool IsValid() const override;
    const RHIPipelineBindingLayoutDesc& GetDesc() const override { return m_Desc; }
    ID3D12RootSignature* GetRootSignature() const {return m_RootSignature.Get(); }

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
    friend class D3D12Device;
    D3D12PipelineBindingLayout(D3D12Device& inDevice, const RHIPipelineBindingLayoutDesc& inBindingItems);
    void ShutdownInternal();

    D3D12Device& m_Device;
    RHIPipelineBindingLayoutDesc m_Desc;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;
};

class D3D12Shader : public RHIShader
{
public:
    ~D3D12Shader() override = default;
    bool Init() override { return true; }
    void Shutdown() override {}
    bool IsValid() const override {return m_ShaderBlob != nullptr && !m_ShaderBlob->IsEmpty();}

    ERHIShaderType GetType() const override {return m_Type;}
    void SetEntryName(const char* inName) override { m_EntryName = inName; }
    const char* GetEntryName() const override { return m_EntryName; }
    std::shared_ptr<Blob> GetByteCode() const override {return m_ShaderBlob;}
    void SetByteCode(std::shared_ptr<Blob> inByteCode) override { m_ShaderBlob = inByteCode;}

private:
    friend class D3D12Device;
    D3D12Shader(ERHIShaderType inType)
        : m_Type(inType)
        , m_EntryName("main")
        , m_ShaderBlob(nullptr) {}

    ERHIShaderType m_Type;
    const char* m_EntryName;
    std::shared_ptr<Blob> m_ShaderBlob;
};

class D3D12ComputePipeline : public RHIComputePipeline
{
public:
    ~D3D12ComputePipeline() override;
    bool Init() override;
    void Shutdown() override;
    bool IsValid() const override;
    const RHIComputePipelineDesc& GetDesc() const override { return m_Desc; }

protected:
    void SetNameInternal() override;
    
private:
    friend class D3D12Device;
    D3D12ComputePipeline(D3D12Device& inDevice, const RHIComputePipelineDesc& inDesc);
    void ShutdownInternal();

    D3D12Device& m_Device;
    RHIComputePipelineDesc m_Desc;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;
};