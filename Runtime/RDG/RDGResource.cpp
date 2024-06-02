#include "RDGResource.h"
#include "RDGraph.h"

RDGResource::RDGResource(std::string_view inName, RDGNodeHandle inHandle, bool isExternal)
    : RDGNode(inName, inHandle), m_IsExternal(isExternal)
{
    
}

RDGBuffer::RDGBuffer(std::string_view inName, RDGNodeHandle inHandle, const RHIBufferDesc& inDesc)
    : RDGResource(inName, inHandle, false), m_Desc(inDesc)
{
    
}

RDGBuffer::~RDGBuffer()
{
    m_Buffer.SafeRelease();
}

void RDGBuffer::InitRHI()
{
    if(m_Buffer.GetReference() == nullptr || !m_Buffer->IsValid())
    {
        m_Buffer = RHI::GetDevice()->CreateBuffer(m_Desc);
        m_Buffer->SetName(m_Name);
    }
}

RDGTexture::RDGTexture(std::string_view inName, RDGNodeHandle inHandle, const RHITextureDesc& inDesc)
    : RDGResource(inName, inHandle, false), m_Desc(inDesc)
{
    
}

RDGTexture::~RDGTexture()
{
    m_Texture.SafeRelease();
}

void RDGTexture::InitRHI()
{
    if(m_Texture.GetReference() == nullptr || !m_Texture->IsValid())
    {
        m_Texture = RHI::GetDevice()->CreateTexture(m_Desc);
        m_Texture->SetName(m_Name);
    }
}