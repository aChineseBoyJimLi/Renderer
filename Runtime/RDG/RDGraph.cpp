#include "RDGraph.h"

RDGraph::~RDGraph()
{
    
}

bool RDGraph::Init()
{
    return true;
}

void RDGraph::Shutdown()
{
    for(auto pass : m_MangedPasses)
        delete pass;

    for(auto res : m_ManagedResources)
        delete res;

    m_MangedPasses.clear();
    m_ManagedResources.clear();
}

RDGNodeHandle RDGraph::AddResource(std::string_view inName, RHIBufferRef inBuffer)
{
    return 0;
}

RDGNodeHandle RDGraph::AddResource(std::string_view inName, RHITextureRef inTexture)
{
    return 0;
}

void RDGraph::Compile()
{
    
}

void RDGraph::Execute()
{
    for(auto pass : m_MangedPasses)
        pass->Execute();
}