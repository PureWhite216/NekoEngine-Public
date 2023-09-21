#pragma once
#include "Core.h"
#include "Constraint.h"
namespace NekoEngine
{
    class RigidBody3D;

    enum class Axes : uint8_t
    {
        X = 0,
        Y,
        Z,
        XY,
        XZ,
        YZ,
        XYZ
    };

    class AxisConstraint : public Constraint
    {
    protected:
        RigidBody3D* m_pObj1;
        Axes m_Axes;
    public:
        AxisConstraint(RigidBody3D* obj1, Axes axes);

        virtual void ApplyImpulse() override;
        virtual void DebugDraw() const override;
        Axes GetAxes() { return m_Axes; }
    };

} // NekoEngine

