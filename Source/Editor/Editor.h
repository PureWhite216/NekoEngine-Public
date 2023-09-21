#pragma once
#include "GUI/ImGuiUtility.h"
#include "entt/entt.hpp"
#include "Panels/EditorPanel.h"
#include "Engine.h"
#include "Renderable/Camera.h"
#include "Transform.h"
#include "Renderable/EditorCamera.h"
#include "Panels/FileBrowserPanel.h"
#include "Utility/IniFile.h"

#define BIND_FILEBROWSER_FN(fn) [this](auto&&... args) -> decltype(auto) { \
    return this->fn(std::forward<decltype(args)>(args)...);                \
}

namespace NekoEngine
{
    class Level;
    class Event;
    class WindowCloseEvent;
    class WindowResizeEvent;
    class WindowFileEvent;
    class TimeStep;

    class Texture2D;
    class GridRenderer;
    class Mesh;

    enum EditorDebugFlags : uint32_t
    {
        Grid              = 1,
        Gizmo             = 2,
        ViewSelected      = 4,
        CameraFrustum     = 8,
        MeshBoundingBoxes = 16,
        SpriteBoxes       = 32,
        EntityNames       = 64,

    };


    struct EditorSettings
    {
        float m_GridSize               = 10.0f;
        uint32_t m_DebugDrawFlags      = 0;
        uint32_t m_Physics2DDebugFlags = 0;
        uint32_t m_Physics3DDebugFlags = 0;

        bool m_ShowGrid         = true;
        bool m_ShowGizmos       = true;
        bool m_ShowViewSelected = false;
        bool m_SnapQuizmo       = false;
        bool m_ShowImGuiDemo    = true;
        bool m_View2D           = false;
        bool m_FullScreenOnPlay = false;
        float m_SnapAmount      = 1.0f;
        bool m_SleepOutofFocus  = true;
        float m_ImGuizmoScale   = 0.25f;

        bool m_FullScreenSceneView    = false;
        ImGuiUtility::Theme m_Theme = ImGuiUtility::Theme::Black;
        bool m_FreeAspect             = true;
        float m_FixedAspect           = 1.0f;
        bool m_HalfRes                = false;
        float m_AspectRatio           = 1.0f;

        // Camera Settings
        float m_CameraSpeed = 1000.0f;
        float m_CameraNear  = 0.01f;
        float m_CameraFar   = 1000.0f;

        std::vector<std::string> m_RecentProjects;
    };

    class Editor : public Engine
    {
        friend class SceneViewPanel;
    protected:

        Engine* m_Application = nullptr;

        uint32_t m_ImGuizmoOperation = 14463;
        std::vector<entt::entity> m_SelectedEntities;
        std::vector<entt::entity> m_CopiedEntities;

        bool m_CutCopyEntity              = false;
        float m_CurrentSceneAspectRatio   = 0.0f;
        float m_CameraTransitionStartTime = 0.0f;
        float m_CameraTransitionSpeed     = 0.0f;
        bool m_TransitioningCamera        = false;
        glm::vec3 m_CameraDestination;
        glm::vec3 m_CameraStartPosition;
        bool m_SceneViewActive     = false;
        bool m_NewProjectPopupOpen = false;

        EditorSettings m_Settings;
        std::vector<SharedPtr<EditorPanel>> m_Panels;

        std::unordered_map<size_t, const char*> m_ComponentIconMap;

        FileBrowserPanel m_FileBrowserPanel;
        Camera* m_CurrentCamera = nullptr;
        EditorCameraController m_EditorCameraController;
        Transform m_EditorCameraTransform;

        SharedPtr<Camera> m_EditorCamera = nullptr;
        // SharedPtr<ForwardRenderer> m_PreviewRenderer;
        SharedPtr<Texture2D> m_PreviewTexture;
        SharedPtr<Mesh> m_PreviewSphere;
        SharedPtr<GridRenderer> m_GridRenderer;
        std::string m_TempSceneSaveFilePath;
        int m_AutoSaveSettingsTime = 15000;

        IniFile m_IniFile;

        static Editor* s_Editor;
    protected:
        Editor(const Editor&) = delete;
        Editor& operator=(const Editor&) = delete;

        bool OnWindowResize(WindowResizeEvent& e);

    public:
        Editor();
        virtual ~Editor();

        void Init() override;
        void OnImGui() override;
        void OnRender() override;
        void OnDebugDraw() override;
        void OnEvent(Event& e) override;
        void OnQuit() override;

        void DrawMenuBar();
        void BeginDockSpace(bool gameFullScreen);
        void EndDockSpace();

