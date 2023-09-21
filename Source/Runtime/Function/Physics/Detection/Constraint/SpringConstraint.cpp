#include "SpringConstraint.h"

namespace NekoEngine
{
    SpringConstraint::SpringConstraint(const SharedPtr<RigidBody3D>& obj1, const SharedPtr<RigidBody3D>& obj2, float springConstant, float dampingFactor)
            : m_pObj1(obj1)
            , m_pObj2(obj2)
            , m_springConstant(springConstant)
            , m_dampingFactor(dampingFactor)
    {
        glm::vec3 ab   = obj2->GetPosition() - obj1->GetPosition();
        m_restDistance = glm::length(ab);
        // TODO: UNIMPLEMENTED
        glm::vec3 r1 = (obj1->GetPosition() - m_pObj1->GetPosition());
        glm::vec3 r2 = (obj2->GetPosition() - m_pObj2->GetPosition());
    }

    SpringConstraint::SpringConstraint(const SharedPtr<RigidBody3D>& obj1, const SharedPtr<RigidBody3D>& obj2, const glm::vec3& globalOnA, const glm::vec3& globalOnB, float springConstant, float dampingFactor)
            : m_pObj1(obj1)
            , m_pObj2(obj2)
            , m_springConstant(springConstant)
            , m_dampingFactor(dampingFactor)
    {
        glm::vec3 ab   = globalOnB - globalOnA;
        m_restDistance = glm::length(ab);
    }

    void SpringConstraint::ApplyImpulse()
    {
    }

    void SpringConstraint::DebugDraw() const
    {

    }
} // NekoEngine