#include "GUI//ImGuiUtility.h"
#include "GameViewPanel.h"
#include "ImGui/imgui.h"
#include "ImGui/IconsMaterialDesignIcons.h"
#include "ImGui/Plugins/ImGuizmo.h"
#include "Editor.h"

namespace NekoEngine
{
    GameViewPanel::GameViewPanel()
    {
        m_Name         = ICON_MDI_GAMEPAD_VARIANT " Game###game";
        m_SimpleName   = "Game";
        m_CurrentLevel = nullptr;

        m_Width  = 1280;
        m_Height = 800;

        m_SceneRenderer                          = MakeUnique<SceneRenderer>(m_Width, m_Height);
        m_SceneRenderer->GetSettings().DebugPass = false;
        m_SceneRenderer->m_DebugRenderEnabled    = false;
    }

    static std::string AspectToString(float aspect)
    {
        if(Maths::Equals(aspect, 16.0f / 10.0f))
        {
            return "16:10";
        }
        else if(Maths::Equals(aspect, 16.0f / 9.0f))
        {
            return "16:9";
        }
        else if(Maths::Equals(aspect, 4.0f / 3.0f))
        {
            return "4:3";
        }
        else if(Maths::Equals(aspect, 3.0f * 0.5f))
        {
            return "3:2";
        }
        else if(Maths::Equals(aspect, 9.0f / 16.0f))
        {
            return "9:16";
        }
        else
        {
            return "Unsupported";
        }
    }

    static float StringToAspect(const std::string& aspect)
    {
        if(aspect == "16:10")
        {
            return 16.0f / 10.0f;
        }
        else if(aspect == "16:9")
        {
            return 16.0f / 9.0f;
        }
        else if(aspect == "4:3")
        {
            return 4.0f / 3.0f;
        }
        else if(aspect == "3:2")
        {
            return 3.0f * 0.5f;
        }
        else if(aspect == "9:16")
        {
            return 9.0f / 16.0f;
        }
        else
        {
            return 1.0f;
        }
    }

