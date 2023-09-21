#include "EntityFactory.h"
#include "Math/Maths.h"
#include "Math/Transform.h"
#include "Component/ModelComponent.h"
#include "Renderable/Light.h"
#include "RigidBody/RigidBody3D.h"
#include "Component/RigidBody3DComponent.h"
#include "EntityManager.h"

namespace NekoEngine
{
    Color GenColor(float alpha)
    {
        Color c;
        c.w = alpha;

        c.x = Random32::Rand(0.0f, 1.0f);
        c.y = Random32::Rand(0.0f, 1.0f);
        c.z = Random32::Rand(0.0f, 1.0f);

        return c;
    }

    Entity BuildSphereObject(Level* level, const std::string &name, const glm::vec3 &pos, float radius, bool physics_enabled,
                      float inverse_mass, bool collidable, const glm::vec4 &color)
    {
        auto sphere = level->GetEntityManager()->Create(name);
        sphere.AddComponent<Transform>(glm::translate(glm::mat4(1.0), pos) * glm::scale(glm::mat4(1.0), glm::vec3(radius * 2.0f)));
        auto& model = sphere.AddComponent<ModelComponent>(PrimitiveType::Sphere).model;

        SharedPtr<Material> matInstance = MakeShared<Material>();
        MaterialProperties properties;
        properties.albedoColour       = color;
        properties.roughness          = Random32::Rand(0.0f, 1.0f);
        properties.metallic           = Random32::Rand(0.0f, 1.0f);
        properties.albedoMapFactor    = 0.0f;
        properties.roughnessMapFactor = 0.0f;
        properties.normalMapFactor    = 0.0f;
        properties.metallicMapFactor  = 0.0f;
        matInstance->SetMaterialProperites(properties);

        // auto shader = gEngine->GetShaderLibrary()->GetResource("//CoreShaders/ForwardPBR.shader");
        // matInstance->SetShader(nullptr);//shader);
        model->GetMeshes().front()->SetMaterial(matInstance);

        if(physics_enabled)
        {
            // Otherwise create a physics object, and set it's position etc
            SharedPtr<RigidBody3D> testPhysics = MakeShared<RigidBody3D>();

            testPhysics->SetPosition(pos);
            testPhysics->SetInverseMass(inverse_mass);

            if(!collidable)
            {
                // Even without a collision shape, the inertia matrix for rotation has to be derived from the objects shape
                testPhysics->SetInverseInertia(SphereCollisionShape(radius).BuildInverseInertia(inverse_mass));
            }
            else
            {
                testPhysics->SetCollisionShape(MakeShared<SphereCollisionShape>(radius));
                testPhysics->SetInverseInertia(testPhysics->GetCollisionShape()->BuildInverseInertia(inverse_mass));
            }

            sphere.AddComponent<RigidBody3DComponent>(testPhysics);
        }
        else
        {
            sphere.GetTransform().SetLocalPosition(pos);
        }

        return sphere;
    }

    Entity BuildCuboidObject(Level* level, const std::string &name, const glm::vec3 &pos, const glm::vec3 &scale,
                             bool physics_enabled, float inverse_mass, bool collidable, const glm::vec4 &color)
    {
        auto cube = level->GetEntityManager()->Create(name);
        cube.AddComponent<Transform>(glm::translate(glm::mat4(1.0), pos) * glm::scale(glm::mat4(1.0), scale * 2.0f));
        auto& model = cube.AddComponent<ModelComponent>(PrimitiveType::Cube).model;

        auto matInstance = MakeShared<Material>();
        MaterialProperties properties;
        properties.albedoColour       = color;
        properties.roughness          = Random32::Rand(0.0f, 1.0f);
        properties.metallic           = Random32::Rand(0.0f, 1.0f);
        properties.emissive           = 3.0f;
        properties.albedoMapFactor    = 0.0f;
        properties.roughnessMapFactor = 0.0f;
        properties.normalMapFactor    = 0.0f;
        properties.metallicMapFactor  = 0.0f;
        properties.emissiveMapFactor  = 0.0f;
        properties.occlusionMapFactor = 0.0f;
        matInstance->SetMaterialProperites(properties);

        // auto shader = gEngine->GetShaderLibrary()->GetResource("//CoreShaders/ForwardPBR.shader");
        // matInstance->SetShader(shader);
        model->GetMeshes().front()->SetMaterial(matInstance);

        if(physics_enabled)
        {
            // Otherwise create a physics object, and set it's position etc
            SharedPtr<RigidBody3D> testPhysics = MakeShared<RigidBody3D>();

            testPhysics->SetPosition(pos);
            testPhysics->SetInverseMass(inverse_mass);

            if(!collidable)
            {
                // Even without a collision shape, the inertia matrix for rotation has to be derived from the objects shape
                testPhysics->SetInverseInertia(CuboidCollisionShape(scale).BuildInverseInertia(inverse_mass));
            }
            else
            {
                testPhysics->SetCollisionShape(MakeShared<CuboidCollisionShape>(scale));
                testPhysics->SetInverseInertia(testPhysics->GetCollisionShape()->BuildInverseInertia(inverse_mass));
            }

            cube.AddComponent<RigidBody3DComponent>(testPhysics);
        }
        else
        {
            cube.GetTransform().SetLocalPosition(pos);
        }

        return cube;
    }

    Entity BuildPyramidObject(Level* Level, const std::string &name, const glm::vec3 &pos, const glm::vec3 &scale,
                              bool physics_enabled, float inverse_mass, bool collidable, const glm::vec4 &color)
    {
        return Entity();
    }

    void AddLightCube(Level* level, const glm::vec3 &pos, const glm::vec3 &dir)
    {
        glm::vec4 color = glm::vec4(Random32::Rand(0.0f, 1.0f),
                                     Random32::Rand(0.0f, 1.0f),
                                     Random32::Rand(0.0f, 1.0f),
                                     1.0f);

        entt::registry& registry = level->GetRegistry();

        auto cube = BuildCuboidObject(
                level,
                "light Cube",
                pos,
                glm::vec3(0.5f, 0.5f, 0.5f),
                true,
                1.0f,
                true,
                color);

        // cube.GetComponent<RigidBody3DComponent>().GetRigidBody()->SetIsAtRest(true);
        const float radius    = Random32::Rand(1.0f, 30.0f);
        const float intensity = Random32::Rand(0.0f, 2.0f) * 120000.0f;

        cube.AddComponent<Light>(pos, color, intensity, LightType::PointLight, pos, radius);
        const glm::vec3 forward = dir;
        cube.GetComponent<RigidBody3DComponent>().GetRigidBody()->SetLinearVelocity(forward * 30.0f);
    }

    void AddSphere(Level* level, const glm::vec3 &pos, const glm::vec3 &dir)
    {
        entt::registry& registry = level->GetRegistry();

        auto sphere = BuildSphereObject(
                level,
                "Sphere",
                pos,
                0.5f,
                true,
                1.0f,
                true,
                glm::vec4(Random32::Rand(0.0f, 1.0f),
                          Random32::Rand(0.0f, 1.0f),
                          Random32::Rand(0.0f, 1.0f),
                          1.0f));

        const glm::vec3 forward = dir;
        sphere.GetComponent<RigidBody3DComponent>().GetRigidBody()->SetLinearVelocity(forward * 30.0f);
    }

    void AddPyramid(Level* Level, const glm::vec3 &pos, const glm::vec3 &dir)
    {

    }
} // NekoEngine