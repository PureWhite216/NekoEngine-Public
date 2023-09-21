#include "ImGui/imgui.h"
#include "RigidBody3DComponent.h"
#include "RigidBody/RigidBody3D.h"
namespace NekoEngine
{

    RigidBody3DComponent::RigidBody3DComponent()
            : m_RigidBody(MakeShared<RigidBody3D>())
    {
    }

    RigidBody3DComponent::RigidBody3DComponent(SharedPtr<RigidBody3D>& physics)
            : m_RigidBody(physics)
    {
    }

    RigidBody3DComponent::RigidBody3DComponent(const RigidBody3DComponent& other)
    {
        m_RigidBody = MakeShared<RigidBody3D>(*other.GetRigidBody().get());
    }

    void RigidBody3DComponent::Init()
    {
    }

    void RigidBody3DComponent::Update()
    {
    }

    void RigidBody3DComponent::OnImGui()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        auto pos             = m_RigidBody->GetPosition();
        auto torque          = m_RigidBody->GetTorque();
        auto orientation     = m_RigidBody->GetOrientation();
        auto angularVelocity = m_RigidBody->GetAngularVelocity();
        auto friction        = m_RigidBody->GetFriction();
        auto isStatic        = m_RigidBody->GetIsStatic();
        auto isRest          = m_RigidBody->GetIsAtRest();
        auto mass            = 1.0f / m_RigidBody->GetInverseMass();
        auto velocity        = m_RigidBody->GetLinearVelocity();
        auto elasticity      = m_RigidBody->GetElasticity();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Position");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat3("##Position", glm::value_ptr(pos)))
            m_RigidBody->SetPosition(pos);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Velocity");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat3("##Velocity", glm::value_ptr(velocity)))
            m_RigidBody->SetLinearVelocity(velocity);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Torque");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat3("##Torque", glm::value_ptr(torque)))
            m_RigidBody->SetTorque(torque);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Orientation");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat4("##Orientation", glm::value_ptr(orientation)))
            m_RigidBody->SetOrientation(orientation);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Angular Velocity");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat3("##Angular Velocity", glm::value_ptr(angularVelocity)))
            m_RigidBody->SetAngularVelocity(angularVelocity);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Friction");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat("##Friction", &friction))
            m_RigidBody->SetFriction(friction);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Mass");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat("##Mass", &mass))
            m_RigidBody->SetInverseMass(1.0f / mass);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Elasticity");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::DragFloat("##Elasticity", &elasticity))
            m_RigidBody->SetElasticity(elasticity);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Static");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::Checkbox("##Static", &isStatic))
            m_RigidBody->SetIsStatic(isStatic);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("At Rest");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::Checkbox("##At Rest", &isRest))
            m_RigidBody->SetIsAtRest(isRest);

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();
    }
} // NekoEngine