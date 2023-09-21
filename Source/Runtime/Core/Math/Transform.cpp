#include "Transform.h"
#include "glm/gtx/matrix_decompose.hpp"
#include "glm/gtx/quaternion.hpp"
namespace NekoEngine
{

    Transform::Transform()
    {
        localPosition    = glm::vec3(0.0f, 0.0f, 0.0f);
        localOrientation = glm::quat(glm::vec3(0.0f, 0.0f, 0.0f));
        localScale       = glm::vec3(1.0f, 1.0f, 1.0f);
        localMatrix      = glm::mat4(1.0f);
        worldMatrix      = glm::mat4(1.0f);
        parentMatrix     = glm::mat4(1.0f);
    }

    Transform::Transform(const glm::mat4& matrix)
    {
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(matrix, localScale, localOrientation, localPosition, skew, perspective);

        localMatrix  = matrix;
        worldMatrix  = matrix;
        parentMatrix = glm::mat4(1.0f);
    }

    Transform::Transform(const glm::vec3& position)
    {
        localPosition    = position;
        localOrientation = glm::quat(glm::vec3(0.0f, 0.0f, 0.0f));
        localScale       = glm::vec3(1.0f, 1.0f, 1.0f);
        localMatrix      = glm::mat4(1.0f);
        worldMatrix      = glm::mat4(1.0f);
        parentMatrix     = glm::mat4(1.0f);
    }

    void Transform::UpdateMatrices()
    {
        localMatrix = glm::translate(glm::mat4(1.0), localPosition) * glm::toMat4(localOrientation) * glm::scale(glm::mat4(1.0), localScale);

        worldMatrix = parentMatrix * localMatrix;
        isDirty       = false;
        hasUpdated  = true;
    }

    void Transform::ApplyTransform()
    {
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(localMatrix, localScale, localOrientation, localPosition, skew, perspective);
        isDirty      = false;
        hasUpdated = true;
    }

    void Transform::SetWorldMatrix(const glm::mat4& mat)
    {
        if(isDirty)
            UpdateMatrices();
        parentMatrix = mat;
        worldMatrix  = parentMatrix * localMatrix;
    }

    void Transform::SetLocalTransform(const glm::mat4 &localMat)
    {
        localMatrix = localMat;
        hasUpdated = true;

        ApplyTransform();

        worldMatrix = parentMatrix * localMatrix;
    }

    const glm::mat4 &Transform::GetWorldMatrix()
    {
        if(isDirty) UpdateMatrices();
        return worldMatrix;
    }

    const glm::vec3 Transform::GetWorldPosition()
    {
        if(isDirty) UpdateMatrices();
        return worldMatrix[3];
    }

    const glm::vec3 &Transform::GetLocalPosition() const
    {
        return localPosition;
    }

    const glm::vec3 &Transform::GetLocalScale() const
    {
        return localScale;
    }

    const glm::quat &Transform::GetLocalOrientation() const
    {
        return localOrientation;
    }

    const glm::quat Transform::GetWorldOrientation()
    {
        if(isDirty) UpdateMatrices();
        return glm::toQuat(worldMatrix);
    }

    glm::mat4 Transform::GetLocalMatrix()
    {
        return glm::translate(glm::mat4(1.0), localPosition) * glm::toMat4(localOrientation) * glm::scale(glm::mat4(1.0), localScale);
    }

    void Transform::SetLocalPosition(const glm::vec3 &localPos)
    {
        isDirty = true;
        localPosition = localPos;
    }

    void Transform::SetLocalOrientation(const glm::quat &quat)
    {
        isDirty = true;
        localOrientation = quat;
    }

    void Transform::SetLocalScale(const glm::vec3 &newScale)
    {
        localScale = newScale;
    }
} // NekoEngine