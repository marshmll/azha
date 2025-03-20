#pragma once

#include "System/Core/Device.hpp"
#include "System/Core/Window.hpp"

namespace zh
{
class Swapchain
{
  public:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    Swapchain(Device &device, Window &window);

    Swapchain(Device &device, Window &window, VkSwapchainKHR old_swapchain);

    ~Swapchain();

    Swapchain() = delete;
    Swapchain(const Swapchain &) = delete;
    Swapchain operator=(const Swapchain &) = delete;

    const bool compareSwapFormats(const Swapchain &swapchain) const;

    VkResult acquireNextImage(uint32_t *image_index);

    VkResult submitCommandBuffers(const VkCommandBuffer &buffers, uint32_t &image_index);

    VkSwapchainKHR &getHandle();

    VkRenderPass &getRenderPass();

    VkExtent2D &getExtent();

    const VkFormat &getDepthFormat() const;

    const VkFormat &getImageFormat() const;

    const float getAspectRatio() const;

    VkFramebuffer &getFramebuffer(const int index);

  private:
    Device &device;
    Window &window;

    VkSwapchainKHR swapchain;
    VkSwapchainKHR oldSwapchain;

    VkFormat imageFormat;
    VkFormat depthFormat;
    VkExtent2D extent;

    VkRenderPass renderPass;

    std::vector<VkImage> depthImages;
    std::vector<VmaAllocation> depthImagesMemory;
    std::vector<VkImageView> depthImageViews;

    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;

    std::vector<VkFramebuffer> framebuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    size_t currentFrame;

    void create();

    void createSwapchain();

    void createImageViews();

    void createDepthResources();

    void createRenderPass();

    void createFramebuffers();

    void createSyncObjects();

    VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &available_formats);

    VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR> &available_modes);

    VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR &capabilities);

    VkFormat findDepthFormat();
};
} // namespace zh
