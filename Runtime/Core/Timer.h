#pragma once
#include <chrono>
#include <cstdint>

class Timer
{
public:
    Timer() = default;
    
    void Tick();
    void Reset();

    float       TotalTime() const { return m_TotalTime; }
    float       DeltaTime() const { return m_DeltaTime; }
    uint64_t    FrameCount() const { return m_FrameCount; }
    uint32_t    FramePerSecond() const { return m_FramePerSecond; }
    
private:
    float       m_TotalTime = 0;
    uint64_t    m_FrameCount = 0;
    float       m_DeltaTime = 0;
    uint32_t    m_FramePerSecond = 0;
};
