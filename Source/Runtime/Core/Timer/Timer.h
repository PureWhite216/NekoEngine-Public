#pragma once
#include <windows.h>
typedef LARGE_INTEGER TimeStamp;
namespace NekoEngine
{

    class Timer
    {
    private:
        TimeStamp m_Start;     // Start of timer
        TimeStamp m_Frequency; // Ticks Per Second
        TimeStamp m_LastTime;  // Last time GetTimedMS was called

    public:
        Timer()
                : m_Start(Now())
                , m_Frequency()
        {
            m_LastTime = m_Start;
        }
        ~Timer() = default;

        static TimeStamp Now();
        static double Duration(TimeStamp start, TimeStamp end, double timeResolution = 1.0);
        static float Duration(TimeStamp start, TimeStamp end, float timeResolution);

        float GetTimedMS();

        float GetElapsed(const float timeResolution) const
        {
            return Duration(m_Start, Now(), timeResolution);
        }

        double GetElapsed(const double timeResolution = 1.0) const
        {
            return Duration(m_Start, Now(), timeResolution);
        }

        float GetElapsedMS()
        {
            return GetElapsed(1000.0f);
        }

        float GetElapsedS()
        {
            return GetElapsed(1.0f);
        }

        double GetElapsedMSD()
        {
            return GetElapsed(1000.0);
        }

        double GetElapsedSD()
        {
            return GetElapsed(1.0);
        }
    };

} // NekoEngine

