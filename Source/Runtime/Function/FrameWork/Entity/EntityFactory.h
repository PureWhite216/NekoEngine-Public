#pragma once
#include "Core.h"
#include "Entity.h"
#include "Level/Level.h"

namespace NekoEngine
{

    Color GenColor(float alpha);

    Entity BuildSphereObject(
            Level* Level,
            const std::string& name,
            const glm::vec3& pos,
            float radius,
            bool physics_enabled    = false,
            float inverse_mass      = 0.0f, // requires physics_enabled = true
            bool collidable         = true, // requires physics_enabled = true
            const glm::vec4& colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

    // Generates a default Cuboid object with the parameters specified
    Entity BuildCuboidObject(
            Level* Level,
            const std::string& name,
            const glm::vec3& pos,
            const glm::vec3& scale,
            bool physics_enabled    = false,
            float inverse_mass      = 0.0f, // requires physics_enabled = true
            bool collidable         = true, // requires physics_enabled = true
            const glm::vec4& colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

    // Generates a default Cuboid object with the parameters specified
    Entity BuildPyramidObject(
            Level* Level,
            const std::string& name,
            const glm::vec3& pos,
            const glm::vec3& scale,
            bool physics_enabled    = false,
            float inverse_mass      = 0.0f, // requires physics_enabled = true
            bool collidable         = true, // requires physics_enabled = true
            const glm::vec4& colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

    void AddLightCube(Level* Level, const glm::vec3& pos, const glm::vec3& dir);
    void AddSphere(Level* Level, const glm::vec3& pos, const glm::vec3& dir);
    void AddPyramid(Level* Level, const glm::vec3& pos, const glm::vec3& dir);

} // NekoEngine

