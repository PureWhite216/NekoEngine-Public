#include "GUI/ImGuiUtility.h"
#include "SceneViewPanel.h"
#include "Editor.h"
#include "imgui/Plugins/ImGuizmo.h"
#include "PhysicsEngine.h"
#include "ImGui/IconsMaterialDesignIcons.h"
#include "Renderer/GridRenderer.h"
#include "KeyCode.h"
namespace NekoEngine
{
    SceneViewPanel::SceneViewPanel()
    {
//        m_Name         = ICON_MDI_GAMEPAD_VARIANT " Scene###scene";
        m_Name = "Scene";
        m_SimpleName = "Scene";
        m_CurrentLevel = nullptr;

        m_ShowComponentGizmoMap[typeid(Light).hash_code()] = true;
        m_ShowComponentGizmoMap[typeid(Camera).hash_code()] = true;
//        m_ShowComponentGizmoMap[typeid(SoundComponent).hash_code()]  = true;

        m_Width = 1280;
        m_Height = 800;

        gEngine->GetSceneRenderer()->SetDisablePostProcess(false);
    }

    void SceneViewPanel::OnImGui()
    {

        ImGuiUtility::ScopedStyle windowPadding(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        auto flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        if(!ImGui::Begin(m_Name.c_str(), &m_Active, flags) || !m_CurrentLevel)
        {
            gEngine->SetDisableMainSceneRenderer(true);
            ImGui::End();
            return;
        }

        Camera* camera = nullptr;
        Transform* transform = nullptr;

        gEngine->SetDisableMainSceneRenderer(false);

        // if(gEngine->GetEditorState() == EditorState::Preview)
        {
            camera = m_Editor->GetCamera();
            transform = &m_Editor->GetEditorCameraTransform();

            gEngine->GetSceneRenderer()->SetOverrideCamera(camera, transform);
        }

        ImVec2 offset = {0.0f, 0.0f};

        {
            ToolBar();
            offset = ImGui::GetCursorPos(); // Usually ImVec2(0.0f, 50.0f);
        }

        if(!camera)
        {
            ImGui::End();
            return;
        }

        ImGuizmo::SetDrawlist();
        auto sceneViewSize = ImGui::GetWindowContentRegionMax() - ImGui::GetWindowContentRegionMin() -
                             offset * 0.5f; // - offset * 0.5f;
        auto sceneViewPosition = ImGui::GetWindowPos() + offset;

        sceneViewSize.x -= static_cast<int>(sceneViewSize.x) % 2 != 0 ? 1.0f : 0.0f;
        sceneViewSize.y -= static_cast<int>(sceneViewSize.y) % 2 != 0 ? 1.0f : 0.0f;

        float aspect = static_cast<float>(sceneViewSize.x) / static_cast<float>(sceneViewSize.y);

        if(!Maths::Equals(aspect, camera->GetAspectRatio()))
        {
            camera->SetAspectRatio(aspect);
        }
        m_Editor->m_SceneViewPanelPosition = glm::vec2(sceneViewPosition.x, sceneViewPosition.y);

        if(m_Editor->GetSettings().m_HalfRes)
            sceneViewSize *= 0.5f;

        sceneViewSize.x -= static_cast<int>(sceneViewSize.x) % 2 != 0 ? 1.0f : 0.0f;
        sceneViewSize.y -= static_cast<int>(sceneViewSize.y) % 2 != 0 ? 1.0f : 0.0f;

        Resize(static_cast<uint32_t>(sceneViewSize.x), static_cast<uint32_t>(sceneViewSize.y));

        if(m_Editor->GetSettings().m_HalfRes)
            sceneViewSize *= 2.0f;

        ImGuiUtility::Image(m_GameViewTexture.get(), glm::vec2(sceneViewSize.x, sceneViewSize.y));

        auto windowSize = ImGui::GetWindowSize();
        ImVec2 minBound = sceneViewPosition;

        ImVec2 maxBound = {minBound.x + windowSize.x, minBound.y + windowSize.y};
        bool updateCamera = ImGui::IsMouseHoveringRect(minBound,
                                                       maxBound); // || gEngine->GetInput()GetMouseMode() == MouseMode::Captured;

        gEngine->SetSceneActive(ImGui::IsWindowFocused() && !ImGuizmo::IsUsing() && updateCamera);

        ImGuizmo::SetRect(sceneViewPosition.x, sceneViewPosition.y, sceneViewSize.x, sceneViewSize.y);

        m_Editor->SetSceneViewActive(updateCamera);
        {
            ImGui::GetWindowDrawList()->PushClipRect(sceneViewPosition, {sceneViewSize.x + sceneViewPosition.x,
                                                                         sceneViewSize.y + sceneViewPosition.y - 2.0f});
        }

        if(m_Editor->ShowGrid())
        {
            if(camera->IsOrthographic())
            {
                m_Editor->Draw2DGrid(ImGui::GetWindowDrawList(),
                                     {transform->GetWorldPosition().x, transform->GetWorldPosition().y},
                                     sceneViewPosition, {sceneViewSize.x, sceneViewSize.y}, camera->GetScale(), 1.5f);
            }
        }

        m_Editor->OnImGuizmo();

        if(updateCamera && gEngine->GetSceneActive() && !ImGuizmo::IsUsing() &&
           gEngine->GetInput()->GetMouseClicked(MouseKeyCode::ButtonLeft))
        {

            float dpi = gEngine->GetWindowDPI();
            auto clickPos = gEngine->GetInput()->GetMousePosition() -
                            glm::vec2(sceneViewPosition.x / dpi, sceneViewPosition.y / dpi);

            Ray ray = m_Editor->GetScreenRay(int(clickPos.x), int(clickPos.y), camera, int(sceneViewSize.x / dpi),
                                             int(sceneViewSize.y / dpi));
            m_Editor->SelectObject(ray);
        }

        const ImGuiPayload* payload = ImGui::GetDragDropPayload();

        if(ImGui::BeginDragDropTarget())
        {
            auto data = ImGui::AcceptDragDropPayload("AssetFile", ImGuiDragDropFlags_AcceptNoDrawDefaultRect);
            if(data)
            {
                std::string file = (char*) data->Data;
                m_Editor->FileOpenCallback(file);
            }
            ImGui::EndDragDropTarget();
        }

        if(gEngine->GetLevelManager()->GetCurrentLevel())
            DrawGizmos(sceneViewSize.x, sceneViewSize.y, offset.x, offset.y,
                       gEngine->GetLevelManager()->GetCurrentLevel());

        ImGui::GetWindowDrawList()->PopClipRect();
        ImGui::End();
    }

    void SceneViewPanel::DrawGizmos(float width, float height, float xpos, float ypos, Level* level)
    {
        Camera* camera = m_Editor->GetCamera();
        Transform &cameraTransform = m_Editor->GetEditorCameraTransform();
        auto &registry = level->GetRegistry();
        glm::mat4 view = glm::inverse(cameraTransform.GetWorldMatrix());
        glm::mat4 proj = camera->GetProjectionMatrix();
        glm::mat4 viewProj = proj * view;
        const Frustum &f = camera->GetFrustum(view);

        ShowComponentGizmo<Light>(width, height, xpos, ypos, viewProj, f, registry);
        ShowComponentGizmo<Camera>(width, height, xpos, ypos, viewProj, f, registry);
    }

    void SceneViewPanel::ToolBar()
    {
        ImGui::Indent();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        bool selected = false;

        {
            selected = m_Editor->GetImGuizmoOperation() == 4;
            if(selected)
                ImGui::PushStyleColor(ImGuiCol_Text, ImGuiUtility::GetSelectedColour());
            ImGui::SameLine();

            if(ImGui::Button(ICON_MDI_CURSOR_DEFAULT))
                m_Editor->SetImGuizmoOperation(4);

            if(selected)
                ImGui::PopStyleColor();
            ImGuiUtility::Tooltip("Select");
        }

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        {
            selected = m_Editor->GetImGuizmoOperation() == ImGuizmo::TRANSLATE;
            if(selected)
                ImGui::PushStyleColor(ImGuiCol_Text, ImGuiUtility::GetSelectedColour());
            ImGui::SameLine();
            if(ImGui::Button(ICON_MDI_ARROW_ALL))
                m_Editor->SetImGuizmoOperation(ImGuizmo::TRANSLATE);

            if(selected)
                ImGui::PopStyleColor();
            ImGuiUtility::Tooltip("Translate");
        }

        {
            selected = m_Editor->GetImGuizmoOperation() == ImGuizmo::ROTATE;
            if(selected)
                ImGui::PushStyleColor(ImGuiCol_Text, ImGuiUtility::GetSelectedColour());

            ImGui::SameLine();
            if(ImGui::Button(ICON_MDI_ROTATE_ORBIT))
                m_Editor->SetImGuizmoOperation(ImGuizmo::ROTATE);

            if(selected)
                ImGui::PopStyleColor();
            ImGuiUtility::Tooltip("Rotate");
        }

        {
            selected = m_Editor->GetImGuizmoOperation() == ImGuizmo::SCALE;
            if(selected)
                ImGui::PushStyleColor(ImGuiCol_Text, ImGuiUtility::GetSelectedColour());

            ImGui::SameLine();
            if(ImGui::Button(ICON_MDI_ARROW_EXPAND_ALL))
                m_Editor->SetImGuizmoOperation(ImGuizmo::SCALE);

            if(selected)
                ImGui::PopStyleColor();
            ImGuiUtility::Tooltip("Scale");
        }

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        {
            selected = m_Editor->GetImGuizmoOperation() == ImGuizmo::UNIVERSAL;
            if(selected)
                ImGui::PushStyleColor(ImGuiCol_Text, ImGuiUtility::GetSelectedColour());

            ImGui::SameLine();
            if(ImGui::Button(ICON_MDI_CROP_ROTATE))
                m_Editor->SetImGuizmoOperation(ImGuizmo::UNIVERSAL);

            if(selected)
                ImGui::PopStyleColor();
            ImGuiUtility::Tooltip("Universal");
        }

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        {
            selected = m_Editor->GetImGuizmoOperation() == ImGuizmo::BOUNDS;
            if(selected)
                ImGui::PushStyleColor(ImGuiCol_Text, ImGuiUtility::GetSelectedColour());

            ImGui::SameLine();
            if(ImGui::Button(ICON_MDI_BORDER_NONE))
                m_Editor->SetImGuizmoOperation(ImGuizmo::BOUNDS);

            if(selected)
                ImGui::PopStyleColor();
            ImGuiUtility::Tooltip("Bounds");
        }

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        ImGui::SameLine();
        {
            selected = (m_Editor->SnapGuizmo() == true);

            if(selected)
                ImGui::PushStyleColor(ImGuiCol_Text, ImGuiUtility::GetSelectedColour());

            if(ImGui::Button(ICON_MDI_MAGNET))
                m_Editor->SnapGuizmo() = !selected;

            if(selected)
                ImGui::PopStyleColor();
            ImGuiUtility::Tooltip("Snap");
        }

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();

        if(ImGui::Button("Gizmos " ICON_MDI_CHEVRON_DOWN))
        ImGui::OpenPopup("GizmosPopup");
        if(ImGui::BeginPopup("GizmosPopup"))
        {
            {
                ImGui::Checkbox("Grid", &m_Editor->ShowGrid());
                ImGui::Checkbox("Selected Gizmos", &m_Editor->ShowGizmos());
                ImGui::Checkbox("View Selected", &m_Editor->ShowViewSelected());

                ImGui::Separator();
                ImGui::Checkbox("Camera", &m_ShowComponentGizmoMap[typeid(Camera).hash_code()]);
                ImGui::Checkbox("Light", &m_ShowComponentGizmoMap[typeid(Light).hash_code()]);

                ImGui::Separator();

                uint32_t flags = m_Editor->GetSettings().m_DebugDrawFlags;
                bool showEntityNames = flags & EditorDebugFlags::EntityNames;
                if(ImGui::Checkbox("Entity Names", &showEntityNames))
                {
                    if(showEntityNames)
                        flags += EditorDebugFlags::EntityNames;
                    else
                        flags -= EditorDebugFlags::EntityNames;
                }

                bool showAABB = flags & EditorDebugFlags::MeshBoundingBoxes;
                if(ImGui::Checkbox("Mesh AABB", &showAABB))
                {
                    if(showAABB)
                        flags += EditorDebugFlags::MeshBoundingBoxes;
                    else
                        flags -= EditorDebugFlags::MeshBoundingBoxes;
                }

                bool showSpriteBox = flags & EditorDebugFlags::SpriteBoxes;
                if(ImGui::Checkbox("Sprite Box", &showSpriteBox))
                {
                    if(showSpriteBox)
                        flags += EditorDebugFlags::SpriteBoxes;
                    else
                        flags -= EditorDebugFlags::SpriteBoxes;
                }

                bool showCameraFrustums = flags & EditorDebugFlags::CameraFrustum;
                if(ImGui::Checkbox("Camera Frustums", &showCameraFrustums))
                {
                    if(showCameraFrustums)
                        flags += EditorDebugFlags::CameraFrustum;
                    else
                        flags -= EditorDebugFlags::CameraFrustum;
                }

                m_Editor->GetSettings().m_DebugDrawFlags = flags;
                ImGui::Separator();

//                auto physics2D = gEngine->GetSystem<B2PhysicsEngine>();

//                if(physics2D)
//                {
//                    uint32_t flags = physics2D->GetDebugDrawFlags();
//
//                    bool show2DShapes = flags & b2Draw::e_shapeBit;
//                    if(ImGui::Checkbox("Shapes (2D)", &show2DShapes))
//                    {
//                        if(show2DShapes)
//                            flags += b2Draw::e_shapeBit;
//                        else
//                            flags -= b2Draw::e_shapeBit;
//                    }
//
//                    bool showCOG = flags & b2Draw::e_centerOfMassBit;
//                    if(ImGui::Checkbox("Centre of Mass (2D)", &showCOG))
//                    {
//                        if(showCOG)
//                            flags += b2Draw::e_centerOfMassBit;
//                        else
//                            flags -= b2Draw::e_centerOfMassBit;
//                    }
//
//                    bool showJoint = flags & b2Draw::e_jointBit;
//                    if(ImGui::Checkbox("Joint Connection (2D)", &showJoint))
//                    {
//                        if(showJoint)
//                            flags += b2Draw::e_jointBit;
//                        else
//                            flags -= b2Draw::e_jointBit;
//                    }
//
//                    bool showAABB = flags & b2Draw::e_aabbBit;
//                    if(ImGui::Checkbox("AABB (2D)", &showAABB))
//                    {
//                        if(showAABB)
//                            flags += b2Draw::e_aabbBit;
//                        else
//                            flags -= b2Draw::e_aabbBit;
//                    }
//
//                    bool showPairs = static_cast<bool>(flags & b2Draw::e_pairBit);
//                    if(ImGui::Checkbox("Broadphase Pairs  (2D)", &showPairs))
//                    {
//                        if(showPairs)
//                            flags += b2Draw::e_pairBit;
//                        else
//                            flags -= b2Draw::e_pairBit;
//                    }
//
//                    physics2D->SetDebugDrawFlags(flags);
//                }

                auto physics3D = gEngine->GetSystem<PhysicsEngine>();

                if(physics3D)
                {
                    uint32_t flags = physics3D->GetDebugDrawFlags();

                    bool showCollisionShapes = flags & PhysicsDebugFlags::COLLISIONVOLUMES;
                    if(ImGui::Checkbox("Collision Volumes", &showCollisionShapes))
                    {
                        if(showCollisionShapes)
                            flags += PhysicsDebugFlags::COLLISIONVOLUMES;
                        else
                            flags -= PhysicsDebugFlags::COLLISIONVOLUMES;
                    }

                    bool showConstraints = static_cast<bool>(flags & PhysicsDebugFlags::CONSTRAINT);
                    if(ImGui::Checkbox("Constraints", &showConstraints))
                    {
                        if(showConstraints)
                            flags += PhysicsDebugFlags::CONSTRAINT;
                        else
                            flags -= PhysicsDebugFlags::CONSTRAINT;
                    }

                    bool showManifolds = static_cast<bool>(flags & PhysicsDebugFlags::MANIFOLD);
                    if(ImGui::Checkbox("Manifolds", &showManifolds))
                    {
                        if(showManifolds)
                            flags += PhysicsDebugFlags::MANIFOLD;
                        else
                            flags -= PhysicsDebugFlags::MANIFOLD;
                    }

                    bool showCollisionNormals = flags & PhysicsDebugFlags::COLLISIONNORMALS;
                    if(ImGui::Checkbox("Collision Normals", &showCollisionNormals))
                    {
                        if(showCollisionNormals)
                            flags += PhysicsDebugFlags::COLLISIONNORMALS;
                        else
                            flags -= PhysicsDebugFlags::COLLISIONNORMALS;
                    }

                    bool showAABB = flags & PhysicsDebugFlags::AABB;
                    if(ImGui::Checkbox("AABB", &showAABB))
                    {
                        if(showAABB)
                            flags += PhysicsDebugFlags::AABB;
                        else
                            flags -= PhysicsDebugFlags::AABB;
                    }

                    bool showLinearVelocity = flags & PhysicsDebugFlags::LINEARVELOCITY;
                    if(ImGui::Checkbox("Linear Velocity", &showLinearVelocity))
                    {
                        if(showLinearVelocity)
                            flags += PhysicsDebugFlags::LINEARVELOCITY;
                        else
                            flags -= PhysicsDebugFlags::LINEARVELOCITY;
                    }

                    bool LINEARFORCE = flags & PhysicsDebugFlags::LINEARFORCE;
                    if(ImGui::Checkbox("Linear Force", &LINEARFORCE))
                    {
                        if(LINEARFORCE)
                            flags += PhysicsDebugFlags::LINEARFORCE;
                        else
                            flags -= PhysicsDebugFlags::LINEARFORCE;
                    }

                    bool BROADPHASE = flags & PhysicsDebugFlags::BROADPHASE;
                    if(ImGui::Checkbox("Broadphase", &BROADPHASE))
                    {
                        if(BROADPHASE)
                            flags += PhysicsDebugFlags::BROADPHASE;
                        else
                            flags -= PhysicsDebugFlags::BROADPHASE;
                    }

                    bool showPairs = flags & PhysicsDebugFlags::BROADPHASE_PAIRS;
                    if(ImGui::Checkbox("Broadphase Pairs", &showPairs))
                    {
                        if(showPairs)
                            flags += PhysicsDebugFlags::BROADPHASE_PAIRS;
                        else
                            flags -= PhysicsDebugFlags::BROADPHASE_PAIRS;
                    }

                    physics3D->SetDebugDrawFlags(flags);
                }

                ImGui::EndPopup();
            }
        }

        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();
        // Editor Camera Settings

        auto &camera = *m_Editor->GetCamera();
        bool ortho = camera.IsOrthographic();

        selected = !ortho;
        if(selected)
            ImGui::PushStyleColor(ImGuiCol_Text, ImGuiUtility::GetSelectedColour());
        if(ImGui::Button(ICON_MDI_AXIS_ARROW " 3D"))
        {
            if(ortho)
            {
                camera.SetIsOrthographic(false);
                camera.SetNear(0.01f);
                m_Editor->GetEditorCameraController().SetCurrentMode(EditorCameraMode::FLYCAM);
            }
        }
        if(selected)
            ImGui::PopStyleColor();
        ImGui::SameLine();

        selected = ortho;
        if(selected)
            ImGui::PushStyleColor(ImGuiCol_Text, ImGuiUtility::GetSelectedColour());
        if(ImGui::Button(ICON_MDI_ANGLE_RIGHT "2D"))
        {
            if(!ortho)
            {
                camera.SetIsOrthographic(true);
                auto camPos = m_Editor->GetEditorCameraTransform().GetLocalPosition();
                camPos.z = 0.0f;
                camera.SetNear(-10.0f);
                m_Editor->GetEditorCameraTransform().SetLocalPosition(camPos);
                m_Editor->GetEditorCameraTransform().SetLocalOrientation(glm::quat(glm::vec3(0.0f, 0.0f, 0.0f)));
                m_Editor->GetEditorCameraTransform().SetLocalScale(glm::vec3(1.0f, 1.0f, 1.0f));

                m_Editor->GetEditorCameraController().SetCurrentMode(EditorCameraMode::TWODIM);
            }
        }
        if(selected)
            ImGui::PopStyleColor();

        ImGui::PopStyleColor();
        ImGui::Unindent();
    }

    void SceneViewPanel::OnNewLevel(Level* level)
    {

        m_Editor->GetSettings().m_AspectRatio = 1.0f;
        m_CurrentLevel = level;

        auto SceneRenderer = gEngine->GetSceneRenderer();
        SceneRenderer->SetRenderTarget(m_GameViewTexture.get(), true);
        SceneRenderer->SetOverrideCamera(m_Editor->GetCamera(), &m_Editor->GetEditorCameraTransform());
        m_Editor->GetGridRenderer()->SetRenderTarget(m_GameViewTexture.get(), true);
        m_Editor->GetGridRenderer()->SetDepthTarget(SceneRenderer->GetForwardData().m_DepthTexture);
    }

    void SceneViewPanel::Resize(uint32_t width, uint32_t height)
    {

        bool resize = false;
        gEngine->SetSceneViewDimensions(width, height);

        if(m_Width != width || m_Height != height)
        {
            resize = true;
            m_Width = width;
            m_Height = height;
        }

        if(!m_GameViewTexture)
        {
            TextureDesc mainRenderTargetDesc;
            mainRenderTargetDesc.format = RHIFormat::R8G8B8A8_Unorm;
            mainRenderTargetDesc.flags = TextureFlags::Texture_RenderTarget;

            m_GameViewTexture = SharedPtr<Texture2D>(GET_RHI_FACTORY()->CreateTexture2D(mainRenderTargetDesc, m_Width, m_Height));
        }

        if(resize)
        {
            m_GameViewTexture->Resize(m_Width, m_Height);

            auto SceneRenderer = gEngine->GetSceneRenderer();
            SceneRenderer->SetRenderTarget(m_GameViewTexture.get(), true, false);

            if(!m_Editor->GetGridRenderer())
                m_Editor->CreateGridRenderer();
            m_Editor->GetGridRenderer()->SetRenderTarget(m_GameViewTexture.get(), false);
            m_Editor->GetGridRenderer()->SetDepthTarget(SceneRenderer->GetForwardData().m_DepthTexture);

            WindowResizeEvent e(width, height);
            gEngine->GetSceneRenderer()->OnResize(width, height);

            SceneRenderer->OnEvent(e);

            m_Editor->GetGridRenderer()->OnResize(m_Width, m_Height);

            // Should be just build texture and scene renderer set render target
            // Renderer::GetGraphicsContext()->WaitIdle();
        }
    }
} // NekoEngine