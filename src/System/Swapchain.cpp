#include "stdafx.hpp"
#include "System/Swapchain.hpp"

zh::Swapchain::Swapchain(Device &device, Window &window) : device(device), window(window)
{
    create();
}

VkRenderPass &zh::Swapchain::getRenderPass()
{
    return renderPass;
}

zh::Swapchain::~Swapchain()
{
    vkDestroyRenderPass(device.getLogicalDevice(), renderPass, nullptr);

    for (auto &framebuffer : swapchainFramebuffers)
        vkDestroyFramebuffer(device.getLogicalDevice(), framebuffer, nullptr);

    for (auto &view : swapchainImageViews)
        vkDestroyImageView(device.getLogicalDevice(), view, nullptr);

    vkDestroySwapchainKHR(device.getLogicalDevice(), swapchain, nullptr);
}

void zh::Swapchain::create()
{
    createSwapchain();
    createImageViews();
    createFramebuffers();
    createRenderPass();
}

void zh::Swapchain::createSwapchain()
{
    Device::SwapchainSupportDetails swap_chain_support = device.querySwapchainSupport();
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

    Device::QueueFamilyIndices indices = device.findQueueFamilies();
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
    swapchainImages.resize(image_count);
    vkGetSwapchainImagesKHR(device.getLogicalDevice(), swapchain, &image_count, swapchainImages.data());

    swapchainImageFormat = format.format;
    swapchainExtent = extent;
}

void zh::Swapchain::createImageViews()
{
    swapchainImageViews.resize(swapchainImages.size());

    for (int i = 0; i < swapchainImages.size(); ++i)
    {
        VkImageViewCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = swapchainImages[i];
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = swapchainImageFormat;
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device.getLogicalDevice(), &create_info, nullptr, &swapchainImageViews[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("zh::Swapchain::createImageViews: FAILED TO CREATE IMAGE VIEW");
        }
    }
}

void zh::Swapchain::createFramebuffers()
{
    swapchainFramebuffers.resize(swapchainImageViews.size());

    for (size_t i = 0; i < swapchainImageViews.size(); ++i)
    {
        VkImageView attachments[] = {swapchainImageViews[i]};

        VkFramebufferCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        create_info.renderPass = renderPass;
        create_info.attachmentCount = 1;
        create_info.pAttachments = attachments;
        create_info.width = swapchainExtent.width;
        create_info.height = swapchainExtent.height;
        create_info.layers = 1;

        if (vkCreateFramebuffer(device.getLogicalDevice(), &create_info, nullptr, &swapchainFramebuffers[i]) != VK_SUCCESS)
            throw std::runtime_error("zh::Swapchain::createImageViews: FAILED TO CREATE FRAMEBUFFER");
    }
}

void zh::Swapchain::createRenderPass()
{
    VkAttachmentDescription color_attachment{};
    color_attachment.format = swapchainImageFormat;
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
