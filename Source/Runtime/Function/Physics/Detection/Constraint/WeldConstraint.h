#pragma once
#include "Constraint.h"
#include "RigidBody/RigidBody3D.h"

namespace NekoEngine
{

    class WeldConstraint : public Constraint
    {
    protected:
        RigidBody3D* m_pObj1;
        RigidBody3D* m_pObj2;

        glm::vec3 m_positionOffset;
        glm::quat m_orientation;

    public:
        WeldConstraint(RigidBody3D* obj1, RigidBody3D* obj2);

        virtual void ApplyImpulse() override;
        virtual void DebugDraw() const override;
    };

} // NekoEngine

