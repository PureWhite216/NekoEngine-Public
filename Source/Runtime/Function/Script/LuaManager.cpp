#include "LuaManager.h"
#include "sol/sol.hpp"
#include "File/FileSystem.h"
#include "entt/entt.hpp"
#include "LuaBinding.h"
#include "Engine.h"
#include "Level/Level.h"
#include "LuaScriptComponent.h"
#include "File/VirtualFileSystem.h"
#include "filesystem"
#include "PhysicsEngine.h"
#include "Entity/EntityFactory.h"
#include "Renderable/Light.h"
#include "Entity/EntityManager.h"
#include "Entity/Entity.h"
#include "Renderable/Camera.h"

namespace sol
{

    template<typename T>
    struct unique_usertype_traits<SharedPtr<T>>
    {
        typedef T type;
        typedef SharedPtr<T> actual_type;
        static const bool value = true;

        static bool is_null(const actual_type &ptr)
        {
            return ptr == nullptr;
        }

        static type* get(const actual_type &ptr)
        {
            return ptr.get();
        }
    };

    template<typename T>
    struct unique_usertype_traits<UniquePtr<T>>
    {
        typedef T type;
        typedef UniquePtr<T> actual_type;
        static const bool value = true;

        static bool is_null(const actual_type &ptr)
        {
            return ptr == nullptr;
        }

        static type* get(const actual_type &ptr)
        {
            return ptr.get();
        }
    };

    template<typename T>
    struct unique_usertype_traits<WeakPtr < T>>
    {
        typedef T type;
        typedef WeakPtr <T> actual_type;
        static const bool value = true;

        static bool is_null(const actual_type &ptr)
        {
            return ptr == nullptr;
        }

        static type* get(const actual_type &ptr)
        {
            return ptr.get();
        }
    };
}


namespace NekoEngine
{
    template <typename, typename>
    struct _ECS_export_view;

