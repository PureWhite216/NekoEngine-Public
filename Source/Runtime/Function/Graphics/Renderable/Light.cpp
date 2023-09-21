#include "Light.h"
#include "ImGui/imgui.h"
#include "GUI/ImGuiUtility.h"

namespace NekoEngine
{
    Light::Light(const glm::vec3& direction, const glm::vec4& colour, float intensity, const LightType& type, const glm::vec3& position, float radius, float angle)
            : Direction(glm::vec4(direction, 1.0f))
            , Colour(colour)
            , Position(glm::vec4(position, 1.0f))
            , Intensity(intensity)
            , Radius(radius)
            , Type(float(type))
            , Angle(angle)
    {
    }

    std::string Light::LightTypeToString(LightType type)
    {
        switch(type)
        {
            case LightType::DirectionalLight:
                return "Directional Light";
            case LightType::SpotLight:
                return "Spot Light";
            case LightType::PointLight:
                return "Point Light";
            default:
                return "ERROR";
        }
    }

    float Light::StringToLightType(const std::string& type)
    {
        if(type == "Directional")
            return float(LightType::DirectionalLight);

        if(type == "Point")
            return float(LightType::PointLight);

        if(type == "Spot")
            return float(LightType::SpotLight);

        LOG("Unknown Light Type");
        return 0.0f;
    }

    void Light::OnImGui()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
        ImGui::Columns(2);
        ImGui::Separator();

        if(Type != 0)
            ImGuiUtility::Property("Position", Position);

        if(Type != 2)
            ImGuiUtility::Property("Direction", Direction);

        if(Type != 0)
            ImGuiUtility::Property("Radius", Radius, 0.0f, 100.0f);
        ImGuiUtility::Property("Colour", Colour, true, ImGuiUtility::PropertyFlag::ColourProperty);
        ImGuiUtility::Property("Intensity", Intensity, 0.0f, 100.0f);

        if(Type == 1)
            ImGuiUtility::Property("Angle", Angle, -1.0f, 1.0f);

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Light Type");
        ImGui::NextColumn();
        ImGui::PushItemWidth(-1);
        if(ImGui::BeginMenu(LightTypeToString(LightType(int(Type))).c_str()))
        {
            if(ImGui::MenuItem("Directional Light", "", static_cast<int>(Type) == 0, true))
            {
                Type = float(int(LightType::DirectionalLight));
            }
            if(ImGui::MenuItem("Spot Light", "", static_cast<int>(Type) == 1, true))
            {
                Type = float(int(LightType::SpotLight));
            }
            if(ImGui::MenuItem("Point Light", "", static_cast<int>(Type) == 2, true))
            {
                Type = float(int(LightType::PointLight));
            }
            ImGui::EndMenu();
        }

        ImGui::PopItemWidth();
        ImGui::NextColumn();

        ImGui::Columns(1);
        ImGui::Separator();
        ImGui::PopStyleVar();
    }
} // NekoEngine