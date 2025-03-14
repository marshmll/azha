#pragma once

namespace zh
{
class Window
{
  private:
    GLFWwindow *window;

  public:
    Window(const unsigned int width, const unsigned int height, const std::string &title);

    ~Window();

    void pollEvents();

    const bool shouldClose();
};
} // namespace zh
