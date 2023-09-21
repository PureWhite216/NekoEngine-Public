#pragma once
#include "KeyCode.h"
#include "GLFW/glfw3.h"

namespace NekoEngine
{
    namespace GLFWKeyCodes
    {
        static KeyCode GLFWToKeyboardKey(uint32_t glfwKey)
        {
            static std::map<uint32_t, KeyCode> keyMap = {
                    { GLFW_KEY_A, KeyCode::A },
                    { GLFW_KEY_B, KeyCode::B },
                    { GLFW_KEY_C, KeyCode::C },
                    { GLFW_KEY_D, KeyCode::D },
                    { GLFW_KEY_E, KeyCode::E },
                    { GLFW_KEY_F, KeyCode::F },
                    { GLFW_KEY_G, KeyCode::G },
                    { GLFW_KEY_H, KeyCode::H },
                    { GLFW_KEY_I, KeyCode::I },
                    { GLFW_KEY_J, KeyCode::J },
                    { GLFW_KEY_K, KeyCode::K },
                    { GLFW_KEY_L, KeyCode::L },
                    { GLFW_KEY_M, KeyCode::M },
                    { GLFW_KEY_N, KeyCode::N },
                    { GLFW_KEY_O, KeyCode::O },
                    { GLFW_KEY_P, KeyCode::P },
                    { GLFW_KEY_Q, KeyCode::Q },
                    { GLFW_KEY_R, KeyCode::R },
                    { GLFW_KEY_S, KeyCode::S },
                    { GLFW_KEY_T, KeyCode::T },
                    { GLFW_KEY_U, KeyCode::U },
                    { GLFW_KEY_V, KeyCode::V },
                    { GLFW_KEY_W, KeyCode::W },
                    { GLFW_KEY_X, KeyCode::X },
                    { GLFW_KEY_Y, KeyCode::Y },
                    { GLFW_KEY_Z, KeyCode::Z },

                    { GLFW_KEY_0, KeyCode::D0 },
                    { GLFW_KEY_1, KeyCode::D1 },
                    { GLFW_KEY_2, KeyCode::D2 },
                    { GLFW_KEY_3, KeyCode::D3 },
                    { GLFW_KEY_4, KeyCode::D4 },
                    { GLFW_KEY_5, KeyCode::D5 },
                    { GLFW_KEY_6, KeyCode::D6 },
                    { GLFW_KEY_7, KeyCode::D7 },
                    { GLFW_KEY_8, KeyCode::D8 },
                    { GLFW_KEY_9, KeyCode::D9 },

                    { GLFW_KEY_F1, KeyCode::F1 },
                    { GLFW_KEY_F2, KeyCode::F2 },
                    { GLFW_KEY_F3, KeyCode::F3 },
                    { GLFW_KEY_F4, KeyCode::F4 },
                    { GLFW_KEY_F5, KeyCode::F5 },
                    { GLFW_KEY_F6, KeyCode::F6 },
                    { GLFW_KEY_F7, KeyCode::F7 },
                    { GLFW_KEY_F8, KeyCode::F8 },
                    { GLFW_KEY_F9, KeyCode::F9 },
                    { GLFW_KEY_F10, KeyCode::F10 },
                    { GLFW_KEY_F11, KeyCode::F11 },
                    { GLFW_KEY_F12, KeyCode::F12 },

                    { GLFW_KEY_MINUS, KeyCode::Minus },
                    { GLFW_KEY_DELETE, KeyCode::Delete },
                    { GLFW_KEY_SPACE, KeyCode::Space },
                    { GLFW_KEY_LEFT, KeyCode::Left },
                    { GLFW_KEY_RIGHT, KeyCode::Right },
                    { GLFW_KEY_UP, KeyCode::Up },
                    { GLFW_KEY_DOWN, KeyCode::Down },
                    { GLFW_KEY_LEFT_SHIFT, KeyCode::LeftShift },
                    { GLFW_KEY_RIGHT_SHIFT, KeyCode::RightShift },
                    { GLFW_KEY_ESCAPE, KeyCode::Escape },
                    { GLFW_KEY_KP_ADD, KeyCode::A },
                    { GLFW_KEY_COMMA, KeyCode::Comma },
                    { GLFW_KEY_BACKSPACE, KeyCode::Backspace },
                    { GLFW_KEY_ENTER, KeyCode::Enter },
                    { GLFW_KEY_LEFT_SUPER, KeyCode::LeftSuper },
                    { GLFW_KEY_RIGHT_SUPER, KeyCode::RightSuper },
                    { GLFW_KEY_LEFT_ALT, KeyCode::LeftAlt },
                    { GLFW_KEY_RIGHT_ALT, KeyCode::RightAlt },
                    { GLFW_KEY_LEFT_CONTROL, KeyCode::LeftControl },
                    { GLFW_KEY_RIGHT_CONTROL, KeyCode::RightControl },
                    { GLFW_KEY_TAB, KeyCode::Tab }
            };

            return keyMap[glfwKey];
        }

        static MouseKeyCode GLFWToMouseKey(uint32_t glfwKey)
        {

            static std::map<uint32_t, MouseKeyCode> keyMap = {
                    { GLFW_MOUSE_BUTTON_LEFT, MouseKeyCode::ButtonLeft },
                    { GLFW_MOUSE_BUTTON_RIGHT, MouseKeyCode::ButtonRight },
                    { GLFW_MOUSE_BUTTON_MIDDLE, MouseKeyCode::ButtonMiddle }
            };

            return keyMap[glfwKey];
        }
    }

}