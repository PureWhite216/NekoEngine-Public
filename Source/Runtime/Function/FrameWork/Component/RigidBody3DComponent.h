#pragma once
#include "ConstraintComponent.h"

namespace NekoEngine
{

    class RigidBody3DComponent
    {
    private:
        SharedPtr<RigidBody3D> m_RigidBody;

    public:
        RigidBody3DComponent();
        RigidBody3DComponent(const RigidBody3DComponent& other);

        explicit RigidBody3DComponent(SharedPtr<RigidBody3D>& physics);

        ~RigidBody3DComponent() = default;

        void Init();
        void Update();
        void OnImGui();

        const SharedPtr<RigidBody3D>& GetRigidBody() const
        {
            return m_RigidBody;
        }

        template <typename Archive>
        void save(Archive& archive) const
        {
            archive(*m_RigidBody.get());
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            m_RigidBody = MakeShared<RigidBody3D>();
            archive(*m_RigidBody.get());
        }
    };

} // NekoEngine

