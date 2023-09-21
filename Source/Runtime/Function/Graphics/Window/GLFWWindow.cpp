#include "Vulkan/VulkanContext.h"
#include "Vulkan/VulkanSwapChain.h"
#include "GLFWWindow.h"
#include "Event/ApplicationEvent.h"
#include "Engine.h"
#include "glad/glad.h"
#include "GLFWKeyCodes.h"
namespace NekoEngine
{
    static GLFWcursor* g_MouseCursors[ImGuiMouseCursor_COUNT] = { 0 };

    GLFWWindow::GLFWWindow(const Window::CreateInfo& createInfo)
    {
        isInit = false;
        isResized = true;

        width = createInfo.width;
        height = createInfo.height;
        isVSync = createInfo.isVSync;
        isFullscreen = createInfo.isFullscreen;
        title = createInfo.title;

        Init();

        gVulkanContext.Init();
        graphicsContext = &gVulkanContext;
        swapChain = MakeShared<VulkanSwapChain>(width, height);
        swapChain->Init(isVSync, this);
        //TODO swapchain
    }

    GLFWWindow::~GLFWWindow()
    {
        if(windowHandle) glfwDestroyWindow(windowHandle);
        glfwTerminate();
    }

    bool GLFWWindow::Init()
    {
        LOG("Initializing GLFW window");
        static bool isGLFWInitialized = false;

        if(!isGLFWInitialized)
        {
            glfwInit();
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
            glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
            isGLFWInitialized = true;
        }

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        float xScale, yScale;
        glfwGetMonitorContentScale(monitor, &xScale, &yScale);

        //TODO Set borderless window

        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        if(isFullscreen)
        {
            width = mode->width;
            height = mode->height;
        }

        windowHandle = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        if(!windowHandle)
        {
            LOG("Failed to create GLFW window.");
            return false;
        }

        glfwSetWindowUserPointer(windowHandle, this);

        SetTitle(title);

        if(glfwRawMouseMotionSupported())
            glfwSetInputMode(windowHandle, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

        //TODO Set icon

        glfwSetInputMode(windowHandle, GLFW_STICKY_KEYS, true);

        //TODO Set callback

        glfwSetWindowSizeCallback(windowHandle, [](GLFWwindow* window, int width, int height)
        {
            WindowData& data = *static_cast<WindowData*>((glfwGetWindowUserPointer(window)));

            int w, h;
            glfwGetFramebufferSize(window, &w, &h);

            data.DPIScale = (float)w / (float)width;

            data.Width = uint32_t(width * data.DPIScale);
            data.Height = uint32_t(height * data.DPIScale);

            WindowResizeEvent event(data.Width, data.Height, data.DPIScale);
            data.EventCallback(event); });

        glfwSetWindowCloseCallback(windowHandle, [](GLFWwindow* window)
        {
            WindowData& data = *static_cast<WindowData*>((glfwGetWindowUserPointer(window)));
            WindowCloseEvent event;
            data.EventCallback(event);
            data.Exit = true; });

        glfwSetWindowFocusCallback(windowHandle, [](GLFWwindow* window, int focused)
        {
            Window* lmWindow = gEngine->GetWindow();

            if(lmWindow)
                lmWindow->SetWindowFocus(focused); });

        glfwSetWindowIconifyCallback(windowHandle, [](GLFWwindow* window, int32_t state)
        {
            switch(state)
            {
                case GL_TRUE:
                    gEngine->GetWindow()->SetWindowFocus(false);
                    break;
                case GL_FALSE:
                    gEngine->GetWindow()->SetWindowFocus(true);
                    break;
                default:
                    LOG("Unsupported window iconify state from callback");
            } });

        glfwSetKeyCallback(windowHandle, [](GLFWwindow* window, int key, int scancode, int action, int mods)
        {
            WindowData& data = *static_cast<WindowData*>((glfwGetWindowUserPointer(window)));

            switch(action)
            {
                case GLFW_PRESS:
                {
                    KeyPressedEvent event(GLFWKeyCodes::GLFWToKeyboardKey(key), 0);
                    data.EventCallback(event);
                    break;
                }
                case GLFW_RELEASE:
                {
                    KeyReleasedEvent event(GLFWKeyCodes::GLFWToKeyboardKey(key));
                    data.EventCallback(event);
                    break;
                }
                case GLFW_REPEAT:
                {
                    KeyPressedEvent event(GLFWKeyCodes::GLFWToKeyboardKey(key), 1);
                    data.EventCallback(event);
                    break;
                }
            } });

        glfwSetMouseButtonCallback(windowHandle, [](GLFWwindow* window, int button, int action, int mods)
        {
            WindowData& data = *static_cast<WindowData*>((glfwGetWindowUserPointer(window)));

            switch(action)
            {
                case GLFW_PRESS:
                {
                    MouseButtonPressedEvent event(GLFWKeyCodes::GLFWToMouseKey(button));
                    data.EventCallback(event);
                    break;
                }
                case GLFW_RELEASE:
                {
                    MouseButtonReleasedEvent event(GLFWKeyCodes::GLFWToMouseKey(button));
                    data.EventCallback(event);
                    break;
                }
            } });

        glfwSetScrollCallback(windowHandle, [](GLFWwindow* window, double xOffset, double yOffset)
        {
            WindowData& data = *static_cast<WindowData*>((glfwGetWindowUserPointer(window)));
            MouseScrolledEvent event((float)xOffset, (float)yOffset);
            data.EventCallback(event); });

        glfwSetCursorPosCallback(windowHandle, [](GLFWwindow* window, double xPos, double yPos)
        {
            WindowData& data = *static_cast<WindowData*>((glfwGetWindowUserPointer(window)));
            MouseMovedEvent event((float)xPos /* * data.DPIScale*/, (float)yPos /* * data.DPIScale*/);
            data.EventCallback(event); });

        glfwSetCursorEnterCallback(windowHandle, [](GLFWwindow* window, int enter)
        {
            WindowData& data = *static_cast<WindowData*>((glfwGetWindowUserPointer(window)));

            MouseEnterEvent event(enter > 0);
            data.EventCallback(event); });

        glfwSetCharCallback(windowHandle, [](GLFWwindow* window, unsigned int keycode)
        {
            WindowData& data = *static_cast<WindowData*>((glfwGetWindowUserPointer(window)));

            KeyTypedEvent event(GLFWKeyCodes::GLFWToKeyboardKey(keycode), char(keycode));
            data.EventCallback(event); });

        glfwSetDropCallback(windowHandle, [](GLFWwindow* window, int numDropped, const char** filenames)
        {
            WindowData& data = *static_cast<WindowData*>((glfwGetWindowUserPointer(window)));

            std::string filePath = filenames[0];
            WindowFileEvent event(filePath);
            data.EventCallback(event); });

        g_MouseCursors[ImGuiMouseCursor_Arrow]      = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
        g_MouseCursors[ImGuiMouseCursor_TextInput]  = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
        g_MouseCursors[ImGuiMouseCursor_ResizeAll]  = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
        g_MouseCursors[ImGuiMouseCursor_ResizeNS]   = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
        g_MouseCursors[ImGuiMouseCursor_ResizeEW]   = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
        g_MouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
        g_MouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
        g_MouseCursors[ImGuiMouseCursor_Hand]       = glfwCreateStandardCursor(GLFW_HAND_CURSOR);

        isInit = true;
        return true;
    }

    void GLFWWindow::OnUpdate()
    {

    }

    bool GLFWWindow::ShouldExit()
    {
        return glfwWindowShouldClose(windowHandle);
    }

    void GLFWWindow::HideMouse(bool isHide)
    {
        if(isHide)
        {
            glfwSetInputMode(windowHandle, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        }
        else
        {
            glfwSetInputMode(windowHandle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    void GLFWWindow::SetMousePosition(const FVector2 pos)
    {
        glfwSetCursorPos(windowHandle, pos.x, pos.y);
    }

    void GLFWWindow::ProcessInput()
    {
        glfwPollEvents();

        if(glfwGetKey(windowHandle, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(windowHandle, true);
    }

    void GLFWWindow::Maximise()
    {
        glfwMaximizeWindow(windowHandle);
    }

    void GLFWWindow::ToggleVSync()
    {
        isVSync = !isVSync;
        glfwSwapInterval(isVSync);
    }

    void GLFWWindow::SetVSync(bool vysnc)
    {
        isVSync = vysnc;
        glfwSwapInterval(isVSync);
    }

    void GLFWWindow::SetTitle(const String& _title)
    {
        title = _title;
        glfwSetWindowTitle(windowHandle, title.c_str());
    }


} // NekoEngine