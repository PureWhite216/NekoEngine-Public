#include "AxisConstraint.h"
#include "RigidBody/RigidBody3D.h"

namespace NekoEngine
{
    AxisConstraint::AxisConstraint(RigidBody3D* obj1, Axes axes)
            : m_pObj1(obj1)
            , m_Axes(axes)
    {
    }

    void AxisConstraint::ApplyImpulse()
    {
        auto velocity = m_pObj1->GetAngularVelocity();

        switch(m_Axes)
        {
            case Axes::X:
                velocity.x = 0.0f;
                break;
            case Axes::Y:
                velocity.y = 0.0f;
                break;
            case Axes::Z:
                velocity.z = 0.0f;
                break;
            case Axes::XZ:
                velocity.x = 0.0f;
                velocity.z = 0.0f;
                break;
            case Axes::XY:
                velocity.x = 0.0f;
                velocity.y = 0.0f;
                break;
            case Axes::YZ:
                velocity.y = 0.0f;
                velocity.z = 0.0f;
                break;
            case Axes::XYZ:
                velocity.x = 0.0f;
                velocity.y = 0.0f;
                velocity.z = 0.0f;
                break;
            default:
                break;
        }

        m_pObj1->SetAngularVelocity(velocity);
    }

    void AxisConstraint::DebugDraw() const
    {
    }
} // NekoEngine