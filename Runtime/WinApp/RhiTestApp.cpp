#include "RhiTestApp.h"
#include <random>
#include "Camera.h"
#include "../Transform.h"
#include "../Core/Log.h"
#include "Light.h"

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
    // CreateResources();
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

    m_CommandList->SetPipelineState(m_GraphicsPipeline);
    m_CommandList->SetFrameBuffer(m_FrameBuffers[currentFrame], &RHIClearValue::White, 1);
    
    
    m_CommandList->SetViewports(viewports);
    m_CommandList->SetScissorRects(scissorRects);
    // m_CommandList->SetVertexBuffer(m_VertexBuffer);
    // m_CommandList->SetIndexBuffer(m_IndexBuffer);
    // m_CommandList->SetResourceSet(m_ResourceSet);
    // m_CommandList->DrawIndexedIndirect(m_IndirectDrawCommandsBuffer, 1);
    m_CommandList->ResourceBarrier(colorAttachment, ERHIResourceStates::Present);
    m_CommandList->End();
    
    // RHI::GetDevice()->ExecuteCommandList(m_CommandList, m_Fence);
    // m_SwapChain->Present();
    // m_Fence->CpuWait();
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
    m_Textures[0] = AssetsManager::LoadTextureImmediately("3DLABbg_UV_Map_Checker_01_1024x1024.jpg");
    m_Textures[1] = AssetsManager::LoadTextureImmediately("3DLABbg_UV_Map_Checker_02_1024_1024.jpg");
    m_Textures[2] = AssetsManager::LoadTextureImmediately("3DLABbg_UV_Map_Checker_03_1024_1024.jpg");
    m_Textures[3] = AssetsManager::LoadTextureImmediately("3DLABbg_UV_Map_Checker_04_1024_1024.jpg");
    m_Textures[4] = AssetsManager::LoadTextureImmediately("3DLABbg_UV_Map_Checker_05_1024_1024.jpg");
}

struct VertexData
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoord;
};

struct MaterialData
{
    glm::vec4 Color;
    float Smooth;
    uint32_t TexIndex;
    uint32_t Padding0;
    uint32_t Padding1;
};

struct InstanceData
{
    glm::mat4 LocalToWorld;
    glm::mat4 WorldToLocal;
    uint32_t MaterialIndex;
    uint32_t Padding0;
    uint32_t Padding1;
    uint32_t Padding2;
};

