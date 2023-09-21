#include <filesystem>
#include "Window/GLFWWindow.h"
#include "Vulkan/VulkanRenderer.h"
#include "GUI/ImGuiManager.h"
#include "JobSystem/JobSystem.h"
#include "Input/Input.h"
#include "ImGui/Plugins/implot/implot.h"
#include "StringUtility.h"
#include "File/FileSystem.h"
#include "File/VirtualFileSystem.h"
#include "PhysicsEngine.h"
#include "OS/OS.h"
#include "Engine.h"
#include <cereal/archives/json.hpp>
#include <cereal/types/vector.hpp>
//#include "ImGui/Plugins/implot/implot_internal.h"

namespace NekoEngine
{
    SharedPtr<Engine> gEngine;

    Engine::Engine()
    {

    }

    Engine::~Engine()
    {
        ImGui::DestroyContext();
        ImPlot::DestroyContext();
    }

    void Engine::OpenProject(const std::string &filePath)
    {
        m_ProjectSettings.m_ProjectName = StringUtility::GetFileName(filePath);
        m_ProjectSettings.m_ProjectName = StringUtility::RemoveFilePathExtension(m_ProjectSettings.m_ProjectName);

        if(!FileSystem::FolderExists(m_ProjectSettings.m_ProjectRoot + "Assets/Prefabs"))
            std::filesystem::create_directory(m_ProjectSettings.m_ProjectRoot + "Assets/Prefabs");

        if(!FileSystem::FolderExists(m_ProjectSettings.m_ProjectRoot + "Assets/Materials"))
            std::filesystem::create_directory(m_ProjectSettings.m_ProjectRoot + "Assets/Materials");


        Deserialise();

        levelManager->LoadCurrentList();
        levelManager->ApplyLevelSwitch();

        luaManager->OnNewProject(m_ProjectSettings.m_ProjectRoot);
    }

    void Engine::OpenNewProject(const std::string &path, const std::string &name)
    {
        m_ProjectSettings.m_ProjectRoot = path + name + "/";
        m_ProjectSettings.m_ProjectName = name;

        std::filesystem::create_directory(m_ProjectSettings.m_ProjectRoot);

        levelManager = MakeUnique<LevelManager>();

        MountVFSPaths();
        // Set Default values
        m_ProjectSettings.RenderAPI   = 1;
        m_ProjectSettings.Width       = 1200;
        m_ProjectSettings.Height      = 800;
        m_ProjectSettings.Borderless  = false;
        m_ProjectSettings.VSync       = true;
        m_ProjectSettings.Title       = "App";
        m_ProjectSettings.ShowConsole = false;
        m_ProjectSettings.Fullscreen  = false;

        levelManager->EnqueueLevel(new Level("Empty Scene"));
        levelManager->SwitchLevel(0);

        // Set Default values
        m_ProjectSettings.Title      = "App";
        m_ProjectSettings.Fullscreen = false;

        levelManager->ApplyLevelSwitch();

        m_ProjectLoaded = true;

        Serialise();

        luaManager->OnNewProject(m_ProjectSettings.m_ProjectRoot);
    }

    void Engine::MountVFSPaths()
    {
        VirtualFileSystem::Mount("Meshes", m_ProjectSettings.m_ProjectRoot + std::string("Assets/Meshes"), true);
        VirtualFileSystem::Mount("Textures", m_ProjectSettings.m_ProjectRoot + std::string("Assets/Textures"), true);
        VirtualFileSystem::Mount("Sounds", m_ProjectSettings.m_ProjectRoot + std::string("Assets/Sounds"), true);
        VirtualFileSystem::Mount("Scripts", m_ProjectSettings.m_ProjectRoot + std::string("Assets/Scripts"), true);
        VirtualFileSystem::Mount("Levels", m_ProjectSettings.m_ProjectRoot + std::string("Assets/Levels"), true);
        VirtualFileSystem::Mount("Assets", m_ProjectSettings.m_ProjectRoot + std::string("Assets"), true);
        VirtualFileSystem::Mount("Prefabs", m_ProjectSettings.m_ProjectRoot + std::string("Assets/Prefabs"), true);
        VirtualFileSystem::Mount("Materials", m_ProjectSettings.m_ProjectRoot + std::string("Assets/Materials"), true);
    }


