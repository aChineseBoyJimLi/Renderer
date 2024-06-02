#include "RDGTestApp.h"

bool RDGTestApp::Init()
{
    m_TestPass = std::make_unique<TestPass>();
    m_TestPass->Setup();
    return true;
}

void RDGTestApp::Tick()
{
    RDG::GetGraph()->Execute();
}

void RDGTestApp::Shutdown()
{
    
}

void RDGTestApp::OnBeginResize()
{
    
}

void RDGTestApp::OnResize(int width, int height)
{
    
}

void RDGTestApp::OnEndResize()
{
    
}