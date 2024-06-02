#include "RDG.h"
#include "RDGraph.h"
#include <mutex>

namespace RDG
{
    static RDGraph* s_Graph = nullptr;
    static std::mutex s_Mtx;

    bool Init()
    {
        if(s_Graph == nullptr)
        {
            std::lock_guard locker(s_Mtx);
            s_Graph = new RDGraph;
            return s_Graph->Init();
        }
        return true;
    }
    
    void Shutdown()
    {
        if(s_Graph != nullptr)
        {
            std::lock_guard locker(s_Mtx);
            s_Graph->Shutdown();
            delete s_Graph;
            s_Graph = nullptr;
        }
    }

    RDGraph* GetGraph()
    {
        if(!Init() || s_Graph == nullptr)
        {
            throw std::runtime_error("[RDG] RDG not validated!");
        }
        return s_Graph;
    }
}
