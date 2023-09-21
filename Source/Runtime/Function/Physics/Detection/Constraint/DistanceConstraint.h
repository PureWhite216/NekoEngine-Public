#pragma once
#include "Constraint.h"
#include "RigidBody/RigidBody3D.h"

namespace NekoEngine
{

    class DistanceConstraint : public Constraint
    {
    protected:
        float m_Distance;
        RigidBody3D* m_pObj1;
        RigidBody3D* m_pObj2;
        glm::vec3 m_LocalOnA;
        glm::vec3 m_LocalOnB;
    public:
        DistanceConstraint(RigidBody3D* obj1, RigidBody3D* obj2, const glm::vec3& globalOnA, const glm::vec3& globalOnB);

        virtual void ApplyImpulse() override;
        virtual void DebugDraw() const override;
    };

} // NekoEngine

