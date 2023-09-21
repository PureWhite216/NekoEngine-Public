#pragma once
#include "Event/KeyEvent.h"
#include "Event/MouseEvent.h"
#include "Event/ApplicationEvent.h"
#include "ImGui/imgui.h"
#include "RHI/ImGuiRenderer.h"

namespace NekoEngine
{
    class Level;
    class TimeStep;

    class ImGuiManager
    {
    private:
        float m_FontSize = 14.0f;
        float m_DPIScale;

        UniquePtr<ImGuiRenderer> m_IMGUIRenderer;
        bool m_ClearScreen;

        bool OnMouseButtonPressedEvent(MouseButtonPressedEvent& e);
        bool OnMouseButtonReleasedEvent(MouseButtonReleasedEvent& e);
        bool OnMouseMovedEvent(MouseMovedEvent& e);
        bool OnMouseScrolledEvent(MouseScrolledEvent& e);
        bool OnKeyPressedEvent(KeyPressedEvent& e);
        bool OnKeyReleasedEvent(KeyReleasedEvent& e);
        bool OnKeyTypedEvent(KeyTypedEvent& e);
        bool OnWindowResizeEvent(WindowResizeEvent& e);

        void SetImGuiKeyCodes();
        void SetImGuiStyle();
        void AddIconFont();
    public:
        ImGuiManager(bool clearScreen = false);
        ~ImGuiManager() = default;

        void OnInit();
        void OnUpdate(const TimeStep& dt, Level* level);
        void OnEvent(Event& event);
        void OnRender(Level* level);
        void OnNewLevel(Level* level);
    };

} // NekoEngine


