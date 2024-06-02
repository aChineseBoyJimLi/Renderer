#include "Timer.h"
#include <chrono>

void Timer::Tick()
{
    const auto currentTime = std::chrono::high_resolution_clock::now();
    m_DeltaTime = std::chrono::duration<double, std::chrono::seconds::period>(currentTime - m_PreviousTimestamp).count();
    m_FrameRate = static_cast<uint32_t>(1.0 / m_DeltaTime);
    m_TotalTime += m_DeltaTime;
    m_TotalFrameCount++;
    m_PreviousTimestamp = currentTime;
    m_AverageFrameTime = m_TotalTime / static_cast<double>(m_TotalFrameCount);
    m_AverageFrameRate = static_cast<uint32_t>(static_cast<double>(m_TotalFrameCount) / m_TotalTime);
}

void Timer::Reset()
{
    m_PreviousTimestamp = std::chrono::high_resolution_clock::now();
    m_DeltaTime = 0;
    m_FrameRate = 0;
    m_TotalTime = 0;
    m_TotalFrameCount = 0;
    m_AverageFrameTime = 0;
    m_AverageFrameRate = 0;
}
