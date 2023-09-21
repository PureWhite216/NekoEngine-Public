#include "Ray.h"
#include "Maths.h"

namespace NekoEngine
{
    Ray::Ray()
    {
        Origin    = glm::vec3(0.0f);
        Direction = glm::vec3(0.0f);
    }

    Ray::Ray(const glm::vec3& origin, const glm::vec3& direction)
    {
        Origin    = origin;
        Direction = direction;
    }

    bool Ray::Intersects(const BoundingBox& bb, float& t) const
    {
        t = 0.0f;
        // Check for ray origin being inside the bb
        if(bb.IsInside(Origin))
            return true;

        float dist = Maths::M_INFINITY;

        // Check for intersecting in the X-direction
        if(Origin.x < bb.GetMin().x && Direction.x > 0.0f)
        {
            float x = (bb.GetMin().x - Origin.x) / Direction.x;
            if(x < dist)
            {
                glm::vec3 point = Origin + x * Direction;
                if(point.y >= bb.GetMin().y && point.y <= bb.GetMax().y && point.z >= bb.GetMin().z && point.z <= bb.GetMax().z)
                    dist = x;
            }
        }
        if(Origin.x > bb.GetMax().x && Direction.x < 0.0f)
        {
            float x = (bb.GetMax().x - Origin.x) / Direction.x;
            if(x < dist)
            {
                glm::vec3 point = Origin + x * Direction;
                if(point.y >= bb.GetMin().y && point.y <= bb.GetMax().y && point.z >= bb.GetMin().z && point.z <= bb.GetMax().z)
                    dist = x;
            }
        }
        // Check for intersecting in the Y-direction
        if(Origin.y < bb.GetMin().y && Direction.y > 0.0f)
        {
            float x = (bb.GetMin().y - Origin.y) / Direction.y;
            if(x < dist)
            {
                glm::vec3 point = Origin + x * Direction;
                if(point.x >= bb.GetMin().x && point.x <= bb.GetMax().x && point.z >= bb.GetMin().z && point.z <= bb.GetMax().z)
                    dist = x;
            }
        }
        if(Origin.y > bb.GetMax().y && Direction.y < 0.0f)
        {
            float x = (bb.GetMax().y - Origin.y) / Direction.y;
            if(x < dist)
            {
                glm::vec3 point = Origin + x * Direction;
                if(point.x >= bb.GetMin().x && point.x <= bb.GetMax().x && point.z >= bb.GetMin().z && point.z <= bb.GetMax().z)
                    dist = x;
            }
        }
        // Check for intersecting in the Z-direction
        if(Origin.z < bb.GetMin().z && Direction.z > 0.0f)
        {
            float x = (bb.GetMin().z - Origin.z) / Direction.z;
            if(x < dist)
            {
                glm::vec3 point = Origin + x * Direction;
                if(point.x >= bb.GetMin().x && point.x <= bb.GetMax().x && point.y >= bb.GetMin().y && point.y <= bb.GetMax().y)
                    dist = x;
            }
        }
        if(Origin.z > bb.GetMax().z && Direction.z < 0.0f)
        {
            float x = (bb.GetMax().z - Origin.z) / Direction.z;
            if(x < dist)
            {
                glm::vec3 point = Origin + x * Direction;
                if(point.x >= bb.GetMin().x && point.x <= bb.GetMax().x && point.y >= bb.GetMin().y && point.y <= bb.GetMax().y)
                    dist = x;
            }
        }

        t = dist;
        return dist < Maths::M_INFINITY;
        ;
    }

    bool Ray::Intersects(const BoundingBox& bb) const
    {
        float distance;
        return Intersects(bb, distance);
    }

    bool Ray::IntersectsTriangle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, float& t) const
    {
        const glm::vec3 E1  = b - a;
        const glm::vec3 E2  = c - a;
        const glm::vec3 N   = cross(E1, E2);
        const float det     = -glm::dot(Direction, N);
        const float invdet  = 1.f / det;
        const glm::vec3 AO  = Origin - a;
        const glm::vec3 DAO = glm::cross(AO, Direction);
        const float u       = glm::dot(E2, DAO) * invdet;
        const float v       = -glm::dot(E1, DAO) * invdet;
        t                   = glm::dot(AO, N) * invdet;
        return (det >= 1e-6f && t >= 0.0f && u >= 0.0f && v >= 0.0f && (u + v) <= 1.0f);
    }
} // NekoEngine