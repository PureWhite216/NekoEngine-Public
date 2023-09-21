#include "EditorSettingsPanel.h"
#include "GUI/ImGuiUtility.h"
#include "Editor.h"

namespace NekoEngine
{
    EditorSettingsPanel::EditorSettingsPanel()
    {
        m_Name       = "EditorSettings###editorsettings";
        m_SimpleName = "Editor Settings";
    }

    void EditorSettingsPanel::OnImGui()
    {
        if(!m_CurrentLevel)
            return;

        ImGui::Begin(m_Name.c_str(), &m_Active, 0);
        ImGuiUtility::PushID();

        {
            ImGuiUtility::ScopedStyle frameStyle(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

            auto sceneName   = m_CurrentLevel->GetName();

            auto& editorSettings = m_Editor->GetSettings();
            ImGuiUtility::Property("Theme", (int&)editorSettings.m_Theme);
            ImGui::Columns(1);
            ImGui::TextUnformatted("Camera Settings");
            ImGui::Columns(2);

            if(ImGuiUtility::Property("Camera Speed", editorSettings.m_CameraSpeed))
                m_Editor->GetEditorCameraController().SetSpeed(editorSettings.m_CameraSpeed);
            if(ImGuiUtility::Property("Camera Near", editorSettings.m_CameraNear))
                m_Editor->GetCamera()->SetNear(editorSettings.m_CameraNear);
            if(ImGuiUtility::Property("Camera Far", editorSettings.m_CameraFar))
                m_Editor->GetCamera()->SetFar(editorSettings.m_CameraFar);

            ImGui::TextUnformatted("Camera Transform");
            ImGui::Columns(1);
            // Camera Transform;
            auto& transform = m_Editor->GetEditorCameraTransform();

            auto rotation   = glm::degrees(glm::eulerAngles(transform.GetLocalOrientation()));
            auto position   = transform.GetLocalPosition();
            auto scale      = transform.GetLocalScale();
            float itemWidth = (ImGui::GetContentRegionAvail().x - (ImGui::GetFontSize() * 3.0f)) / 3.0f;

            // Call this to fix alignment with columns
            ImGui::AlignTextToFramePadding();

            if(ImGuiUtility::PorpertyTransform("Position", position, itemWidth))
                transform.SetLocalPosition(position);

            ImGui::SameLine();
            if(ImGuiUtility::PorpertyTransform("Rotation", rotation, itemWidth))
            {
                float pitch = Maths::Min(rotation.x, 89.9f);
                pitch       = Maths::Max(pitch, -89.9f);
                transform.SetLocalOrientation(glm::quat(glm::radians(glm::vec3(pitch, rotation.y, rotation.z))));
            }

            ImGui::SameLine();
            if(ImGuiUtility::PorpertyTransform("Scale", scale, itemWidth))
            {
                transform.SetLocalScale(scale);
            }

            ImGui::Separator();

            ImGui::Columns(1);
            ImGui::TextUnformatted("Scene Panel Settings");
            ImGui::Columns(2);

            ImGuiUtility::Property("Grid Size", editorSettings.m_GridSize);
            ImGuiUtility::Property("Grid Enabled", editorSettings.m_ShowGrid);
            ImGuiUtility::Property("Gizmos Enabled", editorSettings.m_ShowGizmos);
            ImGuiUtility::Property("ImGuizmo Scale", editorSettings.m_ImGuizmoScale);
            ImGuiUtility::Property("Show View Selected", editorSettings.m_ShowViewSelected);
            ImGuiUtility::Property("View 2D", editorSettings.m_View2D);
            ImGuiUtility::Property("Fullscreen on play", editorSettings.m_FullScreenOnPlay);
            ImGuiUtility::Property("Snap Amount", editorSettings.m_SnapAmount);
            ImGuiUtility::Property("Sleep Out of Focus", editorSettings.m_SleepOutofFocus);
            ImGuiUtility::Property("Debug draw flags", (int&)editorSettings.m_DebugDrawFlags);
            ImGuiUtility::Property("Physics 2D debug flags", (int&)editorSettings.m_Physics2DDebugFlags);
            ImGuiUtility::Property("Physics 3D debug flags", (int&)editorSettings.m_Physics3DDebugFlags);
        }
        ImGui::Columns(1);

        if(ImGui::Button("Save Settings"))
            m_Editor->SaveEditorSettings();

        ImGuiUtility::PopID();
        ImGui::End();
    }
} // NekoEngine