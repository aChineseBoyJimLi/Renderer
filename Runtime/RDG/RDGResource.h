#pragma once
#include "RDGDefinitions.h"
#include "../RHI/RHI.h"

class RDGraph;

class RDGResource : public RDGNode
{
public:
    virtual RHIObject* GetRHI() const = 0;
    virtual void InitRHI() = 0;
    
protected:
    friend RDGraph;
    RDGResource(std::string_view inName, RDGNodeHandle inHandle, bool isExternal);
    
    const bool m_IsExternal;
    std::vector<RDGNodeHandle> m_Producers;
    std::vector<RDGNodeHandle> m_Consumers;
};

class RDGBuffer : public RDGResource
{
public:
    ~RDGBuffer() override;
    RHIObject* GetRHI() const override { return m_Buffer.GetReference(); }
    void InitRHI();
    
private:
    friend RDGraph;
    RDGBuffer(std::string_view inName, RDGNodeHandle inHandle, const RHIBufferDesc& inDesc);
    RHIBufferDesc m_Desc;
    RHIBufferRef m_Buffer;
};

class RDGTexture : public RDGResource
{
public:
    ~RDGTexture() override;
    RHIObject* GetRHI() const override { return m_Texture.GetReference(); }
    void InitRHI();
    
private:
    friend RDGraph;
    RDGTexture(std::string_view inName, RDGNodeHandle inHandle, const RHITextureDesc& inDesc);
    RHITextureDesc m_Desc;
    RHITextureRef m_Texture;
};