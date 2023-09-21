#include "GUI/ImGuiUtility.h"
#include "InspectorPanel.h"
#include "Script/LuaScriptComponent.h"
#include "File/VirtualFileSystem.h"
#include "File/FileSystem.h"
#include "Editor.h"
#include "TextEditPanel.h"
#include "Component/RigidBody3DComponent.h"
#include "Collision/CapsuleCollisionShape.h"
#include "StringUtility.h"
#include "Renderable/Environment.h"
#include "Component/ModelComponent.h"
#include "Component/PrefabComponent.h"
#include "Entity/EntityManager.h"

#define INPUT_BUF_SIZE 128

namespace MM
{
    template <>
    void ComponentEditorWidget<NekoEngine::LuaScriptComponent>(entt::registry& reg, entt::registry::entity_type e)
    {
        auto& script = reg.get<NekoEngine::LuaScriptComponent>(e);
        bool loaded  = false;
        if(!script.Loaded())
        {
            ImGui::Text("Script Failed to Load : %s", script.GetFilePath().c_str());
            loaded = false;
        }
        else if(!script.Loaded() && script.GetFilePath().empty())
        {
            ImGui::Text("FilePath empty : %s", script.GetFilePath().c_str());
            loaded = false;
        }
        else
            loaded = true;

        // auto& solEnv         = script.GetSolEnvironment();
        std::string filePath = script.GetFilePath();

        static char objName[INPUT_BUF_SIZE];
        strcpy(objName, filePath.c_str());
        ImGui::PushItemWidth(-1);
        if(ImGui::InputText("##Name", objName, IM_ARRAYSIZE(objName), 0))
            script.SetFilePath(objName);

        bool hasReloaded = false;

        if(ImGui::Button("New File", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f)))
        {
            std::string newFilePath = "//Scripts";
            std::string physicalPath;
            if(!NekoEngine::VirtualFileSystem::ResolvePhysicalPath(newFilePath, physicalPath, true))
            {
                LOG_FORMAT("Failed to Create Lua script %s", physicalPath.c_str());
            }
            else
            {
                std::string defaultScript =
                        R"(--Default Lua Script
                
function OnInit()
end

function OnUpdate(dt)
end

function OnCleanUp()
end
)";
                std::string newScriptFileName = "Script";
                int fileIndex                 = 0;
                while(NekoEngine::FileSystem::FileExists(physicalPath + "/" + newScriptFileName + ".lua"))
                {
                    fileIndex++;
//                    newScriptFileName = fmt::format("Script({0})", fileIndex);
                }

                NekoEngine::FileSystem::WriteTextFile(physicalPath + "/" + newScriptFileName + ".lua", defaultScript);
                script.SetFilePath(newFilePath + "/" + newScriptFileName + ".lua");
                script.Reload();
                hasReloaded = true;
            }
        }

        if(loaded)
        {
            if(ImGui::Button("Edit File", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f)))
            {
                NekoEngine::Editor::GetEditor()->OpenTextFile(script.GetFilePath(), [&]
                {
                    script.Reload();
                    hasReloaded = true;

                    auto textEditPanel = NekoEngine::Editor::GetEditor()->GetTextEditPanel();
                    if(textEditPanel)
                        ((NekoEngine::TextEditPanel*)textEditPanel)->SetErrors(script.GetErrors()); });

                auto textEditPanel = NekoEngine::Editor::GetEditor()->GetTextEditPanel();
                if(textEditPanel)
                    ((NekoEngine::TextEditPanel*)textEditPanel)->SetErrors(script.GetErrors());
            }

            if(ImGui::Button("Open File", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f)))
            {
                NekoEngine::Editor::GetEditor()->GetFileBrowserPanel().Open();
                NekoEngine::Editor::GetEditor()->GetFileBrowserPanel().SetCallback(std::bind(&NekoEngine::LuaScriptComponent::LoadScript, &script, std::placeholders::_1));
            }

            if(ImGui::Button("Reload", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f)))
            {
                script.Reload();
                hasReloaded = true;
            }
        }
        else
        {
            if(ImGui::Button("Load", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f)))
            {
                script.Reload();
                hasReloaded = true;
            }
        }

        if(!script.Loaded() || hasReloaded || !loaded)
        {
            return;
        }

        ImGui::TextUnformatted("Loaded Functions : ");

        /*     ImGui::Indent();
             for(auto&& function : solEnv)
             {
                 if(function.second.is<sol::function>())
                 {
                     ImGui::TextUnformatted(function.first.as<std::string>().c_str());
                 }
             }
             ImGui::Unindent();*/
    }

    template <>
    void ComponentEditorWidget<NekoEngine::Transform>(entt::registry& reg, entt::registry::entity_type e)
    {
        auto& transform = reg.get<NekoEngine::Transform>(e);

        auto rotation   = glm::degrees(glm::eulerAngles(transform.GetLocalOrientation()));
        auto position   = transform.GetLocalPosition();
        auto scale      = transform.GetLocalScale();
        float itemWidth = (ImGui::GetContentRegionAvail().x - (ImGui::GetFontSize() * 3.0f)) / 3.0f;

        // Call this to fix alignment with columns
        ImGui::AlignTextToFramePadding();

        if(NekoEngine::ImGuiUtility::PorpertyTransform("Position", position, itemWidth))
            transform.SetLocalPosition(position);

        ImGui::SameLine();
        if(NekoEngine::ImGuiUtility::PorpertyTransform("Rotation", rotation, itemWidth))
        {
            float pitch = NekoEngine::Maths::Min(rotation.x, 89.9f);
            pitch       = NekoEngine::Maths::Max(pitch, -89.9f);
            transform.SetLocalOrientation(glm::quat(glm::radians(glm::vec3(pitch, rotation.y, rotation.z))));
        }

        ImGui::SameLine();
        if(NekoEngine::ImGuiUtility::PorpertyTransform("Scale", scale, itemWidth))
        {
            transform.SetLocalScale(scale);
        }

        ImGui::Columns(1);
        ImGui::Separator();
    }

    static void CuboidCollisionShapeInspector(NekoEngine::CuboidCollisionShape* shape, const NekoEngine::RigidBody3DComponent& phys)
    {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Half Dimensions");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        glm::vec3 size = shape->GetHalfDimensions();
        if(ImGui::DragFloat3("##CollisionShapeHalfDims", glm::value_ptr(size), 1.0f, 0.0f, 10000.0f, "%.2f"))
        {
            shape->SetHalfDimensions(size);
            phys.GetRigidBody()->CollisionShapeUpdated();
        }
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
    }

    static void SphereCollisionShapeInspector(NekoEngine::SphereCollisionShape* shape, const NekoEngine::RigidBody3DComponent& phys)
    {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Radius");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        float radius = shape->GetRadius();
        if(ImGui::DragFloat("##CollisionShapeRadius", &radius, 1.0f, 0.0f, 10000.0f))
        {
            shape->SetRadius(radius);
            phys.GetRigidBody()->CollisionShapeUpdated();
        }
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
    }

