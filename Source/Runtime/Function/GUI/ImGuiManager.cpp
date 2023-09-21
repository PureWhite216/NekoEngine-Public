#include "ImGuiManager.h"
#include "Core.h"
#include "GLFW/glfw3.h"
#include "Engine.h"
#include "Vulkan/VulkanImGuiRenderer.h"
#include "ImGui/Plugins/ImGuizmo.h"
#include "ImGui/IconsMaterialDesignIcons.h"
#include "ImGuiUtility.h"
#include "ImGui/Plugins/ImGuiAl/fonts/MaterialDesign.inl"
#include <ImGui/Plugins/ImGuiAl/fonts/RobotoMedium.inl>
#include <ImGui/Plugins/ImGuiAl/fonts/RobotoRegular.inl>
#include <ImGui/Plugins/ImGuiAl/fonts/RobotoBold.inl>


namespace NekoEngine
{
    ImGuiManager::ImGuiManager(bool clearScreen)
    {
        m_ClearScreen = clearScreen;
    }

    static const char* ImGui_ImplGlfw_GetClipboardText(void*)
    {
        return glfwGetClipboardString((GLFWwindow*)gEngine->GetWindow()->GetHandle());
    }

    static void ImGui_ImplGlfw_SetClipboardText(void*, const char* text)
    {
        glfwSetClipboardString((GLFWwindow*)(gEngine->GetWindow()->GetHandle()), text);
    }

    void ImGuiManager::OnInit()
    {
        auto window = gEngine->GetWindow();
        ImGuiIO& io      = ImGui::GetIO();
        io.DisplaySize   = ImVec2(static_cast<float>(window->GetWidth()), static_cast<float>(window->GetHeight()));
        // io.DisplayFramebufferScale = ImVec2(app.GetWindow()->GetDPIScale(), app.GetWindow()->GetDPIScale());
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        m_DPIScale = window->GetDPIScale();

        io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
        io.ConfigWindowsMoveFromTitleBarOnly = true;

        m_FontSize *= window->GetDPIScale();

        SetImGuiKeyCodes();
        SetImGuiStyle();

        //TODO: OpenGL
        m_IMGUIRenderer = UniquePtr<ImGuiRenderer>(new VulkanImGuiRenderer(window->GetWidth(), window->GetHeight(), m_ClearScreen));

        if(m_IMGUIRenderer) m_IMGUIRenderer->Init();

        io.SetClipboardTextFn = ImGui_ImplGlfw_SetClipboardText;
        io.GetClipboardTextFn = ImGui_ImplGlfw_GetClipboardText;

    }

    void ImGuiManager::OnUpdate(const TimeStep& dt, Level* level)
    {

        if(m_IMGUIRenderer && m_IMGUIRenderer->Implemented())
        {
//            m_IMGUIRenderer->NewFrame();
        }

        ImGuizmo::BeginFrame();

        gEngine->OnImGui();
    }

