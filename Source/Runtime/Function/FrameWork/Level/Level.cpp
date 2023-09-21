#include "Level.h"
#include "Component/ModelComponent.h"
#include "Entity/EntityManager.h"
#include "Renderable/Camera.h"
#include "Renderable/Light.h"
#include "Transform.h"
#include "Component/RigidBody3DComponent.h"
#include "Engine.h"
#include "PhysicsEngine.h"
#include "iostream"
#include "File/FileSystem.h"
#include "Renderable/Environment.h"
#include "Script/LuaScriptComponent.h"
#include <fstream>
#include "ostream"
#include "Component/PrefabComponent.h"
#include <cereal/cereal.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include "GlmSerizlization.h"
#include "Entity/Entity.h"
#include "Collision/CapsuleCollisionShape.h"
//#include "Renderable/Material.h"

#define ALL_COMPONENTSV1 Transform, NameComponent, ActiveComponent, Hierarchy, Camera, LuaScriptComponent, Model, Light, RigidBody3DComponent, Environment, DefaultCameraController
#define ALL_COMPONENTSV8 ALL_COMPONENTSV1, AxisConstraintComponent, ModelComponent, IDComponent

#define ALL_COMPONENTSLISTV8 ALL_COMPONENTSV8


CEREAL_REGISTER_TYPE(NekoEngine::SphereCollisionShape);
CEREAL_REGISTER_TYPE(NekoEngine::CuboidCollisionShape);
CEREAL_REGISTER_TYPE(NekoEngine::CapsuleCollisionShape);

CEREAL_REGISTER_POLYMORPHIC_RELATION(NekoEngine::CollisionShape, NekoEngine::SphereCollisionShape);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NekoEngine::CollisionShape, NekoEngine::CuboidCollisionShape);
CEREAL_REGISTER_POLYMORPHIC_RELATION(NekoEngine::CollisionShape, NekoEngine::CapsuleCollisionShape);

namespace entt
{

    /**
     * @brief Utility class to restore a snapshot as a whole.
     *
     * A snapshot loader requires that the destination registry be empty and loads
     * all the data at once while keeping intact the identifiers that the entities
     * originally had.<br/>
     * An example of use is the implementation of a save/restore utility.
     *
     * @tparam Entity A valid entity type (see entt_traits for more details).
     */
    template<typename Entity>
    class basic_snapshot_loader_legacy
    {
        /*! @brief A registry is allowed to create snapshot loaders. */
        friend class basic_registry<Entity>;

        using traits_type = entt_traits<Entity>;

        template<typename Type, typename Archive>
        void assign(Archive &archive) const
        {
            typename traits_type::entity_type length{};
            archive(length);

            entity_type entt{};

            if constexpr(std::is_empty_v<Type>)
            {
                while(length--)
                {
                    archive(entt);
                    const auto entity = reg->valid(entt) ? entt : reg->create(entt);
                    // ENTT_ASSERT(entity == entt);
                    reg->template emplace<Type>(entity);
                }
            }
            else
            {
                Type instance{};

                while(length--)
                {
                    archive(entt, instance);
                    const auto entity = reg->valid(entt) ? entt : reg->create(entt);
                    // ENTT_ASSERT(entity == entt);
                    reg->template emplace<Type>(entity, std::move(instance));
                }
            }
        }

    public:
        /*! @brief Underlying entity identifier. */
        using entity_type = Entity;

        /**
         * @brief Constructs an instance that is bound to a given registry.
         * @param source A valid reference to a registry.
         */
        basic_snapshot_loader_legacy(basic_registry<entity_type> &source) noexcept
                : reg{&source}
        {
            // restoring a snapshot as a whole requires a clean registry
            // ENTT_ASSERT(reg->empty());
        }

        /*! @brief Default move constructor. */
        basic_snapshot_loader_legacy(basic_snapshot_loader_legacy &&) = default;

        /*! @brief Default move assignment operator. @return This loader. */
        basic_snapshot_loader_legacy &operator=(basic_snapshot_loader_legacy &&) = default;

