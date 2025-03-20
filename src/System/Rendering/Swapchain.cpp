#include "stdafx.hpp"
#include "System/Rendering/Swapchain.hpp"

zh::Swapchain::Swapchain(Device &device, Window &window)
    : device(device), window(window), oldSwapchain(VK_NULL_HANDLE), currentFrame(0)
{
    create();
}

zh::Swapchain::Swapchain(Device &device, Window &window, VkSwapchainKHR old_swapchain)
    : device(device), window(window), oldSwapchain(old_swapchain), currentFrame(0)
{
    create();
}

const bool zh::Swapchain::compareSwapFormats(const Swapchain &swapchain) const
{
    return swapchain.getDepthFormat() == getDepthFormat() && swapchain.getImageFormat() == getImageFormat();
}

VkResult zh::Swapchain::acquireNextImage(uint32_t *image_index)
{
    vkWaitForFences(device.getLogicalDevice(), 1, &inFlightFences[currentFrame], VK_TRUE,
                    std::numeric_limits<uint64_t>::max());

    VkResult result = vkAcquireNextImageKHR(device.getLogicalDevice(), swapchain, std::numeric_limits<uint64_t>::max(),
                                            imageAvailableSemaphores[currentFrame], // must be a not signaled semaphore
                                            VK_NULL_HANDLE, image_index);

    return result;
}

VkResult zh::Swapchain::submitCommandBuffers(const VkCommandBuffer &buffers, uint32_t &image_index)
{
    if (imagesInFlight[image_index] != VK_NULL_HANDLE)
        vkWaitForFences(device.getLogicalDevice(), 1, &imagesInFlight[image_index], VK_TRUE, UINT64_MAX);

    imagesInFlight[image_index] = inFlightFences[currentFrame];

    VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};

    VkSemaphore wait_semaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;

    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &buffers;

    VkSemaphore signal_semaphores[] = {renderFinishedSemaphores[currentFrame]};
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    vkResetFences(device.getLogicalDevice(), 1, &inFlightFences[currentFrame]);

    if (vkQueueSubmit(device.getGraphicsQueue(), 1, &submit_info, inFlightFences[currentFrame]) != VK_SUCCESS)
        throw std::runtime_error("zh::Swapchain::submitCommandBuffers: FAILED TO SUBMIT DRAW COMMAND BUFFER");

    VkPresentInfoKHR present_info = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;

    VkSwapchainKHR swapchains[] = {swapchain};
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapchains;

    present_info.pImageIndices = &image_index;

    auto result = vkQueuePresentKHR(device.getPresentQueue(), &present_info);

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

    return result;
}

VkSwapchainKHR &zh::Swapchain::getHandle()
{
    return swapchain;
}

VkRenderPass &zh::Swapchain::getRenderPass()
{
    return renderPass;
}

VkExtent2D &zh::Swapchain::getExtent()
{
    return extent;
}

const VkFormat &zh::Swapchain::getDepthFormat() const
{
    return depthFormat;
}

const VkFormat &zh::Swapchain::getImageFormat() const
{
    return imageFormat;
}

const float zh::Swapchain::getAspectRatio() const
{
    return static_cast<float>(extent.width) / static_cast<float>(extent.height);
}

VkFramebuffer &zh::Swapchain::getFramebuffer(const int index)
{
    if (index >= framebuffers.size() || index < 0)
        throw std::runtime_error("zh::Swapchain::getFramebuffer: FRAMEBUFFER INDEX OUT OF BOUNDS");

    return framebuffers[index];
}

zh::Swapchain::~Swapchain()
{
    vkDeviceWaitIdle(device.getLogicalDevice());

    vkDestroyRenderPass(device.getLogicalDevice(), renderPass, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(device.getLogicalDevice(), renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device.getLogicalDevice(), imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device.getLogicalDevice(), inFlightFences[i], nullptr);
    }

    for (auto &framebuffer : framebuffers)
        vkDestroyFramebuffer(device.getLogicalDevice(), framebuffer, nullptr);

    for (int i = 0; i < depthImages.size(); i++)
    {
        vkDestroyImageView(device.getLogicalDevice(), depthImageViews[i], nullptr);
        vmaDestroyImage(device.getAllocator(), depthImages[i], depthImagesMemory[i]);
    }

    for (auto &view : imageViews)
        vkDestroyImageView(device.getLogicalDevice(), view, nullptr);

    vkDestroySwapchainKHR(device.getLogicalDevice(), swapchain, nullptr);
}

void zh::Swapchain::create()
{
    createSwapchain();
    createImageViews();
    createDepthResources();
    createRenderPass();
    createFramebuffers();
    createSyncObjects();
}

