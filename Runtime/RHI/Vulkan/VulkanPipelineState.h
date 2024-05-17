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
    const std::string& GetEntryName() const override { return m_EntryName; }
    std::shared_ptr<Blob> GetByteCode() const override {return m_ShaderBlob;}
    const uint8_t* GetData() const override { return m_ShaderBlob->GetData(); }
    size_t GetSize() const override { return m_ShaderBlob->GetSize(); }
    void SetByteCode(std::shared_ptr<Blob> inByteCode) override { m_ShaderBlob = inByteCode;}

private:
    friend class VulkanDevice;
    VulkanShader(ERHIShaderType inType)
        : m_Type(inType)
        , m_EntryName("main")
        , m_ShaderBlob(nullptr) {}

    ERHIShaderType m_Type;
    std::string m_EntryName;
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

///////////////////////////////////////////////////////////////////////////////////
/// VulkanGraphicsPipeline
///////////////////////////////////////////////////////////////////////////////////
class VulkanVertexInputLayout : public RHIVertexInputLayout
{
public:
    const RHIVertexInputLayoutDesc& GetDesc() const override { return m_Desc; }
    const VkVertexInputAttributeDescription* GetAttributes() const { return m_Attributes.data(); }
    uint32_t GetAttributeCount() const { return static_cast<uint32_t>(m_Attributes.size()); }
    
private:
    VulkanVertexInputLayout(const RHIVertexInputLayoutDesc& inDesc);
    RHIVertexInputLayoutDesc m_Desc;
    std::vector<VkVertexInputAttributeDescription> m_Attributes;
};

class VulkanGraphicsPipeline : public RHIGraphicsPipeline
{
public:
    ~VulkanGraphicsPipeline() override;
    bool Init() override;
    void Shutdown() override;
    bool IsValid() const override;
    const RHIGraphicsPipelineDesc& GetDesc() const override { return m_Desc; }
        
protected:
    void SetNameInternal() override;
    
private:
    friend VulkanDevice;
    VulkanGraphicsPipeline(VulkanDevice& inDevice, const RHIGraphicsPipelineDesc& inDesc);
    void ShutdownInternal();
    
    VulkanDevice& m_Device;
    RHIGraphicsPipelineDesc m_Desc;
    std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStages;
    VkShaderModule  m_VertexShaderModule;
    VkShaderModule  m_FragmentShaderModule;
    VkShaderModule  m_GeometryShaderModule;
    VkShaderModule  m_TessControlShaderModule;
    VkShaderModule  m_TessEvalShaderModule;
    VkPipeline      m_PipelineState;
};

///////////////////////////////////////////////////////////////////////////////////
/// VulkanMeshPipeline
///////////////////////////////////////////////////////////////////////////////////
class VulkanMeshPipeline : public RHIMeshPipeline
{
public:
    ~VulkanMeshPipeline() override;
    bool Init() override;
    void Shutdown() override;
    bool IsValid() const override;
    const RHIMeshPipelineDesc& GetDesc() const override { return m_Desc; }
    
protected:
    void SetNameInternal() override;
    
private:
    friend VulkanDevice;
    VulkanMeshPipeline(VulkanDevice& inDevice, const RHIMeshPipelineDesc& inDesc);
    void ShutdownInternal();

    std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStages;
    VulkanDevice& m_Device;
    RHIMeshPipelineDesc m_Desc;
    VkShaderModule m_TaskShaderModule;
    VkShaderModule m_MeshShaderModule;
    VkShaderModule m_FragmentShaderModule;
    VkPipeline m_PipelineState;
};

///////////////////////////////////////////////////////////////////////////////////
/// VulkanRayTracingPipeline
///////////////////////////////////////////////////////////////////////////////////
class VulkanShaderTable : public RHIShaderTable
{
public:
    ~VulkanShaderTable() override;
    const ShaderTableEntry& GetRayGenShaderEntry() const { return m_RayGenerationShaderRecord; }
    const ShaderTableEntry& GetMissShaderEntry() const { return m_MissShaderRecord; }
    const ShaderTableEntry& GetHitGroupEntry() const { return m_HitGroupRecord; }
    const ShaderTableEntry& GetCallableShaderEntry() const { return m_CallableShaderRecord; }
    
private:
    friend class D3D12RayTracingPipeline;
    VkBuffer                                            m_ShaderTableBuffer;
    VkDeviceMemory                                      m_ShaderTableBufferMemory;
    
    ShaderTableEntry                                    m_RayGenerationShaderRecord;
    ShaderTableEntry                                    m_MissShaderRecord;
    ShaderTableEntry                                    m_HitGroupRecord;
    ShaderTableEntry                                    m_CallableShaderRecord;
};

class VulkanRayTracingPipeline : public RHIRayTracingPipeline
{
public:
    ~VulkanRayTracingPipeline() override;
    bool Init() override;
    void Shutdown() override;
    bool IsValid() const override;
    const RHIRayTracingPipelineDesc& GetDesc() const override { return m_Desc; }
    
protected:
    void SetNameInternal() override;

private:
    friend VulkanDevice;
    VulkanRayTracingPipeline(VulkanDevice& inDevice, const RHIRayTracingPipelineDesc& inDesc);
    void ShutdownInternal();

    VulkanDevice& m_Device;
    RHIRayTracingPipelineDesc m_Desc;
    std::vector<VkShaderModule> m_ShaderModules;
    std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStages;
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_ShaderGroups;
    std::unique_ptr<VulkanShaderTable> m_ShaderTable;
    VkPipeline m_PipelineState;
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_PipelineProperties;
};