#pragma once

#include <Windows.h>
#include <list>
#include <cstdint>

#include "Timer.h"

class Win32Base
{
public:
    Win32Base(uint32_t inWidth, uint32_t inHeight, HINSTANCE inHInstance, const char* inTitle);
    virtual ~Win32Base();
    void Run();
    const Timer& GetTimer() const { return m_Timer; }

protected:
    virtual bool Init() { return true;}
    virtual void Tick() {}
    virtual void Shutdown() {}
    
    virtual void OnResize(int width, int height){ }
    virtual void OnBeginResize() {}
    virtual void OnEndResize() {}
    virtual void OnMouseLeftBtnDown(int x, int y){ }
    virtual void OnMouseLeftBtnUp(int x, int y)  { }
    virtual void OnMouseRightBtnDown(int x, int y){ }
    virtual void OnMouseRightBtnUp(int x, int y)  { }
    virtual void OnMouseMidBtnDown(int x, int y){ }
    virtual void OnMouseMidBtnUp(int x, int y)  { }
    virtual void OnMouseMove(int x, int y){ }
    virtual void OnMouseWheeling(int x, int y, int delta) { }

    uint32_t m_Width;
    uint32_t m_Height;
    HINSTANCE const m_hInstance;
    HWND m_hWnd;
    bool m_IsRunning = true;

private:
    Timer m_Timer;
    static std::list<Win32Base*> s_Listeners;
    static LRESULT CALLBACK MsgProc(HWND HWnd, UINT Msg, WPARAM WParam, LPARAM LParam);
};
