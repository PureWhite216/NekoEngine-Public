#pragma once
#include "Core.h"
#include "RigidBody/RigidBody3D.h"

#define MAX_CONTACT_POINTS 8

namespace NekoEngine
{
    struct ContactPoint
    {
        float sumImpulseContact    = 0.0f;
        float sumImpulseFriction   = 0.0f;
        float elatisity_term       = 0.0f;
        float collisionPenetration = 0.0f;

        glm::vec3 collisionNormal;
        glm::vec3 relPosA; // Position relative to objectA
        glm::vec3 relPosB; // Position relative to objectB
    };

    class Manifold
    {
    protected:
        RigidBody3D* m_pNodeA;
        RigidBody3D* m_pNodeB;
        ContactPoint m_vContacts[MAX_CONTACT_POINTS];
        uint32_t m_ContactCount = 0;
    public:
        Manifold();
        ~Manifold();

        // Initiate for collision pair
        void Initiate(RigidBody3D* nodeA, RigidBody3D* nodeB);

        // Called whenever a new collision contact between A & B are found
        void AddContact(const glm::vec3& globalOnA, const glm::vec3& globalOnB, const glm::vec3& _normal, const float& _penetration);

        // Sequentially solves each contact constraint
        void ApplyImpulse();
        void PreSolverStep(float dt);

        // Debug draws the manifold surface area
        void DebugDraw() const;

        // Get the physics objects
        RigidBody3D* NodeA() const
        {
            return m_pNodeA;
        }
        RigidBody3D* NodeB() const
        {
            return m_pNodeB;
        }
    protected:
        void SolveContactPoint(ContactPoint& c) const;
        void UpdateConstraint(ContactPoint& c);
    };

} // NekoEngine

