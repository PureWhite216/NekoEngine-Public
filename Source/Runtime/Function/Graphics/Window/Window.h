
#pragma once
#include "Core.h"
#include "RHI/GraphicsContext.h"
#include "RHI/SwapChain.h"
#include "Event/Event.h"

namespace NekoEngine
{
    using EventCallbackFn = std::function<void(Event&)>;

    class Window
    {
    public:
        struct CreateInfo
        {
            explicit CreateInfo(uint32_t width = 1600, uint32_t height = 900, int renderAPI = 0, String title = "NekoEngine",
                       bool fullscreen = false, bool vSync = true, bool borderless = false)
                    : width(width), height(height), isFullscreen(fullscreen), isVSync(vSync), isBorderless(borderless),
                      title(std::move(title))
            {
            }

            uint32_t width,height;
            bool isFullscreen;
            bool isVSync;
            bool isBorderless;
            bool ShowConsole = true;
            std::string title;
        };

    public:
        bool isResized = false;

        static Window* instance;
    protected:
        uint32_t height;
        uint32_t width;
        String title;
        bool isInit = false;
        bool isFullscreen;
        bool isVSync;
        bool isBorderless;
        bool isFocused = false;
        FVector2 position;
        SharedPtr<SwapChain> swapChain;
        GraphicsContext* graphicsContext;

        struct WindowData
        {
            std::string Title;
            uint32_t Width, Height;
            bool VSync;
            bool Exit;
            float DPIScale;

            EventCallbackFn EventCallback;
        };

        WindowData windowData;

    public:
        virtual ~Window();

        virtual bool Init(const CreateInfo& properties) = 0;
        bool IsInit() { return isInit; };

        virtual void OnUpdate() = 0;

        virtual bool ShouldExit() = 0;

        virtual void ToggleVSync() = 0;
        virtual void SetVSync(bool vsync) = 0;
        virtual void SetTitle(const String& _title) = 0;
        virtual String GetTitle() { return title; }
        virtual void* GetHandle() { return nullptr; };
//        virtual float GetScreenRatio() const = 0;
        virtual void HideMouse(bool hide){};
        virtual void SetMousePosition(const FVector2 pos) = 0;
//        virtual void SetBorderless(bool isBorderless) = 0;
        virtual void ProcessInput() = 0;
        virtual void Maximise() = 0;

        virtual void SetEventCallback(const EventCallbackFn& callback) { windowData.EventCallback = callback;}

        void SetWindowFocus(bool focus) { isFocused = focus; }
        bool GetWindowFocus() const { return isFocused; }
        const SharedPtr<SwapChain>& GetSwapChain() const { return swapChain; }
//        const SharedPtr<GraphicsContext>& GetGraphicsContext() const { return graphicsContext; }

        uint32_t GetWidth() const { return width; }
        uint32_t GetHeight() const { return height; }
        FVector2 GetPosition() const { return position; }
        float GetDPIScale() const { return 1.0f; }
        bool IsVSync() const { return isVSync; }

//        virtual void SetEventCallBack()
    };

} // NekoEngine

