#include "GUI/ImGuiUtility.h"
#include "File/VirtualFileSystem.h"
#include "filesystem"
#include "Editor.h"
#include "PhysicsEngine.h"
#include "SceneSettingsPanel.h"

namespace NekoEngine
{
    SceneSettingsPanel::SceneSettingsPanel()
    {
        m_Name       = "SceneSettings###scenesettings";
        m_SimpleName = "Scene Settings";
    }

    void SceneSettingsPanel::OnImGui()
    {
        if(!m_CurrentLevel)
            return;

        if(ImGui::Begin(m_Name.c_str(), &m_Active, 0))
        {
            ImGuiUtility::ScopedStyle(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
            ImGuiUtility::PushID();
            {
                auto sceneName      = m_CurrentLevel->GetName();
                auto& sceneSettings = m_CurrentLevel->GetSettings();

                if(m_NameUpdated)
                    sceneName = m_LevelName;

                ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();
                ImGui::PushItemWidth(contentRegionAvailable.x * 0.5f);

                {
                    ImGuiUtility::ScopedFont boldFont(ImGui::GetIO().Fonts->Fonts[1]);
                    if(ImGuiUtility::InputText(sceneName))
                    {
                        m_NameUpdated = true;
                    }

                    if(!ImGui::IsItemActive() && m_NameUpdated)
                    {
                        m_NameUpdated = false;
                        std::string scenePath;
                        if(VirtualFileSystem::ResolvePhysicalPath("//Levels/" + m_CurrentLevel->GetName() + ".lsn", scenePath))
                        {
                            m_CurrentLevel->SetName(sceneName);
                            // m_CurrentLevel->Serialise(m_Editor->GetProjectSettings().m_ProjectRoot + "Assets/Levels/");

                            std::filesystem::rename(scenePath, m_Editor->GetProjectSettings().m_ProjectRoot + "Assets/Levels/" + m_CurrentLevel->GetName() + ".lsn");
                        }
                        else
                            m_CurrentLevel->SetName(sceneName);

                        // Save project with updated scene name
                        m_Editor->Serialise();
                    }
                }

//                ImGui::SameLine();
//                ImGui::Text("Version : %i", (int)sceneVersion);

                ImGui::Columns(1);
                if(ImGui::CollapsingHeader("Renderer", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::Columns(2);
                    ImGuiUtility::Property("Renderer 2D Enabled", sceneSettings.renderSettings.Renderer2DEnabled);
                    ImGuiUtility::Property("Renderer 3D Enabled", sceneSettings.renderSettings.Renderer3DEnabled);
                    ImGuiUtility::Property("Shadow Enabled", sceneSettings.renderSettings.ShadowsEnabled);
                    ImGuiUtility::Property("Skybox Render Enabled", sceneSettings.renderSettings.SkyboxRenderEnabled);
                    ImGuiUtility::Property("Skybox Mip Level", sceneSettings.renderSettings.SkyboxMipLevel, 0.0f, 14.0f, 0.01f);

                    ImGuiUtility::Property("Debug Renderer Enabled", sceneSettings.renderSettings.DebugRenderEnabled);
                    ImGuiUtility::Property("FXAA Enabled", sceneSettings.renderSettings.FXAAEnabled);
                    ImGuiUtility::Property("Debanding Enabled", sceneSettings.renderSettings.DebandingEnabled);
                    ImGuiUtility::Property("ChromaticAberation Enabled", sceneSettings.renderSettings.ChromaticAberationEnabled);
                    ImGuiUtility::Property("Filmic Grain Enabled", sceneSettings.renderSettings.FilmicGrainEnabled);
                    ImGuiUtility::Property("Sharpen Enabled", sceneSettings.renderSettings.SharpenEnabled);

                    ImGuiUtility::Property("Bloom Enabled", sceneSettings.renderSettings.BloomEnabled);
                    ImGuiUtility::Property("Bloom Intensity", sceneSettings.renderSettings.m_BloomIntensity);
                    ImGuiUtility::Property("Bloom Upsample Scale", sceneSettings.renderSettings.BloomUpsampleScale);
                    ImGuiUtility::Property("Bloom Knee", sceneSettings.renderSettings.BloomKnee);
                    ImGuiUtility::Property("Bloom Threshold", sceneSettings.renderSettings.BloomThreshold);
                    ImGuiUtility::Property("Depth Of Field Enabled", sceneSettings.renderSettings.DepthOfFieldEnabled);
                    ImGuiUtility::Property("Depth Of Field Strength", sceneSettings.renderSettings.DepthOfFieldStrength);
                    ImGuiUtility::Property("Depth Of Field Distance", sceneSettings.renderSettings.DepthOfFieldDistance);

                    // ImGui::BeginDisabled();
                    ImGuiUtility::Property("SSAO Enabled", sceneSettings.renderSettings.SSAOEnabled);
                    ImGuiUtility::Property("SSAO Sample Radius", sceneSettings.renderSettings.SSAOSampleRadius, 0.0f, 16.0f, 0.01f);
                    ImGuiUtility::Property("SSAO Blur Radius", sceneSettings.renderSettings.SSAOBlurRadius, 0, 16);
                    ImGuiUtility::Property("SSAO Blur Enabled", sceneSettings.renderSettings.SSAOBlur);
                    ImGuiUtility::Property("SSAO Strength", sceneSettings.renderSettings.SSAOStrength, 0.0f, 16.0f, 0.01f);
                    // ImGui::EndDisabled();

                    static const char* toneMaps[7] = {
                            "None",
                            "Linear",
                            "Simple Reinhard",
                            "Luma Reinhard",
                            "White Preserved Luma Reinard",
                            "Uncharted 2",
                            "Aces"
                    };

                    ImGuiUtility::PropertyDropdown("ToneMap", toneMaps, 7, (int*)&m_CurrentLevel->GetSettings().renderSettings.m_ToneMapIndex);
                    ImGuiUtility::Property("Brightness", sceneSettings.renderSettings.Brightness, -1.0f, 1.0f, 0.01f);
                    ImGuiUtility::Property("Contrast", sceneSettings.renderSettings.Contrast, 0.0f, 2.0f, 0.01f);
                    ImGuiUtility::Property("Saturation", sceneSettings.renderSettings.Saturation, 0.0f, 1.0f, 0.01f);

                    auto& registry  = m_CurrentLevel->GetRegistry();
                    int entityCount = (int)registry.storage<entt::entity>().size();
                    ImGuiUtility::Property("Entity Count", entityCount, 0, 0, ImGuiUtility::PropertyFlag::ReadOnly);

                    static const char* debugModes[7] = {
                            "None",
                            "SSAO",
                            "SSAO1",
                            "Normals",
                            "Bloom",
                            "Noise",
                            "Post Process"
                    };
                    ImGuiUtility::PropertyDropdown("Debug View Mode", debugModes, 7, (int*)&sceneSettings.renderSettings.DebugMode);
                }

                ImGui::Columns(1);
                if(ImGui::CollapsingHeader("Systems"))
                {
                    ImGui::Columns(2);
                    ImGuiUtility::Property("Audio Enabled", sceneSettings.AudioEnabled);
                    ImGuiUtility::Property("Physics 2D Enabled", sceneSettings.PhysicsEnabled2D);
                    ImGuiUtility::Property("Physics 3D Enabled", sceneSettings.PhysicsEnabled3D);
                }

                ImGui::Columns(1);
                if(ImGui::CollapsingHeader("Physics3D"))
                {
                    ImGui::Columns(2);
                    auto physicsSystem = gEngine->GetSystem<PhysicsEngine>();

                    if(physicsSystem)
                    {
                        if(ImGuiUtility::Property("Dampening", sceneSettings.physicsSettings.Dampening))
                            physicsSystem->SetDampingFactor(sceneSettings.physicsSettings.Dampening);
                        if(ImGuiUtility::Property("Max Updates Per Frame", sceneSettings.physicsSettings.m_MaxUpdatesPerFrame))
                            physicsSystem->SetMaxUpdatesPerFrame(sceneSettings.physicsSettings.m_MaxUpdatesPerFrame);
                        if(ImGuiUtility::Property("Position Iterations", sceneSettings.physicsSettings.PositionIterations))
                            physicsSystem->SetPositionIterations(sceneSettings.physicsSettings.PositionIterations);
                        if(ImGuiUtility::Property("Velocity Iterations", sceneSettings.physicsSettings.VelocityIterations))
                            physicsSystem->SetPositionIterations(sceneSettings.physicsSettings.VelocityIterations);
                        if(ImGuiUtility::Property("Gravity", sceneSettings.physicsSettings.Gravity))
                            physicsSystem->SetGravity(sceneSettings.physicsSettings.Gravity);

                        if(ImGuiUtility::Property("BroadPhaseIndex", sceneSettings.physicsSettings.BroadPhaseTypeIndex))
                            physicsSystem->SetBroadphaseType((BroadphaseType)sceneSettings.physicsSettings.BroadPhaseTypeIndex);
                        if(ImGuiUtility::Property("IntegrationType", sceneSettings.physicsSettings.IntegrationTypeIndex))
                            physicsSystem->SetIntegrationType((IntegrationType)sceneSettings.physicsSettings.IntegrationTypeIndex);
                    }
                }
            }
            ImGui::Columns(1);
            ImGuiUtility::PopID();
        }
        ImGui::End();
    }
}