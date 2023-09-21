#include "PhysicsEngine.h"
#include "ImGui/imgui.h"
#include "Transform.h"
#include "Component/RigidBody3DComponent.h"
#include "Detection/BroadPhase/OctreeBroadPhase.h"
#include "Detection/BroadPhase/BruteForceBroadPhase.h"
#include "Engine.h"
#include "Integration.h"
#include "Timer/TimeStep.h"

namespace NekoEngine
{
    float PhysicsEngine::s_UpdateTimestep = 1.0f / 60.0f;

    PhysicsEngine::PhysicsEngine()
            : m_IsPaused(true), m_UpdateAccum(0.0f), m_Gravity(glm::vec3(0.0f, -9.81f, 0.0f)), m_DampingFactor(0.9995f),
              m_BroadphaseDetection(nullptr), m_IntegrationType(IntegrationType::RUNGE_KUTTA_4)
    {
        m_DebugName = "3DPhysicsEngine";
        m_RigidBodys.reserve(100);
        m_BroadphaseCollisionPairs.reserve(1000);
        m_Manifolds.reserve(100);
        collisionDetection = MakeUnique<CollisionDetection>();
    }

    void PhysicsEngine::SetDefaults()
    {
        m_IsPaused = true;
        s_UpdateTimestep = 1.0f / 60.f;
        m_UpdateAccum = 0.0f;
        m_Gravity = glm::vec3(0.0f, -9.81f, 0.0f);
        m_DampingFactor = 0.9995f;
        m_IntegrationType = IntegrationType::RUNGE_KUTTA_4;
    }

    PhysicsEngine::~PhysicsEngine()
    {
        m_RigidBodys.clear();
        m_Constraints.clear();
        m_Manifolds.clear();
        collisionDetection.reset();
    }

    void PhysicsEngine::OnUpdate(const TimeStep &timeStep, Level* scene)
    {
        m_RigidBodys.clear();

        if(!m_IsPaused)
        {
            auto &registry = scene->GetRegistry();
            auto group = registry.group<RigidBody3DComponent>(entt::get<Transform>);

            {
                for(auto entity: group)
                {
                    const auto &phys = group.get<RigidBody3DComponent>(entity);
                    auto &physicsObj = phys.GetRigidBody();
                    m_RigidBodys.push_back(physicsObj.get());
                };
            }

            if(m_RigidBodys.empty())
            {
                return;
            }

            {
                m_Constraints.clear();

                auto viewSpring = registry.view<SpringConstraintComponent>();

                for(auto entity: viewSpring)
                {
                    const auto &constraint = viewSpring.get<SpringConstraintComponent>(entity).GetConstraint();
                    m_Constraints.push_back(constraint.get());
                }
            }

            {

                auto viewAxis = registry.view<AxisConstraintComponent, IDComponent>();

                for(auto entity: viewAxis)
                {
                    const auto &[constraint, idComp] = viewAxis.get<AxisConstraintComponent, IDComponent>(entity);

                    if(constraint.GetEntityID() != idComp.ID)
                        constraint.SetEntity(idComp.ID);
                    if(constraint.GetConstraint())
                        m_Constraints.push_back(constraint.GetConstraint().get());
                }
            }

            {
                auto viewDis = registry.view<DistanceConstraintComponent>();

                for(auto entity: viewDis)
                {
                    const auto &constraint = viewDis.get<DistanceConstraintComponent>(entity).GetConstraint();
                    m_Constraints.push_back(constraint.get());
                }
            }

            {
                auto viewWeld = registry.view<WeldConstraintComponent>();

                for(auto entity: viewWeld)
                {
                    const auto &constraint = viewWeld.get<WeldConstraintComponent>(entity).GetConstraint();
                    m_Constraints.push_back(constraint.get());
                }
            }

            {
                m_UpdateAccum += timeStep.GetSeconds();
                for(uint32_t i = 0; (m_UpdateAccum >= s_UpdateTimestep) && i < m_MaxUpdatesPerFrame; ++i)
                {
                    m_UpdateAccum -= s_UpdateTimestep;
                    UpdatePhysics();
                }

                if(m_UpdateAccum >= s_UpdateTimestep)
                {
                    // Drop Time in the hope that it can continue to run in real-time
                    m_UpdateAccum = 0.0f;
                }
            }

            m_Constraints.clear();
        }
    }

