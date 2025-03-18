#pragma once

#include "System/Device.hpp"
#include "System/Window.hpp"

namespace zh
{
class Swapchain
{
  public:
    Swapchain(Device &device, Window &window);

    ~Swapchain();

    Swapchain(const Swapchain &) = delete;
    Swapchain operator=(const Swapchain &) = delete;

    VkRenderPass &getRenderPass();

  private:
    Device &device;
    Window &window;

    VkSwapchainKHR swapchain;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;

    VkRenderPass renderPass;

    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    std::vector<VkFramebuffer> swapchainFramebuffers;

    void create();

    void createSwapchain();

    void createImageViews();

    void createRenderPass();

    void createFramebuffers();

    VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &available_formats);

    VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR> &available_modes);

    VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR &capabilities);
};
} // namespace zh
