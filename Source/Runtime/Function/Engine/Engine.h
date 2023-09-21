#pragma once

#include <mutex>
#include <thread>
#include "Core.h"
#include "Window/Window.h"
#include "RHI/RHIFactory.h"
#include "FrameWork/Level/LevelManager.h"
#include "Timer/Timer.h"
#include "GUI/ImGuiManager.h"
#include "Input/Input.h"
#include "RHI/SwapChain.h"
#include "Renderer/SceneRenderer.h"
#include "Timer/TimeStep.h"
#include "System/SystemManager.h"
#include "Script/LuaManager.h"
#include "Asset/AssetManager.h"
//#include "RHI/Renderer.h"

#define GET_RHI_FACTORY() gEngine->GetRenderer()->GetRHIFactory()
#define GET_SHADER_LIB() gEngine->GetRenderer()->GetShaderLibrary()
#define GET_SWAP_CHAIN() gEngine->GetSwapChain()
#define GET_INPUT() gEngine->GetInput()
#define GET_SCENE_RENDERER() gEngine->GetSceneRenderer()

namespace NekoEngine
{
    struct ProjectSettings
    {
        std::string m_ProjectRoot;
        std::string m_ProjectName;
        std::string m_EngineAssetPath;
        uint32_t Width = 1200, Height = 800;
        bool Fullscreen  = true;
        bool VSync       = true;
        bool Borderless  = false;
        bool ShowConsole = true;
        std::string Title;
        int RenderAPI;
        int ProjectVersion = 8;
        int8_t DesiredGPUIndex = -1;
        std::string IconPath;
        bool DefaultIcon  = true;
        bool HideTitleBar = false;
    };

    enum class AppState
    {
        Running,
        Loading,
        Closing
    };

    enum class EditorState
    {
        Paused,
        Play,
        Next,
        Preview
    };

    enum class AppType
    {
        Game,
        Editor
    };

    struct Stats
    {
        uint32_t UpdatesPerSecond;
        uint32_t FramesPerSecond;
        uint32_t NumRenderedObjects = 0;
        uint32_t NumShadowObjects   = 0;
        uint32_t NumDrawCalls       = 0;
        double FrameTime            = 0.0;
        float UsedGPUMemory         = 0.0f;
        float UsedRam               = 0.0f;
        float TotalGPUMemory        = 0.0f;
    };


    class Window;
    class Renderer;

    class Engine
    {
        friend class Editor;
    protected:
        bool isExit = false;

        UniquePtr<Renderer> renderer;
        UniquePtr<Window> window;
        UniquePtr<LevelManager> levelManager;
        UniquePtr<Timer> timer;
        UniquePtr<ImGuiManager> imGuiManager;
        UniquePtr<Input> input;
        UniquePtr<SceneRenderer> sceneRenderer;
        UniquePtr<TimeStep> timeStep;
        UniquePtr<SystemManager> systemManager;
        UniquePtr<LuaManager> luaManager;
        UniquePtr<ModelLibrary> modelLibrary;

        uint32_t frames = 0;

        std::vector<std::function<void()>> m_MainThreadQueue;
        std::mutex m_MainThreadQueueMutex;
        std::thread m_UpdateThread;

        AppState m_CurrentState   = AppState::Loading;
        EditorState m_EditorState = EditorState::Preview;
        AppType m_AppType         = AppType::Editor;

        ProjectSettings m_ProjectSettings;
        bool m_ProjectLoaded = false;

        bool m_Minimized               = false;
        bool m_SceneActive             = true;
        bool m_DisableMainSceneRenderer = false;

        uint32_t m_SceneViewWidth   = 0;
        uint32_t m_SceneViewHeight  = 0;
        bool m_SceneViewSizeUpdated = false;
        bool m_RenderDocEnabled     = false;

        std::mutex m_EventQueueMutex;
        std::queue<std::function<void()>> m_EventQueue;

        Stats m_Stats;

    public:
        Engine();
        virtual ~Engine();

        Engine(const Engine&) = delete;
        Engine& operator=(const Engine&) = delete;

        void Run();
        bool OnFrame();

        void OnExitScene();
        void OnSceneViewSizeUpdated(uint32_t width, uint32_t height);
        void OpenProject(const std::string& filePath);
        void OpenNewProject(const std::string& path, const std::string& name = "New Project");


