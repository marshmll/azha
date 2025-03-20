#pragma once

#include "System/Core/Device.hpp"
#include "System/Rendering/Pipeline.hpp"
#include "System/Rendering/Descriptors.hpp"

namespace zh
{
class Renderer
{
  public:
    Renderer(Device &device, Window &window);

    ~Renderer();

    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;

    VkRenderPass &getSwapchainRenderPass() const;

    const float getAspectRatio() const;

    const bool isFrameInProgress() const;

    VkCommandBuffer getCurrentCommandBuffer();

    const int getFrameIndex() const;

    VkCommandBuffer beginFrame();

    void endFrame();

    void beginSwapchainRenderPass(VkCommandBuffer &command_buffer);

    void endSwapchainRenderPass(VkCommandBuffer &command_buffer);

  private:
    Device &device;
    Window &window;
    std::unique_ptr<Swapchain> swapchain;
    std::vector<VkCommandBuffer> commandBuffers;

    uint32_t currentImageIndex;
    int currentFrameIndex;
    bool isFrameStarted;

    void createCommandBuffers();

    void freeCommandBuffers();

    void recreateSwapchain();
};
} // namespace zh
