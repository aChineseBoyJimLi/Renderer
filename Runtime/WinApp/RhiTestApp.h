#pragma once

#include "../Core/Win32Base.h"
#include "../RHI/RHI.h"

class RhiTestApp : public Win32Base
{
public:
    using Win32Base::Win32Base;

protected:
    bool Init() final;
    void Tick()  final;
    void Shutdown() final;
    void OnResize(int width, int height) final;
    void OnEndResize() final;

    void CreateFrameBuffer();
    
private:
    std::shared_ptr<RHIFence> m_Fence;
    std::shared_ptr<RHISemaphore> m_Semaphore;
    std::shared_ptr<RHICommandList> m_CommandList;
    std::shared_ptr<RHIPipelineBindingLayout> m_CullingPassBindlingLayout;
    std::shared_ptr<Blob> m_CullingShaderByteCode;
    std::shared_ptr<RHIShader> m_CullingShader;
    std::shared_ptr<RHIComputePipeline> m_CullingPipeline;
    std::shared_ptr<RHIResourceHeap> m_ResourceHeap;
    std::shared_ptr<RHIResourceHeap> m_ResourceHeap2;

    std::shared_ptr<RHIBuffer> m_VirtualBuffer;
    std::shared_ptr<RHIBuffer> m_ConstantsBuffer;

    std::shared_ptr<RHITexture> m_VirtualTexture;
    std::shared_ptr<RHITexture> m_DepthStencilTexture;

    std::shared_ptr<RHISwapChain> m_SwapChain;
    std::vector<std::shared_ptr<RHIFrameBuffer>> m_FrameBuffers; 
};