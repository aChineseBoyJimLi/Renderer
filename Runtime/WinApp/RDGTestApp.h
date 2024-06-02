#pragma once

#include "../Core/Win32Base.h"
#include "AssetsManager.h"
#include "../Passes/TestPass.h"

class RDGTestApp : public Win32Base
{
public:
    using Win32Base::Win32Base;
    static constexpr uint32_t s_TexturesCount = 5;
    static constexpr uint32_t s_MaterialCount = 100;
    static constexpr uint32_t s_InstanceCountX = 8, s_InstanceCountY = 8, s_InstanceCountZ = 8;
    static constexpr uint32_t s_InstancesCount = s_InstanceCountX * s_InstanceCountY * s_InstanceCountZ;
    
protected:
    bool Init() final;
    void Tick()  final;
    void Shutdown() final;
    void OnBeginResize() override;
    void OnResize(int width, int height) final;
    void OnEndResize() final;

    // void LoadAssets();
    // void CreateFrameBuffer();
    // void CreateResources(); 
    
private:
    std::unique_ptr<TestPass> m_TestPass;
};