    void PhysicsEngine::UpdatePhysics()
    {
        m_Manifolds.clear();

        // Check for collisions
        BroadPhaseCollisions();
        NarrowPhaseCollisions();

        // Solve collision constraints
        SolveConstraints();

        // Update movement
        UpdateRigidBodys();

        for(int i = 0; i < m_RigidBodys.size(); i++)
            m_RigidBodys[i]->RestTest();
    }

    void PhysicsEngine::UpdateRigidBodys()
    {
        for(size_t i = 0; i < m_RigidBodys.size(); i++)
            UpdateRigidBody(m_RigidBodys[i]);
    }

    void PhysicsEngine::SyncTransforms(Level* scene)
    {


        if(!scene)
            return;

        auto &registry = scene->GetRegistry();
        auto group = registry.group<RigidBody3DComponent>(entt::get<Transform>);

        for(auto entity: group)
        {
            const auto &[phys, trans] = group.get<RigidBody3DComponent, Transform>(entity);

            if(!phys.GetRigidBody()->GetIsStatic() && phys.GetRigidBody()->IsAwake())
            {
                trans.SetLocalPosition(phys.GetRigidBody()->GetPosition());
                trans.SetLocalOrientation(phys.GetRigidBody()->GetOrientation());
            }
        };
    }

    glm::quat QuatMulVec3(const glm::quat &quat, const glm::vec3 &b)
    {
        glm::quat ans;

        ans.w = -(quat.x * b.x) - (quat.y * b.y) - (quat.z * b.z);

        ans.x = (quat.w * b.x) + (b.y * quat.z) - (b.z * quat.y);
        ans.y = (quat.w * b.y) + (b.z * quat.x) - (b.x * quat.z);
        ans.z = (quat.w * b.z) + (b.x * quat.y) - (b.y * quat.x);

        return ans;
    }

