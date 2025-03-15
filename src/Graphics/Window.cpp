#include "stdafx.hpp"
#include "Graphics/Window.hpp"

zh::Window::Window(GLFWwindow *window, VkSurfaceKHR &surface) : window(window), surface(surface)
{
}

void zh::Window::pollEvents()
{
    glfwPollEvents();
}

const bool zh::Window::shouldClose()
{
    return glfwWindowShouldClose(window);
}
