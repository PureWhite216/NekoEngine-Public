#pragma once

#include "Core.h"

namespace NekoEngine
{
    class ISystem
    {
    public:
        String m_DebugName;
    public:
        ISystem() = default;

        virtual ~ISystem() = default;

        virtual void OnInit() = 0;

        virtual void OnUpdate(const TimeStep &dt, Level* scene) = 0;

        virtual void OnImGui() = 0;

        virtual void OnDebugDraw() = 0;

        inline const std::string &GetName() const
        {
            return m_DebugName;
        }
    };
} // namespace NekoEngine
