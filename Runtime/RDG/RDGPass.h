#pragma once
#include "RDGDefinitions.h"
#include <vector>

class RDGraph;

class RDGPass : public RDGNode
{
public:
    ~RDGPass() override = default;
    virtual void Execute() {}

protected:
    RDGPass(const std::string_view inName, RDGNodeHandle inHandle)
        : RDGNode(inName, inHandle)
    {
        
    }
    
    std::vector<RDGNodeHandle> m_ReadResources;
    std::vector<RDGNodeHandle> m_WriteResources;
};

template<typename ExecuteLambdaType>
class RDGEmptyLambdaPass : public RDGPass
{
public:
    ~RDGEmptyLambdaPass() override = default;

    void Execute() override
    {
        m_ExecuteLambda();
    }
    
private:
    friend RDGraph;
    RDGEmptyLambdaPass(const std::string_view inName, RDGNodeHandle inHandle, ExecuteLambdaType&& inExecuteLambda)
        : RDGPass(inName, inHandle)
        , m_ExecuteLambda(std::move(inExecuteLambda))
    {
        
    }
    
    ExecuteLambdaType m_ExecuteLambda;
};

template<typename ParameterStructType, typename ExecuteLambdaType>
class RDGLambdaPass : public RDGPass
{
    static constexpr uint32_t s_MaximumLambdaCaptureSize = 1024;
    static_assert(sizeof(ExecuteLambdaType) <= s_MaximumLambdaCaptureSize, "The amount of data of captured for the pass looks abnormally high.");

public:
    ~RDGLambdaPass() override = default;

    void Execute() override
    {
        m_ExecuteLambda();
    }

private:
    ParameterStructType m_Parameter;
    ExecuteLambdaType m_ExecuteLambda;
};