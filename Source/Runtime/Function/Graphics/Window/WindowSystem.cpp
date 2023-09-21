//
// Created by 80529 on 2023/7/2.
//

#include "WindowSystem.h"
#include "iostream"
namespace NekoEngine
{
//    void processInput(GLFWwindow *window);

// settings
    const unsigned int SCR_WIDTH = 800;
    const unsigned int SCR_HEIGHT = 600;

    void WindowSystem::Init()
    {
        CreateWindow();
    }

    bool WindowSystem::CreateWindow()
    {
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        this->window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", nullptr, nullptr);
        if (window == nullptr)
        {
            std::cout << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return false;
        }
        glfwMakeContextCurrent(window);

        return true;
    }

    bool WindowSystem::ShouldClose()
    {
        return glfwWindowShouldClose(window);
    }

//    void processInput(GLFWwindow *window)
//    {
//        if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
//            glfwSetWindowShouldClose(window, true);
//    }

} // NekoEngine