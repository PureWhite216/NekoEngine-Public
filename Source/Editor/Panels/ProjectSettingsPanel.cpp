#include "ProjectSettingsPanel.h"
#include "GUI/ImGuiUtility.h"
#include "filesystem"
#include "Engine.h"
namespace NekoEngine
{
    ProjectSettingsPanel::ProjectSettingsPanel()
    {
        m_Name       = "ProjectSettings###projectsettings";
        m_SimpleName = "Project Settings";
    }

    void ProjectSettingsPanel::OnImGui()
    {


        ImGui::Begin(m_Name.c_str(), &m_Active, 0);
        ImGuiUtility::PushID();

        ImGui::Columns(2);
        ImGuiUtility::ScopedStyle(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

        {
            auto& projectSettings = gEngine->GetProjectSettings();
            auto projectName      = projectSettings.m_ProjectName;

            if(m_NameUpdated)
                projectName = m_ProjectName;

            if(ImGuiUtility::Property("Project Name", projectName, ImGuiUtility::PropertyFlag::None))
            {
                m_NameUpdated = true;
            }

            ImGuiUtility::PropertyConst("Project Root", projectSettings.m_ProjectRoot.c_str());
            ImGuiUtility::PropertyConst("Engine Asset Path", projectSettings.m_EngineAssetPath.c_str());
            ImGuiUtility::Property("App Width", (int&)projectSettings.Width, 0, 0, ImGuiUtility::PropertyFlag::ReadOnly);
            ImGuiUtility::Property("App Height", (int&)projectSettings.Height, 0, 0, ImGuiUtility::PropertyFlag::ReadOnly);
            ImGuiUtility::Property("Fullscreen", projectSettings.Fullscreen);
            ImGuiUtility::Property("VSync", projectSettings.VSync);
            ImGuiUtility::Property("Borderless", projectSettings.Borderless);
            ImGuiUtility::Property("Show Console", projectSettings.ShowConsole);
            ImGuiUtility::Property("Title", projectSettings.Title);
            ImGuiUtility::Property("RenderAPI", projectSettings.RenderAPI, 0, 1);
            ImGuiUtility::Property("Project Version", projectSettings.ProjectVersion, 0, 0, ImGuiUtility::PropertyFlag::ReadOnly);

            if(!ImGui::IsItemActive() && m_NameUpdated)
            {
                m_NameUpdated = false;
                auto fullPath = projectSettings.m_ProjectRoot + projectSettings.m_ProjectName + std::string(".lmproj");
                if(std::filesystem::exists(fullPath))
                {
                    projectSettings.m_ProjectName = projectName;
                    std::filesystem::rename(fullPath, projectSettings.m_ProjectRoot + projectSettings.m_ProjectName + std::string(".lmproj"));
                }
                else
                    projectSettings.m_ProjectName = projectName;
            }
        }
        ImGui::Columns(1);
        ImGuiUtility::PopID();
        ImGui::End();
    }
} // NekoEngine