        /**
         * @brief Restores entities that were in use during serialization.
         *
         * This function restores the entities that were in use during serialization
         * and gives them the versions they originally had.
         *
         * @tparam Archive Type of input archive.
         * @param archive A valid reference to an input archive.
         * @return A valid loader to continue restoring data.
         */
        template<typename Archive>
        const basic_snapshot_loader_legacy &entities(Archive &archive) const
        {
            typename traits_type::entity_type length{};

            archive(length);
            std::vector<entity_type> all(length);

            for(decltype(length) pos{}; pos < length; ++pos)
            {
                archive(all[pos]);
            }

            reg->assign(all.cbegin(), all.cend(), 0);

            return *this;
        }

        /**
         * @brief Restores components and assigns them to the right entities.
         *
         * The template parameter list must be exactly the same used during
         * serialization. In the event that the entity to which the component is
         * assigned doesn't exist yet, the loader will take care to create it with
         * the version it originally had.
         *
         * @tparam Component Types of components to restore.
         * @tparam Archive Type of input archive.
         * @param archive A valid reference to an input archive.
         * @return A valid loader to continue restoring data.
         */
        template<typename... Component, typename Archive>
        const basic_snapshot_loader_legacy &component(Archive &archive) const
        {
            (assign<Component>(archive), ...);
            return *this;
        }

        /**
         * @brief Destroys those entities that have no components.
         *
         * In case all the entities were serialized but only part of the components
         * was saved, it could happen that some of the entities have no components
         * once restored.<br/>
         * This functions helps to identify and destroy those entities.
         *
         * @return A valid loader to continue restoring data.
         */
        const basic_snapshot_loader_legacy &orphans() const
        {
            /*        reg->orphans([this](const auto entt) {
                        reg->destroy(entt);
                        });*/
            return *this;
        }

    private:
        basic_registry<entity_type>* reg;
    };
}

#define ALL_COMPONENTSENTTV8(output) get<entt::entity>(output).get<Transform>(output).get<NameComponent>(output).get<ActiveComponent>(output).get<Hierarchy>(output).get<Camera>(output).get<LuaScriptComponent>(output).get<Model>(output).get<Light>(output).get<RigidBody3DComponent>(output).get<Environment>(output).get<DefaultCameraController>(output).get<IDComponent>(output).get<ModelComponent>(output).get<AxisConstraintComponent>(output);

namespace NekoEngine
{
    Level::Level(const String &name)
    {
        levelName = name;

        m_EntityManager = MakeUnique<EntityManager>(this);
        m_EntityManager->AddDependency<RigidBody3DComponent, Transform>();
//        m_EntityManager->AddDependency<RigidBody2DComponent, Transform>();
        m_EntityManager->AddDependency<Camera, Transform>();
        m_EntityManager->AddDependency<ModelComponent, Transform>();
        m_EntityManager->AddDependency<Light, Transform>();
//        m_EntityManager->AddDependency<Sprite, Transform>();
//        m_EntityManager->AddDependency<AnimatedSprite, Transform>();
//        m_EntityManager->AddDependency<Font, Transform>();
    }

    Level::~Level()
    {
        m_EntityManager->Clear();
    }

    void Level::Init()
    {
        gEngine->GetLuaManager()->GetState().set("registry", &m_EntityManager->GetRegistry());
        gEngine->GetLuaManager()->GetState().set("scene", this);

        // Physics setup
        auto physics3DSystem = gEngine->GetSystem<PhysicsEngine>();
        physics3DSystem->SetDampingFactor(sceneSettings.physicsSettings.Dampening);
        physics3DSystem->SetIntegrationType((IntegrationType) sceneSettings.physicsSettings.IntegrationTypeIndex);
        physics3DSystem->SetMaxUpdatesPerFrame(sceneSettings.physicsSettings.m_MaxUpdatesPerFrame);
        physics3DSystem->SetPositionIterations(sceneSettings.physicsSettings.PositionIterations);
        physics3DSystem->SetVelocityIterations(sceneSettings.physicsSettings.VelocityIterations);
        physics3DSystem->SetBroadphaseType(BroadphaseType(sceneSettings.physicsSettings.BroadPhaseTypeIndex));

        gEngine->GetLuaManager()->OnInit(this);
    }

