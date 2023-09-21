#pragma once
#include "EditorPanel.h"
#include "ImGui/imgui.h"
#include "entt/entt.hpp"
#include "Math/Maths.h"
#include "Math/Frustum.h"
#include "RHI/Texture.h"
#include "Transform.h"
#include "GUI/ImGuiUtility.h"
#include "StringUtility.h"

namespace NekoEngine
{

    class SceneViewPanel : public EditorPanel
    {
    private:
        std::unordered_map<size_t, bool> m_ShowComponentGizmoMap;

        bool m_ShowStats                                 = false;
        SharedPtr<Texture2D> m_GameViewTexture = nullptr;
        Level* m_CurrentLevel                         = nullptr;
        uint32_t m_Width, m_Height;

    public:
        SceneViewPanel();
        ~SceneViewPanel() = default;

        void OnImGui() override;
        void OnNewLevel(Level* level) override;
        void ToolBar();
        void DrawGizmos(float width, float height, float xpos, float ypos, Level* level);

        void Resize(uint32_t width, uint32_t height);

    private:
        template <typename T>
        void ShowComponentGizmo(float width, float height, float xpos, float ypos, const glm::mat4& viewProj, const Frustum& frustum, entt::registry& registry)
        {
            if(m_ShowComponentGizmoMap[typeid(T).hash_code()])
            {
                auto group = registry.group<T>(entt::get<Transform>);

                for(auto entity : group)
                {
                    const auto& [component, trans] = group.template get<T, Transform>(entity);

                    glm::vec3 pos = trans.GetWorldPosition();

                    auto inside = frustum.IsInside(pos);

                    if(inside == Intersection::OUTSIDE)
                        continue;

                    glm::vec2 screenPos = Maths::WorldToScreen(pos, viewProj, width, height, xpos, ypos);
                    ImGui::SetCursorPos({ screenPos.x - ImGui::GetFontSize() * 0.5f, screenPos.y - ImGui::GetFontSize() * 0.5f });
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.7f, 0.7f, 0.0f));

                    // if(ImGui::Button(m_Editor->GetComponentIconMap()[typeid(T).hash_code()]))
                    //{
                    //     m_Editor->SetSelected(entity);
                    // }

                    ImGui::TextUnformatted(m_Editor->GetComponentIconMap()[typeid(T).hash_code()]);
                    ImGui::PopStyleColor();

                    ImGuiUtility::Tooltip(StringUtility::Demangle(typeid(T).name()).c_str());
                }
            }
        }
    };

} // NekoEngine

