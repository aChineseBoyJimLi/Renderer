#pragma once

#include "VulkanDefinitions.h"
#include "../RHIPipelineState.h"

class VulkanTexture;

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
    VkPipeline GetPipeline() const { return m_PipelineState; }
    
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
class VulkanGraphicsPipeline : public RHIGraphicsPipeline
{
public:
    ~VulkanGraphicsPipeline() override;
    bool Init() override;
    void Shutdown() override;
    bool IsValid() const override;
    const RHIGraphicsPipelineDesc& GetDesc() const override { return m_Desc; }
    VkPipeline GetPipeline() const { return m_PipelineState; }
    VkRenderPass GetRenderPass() const { return m_RenderPass; }
        
protected:
    void SetNameInternal() override;
    
private:
    friend VulkanDevice;
    VulkanGraphicsPipeline(VulkanDevice& inDevice, const RHIGraphicsPipelineDesc& inDesc);
    void ShutdownInternal();
    uint32_t InitVertexInputAttributes(std::vector<VkVertexInputAttributeDescription>& outAttributes) const;
    bool InitRenderPass();
    
    VulkanDevice& m_Device;
    RHIGraphicsPipelineDesc m_Desc;
    std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStages;
    VkShaderModule  m_VertexShaderModule;
    VkShaderModule  m_FragmentShaderModule;
    VkShaderModule  m_GeometryShaderModule;
    VkShaderModule  m_TessControlShaderModule;
    VkShaderModule  m_TessEvalShaderModule;
    VkShaderModule m_TaskShaderModule;
    VkShaderModule m_MeshShaderModule;
    VkRenderPass    m_RenderPass;
    VkPipeline      m_PipelineState;
};

///////////////////////////////////////////////////////////////////////////////////
/// VulkanFrameBuffer
///////////////////////////////////////////////////////////////////////////////////
class VulkanFrameBuffer : public RHIFrameBuffer
{
public:
    ~VulkanFrameBuffer() override;
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
    
    VkFramebuffer GetFrameBuffer() const { return m_FrameBufferHandle; }
    
protected:
    void SetNameInternal() override;

private:
    friend VulkanDevice;
    VulkanFrameBuffer(VulkanDevice& inDevice, const RHIFrameBufferDesc& inDesc);
    void ShutdownInternal();
    VulkanDevice& m_Device;
    RHIFrameBufferDesc m_Desc;
    
    VulkanTexture* m_RenderTargets[RHIRenderTargetsMaxCount];
    VulkanTexture* m_DepthStencil;
    uint32_t m_AttachmentCount;
    uint32_t m_FrameBufferWidth;
    uint32_t m_FrameBufferHeight;
    uint32_t m_NumRenderTargets;
    VkFramebuffer m_FrameBufferHandle;

    VkImageView m_RTVHandles[RHIRenderTargetsMaxCount];
    VkImageView m_DSVHandle;
};

///////////////////////////////////////////////////////////////////////////////////
/// VulkanRayTracingPipeline
///////////////////////////////////////////////////////////////////////////////////
class VulkanShaderTable : public RHIShaderTable
{
public:
    ~VulkanShaderTable() override;
    const RHIShaderTableEntry& GetRayGenShaderEntry() const { return m_RayGenerationShaderRecord; }
    const RHIShaderTableEntry& GetMissShaderEntry() const { return m_MissShaderRecord; }
    const RHIShaderTableEntry& GetHitGroupEntry() const { return m_HitGroupRecord; }
    const RHIShaderTableEntry& GetCallableShaderEntry() const { return m_CallableShaderRecord; }
    
private:
    friend class D3D12RayTracingPipeline;
    VkBuffer                                            m_ShaderTableBuffer;
    VkDeviceMemory                                      m_ShaderTableBufferMemory;
    
    RHIShaderTableEntry                                    m_RayGenerationShaderRecord;
    RHIShaderTableEntry                                    m_MissShaderRecord;
    RHIShaderTableEntry                                    m_HitGroupRecord;
    RHIShaderTableEntry                                    m_CallableShaderRecord;
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