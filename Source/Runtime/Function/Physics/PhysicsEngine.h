#pragma once

#include <mutex>
#include "RigidBody/RigidBody3D.h"
#include "Detection/NarrowPhase/Manifold.h"
#include "Detection/BroadPhase/BroadPhase.h"
#include "Level/Level.h"
#include "System/ISystem.h"
#include "Detection/NarrowPhase/CollisionDetection.h"

namespace NekoEngine
{
    enum class IntegrationType : uint32_t
    {
        EXPLICIT_EULER = 0,
        SEMI_IMPLICIT_EULER = 1,
        RUNGE_KUTTA_2 = 2,
        RUNGE_KUTTA_4 = 3
    };

    enum class BroadphaseType : uint32_t
    {
        BRUTE_FORCE = 0,
        SORT_AND_SWEAP = 1,
        OCTREE = 2,
    };

    enum PhysicsDebugFlags : uint32_t
    {
        CONSTRAINT = 1,
        MANIFOLD = 2,
        COLLISIONVOLUMES = 4,
        COLLISIONNORMALS = 8,
        AABB = 16,
        LINEARVELOCITY = 32,
        LINEARFORCE = 64,
        BROADPHASE = 128,
        BROADPHASE_PAIRS = 256,
        BOUNDING_RADIUS = 512,
    };

    class Constraint;

    class TimeStep;

    class PhysicsEngine : public ISystem
    {
    protected:
        UniquePtr<CollisionDetection> collisionDetection;

        bool m_IsPaused;
        float m_UpdateAccum;
        glm::vec3 m_Gravity;
        float m_DampingFactor;
        uint32_t m_MaxUpdatesPerFrame = 5;
        uint32_t m_PositionIterations = 1;
        uint32_t m_VelocityIterations = 50;


        std::vector<RigidBody3D*> m_RigidBodys;
        std::vector<CollisionPair> m_BroadphaseCollisionPairs;

        std::vector<Constraint*> m_Constraints; // Misc constraints between pairs of objects
        std::vector<Manifold> m_Manifolds;      // Contact constraints between pairs of objects
        std::mutex m_ManifoldsMutex;

        SharedPtr<BroadPhase> m_BroadphaseDetection;
        BroadphaseType m_BroadphaseType;
        IntegrationType m_IntegrationType;

        uint32_t m_DebugDrawFlags = 0;
        std::mutex m_ManifoldLock;
        static float s_UpdateTimestep;
    public:
        PhysicsEngine();
        ~PhysicsEngine();

        void SetDefaults();

        // Add Constraints
        void AddConstraint(Constraint* c)
        {
            m_Constraints.push_back(c);
        }

        void OnInit() override {};
        // Update Physics Engine
        void OnUpdate(const TimeStep& timeStep, Level* level) override;

        void SyncTransforms(Level* level);

        // Getters / Setters
        bool IsPaused() const { return m_IsPaused; }
        void SetPaused(bool paused) { m_IsPaused = paused; }

        const glm::vec3& GetGravity() const { return m_Gravity; }
        void SetGravity(const glm::vec3& g) { m_Gravity = g; }

        float GetDampingFactor() const { return m_DampingFactor; }
        void SetDampingFactor(float d) { m_DampingFactor = d; }

        static float GetDeltaTime() { return s_UpdateTimestep; }
        SharedPtr<BroadPhase> GetBroadphase() const { return m_BroadphaseDetection; }

        inline void SetBroadphase(const SharedPtr<BroadPhase>& bp) { m_BroadphaseDetection = bp; }
        int GetNumberCollisionPairs() const { return static_cast<int>(m_BroadphaseCollisionPairs.size()); }
        int GetNumberRigidBodys() const { return static_cast<int>(m_RigidBodys.size()); }
        IntegrationType GetIntegrationType() const { return m_IntegrationType; }
        void SetIntegrationType(const IntegrationType& type) { m_IntegrationType = type; }
        void SetBroadphaseType(BroadphaseType type);

        void ClearConstraints();

        void OnImGui() override;
        void OnDebugDraw() override;

        void SetDebugDrawFlags(uint32_t flags) { m_DebugDrawFlags = flags; }
        uint32_t GetDebugDrawFlags() const { return m_DebugDrawFlags; }

        std::string IntegrationTypeToString(IntegrationType type);
        std::string BroadphaseTypeToString(BroadphaseType type);

        uint32_t GetMaxUpdatesPerFrame() const { return m_MaxUpdatesPerFrame; }
        void SetMaxUpdatesPerFrame(uint32_t updates) { m_MaxUpdatesPerFrame = updates; }

        uint32_t GetVelocityIterations() const { return m_VelocityIterations; }
        void SetVelocityIterations(uint32_t iterations) { m_VelocityIterations = iterations; }

        uint32_t GetPositionIterations() const { return m_PositionIterations; }
        void SetPositionIterations(uint32_t iterations) { m_PositionIterations = iterations; }

    protected:
        // The actual time-independant update function
        void UpdatePhysics();

        // Handles broadphase collision detection
        void BroadPhaseCollisions();

        // Handles narrowphase collision detection
        void NarrowPhaseCollisions();

        // Updates all Rigid Body position, orientation, velocity etc (default method uses symplectic euler integration)
        void UpdateRigidBodys();
        void UpdateRigidBody(RigidBody3D* obj) const;

        // Solves all engine constraints (constraints and manifolds)
        void SolveConstraints();
    };

} // NekoEngine

