//
// Created by 80529 on 2023/7/19.
//

#include "Window.h"

namespace NekoEngine
{
    Window* Window::instance = nullptr;

    Window::~Window()
    {
        swapChain.reset();
    }
} // NekoEngine