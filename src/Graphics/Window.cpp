#include "stdafx.hpp"
#include "Graphics/Window.hpp"

zh::Window::Window(const unsigned int width, const unsigned int height, const std::string &title)
{
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
}

zh::Window::~Window()
{
    if (window)
        glfwDestroyWindow(window);
}

void zh::Window::pollEvents()
{
    glfwPollEvents();
}

const bool zh::Window::shouldClose()
{
    return glfwWindowShouldClose(window);
}