//    static void PyramidCollisionShapeInspector(NekoEngine::PyramidCollisionShape* shape, const NekoEngine::RigidBody3DComponent& phys)
//    {
//        
//        ImGui::AlignTextToFramePadding();
//        ImGui::TextUnformatted("Half Dimensions");
//        ImGui::NextColumn();
//        ImGui::PushItemWidth(-1);
//
//        glm::vec3 size = shape->GetHalfDimensions();
//        if(ImGui::DragFloat3("##CollisionShapeHalfDims", glm::value_ptr(size), 1.0f, 0.0f, 10000.0f, "%.2f"))
//        {
//            shape->SetHalfDimensions(size);
//            phys.GetRigidBody()->CollisionShapeUpdated();
//        }
//        ImGui::NextColumn();
//        ImGui::PushItemWidth(-1);
//    }

    static void CapsuleCollisionShapeInspector(NekoEngine::CapsuleCollisionShape* shape, const NekoEngine::RigidBody3DComponent& phys)
    {
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Half Dimensions");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        float radius = shape->GetRadius();
        if(ImGui::DragFloat("##CollisionShapeRadius", &radius, 1.0f, 0.0f, 10000.0f, "%.2f"))
        {
            shape->SetRadius(radius);
            phys.GetRigidBody()->CollisionShapeUpdated();
        }

        float height = shape->GetHeight();
        if(ImGui::DragFloat("##CollisionShapeHeight", &height, 1.0f, 0.0f, 10000.0f, "%.2f"))
        {
            shape->SetHeight(height);
            phys.GetRigidBody()->CollisionShapeUpdated();
        }
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
    }

//    static void HullCollisionShapeInspector(NekoEngine::HullCollisionShape* shape, const NekoEngine::RigidBody3DComponent& phys)
//    {
//        ImGui::TextUnformatted("Hull Collision Shape");
//        ImGui::NextColumn();
//        ImGui::PushItemWidth(-1);
//    }

//    std::string CollisionShape2DTypeToString(NekoEngine::Shape shape)
//    {
//        switch(shape)
//        {
//            case NekoEngine::Shape::Circle:
//                return "Circle";
//            case NekoEngine::Shape::Square:
//                return "Square";
//            case NekoEngine::Shape::Custom:
//                return "Custom";
//        }
//
//        return "Unknown Shape";
//    }

