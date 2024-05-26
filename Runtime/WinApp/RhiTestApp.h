#pragma once

#include "../Core/Win32Base.h"
#include "../RHI/RHI.h"
#include "AssetsManager.h"

class RhiTestApp : public Win32Base
{
public:
    using Win32Base::Win32Base;
    static constexpr uint32_t s_TexturesCount = 5;
    static constexpr uint32_t s_MaterialCount = 100;
    static constexpr uint32_t s_InstanceCountX = 8, s_InstanceCountY = 8, s_InstanceCountZ = 8;
    static constexpr uint32_t s_InstancesCount = s_InstanceCountX * s_InstanceCountY * s_InstanceCountZ;
    
protected:
    bool Init() final;
    void Tick()  final;
    void Shutdown() final;
    void OnBeginResize() override;
    void OnResize(int width, int height) final;
    void OnEndResize() final;

    void LoadAssets();
    void CreateFrameBuffer();
    void CreateResources(); 
    
private:
    RHIFenceRef m_Fence;
    RHISemaphoreRef m_Semaphore;
    RHICommandListRef m_CommandList;
    RHICommandListRef m_CopyCommandList;
    
    RHIPipelineBindingLayoutRef m_CullingPassBindingLayout;
    std::shared_ptr<Blob> m_CullingShaderByteCode;
    RHIShaderRef m_CullingShader;
    RHIComputePipelineRef m_CullingPipeline;
    
    RHIPipelineBindingLayoutRef m_GraphicsPassBindingLayout;
    std::shared_ptr<Blob> m_VertexShaderByteCode;
    std::shared_ptr<Blob> m_PixelShaderByteCode;
    RHIShaderRef m_VertexShader;
    RHIShaderRef m_PixelShader;
    RHIGraphicsPipelineRef m_GraphicsPipeline;

    RHISwapChainRef m_SwapChain;
    RHITextureRef m_DepthStencilTexture;
    std::vector<RHIFrameBufferRef> m_FrameBuffers;
    std::array<RHIDrawIndexedArguments, 1> m_IndirectDrawCommands;

    RHIBufferRef m_VertexBuffer;
    RHIBufferRef m_IndexBuffer;
    std::shared_ptr<AssetsManager::Mesh> m_Mesh;
    std::array<std::shared_ptr<AssetsManager::Texture>, s_TexturesCount>  m_Textures;
    

    RHIBufferRef m_CameraDataBuffer;
    RHIBufferRef m_LightDataBuffer;
    RHIBufferRef m_InstanceBuffer;
    RHIBufferRef m_MaterialBuffer;
    std::vector<RHITextureRef> m_MainTextures;
    RHISamplerRef m_Sampler;
    RHIBufferRef m_IndirectDrawCommandsBuffer;

    RHIResourceSetRef m_ResourceSet;
};