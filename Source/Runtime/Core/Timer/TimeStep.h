#pragma once
#include "Timer.h"

namespace NekoEngine
{

    class TimeStep
    {
    private:
        double m_Timestep; // MilliSeconds
        double m_LastTime;
        double m_Elapsed;

        Timer* m_Timer;

    public:
        TimeStep();
        ~TimeStep();

        void OnUpdate();
        inline double GetMillis() const { return m_Timestep; }
        inline double GetElapsedMillis() const { return m_Elapsed; }

        inline double GetSeconds() const { return m_Timestep * 0.001; }
        inline double GetElapsedSeconds() const { return m_Elapsed * 0.001; }
    };

} // NekoEngine

