#pragma once

namespace NekoEngine
{
    class Constraint
    {
    public:
        Constraint() = default;
        virtual ~Constraint() = default;

        virtual void ApplyImpulse() = 0;

        virtual void PreSolverStep(float dt){}

        virtual void DebugDraw() const{}
    };
}