//    NekoEngine::Shape StringToCollisionShape2DType(const std::string& type)
//    {
//        if(type == "Circle")
//            return NekoEngine::Shape::Circle;
//        if(type == "Square")
//            return NekoEngine::Shape::Square;
//        if(type == "Custom")
//            return NekoEngine::Shape::Custom;
//
//        return NekoEngine::Shape::Circle;
//    }

    const char* CollisionShapeTypeToString(NekoEngine::CollisionShapeType type)
    {
        switch(type)
        {
            case NekoEngine::CollisionShapeType::CollisionCuboid:
                return "Cuboid";
            case NekoEngine::CollisionShapeType::CollisionSphere:
                return "Sphere";
            case NekoEngine::CollisionShapeType::CollisionPyramid:
                return "Pyramid";
            case NekoEngine::CollisionShapeType::CollisionCapsule:
                return "Capsule";
            case NekoEngine::CollisionShapeType::CollisionHull:
                return "Hull";
            default:
                LOG("Unsupported Collision shape");
                break;
        }

        return "Error";
    }

    NekoEngine::CollisionShapeType StringToCollisionShapeType(const std::string& type)
    {
        if(type == "Sphere")
            return NekoEngine::CollisionShapeType::CollisionSphere;
        if(type == "Cuboid")
            return NekoEngine::CollisionShapeType::CollisionCuboid;
        if(type == "Pyramid")
            return NekoEngine::CollisionShapeType::CollisionPyramid;
        if(type == "Capsule")
            return NekoEngine::CollisionShapeType::CollisionCapsule;
        if(type == "Hull")
            return NekoEngine::CollisionShapeType::CollisionHull;
        LOG("Unsupported Collision shape");
        return NekoEngine::CollisionShapeType::CollisionSphere;
    }

    template <>
    void ComponentEditorWidget<NekoEngine::AxisConstraintComponent>(entt::registry& reg, entt::registry::entity_type e)
    {
        using namespace NekoEngine;
        ImGuiUtility::ScopedStyle(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

        ImGui::Columns(2);
        ImGui::Separator();
        AxisConstraintComponent& axisConstraintComponent = reg.get<NekoEngine::AxisConstraintComponent>(e);

        uint64_t entityID = axisConstraintComponent.GetEntityID();
        Axes axes         = axisConstraintComponent.GetAxes();
        Entity entity     = gEngine->GetCurrentLevel()->GetEntityManager()->GetEntityByUUID(entityID);

        bool hasName = entity ? entity.HasComponent<NameComponent>() : false;
        std::string name;
        if(hasName)
            name = entity.GetComponent<NameComponent>().name;
        else
            name = "Empty";
        ImGui::TextUnformatted("Entity");
        ImGui::NextColumn();
        ImGui::Text("%s", name.c_str());
        ImGui::NextColumn();

        std::vector<std::string> entities;
        uint64_t currentEntityID = axisConstraintComponent.GetEntityID();
        int index                = 0;
        int selectedIndex        = 0;

        auto physics3dEntities = gEngine->GetCurrentLevel()->GetEntityManager()->GetRegistry().view<NekoEngine::RigidBody3DComponent>();

        for(auto entity : physics3dEntities)
        {
            if(Entity(entity, gEngine->GetCurrentLevel()).GetID() == currentEntityID)
                selectedIndex = index;

            entities.push_back(Entity(entity, gEngine->GetCurrentLevel()).GetName());

            index++;
        }

        static const char* possibleAxes[7] = { "X", "Y", "Z", "XY", "XZ", "YZ", "XYZ" };

        selectedIndex = (int)axes;

        bool updated = NekoEngine::ImGuiUtility::PropertyDropdown("Axes", possibleAxes, 7, &selectedIndex);
        if(updated)
            axisConstraintComponent.SetAxes((Axes)selectedIndex);

        // bool updated = NekoEngine::ImGuiUtility::PropertyDropdown("Entity", entities.data(), (int)entities.size(), &selectedIndex);

        // if(updated)
        // axisConstraintComponent.SetEntity(Entity(physics3dEntities[selectedIndex], gEngine->GetCurrentLevel()).GetID());

        ImGui::Columns(1);
    }

    template <>
    void ComponentEditorWidget<NekoEngine::RigidBody3DComponent>(entt::registry& reg, entt::registry::entity_type e)
    {
        
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();
        auto& phys = reg.get<NekoEngine::RigidBody3DComponent>(e);

        auto pos             = phys.GetRigidBody()->GetPosition();
        auto force           = phys.GetRigidBody()->GetForce();
        auto torque          = phys.GetRigidBody()->GetTorque();
        auto orientation     = glm::eulerAngles(phys.GetRigidBody()->GetOrientation());
        auto angularVelocity = phys.GetRigidBody()->GetAngularVelocity();
        auto friction        = phys.GetRigidBody()->GetFriction();
        auto isStatic        = phys.GetRigidBody()->GetIsStatic();
        auto isRest          = phys.GetRigidBody()->GetIsAtRest();
        auto mass            = 1.0f / phys.GetRigidBody()->GetInverseMass();
        auto velocity        = phys.GetRigidBody()->GetLinearVelocity();
        auto elasticity      = phys.GetRigidBody()->GetElasticity();
        auto angularFactor   = phys.GetRigidBody()->GetAngularFactor();
        auto collisionShape  = phys.GetRigidBody()->GetCollisionShape();
        auto UUID            = phys.GetRigidBody()->GetUUID();

        NekoEngine::ImGuiUtility::Property("UUID", (uint32_t&)UUID, NekoEngine::ImGuiUtility::PropertyFlag::ReadOnly);

        if(NekoEngine::ImGuiUtility::Property("Position", pos))
            phys.GetRigidBody()->SetPosition(pos);

        if(NekoEngine::ImGuiUtility::Property("Velocity", velocity))
            phys.GetRigidBody()->SetLinearVelocity(velocity);

        if(NekoEngine::ImGuiUtility::Property("Torque", torque))
            phys.GetRigidBody()->SetTorque(torque);

        if(NekoEngine::ImGuiUtility::Property("Orientation", orientation))
            phys.GetRigidBody()->SetOrientation(glm::quat(orientation));

        if(NekoEngine::ImGuiUtility::Property("Force", force))
            phys.GetRigidBody()->SetForce(force);

        if(NekoEngine::ImGuiUtility::Property("Angular Velocity", angularVelocity))
            phys.GetRigidBody()->SetAngularVelocity(angularVelocity);

        if(NekoEngine::ImGuiUtility::Property("Friction", friction, 0.0f, 1.0f))
            phys.GetRigidBody()->SetFriction(friction);

        if(NekoEngine::ImGuiUtility::Property("Mass", mass))
        {
            mass = NekoEngine::Maths::Max(mass, 0.0001f);
            phys.GetRigidBody()->SetInverseMass(1.0f / mass);
        }

        if(NekoEngine::ImGuiUtility::Property("Elasticity", elasticity))
            phys.GetRigidBody()->SetElasticity(elasticity);

        if(NekoEngine::ImGuiUtility::Property("Static", isStatic))
            phys.GetRigidBody()->SetIsStatic(isStatic);

        if(NekoEngine::ImGuiUtility::Property("At Rest", isRest))
            phys.GetRigidBody()->SetIsAtRest(isRest);

        if(NekoEngine::ImGuiUtility::Property("Angular Factor", angularFactor))
            phys.GetRigidBody()->SetAngularFactor(angularFactor);

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();

        std::vector<const char*> shapes = { "Sphere", "Cuboid", "Pyramid", "Capsule", "Hull" };
        int selectedIndex               = 0;
        const char* shape_current       = collisionShape ? CollisionShapeTypeToString(collisionShape->GetType()) : "";
        int index                       = 0;
        for(auto& shape : shapes)
        {
            if(shape == shape_current)
            {
                selectedIndex = index;
                break;
            }
            index++;
        }

        bool updated = NekoEngine::ImGuiUtility::PropertyDropdown("Collision Shape", shapes.data(), 5, &selectedIndex);

        if(updated)
            phys.GetRigidBody()->SetCollisionShape(StringToCollisionShapeType(shapes[selectedIndex]));

        if(collisionShape)
        {
            switch(collisionShape->GetType())
            {
                case NekoEngine::CollisionShapeType::CollisionCuboid:
                    CuboidCollisionShapeInspector(reinterpret_cast<NekoEngine::CuboidCollisionShape*>(collisionShape.get()), phys);
                    break;
                case NekoEngine::CollisionShapeType::CollisionSphere:
                    SphereCollisionShapeInspector(reinterpret_cast<NekoEngine::SphereCollisionShape*>(collisionShape.get()), phys);
                    break;
//                case NekoEngine::CollisionShapeType::CollisionPyramid:
//                    PyramidCollisionShapeInspector(reinterpret_cast<NekoEngine::PyramidCollisionShape*>(collisionShape.get()), phys);
//                    break;
                case NekoEngine::CollisionShapeType::CollisionCapsule:
                    CapsuleCollisionShapeInspector(reinterpret_cast<NekoEngine::CapsuleCollisionShape*>(collisionShape.get()), phys);
                    break;
//                case NekoEngine::CollisionShapeType::CollisionHull:
//                    HullCollisionShapeInspector(reinterpret_cast<NekoEngine::HullCollisionShape*>(collisionShape.get()), phys);
//                    break;
                default:
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);
                    LOG("Unsupported Collision shape");
                    break;
            }
        }
        else
        {
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
        }

        ImGui::PopItemWidth();
        ImGui::Columns(1);

        ImGui::Separator();
    }

    template <>
    void ComponentEditorWidget<NekoEngine::Camera>(entt::registry& reg, entt::registry::entity_type e)
    {
        
        auto& camera = reg.get<NekoEngine::Camera>(e);

        NekoEngine::ImGuiUtility::ScopedStyle(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        using namespace NekoEngine;

        float aspect = camera.GetAspectRatio();
        if(ImGuiUtility::Property("Aspect", aspect, 0.0f, 10.0f))
            camera.SetAspectRatio(aspect);

        float fov = camera.GetFOV();
        if(ImGuiUtility::Property("Fov", fov, 1.0f, 120.0f))
            camera.SetFOV(fov);

        float n = camera.GetNear();
        if(ImGuiUtility::Property("Near", n, 0.0f, 10.0f))
            camera.SetNear(n);

        float f = camera.GetFar();
        if(ImGuiUtility::Property("Far", f, 10.0f, 10000.0f))
            camera.SetFar(f);

        float scale = camera.GetScale();
        if(ImGuiUtility::Property("Scale", scale, 0.0f, 1000.0f))
            camera.SetScale(scale);

        bool ortho = camera.IsOrthographic();
        if(ImGuiUtility::Property("Orthograhic", ortho))
            camera.SetIsOrthographic(ortho);

        float aperture = camera.GetAperture();
        if(ImGuiUtility::Property("Aperture", aperture, 0.0f, 200.0f))
            camera.SetAperture(aperture);

        float shutterSpeed = camera.GetShutterSpeed();
        if(ImGuiUtility::Property("Shutter Speed", shutterSpeed, 0.0f, 1.0f))
            camera.SetShutterSpeed(shutterSpeed);

        float sensitivity = camera.GetSensitivity();
        if(ImGuiUtility::Property("ISO", sensitivity, 0.0f, 5000.0f))
            camera.SetSensitivity(sensitivity);

        float exposure = camera.GetExposure();
        ImGuiUtility::Property("Exposure", exposure, 0.0f, 0.0f, 0.0f, ImGuiUtility::PropertyFlag::ReadOnly);

        ImGui::Columns(1);
        ImGui::Separator();
    }

    template <>
    void ComponentEditorWidget<NekoEngine::Light>(entt::registry& reg, entt::registry::entity_type e)
    {
        
        auto& light = reg.get<NekoEngine::Light>(e);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        if(light.Type != 0)
            NekoEngine::ImGuiUtility::Property("Position", light.Position);

        if(light.Type != 2)
            NekoEngine::ImGuiUtility::Property("Direction", light.Direction);

        if(light.Type != 0)
            NekoEngine::ImGuiUtility::Property("Radius", light.Radius, 0.0f, 100.0f);
        NekoEngine::ImGuiUtility::Property("Colour", light.Colour, true, NekoEngine::ImGuiUtility::PropertyFlag::ColourProperty);
        NekoEngine::ImGuiUtility::Property("Intensity", light.Intensity, 0.0f, 4.0f);

        if(light.Type == 1)
            NekoEngine::ImGuiUtility::Property("Angle", light.Angle, -1.0f, 1.0f);

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Light Type");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        const char* types[]       = { "Directional", "Spot", "Point" };
        std::string light_current = NekoEngine::Light::LightTypeToString(NekoEngine::LightType(int(light.Type)));
        if(ImGui::BeginCombo("", light_current.c_str(), 0)) // The second parameter is the label previewed before opening the combo.
        {
            for(auto & type : types)
            {
                bool is_selected = (light_current.c_str() == type);
                if(ImGui::Selectable(type, light_current.c_str()))
                {
                    light.Type = NekoEngine::Light::StringToLightType(type);
                }
                if(is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();
    }

    NekoEngine::PrimitiveType GetPrimativeName(const std::string& type)
    {
        
        if(type == "Cube")
        {
            return NekoEngine::PrimitiveType::Cube;
        }
        else if(type == "Quad")
        {
            return NekoEngine::PrimitiveType::Quad;
        }
        else if(type == "Sphere")
        {
            return NekoEngine::PrimitiveType::Sphere;
        }
        else if(type == "Pyramid")
        {
            return NekoEngine::PrimitiveType::Pyramid;
        }
        else if(type == "Capsule")
        {
            return NekoEngine::PrimitiveType::Capsule;
        }
        else if(type == "Cylinder")
        {
            return NekoEngine::PrimitiveType::Cylinder;
        }
        else if(type == "Terrain")
        {
            return NekoEngine::PrimitiveType::Terrain;
        }

        LOG("Primitive not supported");
        return NekoEngine::PrimitiveType::Cube;
    }

    std::string GetPrimativeName(NekoEngine::PrimitiveType type)
    {
        
        switch(type)
        {
            case NekoEngine::PrimitiveType::Cube:
                return "Cube";
            case NekoEngine::PrimitiveType::Plane:
                return "Plane";
            case NekoEngine::PrimitiveType::Quad:
                return "Quad";
            case NekoEngine::PrimitiveType::Sphere:
                return "Sphere";
            case NekoEngine::PrimitiveType::Pyramid:
                return "Pyramid";
            case NekoEngine::PrimitiveType::Capsule:
                return "Capsule";
            case NekoEngine::PrimitiveType::Cylinder:
                return "Cylinder";
            case NekoEngine::PrimitiveType::Terrain:
                return "Terrain";
            case NekoEngine::PrimitiveType::File:
                return "File";
            case NekoEngine::PrimitiveType::None:
                return "None";
        }

        LOG("Primitive not supported");
        return "";
    }

    void TextureWidget(const char* label, NekoEngine::Material* material, NekoEngine::Texture2D* tex, bool flipImage, float& usingMapProperty, glm::vec4& colourProperty, const std::function<void(const std::string&)>& callback, const ImVec2& imageButtonSize = ImVec2(64, 64))
    {
        using namespace NekoEngine;
        if(ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            // ImGui::Columns(2);
            ImGui::BeginColumns("TextureWidget", 2, ImGuiOldColumnFlags_NoResize);
            ImGui::SetColumnWidth(0, imageButtonSize.x + 10.0f);

            ImGui::Separator();

            ImGui::AlignTextToFramePadding();

            const ImGuiPayload* payload = ImGui::GetDragDropPayload();
            auto min                    = ImGui::GetCurrentWindow()->DC.CursorPos;
            auto max                    = min + imageButtonSize + ImGui::GetStyle().FramePadding;

            bool hoveringButton = ImGui::IsMouseHoveringRect(min, max, false);
            bool showTexture    = !(hoveringButton && (payload != nullptr && payload->IsDataType("AssetFile")));
            if(tex && showTexture)
            {
                if(ImGui::ImageButton((const char*)(tex), tex->GetHandle(), imageButtonSize, ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f)))
                {
                    NekoEngine::Editor::GetEditor()->GetFileBrowserPanel().Open();
                    NekoEngine::Editor::GetEditor()->GetFileBrowserPanel().SetCallback(callback);
                }

                if(ImGui::IsItemHovered() && tex)
                {
                    ImGui::BeginTooltip();
                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                    ImGui::TextUnformatted(tex->GetFilepath().c_str());
                    ImGui::PopTextWrapPos();
                    ImGui::Image(tex->GetHandle(), imageButtonSize * 3.0f, ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
                    ImGui::EndTooltip();
                }
            }
            else
            {
                if(ImGui::Button(tex ? "" : "Empty", imageButtonSize))
                {
                    NekoEngine::Editor::GetEditor()->GetFileBrowserPanel().Open();
                    NekoEngine::Editor::GetEditor()->GetFileBrowserPanel().SetCallback(callback);
                }
            }

            if(payload != nullptr && payload->IsDataType("AssetFile"))
            {
                auto filePath = std::string(reinterpret_cast<const char*>(payload->Data));
                if(NekoEngine::Editor::GetEditor()->IsTextureFile(filePath))
                {
                    if(ImGui::BeginDragDropTarget())
                    {
                        // Drop directly on to node and append to the end of it's children list.
                        if(ImGui::AcceptDragDropPayload("AssetFile"))
                        {
                            callback(filePath);
                            ImGui::EndDragDropTarget();

                            ImGui::Columns(1);
                            ImGui::Separator();
                            ImGui::PopStyleVar();

                            ImGui::TreePop();
                            return;
                        }

                        ImGui::EndDragDropTarget();
                    }
                }
            }

            ImGui::NextColumn();
            // ImGui::PushItemWidth(-1);

            if(tex)
            {
                ImGui::Text("%u x %u", tex->GetWidth(), tex->GetHeight());
                ImGui::Text("Mip Levels : %u", tex->GetMipMapLevels());
            }

            // ImGui::TextUnformatted("Use Map");
            // ImGui::SameLine();
            // ImGui::PushItemWidth(-1);

            ImGui::SliderFloat(NekoEngine::ImGuiUtility::GenerateLabelID("Use Map"), &usingMapProperty, 0.0f, 1.0f);

            ImGui::ColorEdit4(NekoEngine::ImGuiUtility::GenerateLabelID("Colour"), glm::value_ptr(colourProperty));
            /*       ImGui::TextUnformatted("Value");
                   ImGui::SameLine();
                   ImGui::PushItemWidth(-1);*/

            // ImGui::DragFloat(NekoEngine::ImGuiUtility::GenerateID(), &amount, 0.0f, 20.0f);

            // ImGui::PopItemWidth();
            // ImGui::NextColumn();

            // ImGuiUtility::Property("Use Map", usingMapProperty, 0.0f, 1.0f);
            // ImGuiUtility::Property("Colour", colourProperty, 0.0f, 1.0f, false, NekoEngine::ImGuiUtility::PropertyFlag::ColourProperty);

            ImGui::Columns(1);

            ImGui::Separator();
            ImGui::PopStyleVar();

            ImGui::TreePop();
        }
    }

    void TextureWidget(const char* label, NekoEngine::Material* material, NekoEngine::Texture2D* tex, bool flipImage, float& usingMapProperty, float& amount, bool hasAmountValue, const std::function<void(const std::string&)>& callback, const ImVec2& imageButtonSize = ImVec2(64, 64))
    {
        using namespace NekoEngine;
        if(ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanFullWidth))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGui::BeginColumns("TextureWidget", 2, ImGuiOldColumnFlags_NoResize);
            ImGui::SetColumnWidth(0, imageButtonSize.x + 10.0f);
            ImGui::Separator();

            ImGui::AlignTextToFramePadding();

            const ImGuiPayload* payload = ImGui::GetDragDropPayload();
            auto min                    = ImGui::GetCurrentWindow()->DC.CursorPos;
            auto max                    = min + imageButtonSize + ImGui::GetStyle().FramePadding;

            bool hoveringButton = ImGui::IsMouseHoveringRect(min, max, false);
            bool showTexture    = !(hoveringButton && (payload != nullptr && payload->IsDataType("AssetFile")));
            if(tex && showTexture)
            {
                if(ImGui::ImageButton((const char*)(tex), tex->GetHandle(), imageButtonSize, ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f)))
                {
                    NekoEngine::Editor::GetEditor()->GetFileBrowserPanel().Open();
                    NekoEngine::Editor::GetEditor()->GetFileBrowserPanel().SetCallback(callback);
                }

                if(ImGui::IsItemHovered() && tex)
                {
                    ImGui::BeginTooltip();
                    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                    ImGui::TextUnformatted(tex->GetFilepath().c_str());
                    ImGui::PopTextWrapPos();

                    ImGui::Image(tex->GetHandle(), imageButtonSize * 3.0f, ImVec2(0.0f, flipImage ? 1.0f : 0.0f), ImVec2(1.0f, flipImage ? 0.0f : 1.0f));
                    ImGui::EndTooltip();
                }
            }
            else
            {
                if(ImGui::Button(tex ? "" : "Empty", imageButtonSize))
                {
                    NekoEngine::Editor::GetEditor()->GetFileBrowserPanel().Open();
                    NekoEngine::Editor::GetEditor()->GetFileBrowserPanel().SetCallback(callback);
                }
            }

            if(payload != nullptr && payload->IsDataType("AssetFile"))
            {
                auto filePath = std::string(reinterpret_cast<const char*>(payload->Data));
                if(NekoEngine::Editor::GetEditor()->IsTextureFile(filePath))
                {
                    if(ImGui::BeginDragDropTarget())
                    {
                        // Drop directly on to node and append to the end of it's children list.
                        if(ImGui::AcceptDragDropPayload("AssetFile"))
                        {
                            callback(filePath);
                            ImGui::EndDragDropTarget();

                            ImGui::Columns(1);
                            ImGui::Separator();
                            ImGui::PopStyleVar();

                            ImGui::TreePop();
                            return;
                        }

                        ImGui::EndDragDropTarget();
                    }
                }
            }

            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            if(tex)
            {
                ImGui::Text("%u x %u", tex->GetWidth(), tex->GetHeight());
                ImGui::Text("Mip Levels : %u", tex->GetMipMapLevels());
            }
            ImGui::PopItemWidth();
            /*      ImGui::TextUnformatted("Use Map");
                  ImGui::SameLine();
                  ImGui::PushItemWidth(-1);*/

            // ImGui::DragFloat(NekoEngine::ImGuiUtility::GenerateID(), &usingMapProperty, 0.0f, 1.0f);
            ImGui::SliderFloat(NekoEngine::ImGuiUtility::GenerateLabelID("Use Map"), &usingMapProperty, 0.0f, 1.0f);

            if(hasAmountValue)
            {
                float maxValue = 20.0f;
                if(std::strcmp(label, "Metallic") == 0 || std::strcmp(label, "Roughness") == 0)
                    maxValue = 1.0f;
                ImGui::SliderFloat(NekoEngine::ImGuiUtility::GenerateLabelID("Value"), &amount, 0.0f, maxValue);
            }
            // ImGui::TextUnformatted("Value");
            // ImGui::SameLine();
            // ImGui::PushItemWidth(-1);

            // ImGui::DragFloat(NekoEngine::ImGuiUtility::GenerateID(), &amount, 0.0f, 20.0f);

            // ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::Columns(1);

            ImGui::Separator();
            ImGui::PopStyleVar();

            ImGui::TreePop();
        }
    }

    template <>
    void ComponentEditorWidget<NekoEngine::ModelComponent>(entt::registry& reg, entt::registry::entity_type e)
    {
        
        auto& model = *reg.get<NekoEngine::ModelComponent>(e).model.get();

        auto primitiveType = reg.get<NekoEngine::ModelComponent>(e).model ? model.GetPrimitiveType() : NekoEngine::PrimitiveType::None;

        NekoEngine::ImGuiUtility::PushID();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        ImGui::TextUnformatted("Primitive Type");

        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        const char* shapes[]      = { "Sphere", "Cube", "Pyramid", "Capsule", "Cylinder", "Terrain", "File", "Quad", "None" };
        std::string shape_current = GetPrimativeName(primitiveType);
        if(ImGui::BeginCombo("", shape_current.c_str(), 0)) // The second parameter is the label previewed before opening the combo.
        {
            for(int n = 0; n < 8; n++)
            {
                bool is_selected = (shape_current.c_str() == shapes[n]);
                if(ImGui::Selectable(shapes[n], shape_current.c_str()))
                {
                    if(reg.get<NekoEngine::ModelComponent>(e).model)
                        model.GetMeshes().clear();

                    if(strcmp(shapes[n], "File") != 0)
                    {
                        if(reg.get<NekoEngine::ModelComponent>(e).model)
                        {
                            model.GetMeshes().push_back(SharedPtr<NekoEngine::Mesh>(NekoEngine::MeshFactory::CreatePrimative(GetPrimativeName(shapes[n]))));
                            model.SetPrimitiveType(GetPrimativeName(shapes[n]));
                        }
                        else
                        {
                            reg.get<NekoEngine::ModelComponent>(e).LoadPrimitive(GetPrimativeName(shapes[n]));
                        }
                    }
                    else
                    {
                        if(reg.get<NekoEngine::ModelComponent>(e).model)
                            model.SetPrimitiveType(NekoEngine::PrimitiveType::File);
                    }
                }
                if(is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        if(primitiveType == NekoEngine::PrimitiveType::File)
        {
            ImGui::TextUnformatted("FilePath");

            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            ImGui::TextUnformatted(model.GetFilePath().c_str());
            NekoEngine::ImGuiUtility::Tooltip(model.GetFilePath().c_str());

            ImGui::PopItemWidth();
            ImGui::NextColumn();
        }

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();

        int matIndex = 0;

        auto model2 = reg.get<NekoEngine::ModelComponent>(e).model;
        if(!model2)
        {
            NekoEngine::ImGuiUtility::PopID();
            return;
        }

        ImGui::Separator();
        const auto& meshes = model2->GetMeshes();
        if(ImGui::TreeNode("Meshes"))
        {
            for(const auto& mesh : meshes)
            {
                if(!mesh->GetName().empty())
                    ImGui::TextUnformatted(mesh->GetName().c_str());
            }
            ImGui::TreePop();
        }

        ImGui::Separator();


        ImGui::Separator();
        if(ImGui::TreeNode("Materials"))
        {
            NekoEngine::Material* MaterialShown[1000];
            uint32_t MaterialCount = 0;
            for(const auto& mesh : meshes)
            {
                auto material       = mesh->GetMaterial();
                std::string matName = material ? material->GetName() : "";

                bool materialFound = false;
                for(uint32_t i = 0; i < MaterialCount; i++)
                {
                    if(MaterialShown[i] == material.get())
                        materialFound = true;
                }

                if(materialFound)
                    continue;

                MaterialShown[MaterialCount++] = material.get();

                if(matName.empty())
                {
                    matName = "Material";
                    matName += std::to_string(matIndex);
                }

                matName += "##" + std::to_string(matIndex);
                matIndex++;
                if(!material)
                {
                    ImGui::TextUnformatted("Empty Material");
                    if(ImGui::Button("Add Material", ImVec2(ImGui::GetContentRegionAvail().x, 0.0f)))
                        mesh->SetMaterial(MakeShared<NekoEngine::Material>());
                }
                else if(ImGui::TreeNodeEx(matName.c_str(), ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanFullWidth))
                {
                    using namespace NekoEngine;
                    ImGui::Indent();
                    if(ImGui::Button("Save to file"))
                    {
                        std::string filePath = "//Meshes"; // Materials/" + matName + ".lmat";
                        std::string physicalPath;
                        if(VirtualFileSystem::ResolvePhysicalPath(filePath, physicalPath))
                        {
                            physicalPath += "/Materials/" + matName + ".lmat";
                            std::stringstream storage;

                            cereal::JSONOutputArchive output { storage };
                            material->save(output);

                            FileSystem::WriteTextFile(physicalPath, storage.str());
                        }
                    }
                    bool flipImage = gEngine->GetRenderer()->GetGraphicsContext()->FlipImGUITexture();

                    bool twoSided     = material->GetFlag(NekoEngine::Material::RenderFlags::TWOSIDED);
                    bool depthTested  = material->GetFlag(NekoEngine::Material::RenderFlags::DEPTHTEST);
                    bool alphaBlended = material->GetFlag(NekoEngine::Material::RenderFlags::ALPHABLEND);

                    ImGui::Columns(2);
                    ImGui::Separator();

                    ImGui::AlignTextToFramePadding();

                    if(ImGuiUtility::Property("Alpha Blended", alphaBlended))
                        material->SetFlag(NekoEngine::Material::RenderFlags::ALPHABLEND, alphaBlended);

                    if(ImGuiUtility::Property("Two Sided", twoSided))
                        material->SetFlag(NekoEngine::Material::RenderFlags::TWOSIDED, twoSided);

                    if(ImGuiUtility::Property("Depth Tested", depthTested))
                        material->SetFlag(NekoEngine::Material::RenderFlags::DEPTHTEST, depthTested);

                    ImGui::Columns(1);

                    MaterialProperties* prop = material->GetProperties();
                    auto colour                        = glm::vec4();
                    float normal                       = 0.0f;
                    auto& textures                     = material->GetTextures();
                    glm::vec2 textureSize              = glm::vec2(100.0f, 100.0f);
                    TextureWidget("Albedo", material.get(), textures.albedo.get(), flipImage, prop->albedoMapFactor, prop->albedoColour, [material](auto && PH1) mutable { material->SetAlbedoTexture(std::forward<decltype(PH1)>(PH1)); }, textureSize * gEngine->GetWindowDPI());
                    ImGui::Separator();

                    TextureWidget("Normal", material.get(), textures.normal.get(), flipImage, prop->normalMapFactor, normal, false, std::bind(&Material::SetNormalTexture, material, std::placeholders::_1), textureSize * gEngine->GetWindowDPI());
                    ImGui::Separator();

                    TextureWidget("Metallic", material.get(), textures.metallic.get(), flipImage, prop->metallicMapFactor, prop->metallic, true, std::bind(&Material::SetMetallicTexture, material, std::placeholders::_1), textureSize * gEngine->GetWindowDPI());
                    ImGui::Separator();

                    TextureWidget("Roughness", material.get(), textures.roughness.get(), flipImage, prop->roughnessMapFactor, prop->roughness, true, std::bind(&Material::SetRoughnessTexture, material, std::placeholders::_1), textureSize * gEngine->GetWindowDPI());

                    if(ImGui::TreeNodeEx("Reflectance", ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanFullWidth))
                    {
                        ImGui::SliderFloat("##Reflectance", &prop->reflectance, 0.0f, 1.0f);
                        ImGui::TreePop();
                    }

                    ImGui::Separator();

                    TextureWidget("AO", material.get(), textures.ao.get(), flipImage, prop->occlusionMapFactor, normal, false, std::bind(&Material::SetAOTexture, material, std::placeholders::_1), textureSize * gEngine->GetWindowDPI());
                    ImGui::Separator();

                    TextureWidget("Emissive", material.get(), textures.emissive.get(), flipImage, prop->emissiveMapFactor, prop->emissive, true, std::bind(&Material::SetEmissiveTexture, material, std::placeholders::_1), textureSize * gEngine->GetWindowDPI());

                    ImGui::Columns(2);

                    ImGui::AlignTextToFramePadding();
                    ImGui::TextUnformatted("WorkFlow");
                    ImGui::NextColumn();
                    ImGui::PushItemWidth(-1);

                    int workFlow = (int)material->GetProperties()->workflow;

                    if(ImGui::DragInt("##WorkFlow", &workFlow, 0.3f, 0, 2))
                    {
                        material->GetProperties()->workflow = (float)workFlow;
                    }

                    ImGui::PopItemWidth();
                    ImGui::NextColumn();

                    material->SetMaterialProperites(*prop);
                    ImGui::Columns(1);
                    ImGui::Unindent();
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }

        NekoEngine::ImGuiUtility::PopID();
    }

    template <>
    void ComponentEditorWidget<NekoEngine::Environment>(entt::registry& reg, entt::registry::entity_type e)
    {
        
        auto& environment = reg.get<NekoEngine::Environment>(e);
        // Disable image until texturecube is supported
        // NekoEngine::ImGuiUtility::Image(environment.GetEnvironmentMap(), glm::vec2(200, 200));

        uint8_t mode     = environment.GetMode();
        glm::vec4 params = environment.GetParameters();
        ImGui::PushItemWidth(-1);

        const char* modes[]      = { "Textures", "Preetham", "Generic" };
        const char* mode_current = modes[mode];
        if(ImGui::BeginCombo("", mode_current, 0)) // The second parameter is the label previewed before opening the combo.
        {
            for(int n = 0; n < 3; n++)
            {
                bool is_selected = (mode_current == modes[n]);
                if(ImGui::Selectable(modes[n], mode_current))
                {
                    environment.SetMode(n);
                }
                if(is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        if(mode == 0)
        {
            ImGui::TextUnformatted("File Path");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);

            static char filePath[INPUT_BUF_SIZE];
            strcpy(filePath, environment.GetFilePath().c_str());

            if(ImGui::InputText("##filePath", filePath, IM_ARRAYSIZE(filePath), 0))
            {
                environment.SetFilePath(filePath);
            }

            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("File Type");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);

            static char fileType[INPUT_BUF_SIZE];
            strcpy(fileType, environment.GetFileType().c_str());

            if(ImGui::InputText("##fileType", fileType, IM_ARRAYSIZE(fileType), 0))
            {
                environment.SetFileType(fileType);
            }

            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Width");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            int width = environment.GetWidth();

            if(ImGui::DragInt("##Width", &width))
            {
                environment.SetWidth(width);
            }

            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Height");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            int height = environment.GetHeight();

            if(ImGui::DragInt("##Height", &height))
            {
                environment.SetHeight(height);
            }

            ImGui::PopItemWidth();
            ImGui::NextColumn();

            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("Num Mips");
            ImGui::NextColumn();
            ImGui::PushItemWidth(-1);
            int numMips = environment.GetNumMips();
            if(ImGui::InputInt("##NumMips", &numMips))
            {
                environment.SetNumMips(numMips);
            }

            ImGui::PopItemWidth();
            ImGui::NextColumn();
        }
        else if(mode == 1)
        {
            bool valueUpdated = false;
            valueUpdated |= NekoEngine::ImGuiUtility::Property("Turbidity", params.x, 1.7f, 100.0f, 0.01f);
            valueUpdated |= NekoEngine::ImGuiUtility::Property("Azimuth", params.y, -1000.0f, 1000.f, 0.01f);
            valueUpdated |= NekoEngine::ImGuiUtility::Property("Inclination", params.z, -1000.0f, 1000.f, 0.01f);

            if(valueUpdated)
                environment.SetParameters(params);
        }

        ImGui::Columns(1);
        if(ImGui::Button("Reload", ImVec2(ImGui::GetContentRegionAvail().x, 0.0)))
            environment.Load();

        ImGui::Separator();
        ImGui::PopStyleVar();
    }

    template <>
    void ComponentEditorWidget<NekoEngine::DefaultCameraController>(entt::registry& reg, entt::registry::entity_type e)
    {
        
        auto& controllerComp = reg.get<NekoEngine::DefaultCameraController>(e);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Controller Type");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);

        const char* controllerTypes[] = { "Editor", "FPS", "ThirdPerson", "2D", "Custom" };
        std::string currentController = NekoEngine::DefaultCameraController::CameraControllerTypeToString(controllerComp.GetType());
        if(ImGui::BeginCombo("", currentController.c_str(), 0)) // The second parameter is the label previewed before opening the combo.
        {
            for(int n = 0; n < 5; n++)
            {
                bool is_selected = (currentController.c_str() == controllerTypes[n]);
                if(ImGui::Selectable(controllerTypes[n], currentController.c_str()))
                {
                    controllerComp.SetControllerType(NekoEngine::DefaultCameraController::StringToControllerType(controllerTypes[n]));
                }
                if(is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        if(controllerComp.GetController())
            controllerComp.GetController()->OnImGui();

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();
    }


}


namespace NekoEngine
{
    InspectorPanel::InspectorPanel()
    {
        m_Name       = ICON_MDI_INFORMATION " Inspector###inspector";
        m_SimpleName = "Inspector";
    }

    static bool init = false;
    void InspectorPanel::OnNewLevel(Level* level)
    {

        if(init)
            return;

        init = true;

        auto& registry = level->GetRegistry();
        auto& iconMap  = m_Editor->GetComponentIconMap();

#define TRIVIAL_COMPONENT(ComponentType, ComponentName)                      \
    {                                                                        \
        std::string Name;                                                    \
        if(iconMap.find(typeid(ComponentType).hash_code()) != iconMap.end()) \
            Name += iconMap[typeid(ComponentType).hash_code()];              \
        else                                                                 \
            Name += iconMap[typeid(Editor).hash_code()];                     \
        Name += "\t";                                                        \
        Name += (ComponentName);                                             \
        m_EnttEditor.registerComponent<ComponentType>(Name.c_str());         \
    }
        TRIVIAL_COMPONENT(Transform, "Transform");
        // TRIVIAL_COMPONENT(Model, "Model");
        TRIVIAL_COMPONENT(ModelComponent, "ModelComponent");
        TRIVIAL_COMPONENT(Camera, "Camera");
        TRIVIAL_COMPONENT(AxisConstraintComponent, "AxisConstraint");
        TRIVIAL_COMPONENT(RigidBody3DComponent, "Physics3D");
        TRIVIAL_COMPONENT(Light, "Light");
        TRIVIAL_COMPONENT(LuaScriptComponent, "LuaScript");
        TRIVIAL_COMPONENT(Environment, "Environment");
        TRIVIAL_COMPONENT(DefaultCameraController, "Default Camera Controller");
    }

    void InspectorPanel::OnImGui()
    {


        auto selectedEntities = m_Editor->GetSelected();

        if(ImGui::Begin(m_Name.c_str(), &m_Active))
        {
            ImGuiUtility::PushID();

            Level* currentLevel = gEngine->GetLevelManager()->GetCurrentLevel();

            if(!currentLevel)
            {
                m_Editor->SetSelected(entt::null);
                ImGuiUtility::PopID();
                ImGui::End();
                return;
            }

            auto& registry = currentLevel->GetRegistry();
            if(selectedEntities.size() != 1 || !registry.valid(selectedEntities.front()))
            {
                m_Editor->SetSelected(entt::null);
                ImGuiUtility::PopID();
                ImGui::End();
                return;
            }

            auto selected         = selectedEntities.front();
            Entity SelectedEntity = { selected, currentLevel };

            // active checkbox
            auto activeComponent = registry.try_get<ActiveComponent>(selected);
            bool active          = activeComponent ? activeComponent->active : true;
            if(ImGui::Checkbox("##ActiveCheckbox", &active))
            {
                if(!activeComponent)
                    registry.emplace<ActiveComponent>(selected, active);
                else
                    activeComponent->active = active;
            }
            ImGui::SameLine();
            ImGui::TextUnformatted(ICON_MDI_CUBE);
            ImGui::SameLine();

            bool hasName = registry.all_of<NameComponent>(selected);
            std::string name;
            if(hasName)
                name = registry.get<NameComponent>(selected).name;
            else
                name = StringUtility::ToString(entt::to_integral(selected));

            if(m_DebugMode)
            {
                if(registry.valid(selected))
                {
                    // ImGui::Text("ID: %d, Version: %d", static_cast<int>(registry.entity(selected)), registry.version(selected));
                }
                else
                {
                    ImGui::TextUnformatted("INVALID ENTITY");
                }
            }

            ImGui::SameLine();
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetFontSize() * 4.0f);
            {
                ImGuiUtility::ScopedFont boldFont(ImGui::GetIO().Fonts->Fonts[1]);
                if(ImGuiUtility::InputText(name))
                    registry.get_or_emplace<NameComponent>(selected).name = name;
            }
            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.7f, 0.7f, 0.0f));

            if(ImGui::Button(ICON_MDI_FLOPPY))
                ImGui::OpenPopup("SavePrefab");

            ImGuiUtility::Tooltip("Save Entity As Prefab");

            ImGui::SameLine();
            if(ImGui::Button(ICON_MDI_TUNE))
                ImGui::OpenPopup("SetDebugMode");
            ImGui::PopStyleColor();

            if(ImGui::BeginPopup("SetDebugMode", 3))
            {
                if(SelectedEntity.HasComponent<PrefabComponent>() && ImGui::Button("Revert To Prefab"))
                {
                    auto path = SelectedEntity.GetComponent<PrefabComponent>().Path;
                    m_Editor->UnSelect(selected);
                    SelectedEntity.Destroy();

                    SelectedEntity = gEngine->GetLevelManager()->GetCurrentLevel()->InstantiatePrefab(path);
                    selected       = SelectedEntity.GetHandle();
                    m_Editor->SetSelected(selected);
                }

                if(ImGui::Selectable("Debug Mode", m_DebugMode))
                {
                    m_DebugMode = !m_DebugMode;
                }
                ImGui::EndPopup();
            }

            if(ImGui::BeginPopupModal("SavePrefab", NULL, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Save Current Entity as a Prefab?\n\n");
                ImGui::Separator();

                static std::string prefabName = SelectedEntity.GetName();
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Name : ");
                ImGui::SameLine();
                ImGuiUtility::InputText(prefabName);

                static std::string prefabNamePath = "//Assets/Prefabs/";
                ImGui::AlignTextToFramePadding();
                ImGui::TextUnformatted("Path : ");
                ImGui::SameLine();
                ImGuiUtility::InputText(prefabNamePath);

                if(ImGui::Button("OK", ImVec2(120, 0)))
                {
                    std::string physicalPath;
                    VirtualFileSystem::ResolvePhysicalPath(prefabNamePath, physicalPath, true);
                    std::string FullPath = physicalPath + prefabName + std::string(".lprefab");
                    gEngine->GetLevelManager()->GetCurrentLevel()->SavePrefab({ selected, gEngine->GetLevelManager()->GetCurrentLevel() }, FullPath);
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SetItemDefaultFocus();
                ImGui::SameLine();
                if(ImGui::Button("Cancel", ImVec2(120, 0)))
                {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            ImGui::Separator();

            if(m_DebugMode)
            {
                auto idComponent = registry.try_get<IDComponent>(selected);

                ImGui::Text("UUID : %" PRIu64, (uint64_t)idComponent->ID);

                auto hierarchyComp = registry.try_get<Hierarchy>(selected);

                if(hierarchyComp)
                {
                    if(registry.valid(hierarchyComp->Parent()))
                    {
                        idComponent = registry.try_get<IDComponent>(hierarchyComp->Parent());
                        ImGui::Text("Parent : ID: %" PRIu64, (uint64_t)idComponent->ID);
                    }
                    else
                    {
                        ImGui::TextUnformatted("Parent : null");
                    }

                    entt::entity child = hierarchyComp->First();
                    ImGui::TextUnformatted("Children : ");
                    ImGui::Indent(24.0f);

                    while(child != entt::null)
                    {
                        idComponent = registry.try_get<IDComponent>(child);
                        ImGui::Text("ID: %" PRIu64, (uint64_t)idComponent->ID);

                        auto hierarchy = registry.try_get<Hierarchy>(child);

                        if(hierarchy)
                        {
                            child = hierarchy->Next();
                        }
                    }

                    ImGui::Unindent(24.0f);
                }

                ImGui::Separator();
            }

            if(registry.try_get<PrefabComponent>(selected))
            {
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_CheckMark));
                ImGui::Separator();
                ImGui::Text("Prefab %s", registry.get<PrefabComponent>(selected).Path.c_str());
                ImGui::Separator();
                ImGui::PopStyleColor();
            }

            ImGui::BeginChild("Components", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_None);
            m_EnttEditor.RenderImGui(registry, selected);
            ImGui::EndChild();

            ImGuiUtility::PopID();
        }
        ImGui::End();
    }

    void InspectorPanel::SetDebugMode(bool mode)
    {
        m_DebugMode = mode;
    }
}
