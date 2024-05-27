#pragma once

#include "../Core/Win32Base.h"
#include "../RHI/RHI.h"
#include "AssetsManager.h"

#include "imgui.h"


class ImguiTestApp : public Win32Base
{
public:
    using Win32Base::Win32Base;

protected:
    bool Init() final;
    void Tick()  final;
    void Shutdown() final;
    void OnBeginResize() override;
    void OnResize(int width, int height) final;
    void OnEndResize() final;

private:
    bool InitImgui();
};