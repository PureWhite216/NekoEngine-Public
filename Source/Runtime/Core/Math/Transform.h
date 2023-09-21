#pragma once
#include "Core.h"
#include "cereal/cereal.hpp"
#include "GlmSerizlization.h"

namespace NekoEngine
{

    class Transform
    {
    public:
        glm::mat4 localMatrix;
        glm::mat4 worldMatrix;
        glm::mat4 parentMatrix;

        glm::vec3 localPosition;
        glm::vec3 localScale;
        glm::quat localOrientation;

        bool isDirty = false;
        bool hasUpdated = false;
    public:
        Transform();
        Transform(const glm::mat4& matrix);
        Transform(const glm::vec3& position);
        ~Transform() = default;

        void ApplyTransform();
        void UpdateMatrices();

        void SetWorldMatrix(const glm::mat4& mat);

        void SetLocalTransform(const glm::mat4& localMat);

        void SetLocalPosition(const glm::vec3& localPos);
        void SetLocalScale(const glm::vec3& newScale);
        void SetLocalOrientation(const glm::quat& quat);

        const glm::mat4& GetWorldMatrix();
        glm::mat4 GetLocalMatrix();

        const glm::vec3 GetWorldPosition();
        const glm::quat GetWorldOrientation();

        const glm::vec3& GetLocalPosition() const;
        const glm::vec3& GetLocalScale() const;
        const glm::quat& GetLocalOrientation() const;


        glm::vec3 GetUpDirection()
        {
            glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
            up           = GetWorldOrientation() * up;
            return up;
        }

        glm::vec3 GetRightDirection()
        {
            glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
            right           = GetWorldOrientation() * right;
            return right;
        }

        glm::vec3 GetForwardDirection()
        {
            glm::vec3 forward = glm::vec3(0.0f, 0.0f, 1.0f);
            forward           = GetWorldOrientation() * forward;
            return forward;
        }

        template <typename Archive>
        void save(Archive& archive) const
        {
            archive(cereal::make_nvp("Position", localPosition), cereal::make_nvp("Rotation", localOrientation), cereal::make_nvp("Scale", localScale));
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            archive(cereal::make_nvp("Position", localPosition), cereal::make_nvp("Rotation", localOrientation), cereal::make_nvp("Scale", localScale));
            isDirty = true;
        }
    };

} // NekoEngine


