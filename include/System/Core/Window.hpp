#pragma once

namespace zh
{
class Window
{
  public:
    Window(const int width, const int height, const std::string &title);

    Window() = delete;

    Window(const Window &) = delete;

    Window &operator=(const Window &) = delete;

    ~Window();

    void pollEvents();

    const bool shouldClose();

    void createSurface(VkInstance &instance);

    GLFWwindow *&getHandle();

    VkSurfaceKHR &getSurface();

    VkExtent2D getExtent();

    const bool getFramebufferResized() const;

    void setFramebufferResized(const bool resized);

  private:
    GLFWwindow *window;
    VkSurfaceKHR surface;

    int width;
    int height;
    bool framebufferResized;

    static void framebufferResizedCallback(GLFWwindow *window, int width, int height);
};
} // namespace zh
