
#include "Window.h"

namespace NekoEngine
{
    Window* Window::instance = nullptr;

    Window::~Window()
    {
        swapChain.reset();
    }
} // NekoEngine