        bool IsTextFile(const std::string& filePath);
        bool IsAudioFile(const std::string& filePath);
        bool IsSceneFile(const std::string& filePath);
        bool IsModelFile(const std::string& filePath);
        bool IsTextureFile(const std::string& filePath);
        bool IsShaderFile(const std::string& filePath);
        bool IsFontFile(const std::string& filePath);

        void SetImGuizmoOperation(uint32_t operation)
        {
            m_ImGuizmoOperation = operation;
        }
        uint32_t GetImGuizmoOperation() const
        {
            return m_ImGuizmoOperation;
        }

        void OnNewLevel(Level* level) override;
        void OnImGuizmo();
        void OnUpdate(const TimeStep& ts) override;

        void Draw2DGrid(ImDrawList* drawList, const ImVec2& cameraPos, const ImVec2& windowPos, const ImVec2& canvasSize, const float factor, const float thickness);
        void Draw3DGrid();

        bool& ShowGrid()
        {
            return m_Settings.m_ShowGrid;
        }
        const float& GetGridSize() const
        {
            return m_Settings.m_GridSize;
        }

        bool& ShowGizmos()
        {
            return m_Settings.m_ShowGizmos;
        }
        bool& ShowViewSelected()
        {
            return m_Settings.m_ShowViewSelected;
        }

        void ToggleSnap()
        {
            m_Settings.m_SnapQuizmo = !m_Settings.m_SnapQuizmo;
        }

        bool& FullScreenOnPlay()
        {
            return m_Settings.m_FullScreenOnPlay;
        }

        bool& SnapGuizmo()
        {
            return m_Settings.m_SnapQuizmo;
        }
        float& SnapAmount()
        {
            return m_Settings.m_SnapAmount;
        }

        void ClearSelected()
        {
            m_SelectedEntities.clear();
        }

        void SetSelected(entt::entity entity);
        void UnSelect(entt::entity entity);

        const std::vector<entt::entity>& GetSelected() const
        {
            return m_SelectedEntities;
        }

        bool IsSelected(entt::entity entity)
        {
            if(std::find(m_SelectedEntities.begin(), m_SelectedEntities.end(), entity) != m_SelectedEntities.end())
                return true;

            return false;
        }

        bool IsCopied(entt::entity entity)
        {
            if(std::find(m_CopiedEntities.begin(), m_CopiedEntities.end(), entity) != m_CopiedEntities.end())
                return true;

            return false;
        }

        void SetCopiedEntity(entt::entity entity, bool cut = false)
        {
            if(std::find(m_CopiedEntities.begin(), m_CopiedEntities.end(), entity) != m_CopiedEntities.end())
                return;

            m_CopiedEntities.push_back(entity);
            m_CutCopyEntity = cut;
        }

        const std::vector<entt::entity>& GetCopiedEntity() const
        {
            return m_CopiedEntities;
        }

        bool GetCutCopyEntity()
        {
            return m_CutCopyEntity;
        }

        std::unordered_map<size_t, const char*>& GetComponentIconMap()
        {
            return m_ComponentIconMap;
        }

        void FocusCamera(const glm::vec3& point, float distance, float speed = 1.0f);

        void RecompileShaders();
        void DebugDraw();
        void SelectObject(const Ray& ray);

        void OpenTextFile(const std::string& filePath, const std::function<void()>& callback);
        void RemovePanel(EditorPanel* panel);
        EditorPanel* GetTextEditPanel();

        void ShowPreview();
        void DrawPreview();

        static Editor* GetEditor() { return s_Editor; }

        glm::vec2 m_SceneViewPanelPosition;
        Ray GetScreenRay(int x, int y, Camera* camera, int width, int height);

        void FileOpenCallback(const std::string& filepath);
        void ProjectOpenCallback(const std::string& filepath);
        void NewProjectOpenCallback(const std::string& filepath);
        void FileEmbedCallback(const std::string& filepath);
        void NewProjectLocationCallback(const std::string& filepath);

        FileBrowserPanel& GetFileBrowserPanel()
        {
            return m_FileBrowserPanel;
        }

        void AddDefaultEditorSettings();
        void SaveEditorSettings();
        void LoadEditorSettings();

        void OpenFile();
        void EmbedFile();
        const char* GetIconFontIcon(const std::string& fileType);

        Camera* GetCamera() const
        {
            return m_EditorCamera.get();
        }

        void CreateGridRenderer();
        const SharedPtr<GridRenderer>& GetGridRenderer();

        EditorCameraController& GetEditorCameraController()
        {
            return m_EditorCameraController;
        }

        Transform& GetEditorCameraTransform()
        {
            return m_EditorCameraTransform;
        }

        void CacheScene();
        void LoadCachedScene();
        bool OnFileDrop(WindowFileEvent& e);

        EditorSettings& GetSettings() { return m_Settings; }
        void SetSceneViewActive(bool active) { m_SceneViewActive = active; }
    };

} // NekoEngine

