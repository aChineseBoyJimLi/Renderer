#pragma once
#include "../RDG/RDG.h"
#include "../RHI/RHI.h"

class TestPass
{
public:
    void Setup();

    struct TestPassParameter
    {
        
    };
    
private:
    RHIPipelineBindingLayoutRef m_PipelineLayout;
    RHIGraphicsPipelineRef m_PipelineState;
    std::string m_PassName;
    // RDGNodeHandle m_VertexBuffer;
    // RDGNodeHandle m_IndexBuffer;
    RDGNodeHandle m_ConstantBuffer;
    RDGNodeHandle m_Texture;
    // RDGNodeHandle m_Sampler;
    // RDGNodeHandle m_RenderTarget;
    // RDGNodeHandle m_DepthStencil;
    // RDGNodeHandle m_PassHandle;
};