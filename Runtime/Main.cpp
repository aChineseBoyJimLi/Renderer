#include <memory>
#include <iostream>
#include "WinApp/RhiTestApp.h"
#include "Core/Log.h"

void RunApp(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    RHI::Init(true);
    std::unique_ptr<RhiTestApp> app = std::make_unique<RhiTestApp>(1280
            , 720
            , hInstance
            , TEXT("RhiTestApp App"));
        
    app->Run();
}

void PostCleanup()
{
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