        virtual void OnQuit();
        virtual void Init();
        virtual void OnEvent(Event& e);
        virtual void OnNewLevel(Level* level);
        virtual void OnRender();
        virtual void OnUpdate(const TimeStep& dt);
        virtual void OnImGui();
        virtual void OnDebugDraw();

        void UpdateSystems();
        void SubmitToMainThread(const std::function<void()>& function);
        void ExecuteMainThreadQueue();

        void OnExitLevel();
        bool OnWindowResize(WindowResizeEvent& e);

        void MountVFSPaths();

        void ResetStats()
        {
            m_Stats.NumRenderedObjects = 0;
            m_Stats.NumShadowObjects   = 0;
            m_Stats.FrameTime          = 0.0;
            m_Stats.UsedGPUMemory      = 0.0f;
            m_Stats.UsedRam            = 0.0f;
            m_Stats.NumDrawCalls       = 0;
            m_Stats.TotalGPUMemory     = 0.0f;
        }


        template <typename T>
        T* GetSystem()
        {
            return systemManager->GetSystem<T>();
        }

        template <typename Func>
        void QueueEvent(Func&& func)
        {
            m_EventQueue.push(func);
        }

        template <typename TEvent, bool Immediate = false, typename... TEventArgs>
        void DispatchEvent(TEventArgs&&... args)
        {
            SharedPtr<TEvent> event = MakeShared<TEvent>(std::forward<TEventArgs>(args)...);
            if(Immediate)
            {
                OnEvent(*event);
            }
            else
            {
                std::scoped_lock<std::mutex> lock(m_EventQueueMutex);
                m_EventQueue.push([event, this]()
                                  { OnEvent(*event); });
            }
        }

        void SetSceneViewDimensions(uint32_t width, uint32_t height)
        {
            if(width != m_SceneViewWidth)
            {
                m_SceneViewWidth       = width;
                m_SceneViewSizeUpdated = true;
            }

            if(height != m_SceneViewHeight)
            {
                m_SceneViewHeight      = height;
                m_SceneViewSizeUpdated = true;
            }
        }

        virtual void Serialise();
        virtual void Deserialise();


//        void TickOneFrame(float deltaTime);
//        void LogicalTick(float deltaTime);
//        void RendererTick(float deltaTime);
//        bool IsExit(){ return isExit; }

        LevelManager* GetLevelManager() const { return levelManager.get(); }
        Renderer* GetRenderer() const { return renderer.get(); }
        Window* GetWindow() const { return window.get(); }
        SwapChain* GetSwapChain() const;
        Input* GetInput() const { return input.get(); };
        SceneRenderer* GetSceneRenderer() const { return sceneRenderer.get(); }
        TimeStep* GetTimeStep() const { return timeStep.get(); }
        SystemManager* GetSystemManager() const { return systemManager.get(); }
        LuaManager* GetLuaManager() const { return luaManager.get(); }
        ImGuiManager* GetImGuiManager() const { return imGuiManager.get(); }
        ModelLibrary* GetModelLibrary() const { return modelLibrary.get(); }

        Level* GetCurrentLevel() const { return levelManager->GetCurrentLevel(); }
        EditorState GetEditorState() const { return m_EditorState; }
        glm::vec2 GetWindowSize() const {return glm::vec2(static_cast<float>(window->GetWidth()), static_cast<float>(window->GetHeight()));}
        bool GetSceneActive() const { return m_SceneActive; }
        float GetWindowDPI() const;
        Stats& Statistics() { return m_Stats; }
        ProjectSettings& GetProjectSettings() { return m_ProjectSettings; }
        RenderConfig GetRenderConfigSettings() { return renderer->GetRenderConfig(); }

        void SetAppState(AppState state) { m_CurrentState = state; }
        void SetEditorState(EditorState state) { m_EditorState = state; }
        void SetSceneActive(bool active) { m_SceneActive = active; }
        void SetDisableMainSceneRenderer(bool disable) { m_DisableMainSceneRenderer = disable; }

    public:
        template <typename Archive>
        void save(Archive& archive) const;

        template <typename Archive>
        void load(Archive& archive);

    private:
        bool OnWindowClose(WindowCloseEvent& e);
        void AddDefaultLevel();
    };

    extern SharedPtr<Engine> gEngine;
} // NekoEngine
