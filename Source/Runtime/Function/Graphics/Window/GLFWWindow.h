#pragma once

#include <GLFW/glfw3.h>
#include "Window.h"
namespace NekoEngine
{

    class GLFWWindow final : public Window
    {
    private:
        GLFWwindow* windowHandle;

    public:
        explicit GLFWWindow(const CreateInfo& properties);
        ~GLFWWindow() override;
        bool Init();
        bool ShouldExit() override;
        void ToggleVSync() override;
        void SetVSync(bool set) override;
        void SetTitle(const std::string& title) override;
//        void SetBorderlessWindow(bool borderless) override;
        void OnUpdate() override;
        void HideMouse(bool hide) override;
        void SetMousePosition(const FVector2 pos) override;
        void ProcessInput() override;
        void Maximise() override;

        inline void* GetHandle() override
        {
            return windowHandle;
        }

    };

} // NekoEngine