    void GameViewPanel::OnImGui()
    {


        ImGuiUtility::ScopedStyle windowPadding(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        auto flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        if(!ImGui::Begin(m_Name.c_str(), &m_Active, flags) || !m_CurrentLevel)
        {
            m_GameViewVisible = false;
            ImGui::End();
            return;
        }

        m_GameViewVisible           = true;
        Camera* camera              = nullptr;
        Transform* transform = nullptr;

        {
            m_SceneRenderer->SetOverrideCamera(nullptr, nullptr);

            auto& registry  = m_CurrentLevel->GetRegistry();
            auto cameraView = registry.view<Camera>();
            if(!cameraView.empty())
            {
                camera = &registry.get<Camera>(cameraView.front());
            }
        }

        ImVec2 offset = { 0.0f, 0.0f };

        {
            ToolBar();
            offset = ImGui::GetCursorPos(); // Usually ImVec2(0.0f, 50.0f);
        }

        auto sceneViewSize     = ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin() - offset * 0.5f;
        auto sceneViewPosition = ImGui::GetWindowPos() + offset;

        if(m_Editor->GetEditorState() == EditorState::Play)
            ImGui::GetForegroundDrawList()->AddQuad(sceneViewPosition, sceneViewPosition + ImVec2(sceneViewSize.x, 0.0f), sceneViewPosition + ImVec2(sceneViewSize.x, sceneViewSize.y), sceneViewPosition + ImVec2(0.0f, sceneViewSize.y), ImGui::ColorConvertFloat4ToU32(ImGuiUtility::GetSelectedColour()), 5);

        sceneViewSize.x -= static_cast<int>(sceneViewSize.x) % 2 != 0 ? 1.0f : 0.0f;
        sceneViewSize.y -= static_cast<int>(sceneViewSize.y) % 2 != 0 ? 1.0f : 0.0f;

        if(!m_Editor->GetSettings().m_FreeAspect)
        {
            float heightNeededForAspect = sceneViewSize.x / m_Editor->GetSettings().m_FixedAspect;

            if(heightNeededForAspect > sceneViewSize.y)
            {
                sceneViewSize.x = sceneViewSize.y * m_Editor->GetSettings().m_FixedAspect;
                float xOffset   = ((ImGui::GetContentRegionAvail() - sceneViewSize) * 0.5f).x;
                sceneViewPosition.x += xOffset;
                ImGui::SetCursorPos({ xOffset, ImGui::GetCursorPosY() + offset.x });
                offset.x += xOffset;
            }
            else
            {
                sceneViewSize.y = sceneViewSize.x / m_Editor->GetSettings().m_FixedAspect;
                float yOffset   = ((ImGui::GetContentRegionAvail() - sceneViewSize) * 0.5f).y;
                sceneViewPosition.y += yOffset;

                ImGui::SetCursorPos({ ImGui::GetCursorPosX(), yOffset + offset.y });
                offset.y += yOffset;
            }
        }

        float aspect = static_cast<float>(sceneViewSize.x) / static_cast<float>(sceneViewSize.y);

        if(m_Editor->GetSettings().m_HalfRes)
            sceneViewSize *= 0.5f;

        sceneViewSize.x -= static_cast<int>(sceneViewSize.x) % 2 != 0 ? 1.0f : 0.0f;
        sceneViewSize.y -= static_cast<int>(sceneViewSize.y) % 2 != 0 ? 1.0f : 0.0f;

        sceneViewSize.x = Maths::Max(sceneViewSize.x, 2);
        sceneViewSize.y = Maths::Max(sceneViewSize.y, 2);

        Resize(static_cast<uint32_t>(sceneViewSize.x), static_cast<uint32_t>(sceneViewSize.y));

        if(m_Editor->GetSettings().m_HalfRes)
            sceneViewSize *= 2.0f;

        // Moved this exit down to prevent a crash
        if(!camera)
        {
            const char* missingCameraText = "No Active Camera In Scene";
            ImGui::SetCursorPos((sceneViewSize - ImGui::CalcTextSize(missingCameraText)) / 2.0f);
            ImGui::TextUnformatted(missingCameraText);
            ImGui::End();
            return;
        }

        if(!Maths::Equals(aspect, camera->GetAspectRatio()))
            camera->SetAspectRatio(aspect);

        ImGuiUtility::Image(m_GameViewTexture.get(), glm::vec2(sceneViewSize.x, sceneViewSize.y));

        auto windowSize = ImGui::GetWindowSize();
        ImVec2 minBound = sceneViewPosition;

        ImVec2 maxBound   = { minBound.x + windowSize.x, minBound.y + windowSize.y };
        bool updateCamera = ImGui::IsMouseHoveringRect(minBound, maxBound) || gEngine->GetInput()->GetMouseMode() == MouseMode::Captured;

        gEngine->SetSceneActive(ImGui::IsWindowFocused() && !ImGuizmo::IsUsing() && updateCamera);

        if(m_ShowStats) //&& ImGui::IsWindowFocused())
        {
            static bool p_open   = true;
            const float DISTANCE = 5.0f;
            static int corner    = 0;

            if(corner != -1)
            {
                ImVec2 window_pos       = ImVec2((corner & 1) ? (sceneViewPosition.x + sceneViewSize.x - DISTANCE) : (sceneViewPosition.x + DISTANCE), (corner & 2) ? (sceneViewPosition.y + sceneViewSize.y - DISTANCE) : (sceneViewPosition.y + DISTANCE));
                ImVec2 window_pos_pivot = ImVec2((corner & 1) ? 1.0f : 0.0f, (corner & 2) ? 1.0f : 0.0f);
                ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
            }

            ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
            if(ImGui::Begin("Example: Simple overlay", &p_open, (corner != -1 ? ImGuiWindowFlags_NoMove : 0) | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
            {
                ImGuiIO& io = ImGui::GetIO();

                static Stats stats                           = gEngine->Statistics();
                static SceneRendererStats SceneRendererStats = m_SceneRenderer->GetSceneRendererStats();

                static float timer = 1.0f;
                timer += io.DeltaTime;

                if(timer > 1.0f)
                {
                    timer             = 0.0f;
                    stats             = gEngine->Statistics();
                    SceneRendererStats = m_SceneRenderer->GetSceneRendererStats();
                }
                gEngine->ResetStats();

                ImGui::Text("%.2f ms (%i FPS)", stats.FrameTime, stats.FramesPerSecond);
                ImGui::Separator();

                if(ImGui::IsMousePosValid())
                    ImGui::Text("Mouse Position: (%.1f,%.1f)", io.MousePos.x, io.MousePos.y);
                else
                    ImGui::TextUnformatted("Mouse Position: <invalid>");

                ImGui::Text("Num Rendered Objects %u", SceneRendererStats.NumRenderedObjects);
                ImGui::Text("Num Shadow Objects %u", SceneRendererStats.NumShadowObjects);
                ImGui::Text("Num Draw Calls  %u", SceneRendererStats.NumDrawCalls);
                ImGui::Text("Used GPU Memory : %.1f mb | Total : %.1f mb", stats.UsedGPUMemory * 0.000001f, stats.TotalGPUMemory * 0.000001f);

                if(ImGui::BeginPopupContextWindow())
                {
                    if(ImGui::MenuItem("Custom", NULL, corner == -1))
                        corner = -1;
                    if(ImGui::MenuItem("Top-left", NULL, corner == 0))
                        corner = 0;
                    if(ImGui::MenuItem("Top-right", NULL, corner == 1))
                        corner = 1;
                    if(ImGui::MenuItem("Bottom-left", NULL, corner == 2))
                        corner = 2;
                    if(ImGui::MenuItem("Bottom-right", NULL, corner == 3))
                        corner = 3;
                    if(p_open && ImGui::MenuItem("Close"))
                        p_open = false;
                    ImGui::EndPopup();
                }
            }
            ImGui::End();
        }

        ImGui::End();
    }

    void GameViewPanel::OnNewLevel(Level* level)
    {

        m_CurrentLevel = level;

        // m_SceneRenderer
        m_SceneRenderer->OnNewLevel(level);
        m_SceneRenderer->SetRenderTarget(m_GameViewTexture.get(), true);
        m_SceneRenderer->SetOverrideCamera(nullptr, nullptr);
    }

    void GameViewPanel::Resize(uint32_t width, uint32_t height)
    {

        bool resize = false;

        ASSERT(width <= 0 && height <= 0, "Game View Dimensions 0");

        if(m_Width != width || m_Height != height)
        {
            resize   = true;
            m_Width  = width;
            m_Height = height;
        }

        if(!m_GameViewTexture)
        {
            TextureDesc mainRenderTargetDesc;
            mainRenderTargetDesc.format = RHIFormat::R8G8B8A8_Unorm;
            mainRenderTargetDesc.flags  = TextureFlags::Texture_RenderTarget;

            m_GameViewTexture = SharedPtr<Texture2D>(GET_RHI_FACTORY()->CreateTexture2D(mainRenderTargetDesc, m_Width, m_Height));
        }

        if(resize)
        {
            m_GameViewTexture->Resize(m_Width, m_Height);
            m_SceneRenderer->SetRenderTarget(m_GameViewTexture.get(), true, false);
            m_SceneRenderer->OnResize(width, height);
        }
    }

    void GameViewPanel::OnRender()
    {
        if(m_GameViewVisible && m_Active)
        {
            m_SceneRenderer->BeginScene(m_CurrentLevel);
            m_SceneRenderer->OnRender();
        }
    }

    void GameViewPanel::ToolBar()
    {

        ImGui::Indent();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        bool selected = false;

        {
            selected = m_ShowStats;
            if(selected)
                ImGui::PushStyleColor(ImGuiCol_Text, ImGuiUtility::GetSelectedColour());

            ImGui::SameLine();
            if(ImGui::Button("Stats"))
            {
                m_ShowStats = !m_ShowStats;
            }

            if(selected)
                ImGui::PopStyleColor();
            ImGuiUtility::Tooltip("Show Statistics");
        }

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        static std::string supportedAspects[] = { "Free Aspect", "16:10", "16:9", "4:3", "3:2", "9:16" };

        if(ImGui::Button("Aspect " ICON_MDI_CHEVRON_DOWN))
        ImGui::OpenPopup("AspectPopup");
        if(ImGui::BeginPopup("AspectPopup"))
        {
            std::string currentAspect = m_Editor->GetSettings().m_FreeAspect ? "Free Aspect" : AspectToString(m_Editor->GetSettings().m_FixedAspect);

            for(int n = 0; n < 6; n++)
            {
                bool is_selected = (currentAspect == supportedAspects[n]);
                if(ImGui::Checkbox(supportedAspects[n].c_str(), &is_selected))
                {
                    if(supportedAspects[n] == "Free Aspect")
                    {
                        m_Editor->GetSettings().m_FreeAspect = is_selected;
                    }
                    else
                    {
                        m_Editor->GetSettings().m_FreeAspect  = false;
                        m_Editor->GetSettings().m_FixedAspect = StringToAspect(supportedAspects[n]);
                    }
                }
            }
            ImGui::EndPopup();
        }

        ImGui::SameLine();

        {
            selected = m_Editor->GetSettings().m_HalfRes;
            if(selected)
                ImGui::PushStyleColor(ImGuiCol_Text, ImGuiUtility::GetSelectedColour());

            if(ImGui::Button("Half Res"))
                m_Editor->GetSettings().m_HalfRes = !m_Editor->GetSettings().m_HalfRes;

            if(ImGui::IsItemHovered())
                ImGui::SetTooltip("Scene view with halved resolution");

            if(selected)
                ImGui::PopStyleColor();
        }

        ImGui::SameLine();

        {
            selected = m_Editor->FullScreenOnPlay();
            if(m_Editor->FullScreenOnPlay())
                ImGui::PushStyleColor(ImGuiCol_Text, ImGuiUtility::GetSelectedColour());

            if(ImGui::Button("Maximise"))
                m_Editor->FullScreenOnPlay() = !m_Editor->FullScreenOnPlay();

            if(ImGui::IsItemHovered())
                ImGui::SetTooltip("Maximise on play");

            if(selected)
                ImGui::PopStyleColor();
        }

        ImGui::PopStyleColor();
        ImGui::Unindent();
    }
} // NekoEngine