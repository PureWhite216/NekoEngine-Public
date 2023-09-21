#pragma once
#include "Core.h"
#include "BoundingBox.h"
namespace NekoEngine
{

    class Ray
    {
    public:
        FVector3 Origin;
        FVector3 Direction;

    public:
        Ray();
        Ray(const glm::vec3& origin, const glm::vec3& direction);

        bool Intersects(const BoundingBox& bb) const;
        bool Intersects(const BoundingBox& bb, float& t) const;
        bool IntersectsTriangle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, float& t) const;
    };

} // NekoEngine