void zh::Swapchain::createSwapchain()
{
    Device::SwapchainSupportDetails swap_chain_support = device.querySwapchainSupport(device.getPhysicalDevice());
    VkSurfaceFormatKHR format = chooseSurfaceFormat(swap_chain_support.formats);
    VkPresentModeKHR mode = choosePresentMode(swap_chain_support.presentModes);
    VkExtent2D extent = chooseExtent(swap_chain_support.capabilities);
    uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;

    if (swap_chain_support.capabilities.maxImageCount > 0 &&
        image_count > swap_chain_support.capabilities.maxImageCount)
        image_count = swap_chain_support.capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = window.getSurface();
    create_info.minImageCount = image_count;
    create_info.imageFormat = format.format;
    create_info.imageColorSpace = format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    Device::QueueFamilyIndices indices = device.findQueueFamilies(device.getPhysicalDevice());
    uint32_t queue_family_indices[] = {indices.getGraphicsFamily(), indices.getPresentFamily()};

    if (indices.graphicsFamily != indices.presentFamily)
    {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices;
    }
    else
    {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices = nullptr;
    }

    create_info.preTransform = swap_chain_support.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = mode;
    create_info.clipped = VK_TRUE;
    create_info.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device.getLogicalDevice(), &create_info, nullptr, &swapchain) != VK_SUCCESS)
        throw std::runtime_error("Failed to create a swapchain.");

    vkGetSwapchainImagesKHR(device.getLogicalDevice(), swapchain, &image_count, nullptr);
    images.resize(image_count);
    vkGetSwapchainImagesKHR(device.getLogicalDevice(), swapchain, &image_count, images.data());

    imageFormat = format.format;
    this->extent = extent;
}

void zh::Swapchain::createImageViews()
{
    imageViews.resize(images.size());

    for (int i = 0; i < images.size(); ++i)
    {
        VkImageViewCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = images[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = imageFormat;
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device.getLogicalDevice(), &create_info, nullptr, &imageViews[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("zh::Swapchain::createImageViews: FAILED TO CREATE IMAGE VIEW");
        }
    }
}

void zh::Swapchain::createDepthResources()
{
    VkFormat depth_format = findDepthFormat();
    depthFormat = depth_format;
    VkExtent2D swapchain_extent = extent;

    depthImages.resize(images.size());
    depthImagesMemory.resize(images.size());
    depthImageViews.resize(images.size());

    for (int i = 0; i < depthImages.size(); i++)
    {
        VkImageCreateInfo image_info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
        image_info.imageType = VK_IMAGE_TYPE_2D;
        image_info.extent.width = swapchain_extent.width;
        image_info.extent.height = swapchain_extent.height;
        image_info.extent.depth = 1;
        image_info.mipLevels = 1;
        image_info.arrayLayers = 1;
        image_info.format = depthFormat;
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
        image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        image_info.samples = VK_SAMPLE_COUNT_1_BIT;
        image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image_info.flags = 0;

        device.createImageWithInfo(image_info, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImages[i],
                                   depthImagesMemory[i]);

        VkImageViewCreateInfo view_info{};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = depthImages[i];
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = depthFormat;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        view_info.subresourceRange.baseMipLevel = 0;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.baseArrayLayer = 0;
        view_info.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device.getLogicalDevice(), &view_info, nullptr, &depthImageViews[i]) != VK_SUCCESS)
            throw std::runtime_error("zh::Swapchain::createDepthResources: FAILED TO CREATE TEXTURE IMAGE VIEW");
    }
}

void zh::Swapchain::createRenderPass()
{
    VkAttachmentDescription color_attachment{};
    color_attachment.format = imageFormat;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_reference{};
    color_attachment_reference.attachment = 0;
    color_attachment_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_reference;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    if (vkCreateRenderPass(device.getLogicalDevice(), &render_pass_info, nullptr, &renderPass) != VK_SUCCESS)
        throw std::runtime_error("zh::Swapchain::createImageViews: FAILED TO CREATE RENDER PASS");
}

void zh::Swapchain::createFramebuffers()
{
    framebuffers.resize(imageViews.size());

    for (size_t i = 0; i < imageViews.size(); ++i)
    {
        VkImageView attachments[] = {imageViews[i]};

        VkFramebufferCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        create_info.renderPass = renderPass;
        create_info.attachmentCount = 1;
        create_info.pAttachments = attachments;
        create_info.width = extent.width;
        create_info.height = extent.height;
        create_info.layers = 1;

        if (vkCreateFramebuffer(device.getLogicalDevice(), &create_info, nullptr, &framebuffers[i]) != VK_SUCCESS)
            throw std::runtime_error("zh::Swapchain::createImageViews: FAILED TO CREATE FRAMEBUFFER");
    }
}

void zh::Swapchain::createSyncObjects()
{
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    imagesInFlight.resize(images.size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

    VkFenceCreateInfo fence_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        if (vkCreateSemaphore(device.getLogicalDevice(), &semaphore_info, nullptr, &imageAvailableSemaphores[i]) !=
                VK_SUCCESS ||
            vkCreateSemaphore(device.getLogicalDevice(), &semaphore_info, nullptr, &renderFinishedSemaphores[i]) !=
                VK_SUCCESS ||
            vkCreateFence(device.getLogicalDevice(), &fence_info, nullptr, &inFlightFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("zh::Swapchain::createSyncObjects: FAILED TO CREATE SYNC OBJECTS FOR A FRAME");
        }
    }
}

VkSurfaceFormatKHR zh::Swapchain::chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &available_formats)
{
    for (auto const &format : available_formats)
    {
        if (format.format == VK_FORMAT_B8G8R8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return format;
    }

    return available_formats[0];
}

VkPresentModeKHR zh::Swapchain::choosePresentMode(const std::vector<VkPresentModeKHR> &available_modes)
{
    for (auto const &mode : available_modes)
    {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
            return mode;
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D zh::Swapchain::chooseExtent(const VkSurfaceCapabilitiesKHR &capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;

    int width, height;

    glfwGetFramebufferSize(window.getHandle(), &width, &height);

    VkExtent2D actual_extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

    actual_extent.width =
        std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actual_extent.height =
        std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actual_extent;
}

VkFormat zh::Swapchain::findDepthFormat()
{
    return device.findSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                                      VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}