void RhiTestApp::CreateResources()
{
    // Camera Data
    CameraPerspective camera;
    camera.AspectRatio = static_cast<float>(m_Width) / static_cast<float>(m_Height);
    camera.Transform.SetWorldPosition(glm::vec3(0, 20, 20));
    camera.Transform.LookAt(glm::vec3(0, 0, 0));
    RHIBufferDesc cameraDataBufferDesc = RHIBufferDesc::ConstantsBuffer(CameraData::GetAlignedByteSizes());
    CameraData cameraData;
    camera.GetCameraData(cameraData);
    m_CameraDataBuffer = RHI::GetDevice()->CreateBuffer(cameraDataBufferDesc);
    m_CameraDataBuffer->WriteData(&cameraData, CameraData::GetAlignedByteSizes());

    // Light Data
    Light light;
    light.Transform.SetWorldForward(glm::vec3(-1,-1,-1));
    light.Color = glm::vec3(1, 1, 1);
    light.Intensity = 1.0f;
    DirectionalLightData lightData;
    lightData.LightColor = light.Color;
    lightData.LightDirection = light.Transform.GetWorldForward();
    lightData.LightIntensity = light.Intensity;
    RHIBufferDesc lightDataBufferDesc = RHIBufferDesc::ConstantsBuffer(DirectionalLightData::GetAlignedByteSizes());
    m_LightDataBuffer = RHI::GetDevice()->CreateBuffer(lightDataBufferDesc);
    m_LightDataBuffer->WriteData(&lightData, DirectionalLightData::GetAlignedByteSizes());
    
    // Vertex Data
    std::vector<VertexData> verticesData(m_Mesh->GetVerticesCount());
    const aiMesh* mesh = m_Mesh->GetMesh();
    for(uint32_t i = 0; i < m_Mesh->GetVerticesCount(); ++i)
    {
        verticesData[i].Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        verticesData[i].Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        verticesData[i].TexCoord = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis01(0, 1.0f);
    std::uniform_real_distribution<float> dis11(-1.0f, 1.0f);
    
    // Material Data
    std::array<MaterialData, s_MaterialCount> materialsData;
    glm::vec3 zoom(s_InstanceCountX * 2.5f, s_InstanceCountY * 2.5f, s_InstanceCountZ * 2.5f) ;
    
    for(uint32_t i = 0; i < s_MaterialCount; ++i)
    {
        // random generate material data
        materialsData[i].Color = glm::vec4(1);
        materialsData[i].Smooth = dis01(gen) * 5.0f + 0.5f;
        materialsData[i].TexIndex = static_cast<uint32_t>(s_TexturesCount * dis01(gen));
    }

    // Instance Data
    std::array<InstanceData, s_InstancesCount> instancesData;
    for(uint32_t x = 0; x < s_InstanceCountX; x++)
    {
        for(uint32_t y = 0; y < s_InstanceCountY; y++)
        {
            for(uint32_t z = 0; z < s_InstanceCountZ; z++)
            {
                uint32_t i = x * s_InstanceCountY * s_InstanceCountZ + y * s_InstanceCountZ + z;
                Transform transform;
                glm::vec3 pos((float)x / (float)s_InstanceCountX, (float)y / (float)s_InstanceCountY, (float)z / (float)s_InstanceCountZ);
                transform.SetWorldPosition(glm::mix(-zoom, zoom, pos));
                transform.SetLocalRotation(glm::vec3(dis11(rd), dis11(rd), dis11(rd)));
                
                instancesData[i].LocalToWorld = transform.GetLocalToWorldMatrix();
                instancesData[i].WorldToLocal = transform.GetWorldToLocalMatrix();
                instancesData[i].MaterialIndex = i % s_MaterialCount;
                
                
            }
        }
    }

    m_IndirectDrawCommands[0].IndexCount = m_Mesh->GetIndicesCount();
    m_IndirectDrawCommands[0].InstanceCount = s_InstancesCount;
    m_IndirectDrawCommands[0].FirstIndex = 0;
    m_IndirectDrawCommands[0].VertexOffset = 0;
    m_IndirectDrawCommands[0].FirstInstance = 0;

    // Vertex Buffer
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

    // Index Buffer
    RHIBufferDesc ibDesc;
    ibDesc.Usages = ERHIBufferUsage::IndexBuffer;
    ibDesc.Size = m_Mesh->GetIndicesDataByteSize();
    ibDesc.Format = ERHIFormat::R32_UINT;
    m_IndexBuffer = RHI::GetDevice()->CreateBuffer(ibDesc);

    stagingBufferDesc.Size = m_Mesh->GetIndicesDataByteSize();
    RefCountPtr<RHIBuffer> ibStagingBuffer = RHI::GetDevice()->CreateBuffer(stagingBufferDesc);
    ibStagingBuffer->WriteData(m_Mesh->GetIndicesData(), m_Mesh->GetIndicesDataByteSize());

    // Material Data Buffer
    const size_t materialsBufferBytesSize = s_MaterialCount * sizeof(MaterialData);
    RHIBufferDesc mdBuffer;
    mdBuffer.Usages = ERHIBufferUsage::ShaderResource;
    mdBuffer.CpuAccess = ERHICpuAccessMode::Read;
    mdBuffer.Size = materialsBufferBytesSize;
    mdBuffer.Stride = sizeof(MaterialData);
    m_MaterialBuffer = RHI::GetDevice()->CreateBuffer(mdBuffer);

    stagingBufferDesc.Size = materialsBufferBytesSize;
    RefCountPtr<RHIBuffer> mdStagingBuffer = RHI::GetDevice()->CreateBuffer(stagingBufferDesc);
    mdStagingBuffer->WriteData(materialsData.data(), materialsBufferBytesSize);

    // Instance Data Buffer
    const size_t instancesBufferBytesSize = s_InstancesCount * sizeof(InstanceData);
    RHIBufferDesc idDesc;
    idDesc.Usages = ERHIBufferUsage::ShaderResource;
    idDesc.CpuAccess = ERHICpuAccessMode::Read;
    idDesc.Size = instancesBufferBytesSize;
    idDesc.Stride = sizeof(InstanceData);
    m_InstanceBuffer = RHI::GetDevice()->CreateBuffer(idDesc);
    
    stagingBufferDesc.Size = instancesBufferBytesSize;
    RefCountPtr<RHIBuffer> idStagingBuffer = RHI::GetDevice()->CreateBuffer(stagingBufferDesc);
    idStagingBuffer->WriteData(instancesData.data(), instancesBufferBytesSize);

    // Indirect Draw Commands Buffer
    const size_t indirectCommandsBufferBytesSize = s_InstancesCount * sizeof(RHIDrawIndexedArguments);
    RHIBufferDesc idcDesc;
    idcDesc.Usages = ERHIBufferUsage::IndirectCommands;
    idcDesc.Size = indirectCommandsBufferBytesSize;
    m_IndirectDrawCommandsBuffer = RHI::GetDevice()->CreateBuffer(idcDesc);

    stagingBufferDesc.Size = indirectCommandsBufferBytesSize;
    RefCountPtr<RHIBuffer> idcStagingBuffer = RHI::GetDevice()->CreateBuffer(stagingBufferDesc);
    idcStagingBuffer->WriteData(m_IndirectDrawCommands.data(), indirectCommandsBufferBytesSize);
    

    std::array<RefCountPtr<RHIBuffer>, s_TexturesCount> mainTextureStagingBuffers;
    // Main Textures
    for(uint32_t i = 0; i < 5; i++)
    {
        RHITextureDesc desc;
        desc.Width = (uint32_t)m_Textures[i]->GetTextureDesc().width;
        desc.Height = (uint32_t)m_Textures[i]->GetTextureDesc().height;
        desc.Depth = (uint32_t)m_Textures[i]->GetTextureDesc().depth;
        desc.Dimension = ERHITextureDimension::Texture2D;
        desc.Format = ERHIFormat::RGBA8_UNORM;
        desc.Usages = ERHITextureUsage::ShaderResource;
        desc.ArraySize = 1;
        desc.MipLevels = 1;
        desc.SampleCount = 1;
        m_MainTextures[i] = RHI::GetDevice()->CreateTexture(desc);
        
        stagingBufferDesc.Size = m_Textures[i]->GetScratchImage().GetPixelsSize();
        mainTextureStagingBuffers[i] = RHI::GetDevice()->CreateBuffer(stagingBufferDesc);
        mainTextureStagingBuffers[i]->WriteData(m_Textures[i]->GetScratchImage().GetPixels(), m_Textures[i]->GetScratchImage().GetPixelsSize(), 0);
    }

    // Sampler
    RHISamplerDesc samplerDesc;
    m_Sampler = RHI::GetDevice()->CreateSampler(samplerDesc);
    
    m_CopyCommandList->Begin();
    m_CopyCommandList->CopyBuffer(m_VertexBuffer, 0, vbStagingBuffer, 0, vertexBufferSize);
    m_CopyCommandList->CopyBuffer(m_IndexBuffer, 0, ibStagingBuffer, 0, m_Mesh->GetIndicesDataByteSize());
    m_CopyCommandList->CopyBuffer(m_MaterialBuffer, 0, mdStagingBuffer, 0, materialsBufferBytesSize);
    m_CopyCommandList->CopyBuffer(m_InstanceBuffer, 0, idStagingBuffer, 0, instancesBufferBytesSize);
    // m_CopyCommandList->CopyBuffer(m_IndirectDrawCommandsBuffer, 0, idcStagingBuffer, 0, indirectCommandsBufferBytesSize);
    
    for(uint32_t i = 0; i < 5; i++)
    {
        m_CopyCommandList->CopyBufferToTexture(m_MainTextures[i], mainTextureStagingBuffers[i]);
    }
    
    m_CopyCommandList->End();
    
    RHI::GetDevice()->ExecuteCommandList(m_CopyCommandList, m_Fence);
    m_Fence->CpuWait();

    m_CommandList->Begin();
    // m_CommandList->ResourceBarrier(m_IndirectDrawCommandsBuffer, ERHIResourceStates::CopyDst);
    // m_CommandList->CopyBuffer(m_VertexBuffer, 0, vbStagingBuffer, 0, vertexBufferSize);
    // m_CommandList->CopyBuffer(m_IndexBuffer, 0, ibStagingBuffer, 0, m_Mesh->GetIndicesDataByteSize());
    // m_CommandList->CopyBuffer(m_MaterialBuffer, 0, mdStagingBuffer, 0, materialsBufferBytesSize);
    // m_CommandList->CopyBuffer(m_InstanceBuffer, 0, idStagingBuffer, 0, instancesBufferBytesSize);
    m_CommandList->CopyBuffer(m_IndirectDrawCommandsBuffer, 0, idcStagingBuffer, 0, indirectCommandsBufferBytesSize);
    m_CommandList->ResourceBarrier(m_IndirectDrawCommandsBuffer, ERHIResourceStates::UnorderedAccess);
    // for(uint32_t i = 0; i < 5; i++)
    // {
    //     m_CommandList->CopyBufferToTexture(m_MainTextures[i], mainTextureStagingBuffers[i]);
    // }
    m_CommandList->End();
    RHI::GetDevice()->ExecuteCommandList(m_CommandList, m_Fence);
    m_Fence->CpuWait();

    m_ResourceSet = RHI::GetDevice()->CreateResourceSet(m_GraphicsPassBindingLayout.GetReference());

    m_ResourceSet->BindBufferCBV(0, 0, m_CameraDataBuffer);
    m_ResourceSet->BindBufferCBV(1, 0, m_LightDataBuffer);
    m_ResourceSet->BindBufferSRV(0, 0, m_InstanceBuffer);
    m_ResourceSet->BindBufferSRV(1, 0, m_MaterialBuffer);
    m_ResourceSet->BindSampler(0, 0, m_Sampler);
    for(uint32_t i = 0; i < s_TexturesCount; i++)
    {
        m_ResourceSet->BindTextureSRV(i, 1, m_MainTextures[i]);
    }
}