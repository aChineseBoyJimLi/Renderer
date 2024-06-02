#include "TestPass.h"

#include "../Core/Log.h"



void TestPass::Setup()
{
    RHIBufferDesc desc = RHIBufferDesc::ConstantsBuffer(1024);
    RHITextureDesc texDesc = RHITextureDesc::Texture2D(512, 512, ERHIFormat::RGBA8_UNORM);

    
    m_Texture = RDG::GetGraph()->AddResource("tex 0", texDesc);
    m_ConstantBuffer = RDG::GetGraph()->AddResource("constant 0", desc);
    
    m_PassName = "Test Pass";
    std::string& passName = m_PassName;
    RDG::GetGraph()->AddPass(m_PassName, [&passName]()
    {
        // Log::Info("[RDG] %s\n", passName.c_str());
    });
}
