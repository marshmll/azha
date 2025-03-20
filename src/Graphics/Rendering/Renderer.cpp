#include "stdafx.hpp"
#include "Graphics/Rendering/Renderer.hpp"

zh::Renderer::Renderer(Device &device, Window &window) : device(device), window(window)
{
    recreateSwapchain();
    createCommandBuffers();
}

zh::Renderer::~Renderer()
{
    freeCommandBuffers();
}

VkRenderPass &zh::Renderer::getSwapchainRenderPass() const
{
    return swapchain->getRenderPass();
}

const float zh::Renderer::getAspectRatio() const
{
    return swapchain->getAspectRatio();
}

const bool zh::Renderer::isFrameInProgress() const
{
    return isFrameStarted;
}

VkCommandBuffer zh::Renderer::getCurrentCommandBuffer()
{
    if (!isFrameStarted)
        throw std::runtime_error(
            "zh::Renderer::getCurrentCommandBuffer: GETTING COMMAND BUFFER IS ALLOWED ONLY WHEN FRAME IS IN PROGRESS");

    return commandBuffers[currentFrameIndex];
}

const int zh::Renderer::getFrameIndex() const
{
    if (!isFrameStarted)
        throw std::runtime_error(
            "zh::Renderer::getFrameIndex: GETTING FRAME INDEX IS ALLOWED ONLY WHEN FRAME IS IN PROGRESS");

    return currentFrameIndex;
}

VkCommandBuffer zh::Renderer::beginFrame()
{
    if (isFrameStarted)
        throw std::runtime_error("zh::Renderer::beginFrame: CANNOT BEGIN FRAME WHEN ANOTHER FRAME IS IN PROGRESS");

    auto result = swapchain->acquireNextImage(&currentImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapchain();
        return nullptr;
    }

    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        throw std::runtime_error("zh::Renderer::beginFrame: FAILED TO ACQUIRE SWAPCHAIN IMAGE");

    isFrameStarted = true;

    auto command_buffer = getCurrentCommandBuffer();
    VkCommandBufferBeginInfo begin_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
        throw std::runtime_error("zh::Renderer::beginFrame FAILED TO BEGIN RECORDING COMMAND BUFFER");

    return command_buffer;
}

void zh::Renderer::endFrame()
{
    if (!isFrameStarted)
        throw std::runtime_error("zh::Renderer::endFrame: CANNOT END FRAME WHEN NO FRAME IS IN PROGRESS");

    auto command_buffer = getCurrentCommandBuffer();

    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
        throw std::runtime_error("zh::Renderer::endFrame: FAILED TO RECORD COMMAND BUFFER");

    auto result = swapchain->submitCommandBuffers(command_buffer, currentImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window.getFramebufferResized())
    {
        window.setFramebufferResized(false);
        recreateSwapchain();
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("zh::Renderer::endFrame: FAILED TO PRESENT SWAPCHAIN IMAGE");
    }

    isFrameStarted = false;
    currentFrameIndex = (currentFrameIndex + 1) % Swapchain::MAX_FRAMES_IN_FLIGHT;
}

void zh::Renderer::beginSwapchainRenderPass(VkCommandBuffer &command_buffer)
{
    if (isFrameStarted)
        throw std::runtime_error("zh::Renderer::beginSwapchainRenderPass: CANNOT BEGIN SWAPCHAIN RENDER PASS WHEN "
                                 "ANOTHER FRAME IS IN PROGRESS");

    if (command_buffer != getCurrentCommandBuffer())
        throw std::runtime_error("zh::Renderer::beginSwapchainRenderPass: CANNOT BEGIN RENDER PASS ON A COMMAND BUFFER "
                                 "FROM A DIFFERENT FRAME");

    VkRenderPassBeginInfo render_pass_info{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    render_pass_info.renderPass = swapchain->getRenderPass();
    render_pass_info.framebuffer = swapchain->getFramebuffer(currentImageIndex);

    render_pass_info.renderArea.offset = {0, 0};
    render_pass_info.renderArea.extent = swapchain->getExtent();

    std::array<VkClearValue, 2> clear_values{};
    clear_values[0].color = {0.01f, 0.01f, 0.01f, 1.0f};
    clear_values[1].depthStencil = {1.0f, 0};
    render_pass_info.clearValueCount = static_cast<uint32_t>(clear_values.size());
    render_pass_info.pClearValues = clear_values.data();

    vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchain->getExtent().width);
    viewport.height = static_cast<float>(swapchain->getExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{{0, 0}, swapchain->getExtent()};

    vkCmdSetViewport(command_buffer, 0, 1, &viewport);
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);
}

void zh::Renderer::endSwapchainRenderPass(VkCommandBuffer &command_buffer)
{
    if (isFrameStarted)
        throw std::runtime_error(
            "zh::Renderer::endSwapchainRenderPass: CANNOT END SWAPCHAIN RENDER PASS WHEN NO FRAME IS IN PROGRESS");

    if (command_buffer != getCurrentCommandBuffer())
        throw std::runtime_error(
            "zh::Renderer::endSwapchainRenderPass: CANNOT END RENDER PASS ON A COMMAND BUFFER FROM A DIFFERENT FRAME");

    vkCmdEndRenderPass(command_buffer);
}

void zh::Renderer::createCommandBuffers()
{
    commandBuffers.resize(Swapchain::MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo alloc_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = device.getCommandPool();
    alloc_info.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(device.getLogicalDevice(), &alloc_info, commandBuffers.data()) != VK_SUCCESS)
        throw std::runtime_error("zh::Renderer::createCommandBuffers: FAILED TO ALLOCATE COMMAND BUFFERS");
}

void zh::Renderer::freeCommandBuffers()
{
    vkFreeCommandBuffers(device.getLogicalDevice(), device.getCommandPool(),
                         static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

    commandBuffers.clear();
}

void zh::Renderer::recreateSwapchain()
{
    auto extent = window.getExtent();

    while (extent.width == 0 || extent.height == 0)
    {
        extent = window.getExtent();
        glfwWaitEvents();
    }
    vkDeviceWaitIdle(device.getLogicalDevice());

    if (swapchain == nullptr)
    {
        swapchain = std::make_unique<Swapchain>(device, window);
    }
    else
    {
        std::shared_ptr<Swapchain> old_swapchain = std::move(swapchain);
        swapchain = std::make_unique<Swapchain>(device, window, old_swapchain->getHandle());

        if (!old_swapchain->compareSwapFormats(*swapchain))
            throw std::runtime_error("zh::Renderer::recreateSwapchain: SWAPCHAIN IMAGE OR DEPTH FORMAT CHANGED");
    }
}