    void PhysicsEngine::UpdateRigidBody(RigidBody3D* obj) const
    {


        if(!obj->GetIsStatic() && obj->IsAwake())
        {
            const float damping = m_DampingFactor;

            // Apply gravity
            if(obj->m_InvMass > 0.0f)
                obj->m_LinearVelocity += m_Gravity * s_UpdateTimestep;

            switch(m_IntegrationType)
            {
                case IntegrationType::EXPLICIT_EULER:
                {
                    // Update position
                    obj->m_Position += obj->m_LinearVelocity * s_UpdateTimestep;

                    // Update linear velocity (v = u + at)
                    obj->m_LinearVelocity += obj->m_Force * obj->m_InvMass * s_UpdateTimestep;

                    // Linear velocity damping
                    obj->m_LinearVelocity = obj->m_LinearVelocity * damping;

                    // Update orientation
                    obj->m_Orientation += obj->m_Orientation * glm::quat(obj->m_AngularVelocity * s_UpdateTimestep);
                    // obj->m_Orientation = obj->m_Orientation + ((obj->m_AngularVelocity * s_UpdateTimestep * 0.5f) * obj->m_Orientation);
                    obj->m_Orientation = glm::normalize(obj->m_Orientation);

                    // Update angular velocity
                    obj->m_AngularVelocity += obj->m_InvInertia * obj->m_Torque * s_UpdateTimestep;

                    // Angular velocity damping
                    obj->m_AngularVelocity = obj->m_AngularVelocity * damping * obj->m_AngularFactor;

                    break;
                }

                case IntegrationType::SEMI_IMPLICIT_EULER:
                {
                    // Update linear velocity (v = u + at)
                    obj->m_LinearVelocity += obj->m_LinearVelocity * obj->m_InvMass * s_UpdateTimestep;

                    // Linear velocity damping
                    obj->m_LinearVelocity = obj->m_LinearVelocity * damping;

                    // Update position
                    obj->m_Position += obj->m_LinearVelocity * s_UpdateTimestep;

                    // Update angular velocity
                    obj->m_AngularVelocity += obj->m_InvInertia * obj->m_Torque * s_UpdateTimestep;

                    // Angular velocity damping
                    obj->m_AngularVelocity = obj->m_AngularVelocity * damping * obj->m_AngularFactor;

                    auto angularVelocity = obj->m_AngularVelocity * s_UpdateTimestep;

                    // Update orientation
                    obj->m_Orientation += QuatMulVec3(obj->m_Orientation, angularVelocity);
                    obj->m_Orientation = glm::normalize(obj->m_Orientation);

                    break;
                }

                case IntegrationType::RUNGE_KUTTA_2:
                {
                    // RK2 integration for linear motion
                    Integration::State state = {obj->m_Position, obj->m_LinearVelocity, obj->m_Force * obj->m_InvMass};
                    Integration::RK2(state, 0.0f, s_UpdateTimestep);

                    obj->m_Position = state.position;
                    obj->m_LinearVelocity = state.velocity;

                    // Linear velocity damping
                    obj->m_LinearVelocity = obj->m_LinearVelocity * damping;

                    // Update angular velocity
                    obj->m_AngularVelocity += obj->m_InvInertia * obj->m_Torque * s_UpdateTimestep;

                    // Angular velocity damping
                    obj->m_AngularVelocity = obj->m_AngularVelocity * damping * obj->m_AngularFactor;

                    auto angularVelocity = obj->m_AngularVelocity * s_UpdateTimestep * 0.5f;

                    // Update orientation
                    obj->m_Orientation += QuatMulVec3(obj->m_Orientation, angularVelocity);
                    obj->m_Orientation = glm::normalize(obj->m_Orientation);

                    break;
                }

                case IntegrationType::RUNGE_KUTTA_4:
                {
                    // RK4 integration for linear motion
                    Integration::State state = {obj->m_Position, obj->m_LinearVelocity, obj->m_Force * obj->m_InvMass};
                    Integration::RK4(state, 0.0f, s_UpdateTimestep);
                    obj->m_Position = state.position;
                    obj->m_LinearVelocity = state.velocity;

                    // Linear velocity damping
                    obj->m_LinearVelocity = obj->m_LinearVelocity * damping;

                    // Update angular velocity
                    obj->m_AngularVelocity += obj->m_InvInertia * obj->m_Torque * s_UpdateTimestep;

                    // Angular velocity damping
                    obj->m_AngularVelocity = obj->m_AngularVelocity * damping * obj->m_AngularFactor;

                    // Update orientation
                    // Check order of quat multiplication
                    auto angularVelocity = obj->m_AngularVelocity * s_UpdateTimestep * 0.5f;

                    obj->m_Orientation += QuatMulVec3(obj->m_Orientation, angularVelocity);
                    obj->m_Orientation = glm::normalize(obj->m_Orientation);

                    break;
                }
            }

            // Mark cached world transform and AABB as invalid
            obj->m_wsTransformInvalidated = true;
            obj->m_wsAabbInvalidated = true;
        }

        s_UpdateTimestep *= m_PositionIterations;
    }

    glm::quat AngularVelcityToQuaternion(const glm::vec3 &angularVelocity)
    {
        glm::quat q;
        q.x = 0.5f * angularVelocity.x;
        q.y = 0.5f * angularVelocity.y;
        q.z = 0.5f * angularVelocity.z;
        q.w = 0.5f * glm::length(angularVelocity);
        return q;
    }

    void PhysicsEngine::BroadPhaseCollisions()
    {

        m_BroadphaseCollisionPairs.clear();
        if(m_BroadphaseDetection)
            m_BroadphaseDetection->FindPotentialCollisionPairs(m_RigidBodys.data(), (uint32_t) m_RigidBodys.size(),
                                                               m_BroadphaseCollisionPairs);

#ifdef CHECK_COLLISION_PAIR_DUPLICATES

        uint32_t duplicatePairs = 0;
        for(size_t i = 0; i < m_BroadphaseCollisionPairs.size(); ++i)
        {
            auto& pair = m_BroadphaseCollisionPairs[i];
            for(size_t j = i + 1; j < m_BroadphaseCollisionPairs.size(); ++j)
            {
                auto& pair2 = m_BroadphaseCollisionPairs[j];
                if(pair.pObjectA == pair2.pObjectA && pair.pObjectB == pair2.pObjectB)
                {
                    duplicatePairs++;
                }
                else if(pair.pObjectA == pair2.pObjectB && pair.pObjectB == pair2.pObjectA)
                {
                    duplicatePairs++;
                }
            }
        }

        _LOG_INFO(duplicatePairs);
#endif
    }

