//
// Created by 80529 on 2023/7/2.
//

#ifndef NEKOENGINE_WINDOWSYSTEM_H
#define NEKOENGINE_WINDOWSYSTEM_H

#include <GLFW/glfw3.h>

namespace NekoEngine
{
    class WindowSystem
    {
    private:
        GLFWwindow* window;
    public:
        WindowSystem(){};

        ~WindowSystem()
        {
            glfwDestroyWindow(window);
            glfwTerminate();
        }
        void Init();
        bool CreateWindow();
        bool ShouldClose();
    };

} // NekoEngine

#endif //NEKOENGINE_WINDOWSYSTEM_H
