#include "Timer.h"
#include <chrono>

void Timer::Tick()
{
    
}

void Timer::Reset()
{
    m_FrameCount = 0;
    m_DeltaTime = 0;
    m_FramePerSecond = 0;
    m_TotalTime = 0;
}
