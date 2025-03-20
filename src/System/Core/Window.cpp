#include "stdafx.hpp"
#include "System/Core/Window.hpp"

zh::Window::Window(const int width, const int height, const std::string &title)
    : window(nullptr), surface(VK_NULL_HANDLE), width(width), height(height)
{
    assert(width > 0 && height > 0 && "zh::Window::Window: WINDOW DIMENSIONS CANNOT BE ZERO OR NEGATIVE");

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
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

VkExtent2D zh::Window::getExtent()
{
    return VkExtent2D{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
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
