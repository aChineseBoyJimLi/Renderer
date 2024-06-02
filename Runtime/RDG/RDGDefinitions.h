#pragma once
#include <string>

class RDGraph;
using RDGNodeHandle = size_t;

class RDGNode
{
public:
    RDGNode(const std::string_view inName, RDGNodeHandle inHandle)
        : m_Name(inName), m_Handle(inHandle)
    {
        
    }
    
    virtual ~RDGNode() {}
    RDGNode(const RDGNode&) = delete;
    RDGNode& operator=(const RDGNode&) = delete;
    RDGNode(RDGNode&&) = delete;
    RDGNode& operator=(RDGNode&&) = delete;
    
    RDGNodeHandle GetHandle() const { return m_Handle; }
    const std::string& GetName() const { return m_Name; }
    
protected:
    std::string m_Name;
    RDGNodeHandle m_Handle;
};

namespace RDG
{
    bool Init();
    void Shutdown();
    RDGraph* GetGraph();
}
