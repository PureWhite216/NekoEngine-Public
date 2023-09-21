#pragma once

#include "BroadPhase.h"
#include "Core.h"
#include "RigidBody/RigidBody3D.h"

namespace NekoEngine
{

    class BruteForceBroadPhase : public BroadPhase
    {
    private:
        glm::vec3 m_axis;
    public:
        explicit BruteForceBroadPhase(const glm::vec3 &axis = glm::vec3(0.0f));

        virtual ~BruteForceBroadPhase();

        void FindPotentialCollisionPairs(RigidBody3D** objects, uint32_t objectCount,
                                         std::vector<CollisionPair> &collisionPairs) override;

        void DebugDraw() override;
    };

} // NekoEngine

