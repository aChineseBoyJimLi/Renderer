#include "RhiTestApp.h"

#include "../Core/Log.h"

bool RhiTestApp::Init()
{
    LoadAssets();
    
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
    m_CopyCommandList = RHI::GetDevice()->CreateCommandList(ERHICommandQueueType::Copy);
    
    // Compute Pipeline
    RHIPipelineBindingLayoutDesc desc;
    desc.Items.emplace_back(ERHIBindingResourceType::Buffer_CBV, 0);
    desc.Items.emplace_back(ERHIBindingResourceType::Buffer_CBV, 1);
    desc.Items.emplace_back(ERHIBindingResourceType::Buffer_CBV, 2);
    desc.Items.emplace_back(ERHIBindingResourceType::Buffer_SRV, 0);
    desc.Items.emplace_back(ERHIBindingResourceType::Buffer_UAV, 0);
    
    m_CullingPassBindingLayout = RHI::GetDevice()->CreatePipelineBindingLayout(desc);
    m_CullingShader = RHI::GetDevice()->CreateShader(ERHIShaderType::Compute);
    m_CullingShader->SetByteCode(m_CullingShaderByteCode);
    
    RHIComputePipelineDesc cullingPassDesc;
    cullingPassDesc.ComputeShader = m_CullingShader.GetReference();
    cullingPassDesc.BindingLayout = m_CullingPassBindingLayout.GetReference();
    m_CullingPipeline = RHI::GetDevice()->CreatePipeline(cullingPassDesc);
    
    // Graphics Pipeline
    RHIPipelineBindingLayoutDesc graphicsPipelineLayoutDesc;
    graphicsPipelineLayoutDesc.Items.emplace_back(ERHIBindingResourceType::Buffer_CBV, 0);
    graphicsPipelineLayoutDesc.Items.emplace_back(ERHIBindingResourceType::Buffer_CBV, 1);
    graphicsPipelineLayoutDesc.Items.emplace_back(ERHIBindingResourceType::Buffer_SRV, 0);
    graphicsPipelineLayoutDesc.Items.emplace_back(ERHIBindingResourceType::Buffer_SRV, 1);
    graphicsPipelineLayoutDesc.Items.emplace_back(ERHIBindingResourceType::Sampler, 0);
    graphicsPipelineLayoutDesc.Items.emplace_back(ERHIBindingResourceType::Texture_SRV, 0, 1, 5);
    graphicsPipelineLayoutDesc.AllowInputLayout = true;
    graphicsPipelineLayoutDesc.IsRayTracingLocalLayout = false;
    
    m_GraphicsPassBindingLayout = RHI::GetDevice()->CreatePipelineBindingLayout(graphicsPipelineLayoutDesc);
    
    m_VertexShader = RHI::GetDevice()->CreateShader(ERHIShaderType::Vertex);
    m_PixelShader = RHI::GetDevice()->CreateShader(ERHIShaderType::Pixel);
    m_VertexShader->SetByteCode(m_VertexShaderByteCode);
    m_PixelShader->SetByteCode(m_PixelShaderByteCode);
    
    RHIVertexInputLayoutDesc vertexInputLayoutDesc;
    vertexInputLayoutDesc.Items.emplace_back("POSITION", ERHIFormat::RGB32_FLOAT);
    vertexInputLayoutDesc.Items.emplace_back("NORMAL", ERHIFormat::RGBA32_FLOAT);
    vertexInputLayoutDesc.Items.emplace_back("TEXCOORD", ERHIFormat::RG32_FLOAT);
    
    RHIGraphicsPipelineDesc graphicsPassDesc;
    graphicsPassDesc.BindingLayout = m_GraphicsPassBindingLayout;
    graphicsPassDesc.VertexInputLayout = &vertexInputLayoutDesc;
    graphicsPassDesc.VertexShader = m_VertexShader;
    graphicsPassDesc.PixelShader = m_PixelShader;
    graphicsPassDesc.PrimitiveType = ERHIPrimitiveType::TriangleList;
    graphicsPassDesc.RasterizerState = RHIRasterizerDesc();
    graphicsPassDesc.DepthStencilState = RHIDepthStencilDesc();
    graphicsPassDesc.BlendState.NumRenderTarget = 1;
    graphicsPassDesc.BlendState.AlphaToCoverageEnable = false;
    graphicsPassDesc.BlendState.Targets[0] = RHIBlendStateDesc::RenderTarget();
    graphicsPassDesc.UsingMeshShader = false;
    graphicsPassDesc.NumRenderTarget = 1;
    graphicsPassDesc.RTVFormats[0] = ERHIFormat::RGBA8_UNORM;
    graphicsPassDesc.DSVFormat = ERHIFormat::D24S8;
    m_GraphicsPipeline = RHI::GetDevice()->CreatePipeline(graphicsPassDesc);

    RHISwapChainDesc swapChainDesc{};
    swapChainDesc.Width = m_Width;
    swapChainDesc.Height = m_Height;
    swapChainDesc.Format = ERHIFormat::RGBA8_UNORM;
    swapChainDesc.BufferCount = 3;
    swapChainDesc.hwnd = m_hWnd;
    swapChainDesc.hInstance = m_hInstance;
    m_SwapChain = RHI::CreateSwapChain(swapChainDesc);
    CreateFrameBuffer();
    CreateResources();
    return true;
}

