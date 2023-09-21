#include "Renderable2D.h"

namespace NekoEngine
{
    const std::array<glm::vec2, 4>& Renderable2D::GetDefaultUVs()
    {
        static std::array<glm::vec2, 4> results;
        {
            results[0] = glm::vec2(0, 1);
            results[1] = glm::vec2(1, 1);
            results[2] = glm::vec2(1, 0);
            results[3] = glm::vec2(0, 0);
        }
        return results;
    }

    const std::array<glm::vec2, 4>& Renderable2D::GetUVs(const glm::vec2& min, const glm::vec2& max)
    {
        static std::array<glm::vec2, 4> results;
        {
            results[0] = glm::vec2(min.x, max.y);
            results[1] = max;
            results[2] = glm::vec2(max.x, min.y);
            results[3] = min;
        }
        return results;
    }
} // NekoEngine