    void PhysicsEngine::NarrowPhaseCollisions()
    {

        if(m_BroadphaseCollisionPairs.empty())
            return;

        for(auto &cp: m_BroadphaseCollisionPairs)
        {
            auto shapeA = cp.pObjectA->GetCollisionShape();
            auto shapeB = cp.pObjectB->GetCollisionShape();

            if(shapeA && shapeB)
            {
                CollisionData colData;

                // Detects if the objects are colliding - Seperating Axis Theorem
                if(collisionDetection->CheckCollision(cp.pObjectA, cp.pObjectB, shapeA.get(), shapeB.get(), &colData))
                {
                    // Check to see if any of the objects have collision callbacks that dont
                    // want the objects to physically collide
                    const bool okA = cp.pObjectA->FireOnCollisionEvent(cp.pObjectA, cp.pObjectB);
                    const bool okB = cp.pObjectB->FireOnCollisionEvent(cp.pObjectB, cp.pObjectA);

                    if(okA && okB)
                    {
                        // Build full collision manifold that will also handle the collision
                        // response between the two objects in the solver stage
                        m_ManifoldLock.lock();
                        Manifold &manifold = m_Manifolds.emplace_back();
                        manifold.Initiate(cp.pObjectA, cp.pObjectB);

                        // Construct contact points that form the perimeter of the collision manifold
                        if(collisionDetection->BuildCollisionManifold(cp.pObjectA, cp.pObjectB, shapeA.get(),
                                                                      shapeB.get(), colData, &manifold))
                        {
                            // Fire callback
                            cp.pObjectA->FireOnCollisionManifoldCallback(cp.pObjectA, cp.pObjectB, &manifold);
                            cp.pObjectB->FireOnCollisionManifoldCallback(cp.pObjectB, cp.pObjectA, &manifold);
                        }
                        else
                        {
                            m_Manifolds.pop_back();
                        }

                        m_ManifoldLock.unlock();
                    }
                }
            }
        }
    }

    void PhysicsEngine::SolveConstraints()
    {


        {
            for(Manifold &m: m_Manifolds)
                m.PreSolverStep(s_UpdateTimestep);
        }
        {
            for(Constraint* c: m_Constraints)
                c->PreSolverStep(s_UpdateTimestep);
        }

        {
            for(uint32_t i = 0; i < m_VelocityIterations; i++)
            {
                for(Manifold &m: m_Manifolds)
                    m.ApplyImpulse();

                for(Constraint* c: m_Constraints)
                    c->ApplyImpulse();
            }
        }
    }

    void PhysicsEngine::ClearConstraints()
    {
        m_Constraints.clear();
    }

    std::string PhysicsEngine::IntegrationTypeToString(IntegrationType type)
    {
        switch(type)
        {
            case IntegrationType::EXPLICIT_EULER:
                return "EXPLICIT EULER";
            case IntegrationType::SEMI_IMPLICIT_EULER:
                return "SEMI IMPLICIT EULER";
            case IntegrationType::RUNGE_KUTTA_2:
                return "RUNGE KUTTA 2";
            case IntegrationType::RUNGE_KUTTA_4:
                return "RUNGE KUTTA 4";
            default:
                return "";
        }
    }

    std::string PhysicsEngine::BroadphaseTypeToString(BroadphaseType type)
    {
        switch(type)
        {
            case BroadphaseType::BRUTE_FORCE:
                return "Brute Force";
            case BroadphaseType::SORT_AND_SWEAP:
                return "Sort and Sweap";
            case BroadphaseType::OCTREE:
                return "Octree";
            default:
                return "";
        }
    }

