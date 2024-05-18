#pragma once

#include "../Core/Win32Base.h"
#include "../RHI/RHI.h"
#include "AssetsManager.h"

class RhiTestApp : public Win32Base
{
public:
    using Win32Base::Win32Base;

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

    RHIBufferRef m_VertexBuffer;
    RHIBufferRef m_IndexBuffer;
    std::shared_ptr<AssetsManager::Mesh> m_Mesh;

    RHIBufferRef m_CameraDataBuffer;
    RHIBufferRef m_LightDataBuffer;
    RHIBufferRef m_InstanceBuffer;
    RHIBufferRef m_MaterialBuffer;
    std::vector<RHITextureRef> m_MainTextures;
    RHISamplerRef m_Sampler;
};