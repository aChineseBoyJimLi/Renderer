#include "RhiTestApp.h"

#include "../Core/Log.h"

bool RhiTestApp::Init()
{
    ERHIBackend backend = RHI::GetDevice()->GetBackend();
    switch (backend)
    {
    case ERHIBackend::Vulkan:
        Log::Info("RHI backend: Vulkan");
        break;
    case ERHIBackend::D3D12:
        Log::Info("RHI backend: D3D12");
        break;
    }

    m_Fence = RHI::GetDevice()->CreateRhiFence();
    m_Semaphore = RHI::GetDevice()->CreateRhiSemaphore();
    m_CommandList = RHI::GetDevice()->CreateCommandList();
    
    RHIPipelineBindingLayoutDesc desc;
    desc.Items.emplace_back(ERHIBindingResourceType::Buffer_CBV, 0);
    desc.Items.emplace_back(ERHIBindingResourceType::Buffer_CBV, 1);
    desc.Items.emplace_back(ERHIBindingResourceType::Buffer_CBV, 2);
    desc.Items.emplace_back(ERHIBindingResourceType::Buffer_SRV, 0);
    desc.Items.emplace_back(ERHIBindingResourceType::Buffer_UAV, 0);

    m_CullingPassBindlingLayout = RHI::GetDevice()->CreatePipelineBindingLayout(desc);

    m_CullingShaderByteCode = std::make_shared<Blob>();
    
    std::filesystem::path path = std::filesystem::current_path() / ".." / "Shaders" / "VisibleCullingVk.cs.bin";
    m_CullingShaderByteCode->ReadBinaryFile(path);
    
    m_CullingShader = RHI::GetDevice()->CreateShader(ERHIShaderType::Compute);
    m_CullingShader->SetByteCode(m_CullingShaderByteCode);
    
    RHIComputePipelineDesc cullingPassDesc;
    cullingPassDesc.ComputeShader = m_CullingShader;
    cullingPassDesc.BindingLayout = m_CullingPassBindlingLayout;
    m_CullingPipeline = RHI::GetDevice()->CreateComputePipeline(cullingPassDesc);

    
    RHIBufferDesc bufferDesc1{};
    bufferDesc1.Size = 1024;
    bufferDesc1.Stride = 0;
    bufferDesc1.Usages = ERHIBufferUsage::ConstantBuffer;
    bufferDesc1.CpuAccess = ERHICpuAccessMode::Write;
    m_VirtualBuffer = RHI::GetDevice()->CreateBuffer(bufferDesc1, true);

    RHIResourceHeapDesc heapDesc{};
    heapDesc.Alignment = m_VirtualBuffer->GetAlignment();
    heapDesc.Size = heapDesc.Alignment * 128;
    heapDesc.Usage = ERHIHeapUsage::Buffer;
    heapDesc.TypeFilter = m_VirtualBuffer->GetMemTypeFilter();
    heapDesc.Type = ERHIResourceHeapType::Upload;
    
    m_ResourceHeap = RHI::GetDevice()->CreateResourceHeap(heapDesc);

    if(m_ResourceHeap->IsEmpty())
    {
        size_t offset1;
        if(m_ResourceHeap->TryAllocate(128, offset1))
        {
            Log::Info("Offset:%d in heap", offset1);
        }

        size_t offset2;
        if(m_ResourceHeap->TryAllocate(64, offset2))
        {
            Log::Info("Offset:%d in heap", offset2);
        }

        m_ResourceHeap->Free(offset1, 128);

        size_t offset3;
        if(m_ResourceHeap->TryAllocate(500, offset3))
        {
            Log::Info("Offset:%d in heap", offset3);
        }

        if(m_VirtualBuffer->BindMemory(m_ResourceHeap))
        {
            Log::Info("Offset:%d in heap", m_VirtualBuffer->GetOffsetInHeap());
        }
    }
    
    m_ConstantsBuffer = RHI::GetDevice()->CreateBuffer(bufferDesc1);

    RHITextureDesc textureDesc{};
    textureDesc.Width = 1024;
    textureDesc.Height = 1024;
    m_VirtualTexture = RHI::GetDevice()->CreateTexture(textureDesc, true);

    RHIResourceHeapDesc heapDesc2{};
    heapDesc2.Alignment = m_VirtualTexture->GetAlignment();
    heapDesc2.Size = heapDesc.Alignment * 100000;
    heapDesc2.Usage = ERHIHeapUsage::Texture;
    heapDesc2.TypeFilter = m_VirtualTexture->GetMemTypeFilter();

    m_ResourceHeap2 = RHI::GetDevice()->CreateResourceHeap(heapDesc2);

    if(m_VirtualTexture->BindMemory(m_ResourceHeap2))
    {
        Log::Info("Offset:%d in heap", m_VirtualTexture->GetOffsetInHeap());
    }

    RHISwapChainDesc swapChainDesc{};
    swapChainDesc.Width = m_Width;
    swapChainDesc.Height = m_Height;
    swapChainDesc.Format = ERHIFormat::RGBA8_UNORM;
    swapChainDesc.BufferCount = 3;
    swapChainDesc.hwnd = m_hWnd;
    swapChainDesc.hInstance = m_hInstance;
    m_SwapChain = RHI::CreateSwapChain(swapChainDesc);
    m_FrameBuffers.resize(m_SwapChain->GetBackBufferCount());
    CreateFrameBuffer();
    
    return true;
}

