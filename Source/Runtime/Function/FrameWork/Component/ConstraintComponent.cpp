#include "ConstraintComponent.h"
#include "RigidBody3DComponent.h"
#include "Engine.h"
#include "Entity/EntityManager.h"
namespace NekoEngine
{
    AxisConstraintComponent::AxisConstraintComponent(Entity entity, Axes axes)
    {
        m_EntityID    = entity.GetID();
        m_Axes        = axes;
        m_Constraint  = MakeShared<AxisConstraint>(entity.GetComponent<RigidBody3DComponent>().GetRigidBody().get(), axes);
        m_Initialised = true;
    }

    const SharedPtr<AxisConstraint>& AxisConstraintComponent::GetConstraint()
    {
        if(!m_Initialised)
        {
            auto entity = gEngine->GetLevelManager()->GetCurrentLevel()->GetEntityManager()->GetEntityByUUID(m_EntityID);

            if(entity && entity.HasComponent<RigidBody3DComponent>())
                m_Constraint = MakeShared<AxisConstraint>(entity.GetComponent<RigidBody3DComponent>().GetRigidBody().get(), m_Axes);
            m_Initialised = true;
        }
        return m_Constraint;
    }

    SpringConstraintComponent::SpringConstraintComponent(Entity entity, Entity otherEntity, const glm::vec3& pos1, const glm::vec3& pos2, float constant)
    {
        m_Constraint = MakeShared<SpringConstraint>(entity.GetComponent<RigidBody3DComponent>().GetRigidBody(), otherEntity.GetComponent<RigidBody3DComponent>().GetRigidBody(), pos1, pos2, 0.9f, 0.5f);
    }
    SpringConstraintComponent::SpringConstraintComponent(Entity entity, Entity otherEntity)
    {
        m_Constraint = MakeShared<SpringConstraint>(entity.GetComponent<RigidBody3DComponent>().GetRigidBody(), otherEntity.GetComponent<RigidBody3DComponent>().GetRigidBody(), 0.9f, 0.5f);
    }

    WeldConstraintComponent::WeldConstraintComponent(Entity entity, Entity otherEntity, const glm::vec3& pos1, const glm::vec3& pos2, float constant)
    {
        m_Constraint = MakeShared<WeldConstraint>(entity.GetComponent<RigidBody3DComponent>().GetRigidBody().get(), otherEntity.GetComponent<RigidBody3DComponent>().GetRigidBody().get());
    }

    WeldConstraintComponent::WeldConstraintComponent(Entity entity, Entity otherEntity)
    {
        m_Constraint = MakeShared<WeldConstraint>(entity.GetComponent<RigidBody3DComponent>().GetRigidBody().get(), otherEntity.GetComponent<RigidBody3DComponent>().GetRigidBody().get());
    }

    DistanceConstraintComponent::DistanceConstraintComponent(Entity entity, Entity otherEntity, const glm::vec3& pos1, const glm::vec3& pos2, float constant)
    {
        m_Constraint = MakeShared<DistanceConstraint>(entity.GetComponent<RigidBody3DComponent>().GetRigidBody().get(), otherEntity.GetComponent<RigidBody3DComponent>().GetRigidBody().get(), pos1, pos2);
    }

    DistanceConstraintComponent::DistanceConstraintComponent(Entity entity, Entity otherEntity)
    {
        m_Constraint = MakeShared<DistanceConstraint>(entity.GetComponent<RigidBody3DComponent>().GetRigidBody().get(), otherEntity.GetComponent<RigidBody3DComponent>().GetRigidBody().get(),
                                                           entity.GetComponent<RigidBody3DComponent>().GetRigidBody()->GetPosition(), otherEntity.GetComponent<RigidBody3DComponent>().GetRigidBody()->GetPosition());
    }

} // NekoEngine