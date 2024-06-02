#pragma once

#include "../Core/Win32Base.h"
#include "../RHI/RHI.h"
#include "AssetsManager.h"
#include "imgui.h"


class ImGuiTestApp : public Win32Base
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

private:
    bool InitImGui();
    void ShutdownImGui();
    void LoadAssets();
    void CreateFrameBuffer();
    void CreateResources();
    void BuildUI();
    void UpdateGeometry();
    void ReallocateBuffer(RHIBufferRef& inBuffer, size_t requiredSize, size_t reallocateSize);
    
    RHIFenceRef m_Fence;
    RHISemaphoreRef m_Semaphore;
    RHICommandListRef m_CommandList;
    RHICommandListRef m_CopyCommandList;

    RHIPipelineBindingLayoutRef m_GraphicsPassBindingLayout;
    std::shared_ptr<Blob> m_VertexShaderByteCode;
    std::shared_ptr<Blob> m_PixelShaderByteCode;
    std::shared_ptr<Blob> m_Font;
    RHIShaderRef m_VertexShader;
    RHIShaderRef m_PixelShader;
    RHIGraphicsPipelineRef m_GraphicsPipeline;

    RHISwapChainRef m_SwapChain;
    RHITextureRef m_DepthStencilTexture;
    std::vector<RHIFrameBufferRef> m_FrameBuffers;
    
    RHITextureRef m_FontTexture;
    RHISamplerRef m_FontSampler;
    RHIBufferRef m_GUIConstants;

    RHIBufferRef m_VertexBuffer;
    RHIBufferRef m_IndexBuffer;
    RHIBufferRef m_ConstantsBuffer;
    
    std::vector<ImDrawVert> m_VtxBuffer;
    std::vector<ImDrawIdx> m_IdxBuffer;

    RHIResourceSetRef m_ResourceSet;
};