    void Engine::Init()
    {
        LOG("Engine Init.");

        // Init LevelManager
        levelManager = MakeUnique<LevelManager>();
        LOG("Level Manager Init.");

        Deserialise();

        // Init LuaManager
        luaManager = MakeUnique<LuaManager>();
        luaManager->OnInit();
        luaManager->OnNewProject(m_ProjectSettings.m_ProjectRoot);
        LOG("Lua Manager Init.");

        // Create Timer
        timer = MakeUnique<Timer>();
        LOG("Timer Init.");


        // Create Window
        Window::CreateInfo windowDesc;
        windowDesc.width       = m_ProjectSettings.Width;
        windowDesc.height      = m_ProjectSettings.Height;
//        windowDesc.RenderAPI   = m_ProjectSettings.RenderAPI;
        windowDesc.isFullscreen  = m_ProjectSettings.Fullscreen;
        windowDesc.isBorderless  = m_ProjectSettings.Borderless;
        windowDesc.ShowConsole = m_ProjectSettings.ShowConsole;
        windowDesc.title       = m_ProjectSettings.Title;
        windowDesc.isVSync       = m_ProjectSettings.VSync;

        //TODO: load icon
        window = MakeUnique<GLFWWindow>(windowDesc);
        Window::instance = window->instance;
        if(!window->IsInit())
        {
            RUNTIME_ERROR("Failed to init window.");
        }
        window->SetEventCallback([](Event& e)
        {
            LOG("Event: " + e.ToString());
        });

        LOG("Window Init.");

        if(m_ProjectSettings.Fullscreen) window->Maximise();

        uint32_t screenWidth  = window->GetWidth();
        uint32_t screenHeight = window->GetHeight();

        m_EditorState = EditorState::Play;

        // Create ImGUI Context
        ImGui::CreateContext();
        ImPlot::CreateContext();
        ImGui::StyleColorsDark();

        // Load Embedded Shaders
        modelLibrary = MakeUnique<ModelLibrary>();
        LOG("Model Library Init.");

        // Draw Splash Screen

        // Create Renderer
        renderer = MakeUnique<VulkanRenderer>();
        renderer->Init();
        LOG("Renderer Init.");

        // Create System Manager
        systemManager = MakeUnique<SystemManager>();
        LOG("System Manager Init.");

        // Create Input Manager
        input = MakeUnique<Input>();
        LOG("Input Manager Init.");

        // Create Job System
        JobSystem::Context context;

        JobSystem::Execute(context, [this](JobDispatchArgs args)
        {
            systemManager->RegisterSystem<PhysicsEngine>();
        });
        systemManager->RegisterSystem<PhysicsEngine>();

        JobSystem::Execute(context, [this](JobDispatchArgs args)
        {
            levelManager->LoadCurrentList();
        });
//        levelManager->LoadCurrentList();

        JobSystem::Wait(context);

        LOG("Job System Done.");

        // Create ImGUI Manager
        imGuiManager = MakeUnique<ImGuiManager>();
        imGuiManager->OnInit();
        LOG("ImGui Manager Init.");

        // Create Scene Renderer
        sceneRenderer = MakeUnique<SceneRenderer>(screenWidth, screenHeight);
        Material::InitDefaultTexture();
        sceneRenderer->EnableDebugRenderer(false);
        LOG("Scene Renderer Init.");

        m_CurrentState = AppState::Running;

        LOG("///Engine Finish Init///");
    }

    void Engine::OnQuit()
    {
        Serialise();

        Material::ReleaseDefaultTexture();
//        Font::ShutdownDefaultFont();

        Pipeline::ClearCache();
        RenderPass::ClearCache();
        Framebuffer::ClearCache();

        input.reset();
        imGuiManager.reset();
        sceneRenderer.reset();
        systemManager.reset();
        luaManager.reset();
        levelManager.reset();
        renderer.reset();
        window.reset();
        modelLibrary.reset();
        timer.reset();
    }