    void PhysicsEngine::SetBroadphaseType(BroadphaseType type)
    {
        if(type == m_BroadphaseType)
            return;

        switch(type)
        {
            case BroadphaseType::BRUTE_FORCE:
            case BroadphaseType::SORT_AND_SWEAP:
            case BroadphaseType::OCTREE:
                m_BroadphaseDetection = MakeShared<OctreeBroadPhase>(5, 5, MakeShared<BruteForceBroadPhase>());
                break;
            default:
                m_BroadphaseDetection = MakeShared<BruteForceBroadPhase>();
                break;
        }

        m_BroadphaseType = type;
    }

    void PhysicsEngine::OnImGui()
    {

        ImGui::TextUnformatted("3D Physics Engine");

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Number Of Collision Pairs");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::Text("%5.2i", GetNumberCollisionPairs());
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        uint32_t maxCollisionPairs = Maths::nChoosek(uint32_t(m_RigidBodys.size()), 2);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Max Number Of Collision Pairs");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::Text("%5.2i", maxCollisionPairs);
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Number Of Rigid Bodys");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::Text("%5.2i", GetNumberRigidBodys());
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Number Of Constraints");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::Text("%5.2i", static_cast<int>(m_Constraints.size()));
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Paused");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::Checkbox("##Paused", &m_IsPaused);
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Gravity");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::InputFloat3("##Gravity", &m_Gravity.x);
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Damping Factor");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        ImGui::InputFloat("##Damping Factor", &m_DampingFactor);
        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Integration Type");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::BeginMenu(IntegrationTypeToString(m_IntegrationType).c_str()))
        {
            if(ImGui::MenuItem("EXPLICIT EULER", "", static_cast<int>(m_IntegrationType) == 0, true))
            {
                m_IntegrationType = IntegrationType::EXPLICIT_EULER;
            }
            if(ImGui::MenuItem("SEMI IMPLICIT EULER", "", static_cast<int>(m_IntegrationType) == 1, true))
            {
                m_IntegrationType = IntegrationType::SEMI_IMPLICIT_EULER;
            }
            if(ImGui::MenuItem("RUNGE KUTTA 2", "", static_cast<int>(m_IntegrationType) == 2, true))
            {
                m_IntegrationType = IntegrationType::RUNGE_KUTTA_2;
            }
            if(ImGui::MenuItem("RUNGE KUTTA 4", "", static_cast<int>(m_IntegrationType) == 3, true))
            {
                m_IntegrationType = IntegrationType::RUNGE_KUTTA_4;
            }
            ImGui::EndMenu();
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();
    }

    void PhysicsEngine::OnDebugDraw()
    {

        if(m_DebugDrawFlags & PhysicsDebugFlags::MANIFOLD)
        {
            for(Manifold &m: m_Manifolds)
                m.DebugDraw();
        }

        // Draw all constraints
        if(m_DebugDrawFlags & PhysicsDebugFlags::CONSTRAINT)
        {
            for(Constraint* c: m_Constraints)
                c->DebugDraw();
        }

        if(!m_IsPaused && m_BroadphaseDetection && (m_DebugDrawFlags & PhysicsDebugFlags::BROADPHASE))
            m_BroadphaseDetection->DebugDraw();

        auto scene = gEngine->GetLevelManager()->GetCurrentLevel();
        auto &registry = scene->GetRegistry();

        auto group = registry.group<RigidBody3DComponent>(entt::get<Transform>);

        if(group.empty())
            return;

        for(auto entity: group)
        {
            const auto &phys = group.get<RigidBody3DComponent>(entity);

            auto &physicsObj = phys.GetRigidBody();

            if(physicsObj)
            {
                physicsObj->DebugDraw(m_DebugDrawFlags);
                if(physicsObj->GetCollisionShape() && (m_DebugDrawFlags & PhysicsDebugFlags::COLLISIONVOLUMES))
                    physicsObj->GetCollisionShape()->DebugDraw(physicsObj.get());
            }
        }
    }

} // NekoEngine