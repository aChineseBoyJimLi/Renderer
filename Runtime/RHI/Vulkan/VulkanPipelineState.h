#pragma once

#include "VulkanDefinitions.h"
#include "../RHIPipelineState.h"

///////////////////////////////////////////////////////////////////////////////////
/// VulkanPipelineBindingLayout
///////////////////////////////////////////////////////////////////////////////////

class VulkanPipelineBindingLayout : public RHIPipelineBindingLayout
{
public:
    ~VulkanPipelineBindingLayout() override;
    bool Init() override;
    void Shutdown() override;
    bool IsValid() const override;
    const RHIPipelineBindingLayoutDesc& GetDesc() const override { return m_BindingItems; }
    const VkDescriptorSetLayout* GetDescriptorSetLayouts() const { return m_DescriptorSetLayouts.data(); }
    uint32_t GetDescriptorSetLayoutCount() const { return static_cast<uint32_t>(m_DescriptorSetLayouts.size()); }
    VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }

protected:
    void SetNameInternal() override;

private:
    friend class VulkanDevice;
    VulkanPipelineBindingLayout(VulkanDevice& inDevice, const RHIPipelineBindingLayoutDesc& inBindingItems);
    void ShutdownInternal();
    
    VulkanDevice& m_Device;
    RHIPipelineBindingLayoutDesc m_BindingItems;
    std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;
    VkPipelineLayout m_PipelineLayout;
};

///////////////////////////////////////////////////////////////////////////////////
/// VulkanShader
///////////////////////////////////////////////////////////////////////////////////

class VulkanShader : public RHIShader
{
public:
    ~VulkanShader() override = default;
    bool Init() override { return true; }
    void Shutdown() override {}
    bool IsValid() const override {return m_ShaderBlob != nullptr && !m_ShaderBlob->IsEmpty();}

    ERHIShaderType GetType() const override {return m_Type;}
    void SetEntryName(const char* inName) override { m_EntryName = inName; }
    const char* GetEntryName() const override { return m_EntryName; }
    std::shared_ptr<Blob> GetByteCode() const override {return m_ShaderBlob;}
    void SetByteCode(std::shared_ptr<Blob> inByteCode) override { m_ShaderBlob = inByteCode;}

private:
    friend class VulkanDevice;
    VulkanShader(ERHIShaderType inType)
        : m_Type(inType)
        , m_EntryName("main")
        , m_ShaderBlob(nullptr) {}

    ERHIShaderType m_Type;
    const char* m_EntryName;
    std::shared_ptr<Blob> m_ShaderBlob;
};

///////////////////////////////////////////////////////////////////////////////////
/// VulkanComputePipeline
///////////////////////////////////////////////////////////////////////////////////

class VulkanComputePipeline : public RHIComputePipeline
{
public:
    ~VulkanComputePipeline() override;
    bool Init() override;
    void Shutdown() override;
    bool IsValid() const override;
    const RHIComputePipelineDesc& GetDesc() const override { return m_Desc; }
    
protected:
    void SetNameInternal() override;

private:
    friend class VulkanDevice;
    VulkanComputePipeline(VulkanDevice& inDevice, const RHIComputePipelineDesc& inDesc);
    void ShutdownInternal();
    
    VulkanDevice& m_Device;
    RHIComputePipelineDesc m_Desc;
    VkPipeline m_PipelineState;
    VkShaderModule m_ComputeShaderModule;
};