    bool Engine::OnFrame()
    {
        if(levelManager->GetSwitchingLevel())
        {
            renderer->GetGraphicsContext()->WaitIdle();
            levelManager->ApplyLevelSwitch();
            return m_CurrentState != AppState::Closing;
        }

        double now  = timer->GetElapsedSD();
        auto ts    = GetTimeStep();

        if(ts->GetSeconds() > 3)
        {
            LOG("Large Delta Time.");
        }

        ExecuteMainThreadQueue();

        {
            ts->OnUpdate();

            ImGuiIO& io  = ImGui::GetIO();
            io.DeltaTime = (float)ts->GetSeconds();

//            stats.FrameTime = ts->GetMillis();
        }

        input->ResetPressed();
        window->ProcessInput();

        {
            std::scoped_lock<std::mutex> lock(m_EventQueueMutex);

            // Process custom event queue
            while(m_EventQueue.size() > 0)
            {
                auto& func = m_EventQueue.front();
                func();
                m_EventQueue.pop();
            }
        }

        ImGui::NewFrame();

        OnUpdate(*ts);
        UpdateSystems();
        systemManager->GetSystem<PhysicsEngine>()->SyncTransforms(levelManager->GetCurrentLevel());

        if(m_CurrentState == AppState::Closing) return false;

        if(!m_Minimized)
        {
            renderer->Begin();
            OnRender();
            imGuiManager->OnRender(levelManager->GetCurrentLevel());

            renderer->Present();

            Pipeline::DeleteUnusedCache();
            Framebuffer::DeleteUnusedCache();
            RenderPass::DeleteUnusedCache();

            // m_ShaderLibrary->Update(ts.GetElapsedSeconds());
            modelLibrary->Update((float)ts->GetElapsedSeconds());
            auto t = MakeShared<TimeStep>(*ts);
            frames++;
        }
    }

    void Engine::UpdateSystems()
    {
        if(m_EditorState != EditorState::Paused && m_EditorState != EditorState::Preview)
        {
            auto level = levelManager->GetCurrentLevel();
            if(level)
            {
                systemManager->OnUpdate(*timeStep, level);
            }
        }
    }

    void Engine::SubmitToMainThread(const std::function<void()> &function)
    {
        std::scoped_lock<std::mutex> lock(m_MainThreadQueueMutex);
        m_MainThreadQueue.push_back(function);
    }

    void Engine::ExecuteMainThreadQueue()
    {
        std::scoped_lock<std::mutex> lock(m_MainThreadQueueMutex);
        for(auto& func : m_MainThreadQueue)
        {
            func();
        }
        m_MainThreadQueue.clear();
    }

    void Engine::OnRender()
    {
        if(!levelManager->GetCurrentLevel())
            return;

        if(!m_DisableMainSceneRenderer)
        {
            sceneRenderer->BeginScene(levelManager->GetCurrentLevel());
            sceneRenderer->OnRender();

            // Clears debug line and point lists
            DebugRenderer::Reset();
            OnDebugDraw();
        }
    }

    void Engine::OnDebugDraw()
    {
        systemManager->OnDebugDraw();
    }

    void Engine::Run()
    {
        while(OnFrame())
        {
        }

        OnQuit();
    }

    void Engine::OnExitScene()
    {

    }

    void Engine::OnSceneViewSizeUpdated(uint32_t width, uint32_t height)
    {

    }