    template <typename... Component, typename... Exclude>
    struct _ECS_export_view<entt::type_list<Component...>, entt::type_list<Exclude...>>
    {
        static entt::view<entt::get_t<Component...>> view(entt::registry& registry)
        {
            return registry.view<Component...>(entt::exclude<Exclude...>);
        }
    };

#define REGISTER_COMPONENT_WITH_ECS(curLuaState, Comp, assignPtr)                                              \
    {                                                                                                          \
        using namespace entt;                                                                                  \
        auto entity_type = curLuaState["Entity"].get_or_create<sol::usertype<registry>>();                     \
        entity_type.set_function("Add" #Comp, assignPtr);                                                      \
        entity_type.set_function("Remove" #Comp, &Entity::RemoveComponent<Comp>);                              \
        entity_type.set_function("Get" #Comp, &Entity::GetComponent<Comp>);                                    \
        entity_type.set_function("GetOrAdd" #Comp, &Entity::GetOrAddComponent<Comp>);                          \
        entity_type.set_function("TryGet" #Comp, &Entity::TryGetComponent<Comp>);                              \
        entity_type.set_function("AddOrReplace" #Comp, &Entity::AddOrReplaceComponent<Comp>);                  \
        entity_type.set_function("Has" #Comp, &Entity::HasComponent<Comp>);                                    \
        auto entityManager_type = curLuaState["enttRegistry"].get_or_create<sol::usertype<registry>>();        \
        entityManager_type.set_function("view_" #Comp, &_ECS_export_view<type_list<Comp>, type_list<>>::view); \
        auto V = curLuaState.new_usertype<view<entt::get_t<Comp>>>(#Comp "_view");                             \
        V.set_function("each", &view<entt::get_t<Comp>>::each<std::function<void(Comp&)>>);                    \
        V.set_function("front", &view<entt::get_t<Comp>>::front);                                              \
        s_Identifiers.push_back(#Comp);                                                                        \
        s_Identifiers.push_back("Add" #Comp);                                                                  \
        s_Identifiers.push_back("Remove" #Comp);                                                               \
        s_Identifiers.push_back("Get" #Comp);                                                                  \
        s_Identifiers.push_back("GetOrAdd" #Comp);                                                             \
        s_Identifiers.push_back("TryGet" #Comp);                                                               \
        s_Identifiers.push_back("AddOrReplace" #Comp);                                                         \
        s_Identifiers.push_back("Has" #Comp);                                                                  \
    }

    std::vector<std::string>& LuaManager::GetIdentifiers() { return s_Identifiers; }

    std::vector<std::string> LuaManager::s_Identifiers = {
            "Log",
            "Trace",
            "Info",
            "Warn",
            "Error",
            "Critical",
            "Input",
            "GetKeyPressed",
            "GetKeyHeld",
            "GetMouseClicked",
            "GetMouseHeld",
            "GetMousePosition",
            "GetScrollOffset",
            "enttRegistry",
            "Entity",
            "EntityManager",
            "Create",
            "GetRegistry",
            "Valid",
            "Destroy",
            "SetParent",
            "GetParent",
            "IsParent",
            "GetChildren",
            "SetActive",
            "Active",
            "GetEntityByName",
            "AddPyramidEntity",
            "AddSphereEntity",
            "AddLightCubeEntity",
            "NameComponent",
            "GetNameComponent",
            "GetCurrentEntity",
            "SetThisComponent",
            "LuaScriptComponent",
            "GetLuaScriptComponent",
            "Transform",
            "GetTransform"
    };

    LuaManager::LuaManager()
            : m_State(nullptr)
    {
    }

    void LuaManager::OnInit()
    {

        m_State = new sol::state();
        m_State->open_libraries(sol::lib::base, sol::lib::package, sol::lib::math, sol::lib::table, sol::lib::os, sol::lib::string);

        //TODO: Bind Lua
        BindAppLua(*m_State);
        BindInputLua(*m_State);
        BindMathsLua(*m_State);
//        BindImGuiLua(*m_State);
//        BindECSLua(*m_State);
        BindLogLua(*m_State);
//        BindLevelLua(*m_State);
//        BindPhysicsLua(*m_State);
    }

    void LuaManager::OnInit(Level* level)
    {
        auto& registry = level->GetRegistry();

        auto view = registry.view<LuaScriptComponent>();

        if(view.empty())
            return;

        for(auto entity : view)
        {
            auto& luaScript = registry.get<LuaScriptComponent>(entity);
            luaScript.SetThisComponent();
            luaScript.OnInit();
        }
    }

    LuaManager::~LuaManager()
    {
        delete m_State;
    }

    void LuaManager::OnUpdate(Level* level)
    {
        
        auto& registry = level->GetRegistry();

        auto view = registry.view<LuaScriptComponent>();

        if(view.empty())
            return;

        float dt = (float)gEngine->GetTimeStep()->GetSeconds();

        for(auto entity : view)
        {
            auto& luaScript = registry.get<LuaScriptComponent>(entity);
            luaScript.OnUpdate(dt);
        }
    }

    void LuaManager::OnNewProject(const std::string& projectPath)
    {
        auto& state = *m_State;
        std::string ScriptsPath;
        VirtualFileSystem::ResolvePhysicalPath("//Scripts", ScriptsPath);

        // Setup the lua path to see luarocks packages
        auto package_path = std::filesystem::path(ScriptsPath) / "lua" / "?.lua;";
        package_path += std::filesystem::path(ScriptsPath) / "?" / "?.lua;";
        package_path += std::filesystem::path(ScriptsPath) / "?" / "?" / "?.lua;";

        std::string currentPaths = state["package"]["path"];
        state["package"]["path"] = std::string(package_path.string()) + currentPaths;
    }

    entt::entity GetEntityByName(entt::registry& registry, const std::string& name)
    {
        
        entt::entity e = entt::null;
        registry.view<NameComponent>().each([&](const entt::entity& entity, const NameComponent& component)
                                            {
                                                if(name == component.name)
                                                {
                                                    e = entity;
                                                } });

        return e;
    }

    void LuaManager::BindLogLua(sol::state& state)
    {
        
        auto log = state.create_table("Log");

        log.set_function("Trace", [&](sol::this_state s, std::string_view message)
        { LOG(message); });

        log.set_function("Info", [&](sol::this_state s, std::string_view message)
        { LOG(message); });

        log.set_function("Warn", [&](sol::this_state s, std::string_view message)
        { LOG(message); });

        log.set_function("Error", [&](sol::this_state s, std::string_view message)
        { LOG(message); });

        log.set_function("Critical", [&](sol::this_state s, std::string_view message)
        { LOG(message); });
    }

    void LuaManager::BindInputLua(sol::state& state)
    {
        
        auto input = state["Input"].get_or_create<sol::table>();

        input.set_function("GetKeyPressed", [](KeyCode key) -> bool
        { return gEngine->GetInput()->GetKeyPressed(key); });

        input.set_function("GetKeyHeld", [](KeyCode key) -> bool
        { return gEngine->GetInput()->GetKeyHeld(key); });

        input.set_function("GetMouseClicked", [](MouseKeyCode key) -> bool
        { return gEngine->GetInput()->GetMouseClicked(key); });

        input.set_function("GetMouseHeld", [](MouseKeyCode key) -> bool
        { return gEngine->GetInput()->GetMouseHeld(key); });

        input.set_function("GetMousePosition", []() -> glm::vec2
        { return gEngine->GetInput()->GetMousePosition(); });

        input.set_function("GetScrollOffset", []() -> float
        { return gEngine->GetInput()->GetScrollOffset(); });

        input.set_function("GetControllerAxis", [](int id, int axis) -> float
        { return gEngine->GetInput()->GetControllerAxis(id, axis); });

        input.set_function("GetControllerName", [](int id) -> std::string
        { return gEngine->GetInput()->GetControllerName(id); });

        input.set_function("GetControllerHat", [](int id, int hat) -> int
        { return gEngine->GetInput()->GetControllerHat(id, hat); });

        input.set_function("IsControllerButtonPressed", [](int id, int button) -> bool
        { return gEngine->GetInput()->IsControllerButtonPressed(id, button); });

        std::initializer_list<std::pair<sol::string_view, KeyCode>> keyItems = {
                { "A", KeyCode::A },
                { "B", KeyCode::B },
                { "C", KeyCode::C },
                { "D", KeyCode::D },
                { "E", KeyCode::E },
                { "F", KeyCode::F },
                { "H", KeyCode::G },
                { "G", KeyCode::H },
                { "I", KeyCode::I },
                { "J", KeyCode::J },
                { "K", KeyCode::K },
                { "L", KeyCode::L },
                { "M", KeyCode::M },
                { "N", KeyCode::N },
                { "O", KeyCode::O },
                { "P", KeyCode::P },
                { "Q", KeyCode::Q },
                { "R", KeyCode::R },
                { "S", KeyCode::S },
                { "T", KeyCode::T },
                { "U", KeyCode::U },
                { "V", KeyCode::V },
                { "W", KeyCode::W },
                { "X", KeyCode::X },
                { "Y", KeyCode::Y },
                { "Z", KeyCode::Z },
                //{ "UNKOWN", KeyCode::Unknown },
                { "Space", KeyCode::Space },
                { "Escape", KeyCode::Escape },
                { "APOSTROPHE", KeyCode::Apostrophe },
                { "Comma", KeyCode::Comma },
                { "MINUS", KeyCode::Minus },
                { "PERIOD", KeyCode::Period },
                { "SLASH", KeyCode::Slash },
                { "SEMICOLON", KeyCode::Semicolon },
                { "EQUAL", KeyCode::Equal },
                { "LEFT_BRACKET", KeyCode::LeftBracket },
                { "BACKSLASH", KeyCode::Backslash },
                { "RIGHT_BRACKET", KeyCode::RightBracket },
                //{ "BACK_TICK", KeyCode::BackTick },
                { "Enter", KeyCode::Enter },
                { "Tab", KeyCode::Tab },
                { "Backspace", KeyCode::Backspace },
                { "Insert", KeyCode::Insert },
                { "Delete", KeyCode::Delete },
                { "Right", KeyCode::Right },
                { "Left", KeyCode::Left },
                { "Down", KeyCode::Down },
                { "Up", KeyCode::Up },
                { "PageUp", KeyCode::PageUp },
                { "PageDown", KeyCode::PageDown },
                { "Home", KeyCode::Home },
                { "End", KeyCode::End },
                { "CAPS_LOCK", KeyCode::CapsLock },
                { "SCROLL_LOCK", KeyCode::ScrollLock },
                { "NumLock", KeyCode::NumLock },
                { "PrintScreen", KeyCode::PrintScreen },
                { "Pasue", KeyCode::Pause },
                { "LeftShift", KeyCode::LeftShift },
                { "LeftControl", KeyCode::LeftControl },
                { "LEFT_ALT", KeyCode::LeftAlt },
                { "LEFT_SUPER", KeyCode::LeftSuper },
                { "RightShift", KeyCode::RightShift },
                { "RightControl", KeyCode::RightControl },
                { "RIGHT_ALT", KeyCode::RightAlt },
                { "RIGHT_SUPER", KeyCode::RightSuper },
                { "Menu", KeyCode::Menu },
                { "F1", KeyCode::F1 },
                { "F2", KeyCode::F2 },
                { "F3", KeyCode::F3 },
                { "F4", KeyCode::F4 },
                { "F5", KeyCode::F5 },
                { "F6", KeyCode::F6 },
                { "F7", KeyCode::F7 },
                { "F8", KeyCode::F8 },
                { "F9", KeyCode::F9 },
                { "F10", KeyCode::F10 },
                { "F11", KeyCode::F11 },
                { "F12", KeyCode::F12 },
                { "Keypad0", KeyCode::D0 },
                { "Keypad1", KeyCode::D1 },
                { "Keypad2", KeyCode::D2 },
                { "Keypad3", KeyCode::D3 },
                { "Keypad4", KeyCode::D4 },
                { "Keypad5", KeyCode::D5 },
                { "Keypad6", KeyCode::D6 },
                { "Keypad7", KeyCode::D7 },
                { "Keypad8", KeyCode::D8 },
                { "Keypad9", KeyCode::D9 },
                { "Decimal", KeyCode::Period },
                { "Divide", KeyCode::Slash },
                { "Multiply", KeyCode::KPMultiply },
                { "Subtract", KeyCode::Minus },
                { "Add", KeyCode::KPAdd },
                { "KP_EQUAL", KeyCode::KPEqual }
        };
        state.new_enum<KeyCode, false>("Key", keyItems); // false makes it read/write in Lua, but its faster

        std::initializer_list<std::pair<sol::string_view, MouseKeyCode>> mouseItems = {
                { "Left", MouseKeyCode::ButtonLeft },
                { "Right", MouseKeyCode::ButtonRight },
                { "Middle", MouseKeyCode::ButtonMiddle },
        };
        state.new_enum<MouseKeyCode, false>("MouseButton", mouseItems);
    }

    SharedPtr<Texture2D> LoadTexture(const std::string& name, const std::string& path)
    {
        
        return SharedPtr<Texture2D>(GET_RHI_FACTORY()->CreateTexture2DFromFile(name, path));
    }

    SharedPtr<Texture2D> LoadTextureWithParams(const std::string& name, const std::string& path, TextureFilter filter, TextureWrap wrapMode)
    {
        
        return SharedPtr<Texture2D>(GET_RHI_FACTORY()->CreateTexture2DFromFile(name, path, TextureDesc(filter, filter, wrapMode)));
    }

//    void LuaManager::BindECSLua(sol::state& state)
//    {
//
//        sol::usertype<entt::registry> enttRegistry = state.new_usertype<entt::registry>("enttRegistry");
//
//        sol::usertype<Entity> entityType               = state.new_usertype<Entity>("Entity", sol::constructors<sol::types<entt::entity, Level*>>());
//        sol::usertype<EntityManager> entityManagerType = state.new_usertype<EntityManager>("EntityManager");
//        entityManagerType.set_function("Create", static_cast<Entity (EntityManager::*)()>(&EntityManager::Create));
//        entityManagerType.set_function("GetRegistry", &EntityManager::GetRegistry);
//
//        entityType.set_function("Valid", &Entity::Valid);
//        entityType.set_function("Destroy", &Entity::Destroy);
//        entityType.set_function("SetParent", &Entity::SetParent);
//        entityType.set_function("GetParent", &Entity::GetParent);
//        entityType.set_function("IsParent", &Entity::IsParent);
//        entityType.set_function("GetChildren", &Entity::GetChildren);
//        entityType.set_function("SetActive", &Entity::SetActive);
//        entityType.set_function("Active", &Entity::Active);
//
//        state.set_function("GetEntityByName", &GetEntityByName);
//
//        state.set_function("AddPyramidEntity", &AddPyramid);
//        state.set_function("AddSphereEntity", &AddSphere);
//        state.set_function("AddLightCubeEntity", &AddLightCube);
//
//        sol::usertype<NameComponent> nameComponent_type = state.new_usertype<NameComponent>("NameComponent");
//        nameComponent_type["name"]                      = &NameComponent::name;
//        REGISTER_COMPONENT_WITH_ECS(state, NameComponent, static_cast<NameComponent& (Entity::*)()>(&Entity::AddComponent<NameComponent>));
//
//        sol::usertype<LuaScriptComponent> script_type = state.new_usertype<LuaScriptComponent>("LuaScriptComponent", sol::constructors<sol::types<std::string, Level*>>());
//        REGISTER_COMPONENT_WITH_ECS(state, LuaScriptComponent, static_cast<LuaScriptComponent& (Entity::*)(std::string&&, Level*&&)>(&Entity::AddComponent<LuaScriptComponent, std::string, Level*>));
//        script_type.set_function("GetCurrentEntity", &LuaScriptComponent::GetCurrentEntity);
//        script_type.set_function("SetThisComponent", &LuaScriptComponent::SetThisComponent);
//
//        {
//            using namespace entt;
//            auto entity_type = state["Entity"].get_or_create<sol::usertype<registry>>();
//            entity_type.set_function("Add" "Transform",
//                                     static_cast<Transform &(Entity::*)()>(&Entity::AddComponent<Transform>));
//            entity_type.set_function("Remove" "Transform", &Entity::RemoveComponent<Transform>);
//            entity_type.set_function("Get" "Transform", &Entity::GetComponent<Transform>);
//            entity_type.set_function("GetOrAdd" "Transform", &Entity::GetOrAddComponent<Transform>);
//            entity_type.set_function("TryGet" "Transform", &Entity::TryGetComponent<Transform>);
//            entity_type.set_function("AddOrReplace" "Transform", &Entity::AddOrReplaceComponent<Transform>);
//            entity_type.set_function("Has" "Transform", &Entity::HasComponent<Transform>);
//            auto entityManager_type = state["enttRegistry"].get_or_create<sol::usertype<registry>>();
//            entityManager_type.set_function("view_" "Transform",
//                                            &_ECS_export_view<type_list<Transform>, type_list<>>::view);
//            auto V = state.new_usertype<view<entt::get_t<Transform>>>("Transform" "_view");
//            V.set_function("each", &view<entt::get_t<Transform>>::each < std::function < void(Transform & ) >> );
//            V.set_function("front", &view<entt::get_t<Transform>>::front);
//            s_Identifiers.push_back("Transform");
//            s_Identifiers.push_back("Add" "Transform");
//            s_Identifiers.push_back("Remove" "Transform");
//            s_Identifiers.push_back("Get" "Transform");
//            s_Identifiers.push_back("GetOrAdd" "Transform");
//            s_Identifiers.push_back("TryGet" "Transform");
//            s_Identifiers.push_back("AddOrReplace" "Transform");
//            s_Identifiers.push_back("Has" "Transform");
//        };
//
////        sol::usertype<TextComponent> textComponent_type = state.new_usertype<TextComponent>("TextComponent");
////        textComponent_type["TextString"]                = &TextComponent::TextString;
////        textComponent_type["Colour"]                    = &TextComponent::Colour;
////        textComponent_type["MaxWidth"]                  = &TextComponent::MaxWidth;
////
////        REGISTER_COMPONENT_WITH_ECS(state, TextComponent, static_cast<TextComponent& (Entity::*)()>(&Entity::AddComponent<TextComponent>));
////
////        sol::usertype<Sprite> sprite_type = state.new_usertype<Sprite>("Sprite", sol::constructors<sol::types<glm::vec2, glm::vec2, glm::vec4>, Sprite(const SharedPtr<Texture2D>&, const glm::vec2&, const glm::vec2&, const glm::vec4&)>());
////        sprite_type.set_function("SetTexture", &Sprite::SetTexture);
////        sprite_type.set_function("SetSpriteSheet", &Sprite::SetSpriteSheet);
////        sprite_type.set_function("SetSpriteSheetIndex", &Sprite::SetSpriteSheetIndex);
////        sprite_type["SpriteSheetTileSize"] = &Sprite::SpriteSheetTileSize;
//
////        REGISTER_COMPONENT_WITH_ECS(state, Sprite, static_cast<Sprite& (Entity::*)(const glm::vec2&, const glm::vec2&, const glm::vec4&)>(&Entity::AddComponent<Sprite, const glm::vec2&, const glm::vec2&, const glm::vec4&>));
//
//        state.new_usertype<Light>(
//                "Light",
//                "Intensity", &Light::Intensity,
//                "Radius", &Light::Radius,
//                "Colour", &Light::Colour,
//                "Direction", &Light::Direction,
//                "Position", &Light::Position,
//                "Type", &Light::Type,
//                "Angle", &Light::Angle);
//
//        REGISTER_COMPONENT_WITH_ECS(state, Light, static_cast<Light& (Entity::*)()>(&Entity::AddComponent<Light>));
//
//        std::initializer_list<std::pair<sol::string_view, PrimitiveType>> primitives = {
//                { "Cube", PrimitiveType::Cube },
//                { "Plane", PrimitiveType::Plane },
//                { "Quad", PrimitiveType::Quad },
//                { "Pyramid", PrimitiveType::Pyramid },
//                { "Sphere", PrimitiveType::Sphere },
//                { "Capsule", PrimitiveType::Capsule },
//                { "Cylinder", PrimitiveType::Cylinder },
//                { "Terrain", PrimitiveType::Terrain },
//        };
//
//        state.new_enum<PrimitiveType, false>("PrimitiveType", primitives);
//
//        state.new_usertype<Model>("Model",
//                // Constructors
//                                  sol::constructors<
//                                          Model(),
//                                          Model(const std::string&),
//                                          Model(const SharedPtr<Mesh>&, PrimitiveType),
//                                          Model(PrimitiveType)>(),
//                // Properties
//                                  "meshes", &Model::GetMeshes,
//                                  "file_path", &Model::GetFilePath,
//                                  "primitive_type", sol::property(&Model::GetPrimitiveType, &Model::SetPrimitiveType),
//                // Methods
//                                  "add_mesh", &Model::AddMesh,
//                                  "load_model", &Model::LoadModel);
//
//        REGISTER_COMPONENT_WITH_ECS(state, Model, static_cast<Model& (Entity::*)(const std::string&)>(&Entity::AddComponent<Model, const std::string&>));
//
//        // Member functions
//        sol::usertype<Material> material_type = state.new_usertype<Material>("Material",
//
//                                                                             sol::constructors<
//                                                                                     Material()>(),
//                // Setters
//                                                                             "set_albedo_texture", &Material::SetAlbedoTexture,
//                                                                             "set_normal_texture", &Material::SetNormalTexture,
//                                                                             "set_roughness_texture", &Material::SetRoughnessTexture,
//                                                                             "set_metallic_texture", &Material::SetMetallicTexture,
//                                                                             "set_ao_texture", &Material::SetAOTexture,
//                                                                             "set_emissive_texture", &Material::SetEmissiveTexture,
//
//                // Getters
//                                                                             "get_name", &Material::GetName,
//                                                                             "get_properties", &Material::GetProperties,
//                //"get_textures", &Material::GetTextures,
//                                                                             "get_shader", &Material::GetShader,
//
//                // Other member functions
//                                                                             "load_pbr_material", &Material::LoadPBRMaterial,
//                                                                             "load_material", &Material::LoadMaterial,
//                                                                             "set_textures", &Material::SetTextures,
//                                                                             "set_material_properties", &Material::SetMaterialProperites,
//                                                                             "update_material_properties_data", &Material::UpdateMaterialPropertiesData,
//                                                                             "set_name", &Material::SetName,
//                                                                             "bind", &Material::Bind);
//
//        // Enum for RenderFlags
//        std::initializer_list<std::pair<sol::string_view, Material::RenderFlags>> render_flags = {
//                { "NONE", Material::RenderFlags::NONE },
//                { "DEPTHTEST", Material::RenderFlags::DEPTHTEST },
//                { "WIREFRAME", Material::RenderFlags::WIREFRAME },
//                { "FORWARDRENDER", Material::RenderFlags::FORWARDRENDER },
//                { "DEFERREDRENDER", Material::RenderFlags::DEFERREDRENDER },
//                { "NOSHADOW", Material::RenderFlags::NOSHADOW },
//                { "TWOSIDED", Material::RenderFlags::TWOSIDED },
//                { "ALPHABLEND", Material::RenderFlags::ALPHABLEND }
//
//        };
//
//        state.new_enum<Material::RenderFlags, false>("RenderFlags", render_flags);
//
//        sol::usertype<Camera> camera_type = state.new_usertype<Camera>("Camera", sol::constructors<Camera(float, float, float, float), Camera(float, float)>());
//        camera_type["fov"]                = &Camera::GetFOV;
//        camera_type["aspectRatio"]        = &Camera::GetAspectRatio;
//        camera_type["nearPlane"]          = &Camera::GetNear;
//        camera_type["farPlane"]           = &Camera::GetFar;
//        camera_type["SetIsOrthographic"]  = &Camera::SetIsOrthographic;
//        camera_type["SetNearPlane"]       = &Camera::SetNear;
//        camera_type["SetFarPlane"]        = &Camera::SetFar;
//
//        REGISTER_COMPONENT_WITH_ECS(state, Camera, static_cast<Camera& (Entity::*)(const float&, const float&)>(&Entity::AddComponent<Camera, const float&, const float&>));
//
////        sol::usertype<RigidBody3DComponent> RigidBody3DComponent_type = state.new_usertype<RigidBody3DComponent>("RigidBody3DComponent", sol::constructors<sol::types<RigidBody3D*>>());
////        RigidBody3DComponent_type.set_function("GetRigidBody", &RigidBody3DComponent::GetRigidBody);
////
////        REGISTER_COMPONENT_WITH_ECS(state, RigidBody3DComponent, static_cast<RigidBody3DComponent& (Entity::*)(const RigidBody3DProperties&)>(&Entity::AddComponent<RigidBody3DComponent, const RigidBody3DProperties&>));
////        //REGISTER_COMPONENT_WITH_ECS(state, RigidBody3DComponent, static_cast<RigidBody3DComponent& (Entity::*)>(&Entity::AddComponent<RigidBody3DComponent));
////
////        sol::usertype<RigidBody2DComponent> RigidBody2DComponent_type = state.new_usertype<RigidBody2DComponent>("RigidBody2DComponent", sol::constructors<sol::types<const RigidBodyParameters&>>());
////        RigidBody2DComponent_type.set_function("GetRigidBody", &RigidBody2DComponent::GetRigidBody);
////
////        REGISTER_COMPONENT_WITH_ECS(state, RigidBody2DComponent, static_cast<RigidBody2DComponent& (Entity::*)(const RigidBodyParameters&)>(&Entity::AddComponent<RigidBody2DComponent, const RigidBodyParameters&>));
////
////        REGISTER_COMPONENT_WITH_ECS(state, SoundComponent, static_cast<SoundComponent& (Entity::*)()>(&Entity::AddComponent<SoundComponent>));
//
//        auto mesh_type = state.new_usertype<Mesh>("Mesh",
//                                                                   sol::constructors<Mesh(), Mesh(const Mesh&),
//                                                                           Mesh(const std::vector<uint32_t>&, const std::vector<Vertex>&, float)>());
//
//        // Bind the member functions and variables
//        mesh_type["GetMaterial"]    = &Mesh::GetMaterial;
//        mesh_type["SetMaterial"]    = &Mesh::SetMaterial;
//        mesh_type["GetBoundingBox"] = &Mesh::GetBoundingBox;
//        mesh_type["IsActive"]      = &Mesh::IsActive;
//        mesh_type["SetName"]        = &Mesh::SetName;
//
//        std::initializer_list<std::pair<sol::string_view, TextureFilter>> textureFilter = {
//                { "None", TextureFilter::NONE },
//                { "Linear", TextureFilter::LINEAR },
//                { "Nearest", TextureFilter::NEAREST }
//        };
//
//        std::initializer_list<std::pair<sol::string_view, TextureWrap>> textureWrap = {
//                { "None", TextureWrap::NONE },
//                { "Repeat", TextureWrap::REPEAT },
//                { "Clamp", TextureWrap::CLAMP },
//                { "MirroredRepeat", TextureWrap::MIRRORED_REPEAT },
//                { "ClampToEdge", TextureWrap::CLAMP_TO_EDGE },
//                { "ClampToBorder", TextureWrap::CLAMP_TO_BORDER }
//        };
//
//        state.set_function("LoadMesh", &MeshFactory::CreatePrimative);
//
//        state.new_enum<TextureWrap, false>("TextureWrap", textureWrap);
//        state.new_enum<TextureFilter, false>("TextureFilter", textureFilter);
//
//        state.set_function("LoadTexture", &LoadTexture);
//        state.set_function("LoadTextureWithParams", &LoadTextureWithParams);
//    }

    static float LuaRand(float a, float b)
    {
        return Random32::Rand(a, b);
    }

    void LuaManager::BindLevelLua(sol::state& state)
    {
        sol::usertype<Level> level_type = state.new_usertype<Level>("Level");
        level_type.set_function("GetRegistry", &Level::GetRegistry);
        level_type.set_function("GetEntityManager", &Level::GetEntityManager);

        sol::usertype<Texture2D> texture2D_type = state.new_usertype<Texture2D>("Texture2D");
//        texture2D_type.set_function("CreateFromFile", &Texture2D::CreateFromFile);

        state.set_function("Rand", &LuaRand);
    }

    static void SwitchLevelByIndex(int index)
    {
        gEngine->GetLevelManager()->SwitchLevel(index);
    }

    static void SwitchLevel()
    {
        gEngine->GetLevelManager()->SwitchLevel();
    }

    static void SwitchLevelByName(const std::string& name)
    {
        gEngine->GetLevelManager()->SwitchLevel(name);
    }

    static void SetPhysicsDebugFlags(int flags)
    {
        gEngine->GetSystem<PhysicsEngine>()->SetDebugDrawFlags(flags);
    }

    void LuaManager::BindAppLua(sol::state& state)
    {
//        sol::usertype<Application> app_type = state.new_usertype<Application>("Application");
        state.set_function("SwitchLevelByIndex", &SwitchLevelByIndex);
        state.set_function("SwitchLevelByName", &SwitchLevelByName);
        state.set_function("SwitchLevel", &SwitchLevel);
        state.set_function("SetPhysicsDebugFlags", &SetPhysicsDebugFlags);

        std::initializer_list<std::pair<sol::string_view, PhysicsDebugFlags>> physicsDebugFlags = {
                { "CONSTRAINT", PhysicsDebugFlags::CONSTRAINT },
                { "MANIFOLD", PhysicsDebugFlags::MANIFOLD },
                { "COLLISIONVOLUMES", PhysicsDebugFlags::COLLISIONVOLUMES },
                { "COLLISIONNORMALS", PhysicsDebugFlags::COLLISIONNORMALS },
                { "AABB", PhysicsDebugFlags::AABB },
                { "LINEARVELOCITY", PhysicsDebugFlags::LINEARVELOCITY },
                { "LINEARFORCE", PhysicsDebugFlags::LINEARFORCE },
                { "BROADPHASE", PhysicsDebugFlags::BROADPHASE },
                { "BROADPHASE_PAIRS", PhysicsDebugFlags::BROADPHASE_PAIRS },
                { "BOUNDING_RADIUS", PhysicsDebugFlags::BOUNDING_RADIUS },
        };

        state.new_enum<PhysicsDebugFlags, false>("PhysicsDebugFlags", physicsDebugFlags);

//        app_type.set_function("GetWindowSize", &Application::GetWindowSize);
//        state.set_function("GetAppInstance", &Application::Get);
    }

} // NekoEngine