#include "ImguiTestApp.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

bool ImGuiTestApp::Init()
{
    InitImGui();
    LoadAssets();

    m_Fence = RHI::GetDevice()->CreateRhiFence();
    m_Semaphore = RHI::GetDevice()->CreateRhiSemaphore();
    m_CommandList = RHI::GetDevice()->CreateCommandList();
    m_CopyCommandList = RHI::GetDevice()->CreateCommandList(ERHICommandQueueType::Copy);

    RHIPipelineBindingLayoutDesc bindingLayoutDesc;
    bindingLayoutDesc.Items.emplace_back(ERHIBindingResourceType::Buffer_CBV, 0);
    bindingLayoutDesc.Items.emplace_back(ERHIBindingResourceType::Sampler, 0);
    bindingLayoutDesc.Items.emplace_back(ERHIBindingResourceType::Texture_SRV, 0);
    bindingLayoutDesc.AllowInputLayout = true;
    bindingLayoutDesc.IsRayTracingLocalLayout = false;

    m_GraphicsPassBindingLayout = RHI::GetDevice()->CreatePipelineBindingLayout(bindingLayoutDesc);
    
    m_VertexShader = RHI::GetDevice()->CreateShader(ERHIShaderType::Vertex);
    m_PixelShader = RHI::GetDevice()->CreateShader(ERHIShaderType::Pixel);
    m_VertexShader->SetByteCode(m_VertexShaderByteCode);
    m_PixelShader->SetByteCode(m_PixelShaderByteCode);

    RHIVertexInputLayoutDesc vertexInputLayoutDesc;
    vertexInputLayoutDesc.Items.emplace_back("POSITION", ERHIFormat::RG32_FLOAT);
    vertexInputLayoutDesc.Items.emplace_back("TEXCOORD", ERHIFormat::RG32_FLOAT);
    vertexInputLayoutDesc.Items.emplace_back("COLOR", ERHIFormat::RGBA8_UNORM);

    RHIGraphicsPipelineDesc graphicsPassDesc;
    graphicsPassDesc.BindingLayout = m_GraphicsPassBindingLayout;
    graphicsPassDesc.VertexInputLayout = &vertexInputLayoutDesc;
    graphicsPassDesc.VertexShader = m_VertexShader;
    graphicsPassDesc.PixelShader = m_PixelShader;
    graphicsPassDesc.PrimitiveType = ERHIPrimitiveType::TriangleList;
    graphicsPassDesc.RasterizerState.CullMode = ERHIRasterCullMode::None;
    graphicsPassDesc.RasterizerState.FillMode = ERHIRasterFillMode::Solid;
    graphicsPassDesc.RasterizerState.DepthClipEnable = false;
    graphicsPassDesc.DepthStencilState.DepthTestEnable = true;
    graphicsPassDesc.DepthStencilState.DepthWriteEnable = false;
    graphicsPassDesc.DepthStencilState.DepthFunc = ERHIComparisonFunc::Less;
    graphicsPassDesc.BlendState.NumRenderTarget = 1;
    graphicsPassDesc.BlendState.AlphaToCoverageEnable = false;
    graphicsPassDesc.BlendState.Targets[0].BlendEnable = true;
    graphicsPassDesc.BlendState.Targets[0].ColorSrcBlend = ERHIBlendFactor::SrcAlpha;
    graphicsPassDesc.BlendState.Targets[0].ColorDstBlend = ERHIBlendFactor::OneMinusSrcAlpha;
    graphicsPassDesc.BlendState.Targets[0].ColorBlendOp = ERHIBlendOp::Add;
    graphicsPassDesc.BlendState.Targets[0].AlphaSrcBlend = ERHIBlendFactor::One;
    graphicsPassDesc.BlendState.Targets[0].AlphaDstBlend = ERHIBlendFactor::Zero;
    graphicsPassDesc.BlendState.Targets[0].AlphaBlendOp = ERHIBlendOp::Add;
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

bool ImGuiTestApp::InitImGui()
{
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    
    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    return true;
}

void ImGuiTestApp::Tick()
{
    if(!m_IsRunning)
    {
        return;
    }

    
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(float(m_Width), float(m_Height));
    // io.DisplayFramebufferScale.x = 1.0f / 96.0f;
    // io.DisplayFramebufferScale.y = 1.0f / 96.0f;
    io.DisplayFramebufferScale.x = 1.0f;
    io.DisplayFramebufferScale.y = 1.0f;

    // io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
    // io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
    // io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
    // io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];

    io.DeltaTime = (float)GetTimer().DeltaTime();
    io.MouseDrawCursor = false;
    
    ImGui::NewFrame();
    // BuildUI();
    ImGui::Begin("Test Window");
    ImGui::Text("Renderer: %s", "Imgui");
    ImGui::End();
    ImGui::Render();

    ImDrawData *drawData = ImGui::GetDrawData();
    
    glm::vec2 invDisplaySize( 1.f / io.DisplaySize.x, 1.f / io.DisplaySize.y );
    m_ConstantsBuffer->WriteData(&invDisplaySize, sizeof(glm::vec2));
    
    ReallocateBuffer(m_VertexBuffer, drawData->TotalVtxCount * sizeof(ImDrawVert), 
        (drawData->TotalVtxCount + 5000) * sizeof(ImDrawVert));
    ReallocateBuffer(m_IndexBuffer,
        drawData->TotalIdxCount * sizeof(ImDrawIdx),
        (drawData->TotalIdxCount + 5000) * sizeof(ImDrawIdx));
    m_VtxBuffer.resize(m_VertexBuffer->GetDesc().Size / sizeof(ImDrawVert));
    m_IdxBuffer.resize(m_IndexBuffer->GetDesc().Size / sizeof(ImDrawIdx));

    // copy and convert all vertices into a single contiguous buffer
    ImDrawVert *vtxDst = &m_VtxBuffer[0];
    ImDrawIdx *idxDst = &m_IdxBuffer[0];

    for(int n = 0; n < drawData->CmdListsCount; n++)
    {
        const ImDrawList *cmdList = drawData->CmdLists[n];

        memcpy(vtxDst, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(idxDst, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));

        vtxDst += cmdList->VtxBuffer.Size;
        idxDst += cmdList->IdxBuffer.Size;
    }
    
    RHIBufferRef vertexStagingBuffer = RHI::GetDevice()->CreateBuffer(RHIBufferDesc::StagingBuffer(m_VertexBuffer->GetDesc().Size));
    RHIBufferRef indexStagingBuffer = RHI::GetDevice()->CreateBuffer(RHIBufferDesc::StagingBuffer(m_IndexBuffer->GetDesc().Size));
    vertexStagingBuffer->WriteData(m_VtxBuffer.data(), m_VertexBuffer->GetDesc().Size);
    indexStagingBuffer->WriteData(m_IdxBuffer.data(), m_IndexBuffer->GetDesc().Size);
    m_CopyCommandList->Begin();
    m_CopyCommandList->CopyBuffer(m_VertexBuffer, 0, vertexStagingBuffer, 0, m_VertexBuffer->GetDesc().Size);
    m_CopyCommandList->CopyBuffer(m_IndexBuffer, 0, indexStagingBuffer, 0, m_IndexBuffer->GetDesc().Size);
    m_CopyCommandList->End();
    RHI::GetDevice()->AddQueueSignalSemaphore(ERHICommandQueueType::Copy, m_Semaphore);
    RHI::GetDevice()->ExecuteCommandList(m_CopyCommandList);

    drawData->ScaleClipRects(io.DisplayFramebufferScale);
    
    m_CommandList->Begin();
    uint32_t currentFrame = m_SwapChain->GetCurrentBackBufferIndex();

    RefCountPtr<RHITexture> colorAttachment = m_SwapChain->GetCurrentBackBuffer();
    m_CommandList->ResourceBarrier(colorAttachment, ERHIResourceStates::RenderTarget);

    std::vector<RHIViewport> viewports;
    viewports.push_back(RHIViewport::Create((float)m_Width, (float)m_Height));
    std::vector<RHIRect> scissorRects;
    scissorRects.push_back(RHIRect::Create(m_Width, m_Height));

    RHIClearValue clearColor(0.45f, 0.55f, 0.60f, 1.00f);
    
    // Render GUI
    m_CommandList->BeginMark("ImGUI");

    // ImDrawData *drawData = ImGui::GetDrawData();
    m_CommandList->SetPipelineState(m_GraphicsPipeline);
    m_CommandList->SetFrameBuffer(m_FrameBuffers[currentFrame], &clearColor, 1);
    m_CommandList->SetViewports(viewports);
    m_CommandList->SetScissorRects(scissorRects);
    m_CommandList->SetVertexBuffer(m_VertexBuffer);
    m_CommandList->SetIndexBuffer(m_IndexBuffer);
    m_CommandList->SetResourceSet(m_ResourceSet);
    // render command lists
    int vtxOffset = 0;
    int idxOffset = 0;
    for(int n = 0; n < drawData->CmdListsCount; n++)
    {
        const ImDrawList *cmdList = drawData->CmdLists[n];
        for(int i = 0; i < cmdList->CmdBuffer.Size; i++)
        {
            const ImDrawCmd *pCmd = &cmdList->CmdBuffer[i];
            if (pCmd->UserCallback)
            {
                pCmd->UserCallback(cmdList, pCmd);
            }
            else
            {
                
                m_CommandList->DrawIndexed(pCmd->ElemCount, 1, idxOffset, vtxOffset, 0);
            }

            idxOffset += pCmd->ElemCount;
        }

        vtxOffset += cmdList->VtxBuffer.Size;
    }
    
    m_CommandList->EndMark();

    m_CommandList->ResourceBarrier(colorAttachment, ERHIResourceStates::Present);
    m_CommandList->End();
    RHI::GetDevice()->AddQueueWaitForSemaphore(ERHICommandQueueType::Direct, m_Semaphore);
    RHI::GetDevice()->ExecuteCommandList(m_CommandList, m_Fence);
    m_Fence->CpuWait();
    m_SwapChain->Present();
}

void ImGuiTestApp::Shutdown()
{
    m_DepthStencilTexture.SafeRelease();
    for(uint32_t i = 0; i < m_FrameBuffers.size(); ++i)
    {
        m_FrameBuffers[i]->Shutdown();
        m_FrameBuffers[i].SafeRelease();
    }
    m_FrameBuffers.clear();
    ShutdownImGui();
}

void ImGuiTestApp::ShutdownImGui()
{
    ImGui::DestroyContext();
}

void ImGuiTestApp::OnBeginResize()
{
    
}

void ImGuiTestApp::OnResize(int width, int height)
{
    
}

void ImGuiTestApp::OnEndResize()
{
    
}

void ImGuiTestApp::LoadAssets()
{
    m_VertexShaderByteCode = AssetsManager::LoadShaderImmediately("ImGui.vs");
    m_PixelShaderByteCode = AssetsManager::LoadShaderImmediately("ImGui.ps");
    // m_Font = AssetsManager::LoadFontImmediately("OpenSans-Regular.ttf");
}

void ImGuiTestApp::CreateFrameBuffer()
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

void ImGuiTestApp::CreateResources()
{
    RHIBufferDesc vertexBufferDesc = RHIBufferDesc::VertexBuffer(1024, sizeof(ImDrawVert));
    m_VertexBuffer = RHI::GetDevice()->CreateBuffer(vertexBufferDesc);
    RHIBufferDesc indexBufferDesc = RHIBufferDesc::IndexBuffer(1024, ERHIFormat::R16_UINT);
    m_IndexBuffer = RHI::GetDevice()->CreateBuffer(indexBufferDesc);

    RHIBufferDesc constantsBufferDesc = RHIBufferDesc::ConstantsBuffer(sizeof(glm::vec2));
    m_ConstantsBuffer = RHI::GetDevice()->CreateBuffer(constantsBufferDesc);

    ImFontConfig fontConfig;
    fontConfig.FontDataOwnedByAtlas = false;
    // ImFont* imFont = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
    //     (void*)m_Font->GetData(), (int)(m_Font->GetSize()), 14.f, &fontConfig);
    
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    // io.FontDefault = imFont;
    
    unsigned char *pixels;
    int width, height;
    io.Fonts->AddFontDefault();
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    

    RHITextureDesc textureDesc{};
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.Format = ERHIFormat::RGBA8_UNORM;
    textureDesc.Usages = ERHITextureUsage::ShaderResource;
    textureDesc.Dimension = ERHITextureDimension::Texture2D;
    textureDesc.ArraySize = 1;
    textureDesc.MipLevels = 1;
    textureDesc.SampleCount = 1;
    m_FontTexture = RHI::GetDevice()->CreateTexture(textureDesc);
    
    io.Fonts->TexID = m_FontTexture.GetReference();

    RHIBufferDesc textureStagingBufferDesc = RHIBufferDesc::StagingBuffer(width * height * 4);
    RHIBufferRef textureStagingBuffer = RHI::GetDevice()->CreateBuffer(textureStagingBufferDesc);
    textureStagingBuffer->WriteData(pixels, width * height * 4);

    RHISamplerDesc samplerDesc;
    m_FontSampler = RHI::GetDevice()->CreateSampler(samplerDesc);
    
    m_CopyCommandList->Begin();
    m_CopyCommandList->CopyBufferToTexture(m_FontTexture, textureStagingBuffer);
    m_CopyCommandList->End();
    
    RHI::GetDevice()->AddQueueSignalSemaphore(ERHICommandQueueType::Copy, m_Semaphore);
    RHI::GetDevice()->ExecuteCommandList(m_CopyCommandList);
    
    m_CommandList->Begin();
    m_CommandList->ResourceBarrier(m_FontTexture, ERHIResourceStates::GpuReadOnly);
    m_CommandList->End();
    RHI::GetDevice()->AddQueueWaitForSemaphore(ERHICommandQueueType::Direct, m_Semaphore);
    RHI::GetDevice()->ExecuteCommandList(m_CommandList, m_Fence);
    m_Fence->CpuWait();

    m_ResourceSet = RHI::GetDevice()->CreateResourceSet(m_GraphicsPassBindingLayout);
    m_ResourceSet->BindBufferCBV(0, 0, m_ConstantsBuffer);
    m_ResourceSet->BindSampler(0, 0, m_FontSampler);
    m_ResourceSet->BindTextureSRV(0, 0, m_FontTexture);
}

void ImGuiTestApp::BuildUI()
{
    
}

void ImGuiTestApp::UpdateGeometry()
{
    
}

void ImGuiTestApp::ReallocateBuffer(RHIBufferRef& inBuffer, size_t requiredSize, size_t reallocateSize)
{
    if(inBuffer.GetReference() && inBuffer->IsValid())
    {
        RHIBufferDesc desc = inBuffer->GetDesc();
        if(desc.Size < requiredSize)
        {
            desc.Size = reallocateSize;
            inBuffer.SafeRelease();
        }
        inBuffer = RHI::GetDevice()->CreateBuffer(desc);
    }
}