    void Level::OnUpdate(const TimeStep &timeStep)
    {
        const glm::vec2 &mousePos = gEngine->GetInput()->GetMousePosition();

        auto defaultCameraControllerView = m_EntityManager->GetEntitiesWithType<DefaultCameraController>();
        auto cameraView = m_EntityManager->GetEntitiesWithType<Camera>();
        Camera* camera = nullptr;
        if(!cameraView.Empty())
        {
            camera = &cameraView.Front().GetComponent<Camera>();
        }

        if(!defaultCameraControllerView.Empty())
        {
            auto &cameraController = defaultCameraControllerView.Front().GetComponent<DefaultCameraController>();
            auto trans = defaultCameraControllerView.Front().TryGetComponent<Transform>();
            if(gEngine->GetSceneActive() && trans && cameraController.GetController())
            {
                cameraController.GetController()->SetCamera(camera);
                cameraController.GetController()->HandleMouse(*trans, (float) timeStep.GetSeconds(), mousePos.x,
                                                              mousePos.y);
                cameraController.GetController()->HandleKeyboard(*trans, (float) timeStep.GetSeconds());
            }
        }

        m_LevelGraph->Update(m_EntityManager->GetRegistry());

    }

    void Level::OnEvent(Event &e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(OnWindowResize));
    }

    bool Level::OnWindowResize(WindowResizeEvent &e)
    {
        if(!gEngine->GetSceneActive())
            return false;

        auto cameraView = m_EntityManager->GetRegistry().view<Camera>();
        if(!cameraView.empty())
        {
            m_EntityManager->GetRegistry().get<Camera>(cameraView.front()).SetAspectRatio(
                    static_cast<float>(e.GetWidth()) / static_cast<float>(e.GetHeight()));
        }

        return false;
    }

    void Level::SetScreenSize(uint32_t width, uint32_t height)
    {
        screenWidth = width;
        screenHeight = height;

        auto cameraView = m_EntityManager->GetRegistry().view<Camera>();
        if(!cameraView.empty())
        {
            m_EntityManager->GetRegistry().get<Camera>(cameraView.front()).SetAspectRatio(
                    static_cast<float>(screenWidth) / static_cast<float>(screenHeight));
        }
    }

    void Level::OnCleanup()
    {
        DeleteAllGameObjects();

        gEngine->GetLuaManager()->GetState().collect_garbage();
    }

    void Level::DeleteAllGameObjects()
    {
        m_EntityManager->Clear();
    }

    void Level::DuplicateEntity(Entity entity)
    {
       DuplicateEntity(entity, Entity(entt::null, nullptr));
    }

    template <typename T>
    static void CopyComponentIfExists(entt::entity dst, entt::entity src, entt::registry& registry)
    {
        if(registry.all_of<T>(src))
        {
            auto& srcComponent = registry.get<T>(src);
            registry.emplace_or_replace<T>(dst, srcComponent);
        }
    }

    template <typename... Component>
    static void CopyEntity(entt::entity dst, entt::entity src, entt::registry& registry)
    {
        (CopyComponentIfExists<Component>(dst, src, registry), ...);
    }

    void Level::DuplicateEntity(Entity entity, Entity parent)
    {

        m_LevelGraph->DisableOnConstruct(true, m_EntityManager->GetRegistry());

        Entity newEntity = m_EntityManager->Create();

        CopyEntity<ALL_COMPONENTSLISTV8>(newEntity.GetHandle(), entity.GetHandle(), m_EntityManager->GetRegistry());
        newEntity.GetComponent<IDComponent>().ID = UUID();

        auto hierarchyComponent = newEntity.TryGetComponent<Hierarchy>();
        if(hierarchyComponent)
        {
            hierarchyComponent->m_First  = entt::null;
            hierarchyComponent->m_Parent = entt::null;
            hierarchyComponent->m_Next   = entt::null;
            hierarchyComponent->m_Prev   = entt::null;
        }

        auto children = entity.GetChildren();
        std::vector<Entity> copiedChildren;

        for(auto child : children)
        {
            DuplicateEntity(child, newEntity);
        }

        if(parent)
            newEntity.SetParent(parent);

        m_LevelGraph->DisableOnConstruct(false, m_EntityManager->GetRegistry());
    }

    Entity Level::CreateEntity()
    {
        return m_EntityManager->Create();
    }

    Entity Level::CreateEntity(const std::string &name)
    {
        return m_EntityManager->Create(name);
    }

    Entity Level::GetEntityByUUID(uint64_t id)
    {
        return m_EntityManager->GetEntityByUUID(id);
    }

    void DeserializeEntityHierarchy(Entity entity, cereal::JSONInputArchive& archive, int version)
    {
        // Serialize the current entity
//        if(version == 2)
//            DeserialiseEntity<ALL_COMPONENTSLISTV8>(entity, archive);
        entity.ClearChildren();

        // Serialize the children recursively
        int children; // = entity.GetChildren();
        archive(children);

        for(int i = 0; i < children; i++)
        {
            auto child = entity.GetLevel()->GetEntityManager()->Create();
            DeserializeEntityHierarchy(child, archive, version);
            child.SetParent(entity);
        }
    }

    template <typename T>
    static void SerialiseComponentIfExists(Entity entity, cereal::JSONOutputArchive& archive)
    {
//        bool hasComponent = entity.HasComponent<T>();
//        archive(hasComponent);
//        if(hasComponent)
//            archive(entity.GetComponent<T>());
    }

    template <typename... Component>
    static void SerialiseEntity(Entity entity, cereal::JSONOutputArchive& archive)
    {
        (SerialiseComponentIfExists<Component>(entity, archive), ...);
    }

    void SerializeEntityHierarchy(Entity entity, cereal::JSONOutputArchive& archive)
    {
        // Serialize the current entity
        SerialiseEntity<ALL_COMPONENTSLISTV8()>(entity, archive);

        // Serialize the children recursively
        auto children = entity.GetChildren();
        archive((int)children.size());

        for(auto child : children)
        {
            SerializeEntityHierarchy(child, archive);
        }
    }

    Entity Level::InstantiatePrefab(const std::string &path)
    {
        std::string prefabData = VirtualFileSystem::ReadTextFile(path);
        std::stringstream storage(prefabData);
        cereal::JSONInputArchive input(storage);

        int version;
        int SceneVersion;
        input(cereal::make_nvp("Version", version));
        input(cereal::make_nvp("Scene Version", SceneVersion));


        Entity entity = m_EntityManager->Create();
        DeserializeEntityHierarchy(entity, input, version);

        std::string relativePath;
        if(VirtualFileSystem::AbsoulePathToVFS(path, relativePath))
            entity.AddComponent<PrefabComponent>(relativePath);
        else
            entity.AddComponent<PrefabComponent>(path);


        return entity;
    }

    void Level::SavePrefab(Entity entity, const std::string &path)
    {
        std::stringstream storage;

        // output finishes flushing its contents when it goes out of scope
        cereal::JSONOutputArchive output { storage };


        // Serialize a single entity
        SerializeEntityHierarchy(entity, output);

        FileSystem::WriteTextFile(path, storage.str());
        std::string relativePath;
        if(VirtualFileSystem::AbsoulePathToVFS(path, relativePath))
            entity.AddComponent<PrefabComponent>(relativePath);
    }

    void Level::UpdateLevelGraph()
    {
        m_LevelGraph->Update(m_EntityManager->GetRegistry());
    }

    entt::registry &Level::GetRegistry()
    { return m_EntityManager->GetRegistry(); }

    void Level::Serialise(const std::string &filePath, bool binary)
    {
        LOG("Level saved");
        std::string path = filePath;
        path += levelName; // StringUtility::RemoveSpaces(m_LevelName);

        if(binary)
        {
            path += std::string(".bin");

            std::ofstream file(path, std::ios::binary);

            {
                // output finishes flushing its contents when it goes out of scope
                cereal::BinaryOutputArchive output{file};
                output(*this);
                entt::snapshot{m_EntityManager->GetRegistry()}.get<entt::entity>(output).get<Transform>(
                        output).get<NameComponent>(output).get<ActiveComponent>(output).get<Hierarchy>(
                        output).get<Camera>(output).get<LuaScriptComponent>(output).get<Model>(
                        output).get<Light>(output).get<RigidBody3DComponent>(
                        output).get<Environment>(output).get<DefaultCameraController>(
                        output).get<IDComponent>(output).get<ModelComponent>(
                        output).get<AxisConstraintComponent>(output);
            }
            file.close();
        }
        else
        {
            std::stringstream storage;
            path += std::string(".lsn");

            {
                // output finishes flushing its contents when it goes out of scope
                cereal::JSONOutputArchive output{storage};
                output(*this);
                entt::snapshot{m_EntityManager->GetRegistry()}.get<entt::entity>(output).get<Transform>(
                        output).get<NameComponent>(output).get<ActiveComponent>(output).get<Hierarchy>(
                        output).get<Camera>(output).get<LuaScriptComponent>(output).get<Model>(
                        output).get<Light>(output).get<RigidBody3DComponent>(
                        output).get<Environment>(output).get<DefaultCameraController>(
                        output).get<IDComponent>(output).get<ModelComponent>(
                        output).get<AxisConstraintComponent>(output);
            }
            FileSystem::WriteTextFile(path, storage.str());
        }
    }

    void Level::Deserialise(const std::string &filePath, bool binary)
    {
        m_EntityManager->Clear();
        m_LevelGraph->DisableOnConstruct(true, m_EntityManager->GetRegistry());
        std::string path = filePath;
        path += levelName; // StringUtility::RemoveSpaces(m_LevelName);

        if(binary)
        {
            path += std::string(".bin");

            if(!FileSystem::FileExists(path))
            {
                LOG("No saved scene file found");
                return;
            }

            try
            {
                std::ifstream file(path, std::ios::binary);
                cereal::BinaryInputArchive input(file);
                input(*this);

                entt::snapshot_loader { m_EntityManager->GetRegistry() }.get<entt::entity>(input).get<entt::entity>(
                        input).get<Transform>(input).get<NameComponent>(input).get<ActiveComponent>(
                        input).get<Hierarchy>(input).get<Camera>(input).get<LuaScriptComponent>(input).get<Model>(
                        input).get<Light>(input).get<RigidBody3DComponent>(input).get<Environment>(
                        input).get<DefaultCameraController>(input).get<IDComponent>(input).get<ModelComponent>(
                        input).get<AxisConstraintComponent>(input);;
                    
            }
            catch(...)
            {
                LOG("Failed to load scene");
            }
        }
        else
        {
            path += std::string(".lsn");

            if(!FileSystem::FileExists(path))
            {
                LOG("No saved scene file found");
                return;
            }
            try
            {
                std::string data = FileSystem::ReadTextFile(path);
                std::istringstream istr;
                istr.str(data);
                cereal::JSONInputArchive input(istr);
                input(*this);

                entt::snapshot_loader { m_EntityManager->GetRegistry() }.get<entt::entity>(input).ALL_COMPONENTSENTTV8(input);
                
            }
            catch(...)
            {
                LOG_FORMAT("Failed to load scene - %s", path.c_str());
            }
        }

        m_LevelGraph->DisableOnConstruct(false, m_EntityManager->GetRegistry());
        gEngine->OnNewLevel(this);
    }

} // NekoEngine