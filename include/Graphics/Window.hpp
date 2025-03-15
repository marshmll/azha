#pragma once

namespace zh
{
class Window
{
  public:
    Window(GLFWwindow *window, VkSurfaceKHR &surface);

    void pollEvents();

    const bool shouldClose();

  private:
    GLFWwindow *window;
    VkSurfaceKHR &surface;
};
} // namespace zh