void RhiTestApp::Tick()
{
    if(!m_IsRunning)
    {
        return;
    }
    m_CommandList->Begin();
    uint32_t currentFrame = m_SwapChain->GetCurrentBackBufferIndex();
    
    RefCountPtr<RHITexture> colorAttachment = m_SwapChain->GetCurrentBackBuffer();
    m_CommandList->ResourceBarrier(colorAttachment, ERHIResourceStates::RenderTarget);

    std::vector<RHIViewport> viewports;
    viewports.push_back(RHIViewport::Create((float)m_Width, (float)m_Height));
    std::vector<RHIRect> scissorRects;
    scissorRects.push_back(RHIRect::Create(m_Width, m_Height));

    m_CommandList->SetViewports(viewports);
    m_CommandList->SetScissorRects(scissorRects);
    m_CommandList->SetVertexBuffer(m_VertexBuffer);
    m_CommandList->SetIndexBuffer(m_IndexBuffer);
    m_CommandList->SetPipelineState(m_GraphicsPipeline);
    m_CommandList->SetFrameBuffer(m_FrameBuffers[currentFrame], &RHIClearValue::White, 1);
    
    m_CommandList->ResourceBarrier(colorAttachment, ERHIResourceStates::Present);
    m_CommandList->End();
    
    RHI::GetDevice()->ExecuteCommandList(m_CommandList, m_Fence);
    m_SwapChain->Present();
    m_Fence->CpuWait();
}

void RhiTestApp::Shutdown()
{
    m_DepthStencilTexture.SafeRelease();
    for(uint32_t i = 0; i < m_FrameBuffers.size(); ++i)
    {
        m_FrameBuffers[i]->Shutdown();
        m_FrameBuffers[i].SafeRelease();
    }
    m_FrameBuffers.clear();
}

void RhiTestApp::OnBeginResize()
{
    // m_DepthStencilTexture.SafeRelease();
    // for(auto frameBuffer : m_FrameBuffers)
    // {
    //     frameBuffer.SafeRelease();
    // }
}

void RhiTestApp::OnResize(int width, int height)
{
    // m_Width = width;
    // m_Height = height;
}

void RhiTestApp::OnEndResize()
{
    // m_SwapChain->Resize(m_Width, m_Height);
    // CreateFrameBuffer();
}

