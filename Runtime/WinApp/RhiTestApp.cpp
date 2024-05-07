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
    
    return true;
}

void RhiTestApp::Tick()
{
    m_CommandList->Begin();

    m_CommandList->End();
}

void RhiTestApp::Shutdown()
{
    
}

void RhiTestApp::OnResize(int width, int height)
{
    m_Width = width;
    m_Height = height;
    if(m_IsRunning)
    {
        Log::Info("Width:%d, Height:%d", m_Width, m_Height);
    }
    else
    {
        Log::Info("Minimized");
    }
}