void RhiTestApp::Tick()
{
    m_CommandList->Begin();
    uint32_t currentFrame = m_SwapChain->GetCurrentBackBufferIndex();

    std::shared_ptr<RHITexture> colorAttachment = m_SwapChain->GetCurrentBackBuffer();
    m_CommandList->ResourceBarrier(colorAttachment, ERHIResourceStates::RenderTarget);
    
    m_CommandList->BeginRenderPass(m_FrameBuffers[currentFrame], &RHIClearValue::White, 1);
    m_CommandList->EndRenderPass();

    m_CommandList->ResourceBarrier(colorAttachment, ERHIResourceStates::Present);
    m_CommandList->End();

    RHI::GetDevice()->ExecuteCommandList(m_CommandList, m_Fence);
    m_Fence->CpuWait();
    m_SwapChain->Present();
}

void RhiTestApp::Shutdown()
{
    
}

void RhiTestApp::OnResize(int width, int height)
{
    m_Width = width;
    m_Height = height;
    m_DepthStencilTexture.reset();
}

void RhiTestApp::OnEndResize()
{
    m_SwapChain->Resize(m_Width, m_Height);
    
    RHITextureDesc textureDesc{};
    textureDesc.Width = m_Width;
    textureDesc.Height = m_Height;
    textureDesc.Format = ERHIFormat::D24S8;
    textureDesc.Usages = ERHITextureUsage::DepthStencil;
    m_DepthStencilTexture = RHI::GetDevice()->CreateTexture(textureDesc);

    std::shared_ptr<RHITexture> renderTargets[RHIRenderTargetsMaxCount];
    for(uint32_t i = 0; i < m_SwapChain->GetBackBufferCount(); ++i)
    {
        renderTargets[0] = m_SwapChain->GetBackBuffer(i);
        m_FrameBuffers[i]->Resize(renderTargets, m_DepthStencilTexture);
    }
}

void RhiTestApp::CreateFrameBuffer()
{
    RHITextureDesc textureDesc{};
    textureDesc.Width = m_Width;
    textureDesc.Height = m_Height;
    textureDesc.Format = ERHIFormat::D24S8;
    textureDesc.Usages = ERHITextureUsage::DepthStencil;
    m_DepthStencilTexture = RHI::GetDevice()->CreateTexture(textureDesc);

    RHIFrameBufferDesc fbDesc{};
    fbDesc.DepthStencil = m_DepthStencilTexture;
    fbDesc.NumRenderTargets = 1;
    for(uint32_t i = 0; i < m_SwapChain->GetBackBufferCount(); ++i)
    {
        fbDesc.RenderTargets[0] = m_SwapChain->GetBackBuffer(i);
        m_FrameBuffers[i] = RHI::GetDevice()->CreateFrameBuffer(fbDesc);
    }
}