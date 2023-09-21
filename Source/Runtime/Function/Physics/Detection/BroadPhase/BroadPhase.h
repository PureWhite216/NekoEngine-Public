#pragma once
#include "RigidBody/RigidBody3D.h"

namespace NekoEngine
{
    struct CollisionPair
    {
        RigidBody3D* pObjectA;
        RigidBody3D* pObjectB;
    };

    class BroadPhase
    {
    public:
        virtual ~BroadPhase() = default;

        virtual void FindPotentialCollisionPairs(RigidBody3D** objects, uint32_t objectCount,
                                                 std::vector<CollisionPair> &collisionPairs) = 0;

        virtual void DebugDraw() = 0;
    };

} // NekoEngine

