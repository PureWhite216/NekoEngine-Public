#pragma once
#include "Core.h"
#include "Rectangle.h"
#include "Maths.h"

namespace NekoEngine
{
    class BoundingSphere;
    class BoundingBox
    {
    public:
        glm::vec3 m_Min;
        glm::vec3 m_Max;
    public:
        BoundingBox();
        BoundingBox(const glm::vec3& min, const glm::vec3& max);
        BoundingBox(const glm::vec3* points, uint32_t numPoints);
        BoundingBox(const Rect& rect, const glm::vec3& center = glm::vec3(0.0f));
        BoundingBox(const BoundingBox& other);
        BoundingBox(BoundingBox&& other);
        ~BoundingBox();

        void Clear()
        {
            m_Min = glm::vec3(FLT_MAX);
            m_Max = glm::vec3(-FLT_MAX);
        }

        BoundingBox& operator=(const BoundingBox& other);
        BoundingBox& operator=(BoundingBox&& other);

        void Set(const glm::vec3& min, const glm::vec3& max);
        void Set(const glm::vec3* points, uint32_t numPoints);

        void SetFromPoints(const glm::vec3* points, uint32_t numPoints);
        void SetFromPoints(const glm::vec3* points, uint32_t numPoints, const glm::mat4& transform);

        void SetFromTransformedAABB(const BoundingBox& aabb, const glm::mat4& transform);

        void Translate(const glm::vec3& translation);
        void Translate(float x, float y, float z);

        void Scale(const glm::vec3& scale);
        void Scale(float x, float y, float z);

        void Rotate(const glm::mat3& rotation);

        void Transform(const glm::mat4& transform);
        BoundingBox Transformed(const glm::mat4& transform) const;

        void Merge(const BoundingBox& other);
        void Merge(const glm::vec3& point);

        void Merge(const BoundingBox& other, const glm::mat4& transform);
        void Merge(const glm::vec3& point, const glm::mat4& transform);

        void Merge(const BoundingBox& other, const glm::mat3& transform);
        void Merge(const glm::vec3& point, const glm::mat3& transform);

        Intersection IsInside(const glm::vec3& point) const;
        Intersection IsInside(const BoundingBox& box) const;
        Intersection IsInside(const BoundingSphere& sphere) const;

        bool IsInsideFast(const BoundingBox& box) const;

        glm::vec3 Size() const;
        glm::vec3 Center() const;
        glm::vec3 GetMin() const;
        glm::vec3 GetMax() const;

        glm::vec3 GetExtents() const { return m_Max - m_Min; }

    };

} // NekoEngine
