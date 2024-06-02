#include <memory>
#include <iostream>
#include "RDG/RDG.h"
#include "WinApp/AudioTest.h"
#include "Core/Log.h"
#include "WinApp/ImguiTestApp.h"
#include "WinApp/RDGTestApp.h"

void RunApp(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    RHI::Init(true);
    RDG::Init();
    std::unique_ptr<RDGTestApp> app = std::make_unique<RDGTestApp>(380
            , 200
            , hInstance
            , TEXT("RDG Test App"));
        
    app->Run();
}

void PostCleanup()
{
    RDG::Shutdown();
    RHI::Shutdown();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    try
    {
        RunApp(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
        PostCleanup();
        return 0;
    }
    catch (std::runtime_error& err)
    {
        Log::Error(err.what());
        return -1;
    }
}