void RhiTestApp::CreateFrameBuffer()
{
    RHITextureDesc textureDesc{};
    textureDesc.Width = m_Width;
    textureDesc.Height = m_Height;
    textureDesc.Format = ERHIFormat::D24S8;
    textureDesc.Usages = ERHITextureUsage::DepthStencil;
    textureDesc.ClearValue.DepthStencil.Depth = 1;
    textureDesc.ClearValue.DepthStencil.Stencil = 0;
    m_DepthStencilTexture = RHI::GetDevice()->CreateTexture(textureDesc);

    m_FrameBuffers.resize(m_SwapChain->GetBackBufferCount());
    RHIFrameBufferDesc fbDesc{};
    fbDesc.DepthStencil = m_DepthStencilTexture;
    fbDesc.PipelineState = m_GraphicsPipeline;
    for(uint32_t i = 0; i < m_SwapChain->GetBackBufferCount(); ++i)
    {
        fbDesc.RenderTargets[0] = m_SwapChain->GetBackBuffer(i);
        m_FrameBuffers[i] = RHI::GetDevice()->CreateFrameBuffer(fbDesc);
    }
}

void RhiTestApp::LoadAssets()
{
    m_CullingShaderByteCode = AssetsManager::LoadShaderImmediately("VisibleCullingVk.cs");
    m_VertexShaderByteCode = AssetsManager::LoadShaderImmediately("Graphics.vs");
    m_PixelShaderByteCode = AssetsManager::LoadShaderImmediately("Graphics.ps");
    m_Mesh = AssetsManager::LoadMeshImmediately("sphere.fbx");
}

struct VertexData
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoord;
};

void RhiTestApp::CreateResources()
{
    std::vector<VertexData> verticesData(m_Mesh->GetVerticesCount());
    const aiMesh* mesh = m_Mesh->GetMesh();
    for(uint32_t i = 0; i < m_Mesh->GetVerticesCount(); ++i)
    {
        verticesData[i].Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        verticesData[i].Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        verticesData[i].TexCoord = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
    }

    const size_t vertexBufferSize = m_Mesh->GetVerticesCount() * sizeof(VertexData);
    RHIBufferDesc vbDesc;
    vbDesc.Usages = ERHIBufferUsage::VertexBuffer;
    vbDesc.Size = vertexBufferSize;
    vbDesc.Stride = sizeof(VertexData);
    m_VertexBuffer = RHI::GetDevice()->CreateBuffer(vbDesc);
    
    RHIBufferDesc stagingBufferDesc;
    stagingBufferDesc.CpuAccess = ERHICpuAccessMode::Write;
    stagingBufferDesc.Usages = ERHIBufferUsage::None;
    stagingBufferDesc.Size = vertexBufferSize;
    RefCountPtr<RHIBuffer> vbStagingBuffer = RHI::GetDevice()->CreateBuffer(stagingBufferDesc);
    vbStagingBuffer->WriteData(verticesData.data(), vertexBufferSize);

    RHIBufferDesc ibDesc;
    ibDesc.Usages = ERHIBufferUsage::IndexBuffer;
    ibDesc.Size = m_Mesh->GetIndicesDataByteSize();
    ibDesc.Format = ERHIFormat::R32_UINT;
    m_IndexBuffer = RHI::GetDevice()->CreateBuffer(ibDesc);

    stagingBufferDesc.Size = m_Mesh->GetIndicesDataByteSize();
    RefCountPtr<RHIBuffer> ibStagingBuffer = RHI::GetDevice()->CreateBuffer(stagingBufferDesc);
    ibStagingBuffer->WriteData(m_Mesh->GetIndicesData(), m_Mesh->GetIndicesDataByteSize());

    m_CopyCommandList->Begin();
    // m_CopyCommandList->ResourceBarrier(m_VertexBuffer, ERHIResourceStates::CopyDst);
    m_CopyCommandList->CopyBuffer(m_VertexBuffer, 0, vbStagingBuffer, 0, vertexBufferSize);
    m_CopyCommandList->CopyBuffer(m_IndexBuffer, 0, ibStagingBuffer, 0, m_Mesh->GetIndicesDataByteSize());
    m_CopyCommandList->End();

    RHI::GetDevice()->ExecuteCommandList(m_CopyCommandList, m_Fence);
    m_Fence->CpuWait();
}