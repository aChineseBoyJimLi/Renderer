#pragma once
#include <chrono>
#include <cstdint>

class Timer
{
public:
    Timer() = default;
    void Tick();
    void Reset();

    double      TotalTime() const { return m_TotalTime; }
    double      DeltaTime() const { return m_DeltaTime; }
    uint64_t    TotalFrameCount() const { return m_TotalFrameCount; }
    uint32_t    FramePerSecond() const { return m_FrameRate; }
    uint64_t    CurrentTimestamp() const { return std::chrono::high_resolution_clock::now().time_since_epoch().count(); }
    uint64_t    PreviousTimestamp() const { return m_PreviousTimestamp.time_since_epoch().count(); }
    
private:
    double      m_TotalTime = 0;
    double      m_DeltaTime = 0;
    double      m_AverageFrameTime = 0;
    uint64_t    m_TotalFrameCount = 0;
    uint32_t    m_FrameRate = 0;
    uint32_t    m_AverageFrameRate = 0;
    std::chrono::time_point<std::chrono::steady_clock> m_PreviousTimestamp;
};
