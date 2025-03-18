#include "stdafx.hpp"
#include "System/Window.hpp"

zh::Window::Window(const unsigned short width, const unsigned short height, const std::string &title)
    : window(nullptr), surface(VK_NULL_HANDLE)
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow((int)width, (int)height, title.c_str(), nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferResizedCallback);
}

zh::Window::~Window()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

void zh::Window::pollEvents()
{
    glfwPollEvents();
}

const bool zh::Window::shouldClose()
{
    assert(window != nullptr && "zh::Window::shouldClose: WINDOW WAS NOT INITIALIZED");

    return glfwWindowShouldClose(window);
}

void zh::Window::createSurface(VkInstance &instance)
{
    assert(window != nullptr && "zh::Window::createSurface: WINDOW WAS NOT INITIALIZED");

    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        throw std::runtime_error("zh::Window::createSurface: FAILED TO CREATE WINDOW SURFACE");
}

VkSurfaceKHR &zh::Window::getSurface()
{
    return surface;
}

const bool zh::Window::getFramebufferResized() const
{
    return framebufferResized;
}

void zh::Window::setFramebufferResized(const bool resized)
{
    framebufferResized = resized;
}

void zh::Window::framebufferResizedCallback(GLFWwindow *window, int width, int height)
{
    auto pWindow = reinterpret_cast<zh::Window *>(glfwGetWindowUserPointer(window));
    pWindow->setFramebufferResized(true);
}

GLFWwindow *&zh::Window::getHandle()
{
    return window;
}