    void ImGuiManager::OnEvent(Event& event)
    {

        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<MouseButtonPressedEvent>(BIND_EVENT_FN(ImGuiManager::OnMouseButtonPressedEvent));
        dispatcher.Dispatch<MouseButtonReleasedEvent>(BIND_EVENT_FN(ImGuiManager::OnMouseButtonReleasedEvent));
        dispatcher.Dispatch<MouseMovedEvent>(BIND_EVENT_FN(ImGuiManager::OnMouseMovedEvent));
        dispatcher.Dispatch<MouseScrolledEvent>(BIND_EVENT_FN(ImGuiManager::OnMouseScrolledEvent));
        dispatcher.Dispatch<KeyPressedEvent>(BIND_EVENT_FN(ImGuiManager::OnKeyPressedEvent));
        dispatcher.Dispatch<KeyReleasedEvent>(BIND_EVENT_FN(ImGuiManager::OnKeyReleasedEvent));
        dispatcher.Dispatch<KeyTypedEvent>(BIND_EVENT_FN(ImGuiManager::OnKeyTypedEvent));
        dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT_FN(ImGuiManager::OnWindowResizeEvent));
    }

    void ImGuiManager::OnRender(Level* level)
    {

        if(m_IMGUIRenderer && m_IMGUIRenderer->Implemented())
        {
            m_IMGUIRenderer->Render(nullptr);
        }
    }

    void ImGuiManager::OnNewLevel(Level* level)
    {

        m_IMGUIRenderer->Clear();
    }

    int NekoEngineMouseButtonToImGui(MouseKeyCode key)
    {
        switch(key)
        {
            case MouseKeyCode::ButtonLeft:
                return 0;
            case MouseKeyCode::ButtonRight:
                return 1;
            case MouseKeyCode::ButtonMiddle:
                return 2;
            default:
                return 4;
        }

        return 4;
    }

    bool ImGuiManager::OnMouseButtonPressedEvent(MouseButtonPressedEvent& e)
    {
        ImGuiIO& io                                               = ImGui::GetIO();
        io.MouseDown[NekoEngineMouseButtonToImGui(e.GetMouseButton())] = true;

        return false;
    }

    bool ImGuiManager::OnMouseButtonReleasedEvent(MouseButtonReleasedEvent& e)
    {
        ImGuiIO& io                                               = ImGui::GetIO();
        io.MouseDown[NekoEngineMouseButtonToImGui(e.GetMouseButton())] = false;

        return false;
    }

    bool ImGuiManager::OnMouseMovedEvent(MouseMovedEvent& e)
    {
        ImGuiIO& io = ImGui::GetIO();
        if(gEngine->GetInput()->GetMouseMode() == MouseMode::Visible)
            io.MousePos = ImVec2(e.GetX() * m_DPIScale, e.GetY() * m_DPIScale);

        return false;
    }

    bool ImGuiManager::OnMouseScrolledEvent(MouseScrolledEvent& e)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.MouseWheel += e.GetYOffset() / 10.0f;
        io.MouseWheelH += e.GetXOffset() / 10.0f;

        return false;
    }

    bool ImGuiManager::OnKeyPressedEvent(KeyPressedEvent& e)
    {
        ImGuiIO& io                      = ImGui::GetIO();
        io.KeysDown[(int)e.GetKeyCode()] = true;

        io.KeyCtrl  = io.KeysDown[(int)KeyCode::LeftControl] || io.KeysDown[(int)KeyCode::RightControl];
        io.KeyShift = io.KeysDown[(int)KeyCode::LeftShift] || io.KeysDown[(int)KeyCode::RightShift];
        io.KeyAlt   = io.KeysDown[(int)KeyCode::LeftAlt] || io.KeysDown[(int)KeyCode::RightAlt];

#ifdef _WIN32
        io.KeySuper = false;
#else
        io.KeySuper = io.KeysDown[(int)KeyCode::LeftSuper] || io.KeysDown[(int)KeyCode::RightSuper];
#endif

        return io.WantTextInput;
    }

    bool ImGuiManager::OnKeyReleasedEvent(KeyReleasedEvent& e)
    {
        ImGuiIO& io                      = ImGui::GetIO();
        io.KeysDown[(int)e.GetKeyCode()] = false;

        return false;
    }

    bool ImGuiManager::OnKeyTypedEvent(KeyTypedEvent& e)
    {
        ImGuiIO& io = ImGui::GetIO();
        int keycode = (int)e.Character;
        if(keycode > 0 && keycode < 0x10000)
            io.AddInputCharacter((unsigned short)keycode);

        return false;
    }

    bool ImGuiManager::OnWindowResizeEvent(WindowResizeEvent& e)
    {

        ImGuiIO& io = ImGui::GetIO();

        uint32_t width  = Maths::Max(1u, e.GetWidth());
        uint32_t height = Maths::Max(1u, e.GetHeight());

        io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
        // io.DisplayFramebufferScale = ImVec2(e.GetDPIScale(), e.GetDPIScale());
        m_DPIScale = e.GetDPIScale();
        m_IMGUIRenderer->OnResize(width, height);

        return false;
    }

    void ImGuiManager::SetImGuiKeyCodes()
    {
        ImGuiIO& io = ImGui::GetIO();
        io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
        io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
        io.KeyMap[ImGuiKey_Tab]        = (int)KeyCode::Tab;
        io.KeyMap[ImGuiKey_LeftArrow]  = (int)KeyCode::Left;
        io.KeyMap[ImGuiKey_RightArrow] = (int)KeyCode::Right;
        io.KeyMap[ImGuiKey_UpArrow]    = (int)KeyCode::Up;
        io.KeyMap[ImGuiKey_DownArrow]  = (int)KeyCode::Down;
        io.KeyMap[ImGuiKey_PageUp]     = (int)KeyCode::PageUp;
        io.KeyMap[ImGuiKey_PageDown]   = (int)KeyCode::PageDown;
        io.KeyMap[ImGuiKey_Home]       = (int)KeyCode::Home;
        io.KeyMap[ImGuiKey_End]        = (int)KeyCode::End;
        io.KeyMap[ImGuiKey_Insert]     = (int)KeyCode::Insert;
        io.KeyMap[ImGuiKey_Delete]     = (int)KeyCode::Delete;
        io.KeyMap[ImGuiKey_Backspace]  = (int)KeyCode::Backspace;
        io.KeyMap[ImGuiKey_Space]      = (int)KeyCode::Space;
        io.KeyMap[ImGuiKey_Enter]      = (int)KeyCode::Enter;
        io.KeyMap[ImGuiKey_Escape]     = (int)KeyCode::Escape;
        io.KeyMap[ImGuiKey_A]          = (int)KeyCode::A;
        io.KeyMap[ImGuiKey_C]          = (int)KeyCode::C;
        io.KeyMap[ImGuiKey_V]          = (int)KeyCode::V;
        io.KeyMap[ImGuiKey_X]          = (int)KeyCode::X;
        io.KeyMap[ImGuiKey_Y]          = (int)KeyCode::Y;
        io.KeyMap[ImGuiKey_Z]          = (int)KeyCode::Z;
        io.KeyRepeatDelay              = 0.400f;
        io.KeyRepeatRate               = 0.05f;
    }

    void ImGuiManager::SetImGuiStyle()
    {

        ImGuiIO& io = ImGui::GetIO();

        ImGui::StyleColorsDark();

        io.FontGlobalScale = 1.0f;

        ImFontConfig icons_config;
        icons_config.MergeMode   = false;
        icons_config.PixelSnapH  = true;
        icons_config.OversampleH = icons_config.OversampleV = 1;
        icons_config.GlyphMinAdvanceX                       = 4.0f;
        icons_config.SizePixels                             = 12.0f;

        static const ImWchar ranges[] = {
                0x0020,
                0x00FF,
                0x0400,
                0x044F,
                0,
        };

        io.Fonts->AddFontFromMemoryCompressedTTF(RobotoRegular_compressed_data, RobotoRegular_compressed_size, m_FontSize, &icons_config, ranges);
        AddIconFont();

        io.Fonts->AddFontFromMemoryCompressedTTF(RobotoBold_compressed_data, RobotoBold_compressed_size, m_FontSize + 2.0f, &icons_config, ranges);

        io.Fonts->AddFontFromMemoryCompressedTTF(RobotoRegular_compressed_data, RobotoRegular_compressed_size, m_FontSize * 0.8f, &icons_config, ranges);
        AddIconFont();

        io.Fonts->AddFontDefault();
        AddIconFont();

        io.Fonts->TexGlyphPadding = 1;
        for(int n = 0; n < io.Fonts->ConfigData.Size; n++)
        {
            ImFontConfig* font_config       = (ImFontConfig*)&io.Fonts->ConfigData[n];
            font_config->RasterizerMultiply = 1.0f;
        }

        ImGuiStyle& style = ImGui::GetStyle();

        style.WindowPadding     = ImVec2(5, 5);
        style.FramePadding      = ImVec2(4, 4);
        style.ItemSpacing       = ImVec2(6, 2);
        style.ItemInnerSpacing  = ImVec2(2, 2);
        style.IndentSpacing     = 6.0f;
        style.TouchExtraPadding = ImVec2(4, 4);

        style.ScrollbarSize = 10;

        style.WindowBorderSize = 0;
        style.ChildBorderSize  = 1;
        style.PopupBorderSize  = 3;
        style.FrameBorderSize  = 0.0f;

        const int roundingAmount = 2;
        style.PopupRounding      = roundingAmount;
        style.WindowRounding     = roundingAmount;
        style.ChildRounding      = 0;
        style.FrameRounding      = roundingAmount;
        style.ScrollbarRounding  = roundingAmount;
        style.GrabRounding       = roundingAmount;
        style.WindowMinSize      = ImVec2(200.0f, 200.0f);
        style.WindowTitleAlign   = ImVec2(0.5f, 0.5f);

        //TODO: Check
#ifdef IMGUI_HAS_DOCK
        style.TabBorderSize = 1.0f;
        style.TabRounding   = roundingAmount; // + 4;

        if(ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding              = roundingAmount;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }
#endif

        ImGuiUtility::SetTheme(ImGuiUtility::Theme::Dark);
    }

    void ImGuiManager::AddIconFont()
    {
        ImGuiIO& io = ImGui::GetIO();

        static const ImWchar icons_ranges[] = { ICON_MIN_MDI, ICON_MAX_MDI, 0 };
        ImFontConfig icons_config;
        // merge in icons from Font Awesome
        icons_config.MergeMode     = true;
        icons_config.PixelSnapH    = true;
        icons_config.GlyphOffset.y = 1.0f;
        icons_config.OversampleH = icons_config.OversampleV = 1;
        icons_config.GlyphMinAdvanceX                       = 4.0f;
        icons_config.SizePixels                             = 12.0f;

        io.Fonts->AddFontFromMemoryCompressedTTF(MaterialDesign_compressed_data, MaterialDesign_compressed_size, m_FontSize, &icons_config, icons_ranges);
    }


} // NekoEngine