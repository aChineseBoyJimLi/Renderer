#pragma once
#include "../RHI/RHI.h"
#include "RDGDefinitions.h"
#include "RDGResource.h"
#include "RDGPass.h"

class RDGraph
{
public:
    ~RDGraph();
    bool Init();
    void Shutdown();
    
    template<typename ExecuteLambdaType>
    RDGNodeHandle AddPass(std::string_view inName, ExecuteLambdaType&& inExecuteLambda);

    template<typename ParameterStructType, typename ExecuteLambdaType>
    RDGNodeHandle AddPass(std::string_view inName, const ParameterStructType* inParameterStruct, ExecuteLambdaType&& inExecuteLambda);
    
    template<typename ResourceDescType>
    RDGNodeHandle AddResource(std::string_view inName, const ResourceDescType& inDesc);
    
    RDGNodeHandle AddResource(std::string_view inName, RHIBufferRef inBuffer);
    RDGNodeHandle AddResource(std::string_view inName, RHITextureRef inTexture);
    
    void Compile();
    void Execute();

    const RDGPass* GetPass(RDGNodeHandle inHandle) const { return m_MangedPasses[inHandle]; }
    const RDGResource* GetResource(RDGNodeHandle inHandle) const { return m_ManagedResources[inHandle]; }

    RDGraph(const RDGraph&) = delete;
    RDGraph& operator=(const RDGraph&) = delete;
    RDGraph(RDGraph&&) = delete;
    RDGraph& operator=(RDGraph&&) = delete;

    template<typename ResourceDescType>
    struct ResourceTypeTraits {};

    template<>
    struct ResourceTypeTraits<RHIBufferDesc>
    {
        using RDGResourceType = RDGBuffer;
    };

    template<>
    struct ResourceTypeTraits<RHITextureDesc>
    {
        using RDGResourceType = RDGTexture;
    };

private:
    friend bool RDG::Init();
    RDGraph() = default;
    std::vector<RDGPass*>        m_MangedPasses;
    std::vector<RDGResource*>    m_ManagedResources;
};

template<typename ExecuteLambdaType>
RDGNodeHandle RDGraph::AddPass(const std::string_view inName, ExecuteLambdaType&& inExecuteLambda)
{
    RDGNodeHandle handle = m_MangedPasses.size();
    RDGPass* pass = new RDGEmptyLambdaPass<ExecuteLambdaType>(inName, handle, std::forward<ExecuteLambdaType>(inExecuteLambda));
    m_MangedPasses.push_back(pass);
    return handle;
}

template<typename ParameterStructType, typename ExecuteLambdaType>
RDGNodeHandle RDGraph::AddPass(std::string_view inName, const ParameterStructType* inParameterStruct, ExecuteLambdaType&& inExecuteLambda)
{
    RDGNodeHandle handle = m_MangedPasses.size();
    
    return handle;
}

template<typename ResourceDescType>
RDGNodeHandle RDGraph::AddResource(std::string_view inName, const ResourceDescType& inDesc)
{
    using RDGResourceType = typename ResourceTypeTraits<ResourceDescType>::RDGResourceType;
    RDGNodeHandle handle = m_ManagedResources.size();
    RDGResource* res = new RDGResourceType(inName, handle, inDesc);
    m_ManagedResources.push_back(res);
    return handle;
}