    void Engine::OnEvent(Event &e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(OnWindowClose));
        dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(OnWindowResize));

        if(imGuiManager)
            imGuiManager->OnEvent(e);
        if(e.Handled())
            return;

        if(sceneRenderer)
            sceneRenderer->OnEvent(e);

        if(e.Handled())
            return;

        if(levelManager->GetCurrentLevel())
            levelManager->GetCurrentLevel()->OnEvent(e);

        input->OnEvent(e);
    }

    void Engine::OnNewLevel(Level* level)
    {
        sceneRenderer->OnNewLevel(level);
    }

    void Engine::OnUpdate(const TimeStep &dt)
    {
        if(!levelManager->GetCurrentLevel())
            return;

        if(GetEditorState() != EditorState::Paused
           && GetEditorState() != EditorState::Preview)
        {
            luaManager->OnUpdate(levelManager->GetCurrentLevel());
            levelManager->GetCurrentLevel()->OnUpdate(dt);
        }
        imGuiManager->OnUpdate(dt, levelManager->GetCurrentLevel());
    }

    void Engine::OnImGui()
    {
        if(!levelManager->GetCurrentLevel())
            return;

        levelManager->GetCurrentLevel()->OnImGui();
    }

    void Engine::OnExitLevel()
    {

    }

    bool Engine::OnWindowResize(WindowResizeEvent &e)
    {
        auto context = renderer->GetGraphicsContext();
        context->WaitIdle();

        int width = e.GetWidth(), height = e.GetHeight();

        if(width == 0 || height == 0)
        {
            m_Minimized = true;
            return false;
        }
        m_Minimized = false;

        renderer->OnResize(width, height);

        if(sceneRenderer)
            sceneRenderer->OnResize(width, height);

        context->WaitIdle();

        return false;
    }

    bool Engine::OnWindowClose(WindowCloseEvent &e)
    {
        m_CurrentState = AppState::Closing;
        return true;
    }

    void Engine::Serialise()
    {
        std::stringstream storage;
        {
            // output finishes flushing its contents when it goes out of scope
            cereal::JSONOutputArchive output { storage };
            output(*this);
        }
        auto fullPath = m_ProjectSettings.m_ProjectRoot + m_ProjectSettings.m_ProjectName + std::string(".lmproj");
        LOG_FORMAT("Serialising Application %s", fullPath.c_str());
        FileSystem::WriteTextFile(fullPath, storage.str());
    }

    void Engine::Deserialise()
    {
        auto filePath = m_ProjectSettings.m_ProjectRoot + m_ProjectSettings.m_ProjectName + std::string(".lmproj");

        MountVFSPaths();

        levelManager = MakeUnique<LevelManager>();

        if(!FileSystem::FileExists(filePath))
        {
            // Set Default values
            m_ProjectSettings.RenderAPI   = 1;
            m_ProjectSettings.Width       = 1200;
            m_ProjectSettings.Height      = 800;
            m_ProjectSettings.Borderless  = false;
            m_ProjectSettings.VSync       = true;
            m_ProjectSettings.Title       = "App";
            m_ProjectSettings.ShowConsole = false;
            m_ProjectSettings.Fullscreen  = false;

            m_ProjectLoaded = false;

            m_ProjectSettings.m_EngineAssetPath = StringUtility::GetFileLocation(OS::Instance()->GetExecutablePath()) + "Assets/";

            VirtualFileSystem::Mount("CoreShaders", m_ProjectSettings.m_EngineAssetPath + std::string("Shaders"));

            levelManager->EnqueueLevel(new Level("Empty Scene"));
            levelManager->SwitchLevel(0);

            return;
        }

        if(!FileSystem::FolderExists(m_ProjectSettings.m_ProjectRoot + "Assets"))
            std::filesystem::create_directory(m_ProjectSettings.m_ProjectRoot + "Assets");

        if(!FileSystem::FolderExists(m_ProjectSettings.m_ProjectRoot + "Assets/Scripts"))
            std::filesystem::create_directory(m_ProjectSettings.m_ProjectRoot + "Assets/Scripts");

        if(!FileSystem::FolderExists(m_ProjectSettings.m_ProjectRoot + "Assets/Scenes"))
            std::filesystem::create_directory(m_ProjectSettings.m_ProjectRoot + "Assets/Scenes");

        if(!FileSystem::FolderExists(m_ProjectSettings.m_ProjectRoot + "Assets/Textures"))
            std::filesystem::create_directory(m_ProjectSettings.m_ProjectRoot + "Assets/Textures");

        if(!FileSystem::FolderExists(m_ProjectSettings.m_ProjectRoot + "Assets/Meshes"))
            std::filesystem::create_directory(m_ProjectSettings.m_ProjectRoot + "Assets/Meshes");

        if(!FileSystem::FolderExists(m_ProjectSettings.m_ProjectRoot + "Assets/Sounds"))
            std::filesystem::create_directory(m_ProjectSettings.m_ProjectRoot + "Assets/Sounds");

        if(!FileSystem::FolderExists(m_ProjectSettings.m_ProjectRoot + "Assets/Prefabs"))
            std::filesystem::create_directory(m_ProjectSettings.m_ProjectRoot + "Assets/Prefabs");

        if(!FileSystem::FolderExists(m_ProjectSettings.m_ProjectRoot + "Assets/Materials"))
            std::filesystem::create_directory(m_ProjectSettings.m_ProjectRoot + "Assets/Materials");

        m_ProjectLoaded = true;

        std::string data = FileSystem::ReadTextFile(filePath);
        std::istringstream istr;
        istr.str(data);
        try
        {
            cereal::JSONInputArchive inputArchive(istr);
            inputArchive(*this);
        }
        catch(...)
        {
            // Set Default values
            m_ProjectSettings.RenderAPI = 1;
            m_ProjectSettings.Width = 1200;
            m_ProjectSettings.Height = 800;
            m_ProjectSettings.Borderless = false;
            m_ProjectSettings.VSync = true;
            m_ProjectSettings.Title = "App";
            m_ProjectSettings.ShowConsole = false;
            m_ProjectSettings.Fullscreen = false;


            m_ProjectSettings.m_EngineAssetPath =
                    StringUtility::GetFileLocation(OS::Instance()->GetExecutablePath()) + "../../NekoEngine/Assets/";

            VirtualFileSystem::Mount("CoreShaders", m_ProjectSettings.m_EngineAssetPath + std::string("Shaders"));

            levelManager->EnqueueLevel(new Level("Empty Scene"));
            levelManager->SwitchLevel(0);

            LOG("Failed to load project");
        }
    }

    void Engine::AddDefaultLevel()
    {
        if(levelManager->GetLevels().size() == 0)
        {
            levelManager->EnqueueLevel(new Level("Empty Scene"));
            levelManager->SwitchLevel(0);
        }
    }

    template <typename Archive>
    void Engine::save(Archive& archive) const
    {
        int projectVersion = 8;

        archive(cereal::make_nvp("Project Version", projectVersion));

        // Window size and full screen shouldnt be in project

        // Version 8 removed width and height
        archive(cereal::make_nvp("RenderAPI", m_ProjectSettings.RenderAPI),
                cereal::make_nvp("Fullscreen", m_ProjectSettings.Fullscreen),
                cereal::make_nvp("VSync", m_ProjectSettings.VSync),
                cereal::make_nvp("ShowConsole", m_ProjectSettings.ShowConsole),
                cereal::make_nvp("Title", m_ProjectSettings.Title));
        // Version 2

        auto paths = levelManager->GetLevelFilePaths();
        std::vector<std::string> newPaths;
        for(auto& path : paths)
        {
            std::string newPath;
            VirtualFileSystem::AbsoulePathToVFS(path, newPath);
            newPaths.push_back(path);
        }
        archive(cereal::make_nvp("Scenes", newPaths));
        // Version 3
        archive(cereal::make_nvp("SceneIndex", levelManager->GetCurrentLevelIndex()));
        // Version 4
        archive(cereal::make_nvp("Borderless", m_ProjectSettings.Borderless));
        // Version 5
        archive(cereal::make_nvp("EngineAssetPath", m_ProjectSettings.m_EngineAssetPath));
        // Version 6
        archive(cereal::make_nvp("GPUIndex", m_ProjectSettings.DesiredGPUIndex));
    }

    template <typename Archive>
    void Engine::load(Archive& archive)
    {
        int sceneIndex = 0;
        archive(cereal::make_nvp("Project Version", m_ProjectSettings.ProjectVersion));

        std::string test;
        if(m_ProjectSettings.ProjectVersion < 8)
        {
            archive(cereal::make_nvp("RenderAPI", m_ProjectSettings.RenderAPI),
                    cereal::make_nvp("Width", m_ProjectSettings.Width),
                    cereal::make_nvp("Height", m_ProjectSettings.Height),
                    cereal::make_nvp("Fullscreen", m_ProjectSettings.Fullscreen),
                    cereal::make_nvp("VSync", m_ProjectSettings.VSync),
                    cereal::make_nvp("ShowConsole", m_ProjectSettings.ShowConsole),
                    cereal::make_nvp("Title", m_ProjectSettings.Title));
        }
        else
        {
            archive(cereal::make_nvp("RenderAPI", m_ProjectSettings.RenderAPI),
                    cereal::make_nvp("Fullscreen", m_ProjectSettings.Fullscreen),
                    cereal::make_nvp("VSync", m_ProjectSettings.VSync),
                    cereal::make_nvp("ShowConsole", m_ProjectSettings.ShowConsole),
                    cereal::make_nvp("Title", m_ProjectSettings.Title));
        }
        if(m_ProjectSettings.ProjectVersion > 2)
        {
            std::vector<std::string> sceneFilePaths;
            archive(cereal::make_nvp("Levels", sceneFilePaths));

            for(auto& filePath : sceneFilePaths)
            {
                levelManager->AddFileToLoadList(filePath);
            }

            if(sceneFilePaths.size() == sceneIndex)
                AddDefaultLevel();
        }
        if(m_ProjectSettings.ProjectVersion > 3)
        {
            archive(cereal::make_nvp("SceneIndex", sceneIndex));
            levelManager->SwitchLevel(sceneIndex);
        }
        if(m_ProjectSettings.ProjectVersion > 4)
        {
            archive(cereal::make_nvp("Borderless", m_ProjectSettings.Borderless));
        }

        if(m_ProjectSettings.ProjectVersion > 5)
        {
            archive(cereal::make_nvp("EngineAssetPath", m_ProjectSettings.m_EngineAssetPath));
        }
        else
            m_ProjectSettings.m_EngineAssetPath = "/Users/jmorton/dev/NekoEngine/NekoEngine/Assets/";

        if(m_ProjectSettings.ProjectVersion > 6)
            archive(cereal::make_nvp("GPUIndex", m_ProjectSettings.DesiredGPUIndex));

        VirtualFileSystem::Mount("CoreShaders", m_ProjectSettings.m_EngineAssetPath + std::string("Shaders"));
    }



//    void Engine::Run()
//    {
//        LOG("Engine Running");
//        while(!window->ShouldExit())
//        {
//            TickOneFrame(1.0f / 60.0f);
//        }
////        while(!g_RuntimeGlobalContext.m_WindowSystem->ShouldClose())
////        {
////            TickOneFrame(1.0f / 60.0f);
////        }
//    }
//    void Engine::Quit()
//    {
//        LOG("Engine Quit.");
//        window.reset();
//    }
//
//    void Engine::TickOneFrame(float deltaTime)
//    {
//        LogicalTick(deltaTime);
//        RendererTick(deltaTime);
//    }
//
//    void Engine::LogicalTick(float deltaTime)
//    {
//        //TODO tick WorldManager and InputManager
//        window->ProcessInput();
//
//
//
//    }
//
//    void Engine::RendererTick(float deltaTime)
//    {
//        //TODO tick Renderer
//
//        window->OnUpdate();
//    }

    float Engine::GetWindowDPI() const
    {
        return window->GetDPIScale();
    }

    SwapChain* Engine::GetSwapChain() const
    {
        return window->GetSwapChain().get();
    }


} // NekoEngine