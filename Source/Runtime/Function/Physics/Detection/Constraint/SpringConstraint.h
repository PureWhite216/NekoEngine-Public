#pragma once
#include "Constraint.h"
#include "RigidBody/RigidBody3D.h"

namespace NekoEngine
{

    class SpringConstraint : public Constraint
    {
    protected:
        SharedPtr<RigidBody3D> m_pObj1;
        SharedPtr<RigidBody3D> m_pObj2;

        float m_restDistance;

        float m_springConstant;
        float m_dampingFactor;

        glm::vec3 m_LocalOnA;
        glm::vec3 m_LocalOnB;

    public:
        SpringConstraint(const SharedPtr<RigidBody3D>& obj1, const SharedPtr<RigidBody3D>& obj2, float springConstant, float dampingFactor);
        SpringConstraint(const SharedPtr<RigidBody3D>& obj1, const SharedPtr<RigidBody3D>& obj2, const glm::vec3& globalOnA, const glm::vec3& globalOnB, float springConstant, float dampingFactor);

        virtual void ApplyImpulse() override;
        virtual void DebugDraw() const override;
    };

} // NekoEngine

