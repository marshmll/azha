#include "stdafx.hpp"
#include "System/Vulkan.hpp"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PUBLIC METHODS //////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

zh::Vulkan::Vulkan() : physicalDevice(VK_NULL_HANDLE), window(nullptr), cleaned(false)
{
    initValidationLayers();
    initDeviceExtensions();
    initGLFW();
    initVulkanInstance();
    initDebugMessenger();
}

zh::Vulkan::~Vulkan()
{
    cleanup();
}

void zh::Vulkan::cleanup()
{
    if (!cleaned)
    {
        if (enableValidationLayers)
            proxyDestroyDebugUtilsMessengerEXT(vkInstance, debugMessenger, nullptr);

        if (window != nullptr)
        {
            for (auto &view : swapchainImageViews)
                vkDestroyImageView(device, view, nullptr);

            vkDestroySwapchainKHR(device, swapchain, nullptr);
            vkDestroySurfaceKHR(vkInstance, surface, nullptr);
            glfwDestroyWindow(window);
            vkDestroyDevice(device, nullptr);
        }

        vkDestroyInstance(vkInstance, nullptr);
        glfwTerminate();
        cleaned = true;
    }
}

const uint32_t zh::Vulkan::getExtensionCount()
{
    uint32_t extension_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);

    return extension_count;
}

zh::Window zh::Vulkan::createWindow(const unsigned int width, const unsigned int height, const std::string &title)
{
    if (window != nullptr)
        throw std::runtime_error("Failed to create window. Only one window instance is allowed.");

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

    if (glfwCreateWindowSurface(vkInstance, window, nullptr, &surface))
    {
        throw std::runtime_error("Failed to create window surface.");
    }

    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();

    return Window(window, surface);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PRIVATE METHODS /////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

void zh::Vulkan::initValidationLayers()
{
#ifdef NDEBUG
    enableValidationLayers = false;
#else
    enableValidationLayers = true;
#endif

    validationLayers = {"VK_LAYER_KHRONOS_validation"};
}

void zh::Vulkan::initDeviceExtensions()
{
    deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
}

void zh::Vulkan::initGLFW()
{
    glfwInit();
}

void zh::Vulkan::initVulkanInstance()
{
    // Check Validation Layers
    if (enableValidationLayers && !checkValidationLayerSupport())
        throw std::runtime_error("Some validation layers were requested but are not available.");

    // Aplication info
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Azha alpha v0.0.1";
    app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    app_info.pEngineName = "Azha";
    app_info.engineVersion = VK_MAKE_VERSION(0, 0, 1);
    app_info.apiVersion = VK_API_VERSION_1_0;

    // Create info
    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    // Global Extensions
    auto glfw_extensions = getRequiredExtensions();

#ifdef __APPLE__
    // Avoid VK_ERROR_INCOMPATIBLE_DRIVER
    glfw_extensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    create_info.enabledExtensionCount = static_cast<uint32_t>(glfw_extensions.size());
    create_info.ppEnabledExtensionNames = glfw_extensions.data();
#else
    create_info.enabledExtensionCount = static_cast<uint32_t>(glfw_extensions.size());
    create_info.ppEnabledExtensionNames = glfw_extensions.data();
#endif

    // Validation Layers
    VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
    if (enableValidationLayers)
    {
        create_info.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        create_info.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debug_create_info);
        create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debug_create_info;
    }
    else
    {
        create_info.enabledLayerCount = 0;
        create_info.pNext = nullptr;
    }

    if (vkCreateInstance(&create_info, nullptr, &vkInstance) != VK_SUCCESS)
        throw std::runtime_error("Failed to create a Vulkan Instance.");
}

void zh::Vulkan::initDebugMessenger()
{
    if (!enableValidationLayers)
        return;

    VkDebugUtilsMessengerCreateInfoEXT create_info;
    populateDebugMessengerCreateInfo(create_info);

    if (proxyCreateDebugUtilsMessengerEXT(vkInstance, &create_info, nullptr, &debugMessenger) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to initialize debug messenger.");
    }
}

void zh::Vulkan::pickPhysicalDevice()
{
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(vkInstance, &device_count, nullptr);

    if (device_count == 0)
    {
        throw std::runtime_error("No GPU's with Vulkan support found.");
    }

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(vkInstance, &device_count, devices.data());

    std::multimap<int, VkPhysicalDevice> candidate_devices;

    for (auto &device : devices)
    {
        int score = rateDeviceSuitability(device);
        candidate_devices.insert(std::make_pair(score, device));
    }

    if (candidate_devices.rbegin()->first >= 0)
    {
        physicalDevice = candidate_devices.rbegin()->second;
    }
    else
    {
        throw std::runtime_error("Failed to find suitable GPU(s)");
    }
}

void zh::Vulkan::createLogicalDevice()
{
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    std::set<uint32_t> unique_queue_families = {indices.getGraphicsFamily(), indices.getPresentFamily()};

    float queue_priority = 1.f;

    for (auto &queue_family : unique_queue_families)
    {
        VkDeviceQueueCreateInfo queue_create_info{};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = queue_family;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_priority;

        queue_create_infos.push_back(queue_create_info);
    }

    VkPhysicalDeviceFeatures device_features{};

    VkDeviceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_create_infos.size());
    create_info.pQueueCreateInfos = queue_create_infos.data();
    create_info.pEnabledFeatures = &device_features;
    create_info.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    create_info.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers)
    {
        create_info.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        create_info.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
        create_info.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physicalDevice, &create_info, nullptr, &device) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a logical device from the current physical device.");
    }

    // Retrieve Device Queues
    vkGetDeviceQueue(device, indices.getGraphicsFamily(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.getPresentFamily(), 0, &presentQueue);
}

void zh::Vulkan::createSwapChain()
{
    SwapChainSupportDetails swap_chain_support = querySwapChainSupport(physicalDevice);
    VkSurfaceFormatKHR format = chooseSwapSurfaceFormat(swap_chain_support.formats);
    VkPresentModeKHR mode = chooseSwapPresentMode(swap_chain_support.presentModes);
    VkExtent2D extent = chooseSwapExtent(swap_chain_support.capabilities);
    uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;

    if (swap_chain_support.capabilities.maxImageCount > 0 &&
        image_count > swap_chain_support.capabilities.maxImageCount)
        image_count = swap_chain_support.capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = surface;
    create_info.minImageCount = image_count;
    create_info.imageFormat = format.format;
    create_info.imageColorSpace = format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.getGraphicsFamily(), indices.getPresentFamily()};

    if (indices.graphicsFamily != indices.presentFamily)
    {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queueFamilyIndices;
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

    if (vkCreateSwapchainKHR(device, &create_info, nullptr, &swapchain) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a swapchain.");
    }

    vkGetSwapchainImages(device, swapchain, &image_count, nullptr);
    swapchainImages.resize(image_count);
    vkGetSwapchainImages(device, swapchain, &image_count, swapchainImages.data());

    swapchainImageFormat = format;
    swapchainExtent = extent;
}

void zh::Vulkan::createImageViews()
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

        if (vkCreateImageView(device, &create_info, nullptr, &swapchainImageViews[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create an image view.");
        }
    }
}

const int zh::Vulkan::rateDeviceSuitability(VkPhysicalDevice device) const
{
    int score = 0;

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceProperties(device, &properties);
    vkGetPhysicalDeviceFeatures(device, &features);

    // Prefer discrete GPU over integrated GPU.
    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        score += 1000;
    }
    else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
    {
        score += 200;
    }

    // Max possible texture size to max graphics quality.
    score += properties.limits.maxImageDimension2D;

    // Require geometry shader.
    if (!features.geometryShader)
        score = -1;

    // Require a device with a valid queue familiy.
    if (!findQueueFamilies(device).isComplete())
        score = -1;

    // Require a device with extensions support.
    if (!checkDeviceExtensionSupport(device))
    {
        score = -1;
    }
    // Require that swap chain is adequate
    else
    {
        SwapChainSupportDetails details = querySwapChainSupport(device);

        if (details.formats.empty() || details.presentModes.empty())
            score = -1;
    }

    return score;
}

const zh::QueueFamilyIndices zh::Vulkan::findQueueFamilies(VkPhysicalDevice device) const
{
    QueueFamilyIndices indices;

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, queue_families.data());

    for (int i = 0; i < queue_families.size(); ++i)
    {
        // Require VK_QUEUE_GRAPHICS_BIT
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            indices.graphicsFamily = i;

        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &present_support);

        if (present_support)
            indices.presentFamily = i;

        if (indices.isComplete())
            break;
    }

    return indices;
}

const zh::SwapChainSupportDetails zh::Vulkan::querySwapChainSupport(VkPhysicalDevice device) const
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, nullptr);

    if (format_count != 0)
    {
        details.formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count, details.formats.data());
    }

    uint32_t present_modes_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_modes_count, nullptr);

    if (present_modes_count != 0)
    {
        details.presentModes.resize(format_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &present_modes_count, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR zh::Vulkan::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &available_formats)
{
    for (auto const &format : available_formats)
    {
        if (format.format == VK_FORMAT_B8G8R8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return format;
    }

    return available_formats[0];
}

VkPresentModeKHR zh::Vulkan::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &available_modes)
{
    for (auto const &mode : available_modes)
    {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
            return mode;
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D zh::Vulkan::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        return capabilities.currentExtent;
    }

    int width, height;

    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actual_extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

    actual_extent.width =
        std::clamp(actual_extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actual_extent.height =
        std::clamp(actual_extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actual_extent;
}

const bool zh::Vulkan::checkDeviceExtensionSupport(VkPhysicalDevice device) const
{
    uint32_t extension_count = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

    std::vector<VkExtensionProperties> available_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

    std::set<std::string> required_extensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto &extension : available_extensions)
    {
        required_extensions.erase(extension.extensionName);
    }

    return required_extensions.empty();
}

void zh::Vulkan::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &create_info)
{
    create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    create_info.pfnUserCallback = zh::Vulkan::debugCallback;
    create_info.pUserData = nullptr;
}

const bool zh::Vulkan::checkValidationLayerSupport()
{
    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

    std::vector<VkLayerProperties> available_layers(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

    for (const char *layer_name : validationLayers)
    {
        bool layer_found = false;

        for (const auto &layer_properties : available_layers)
        {
            if (strcmp(layer_name, layer_properties.layerName) == 0)
            {
                layer_found = true;
                break;
            }
        }

        if (!layer_found)
            return false;
    }

    return true;
}

std::vector<const char *> zh::Vulkan::getRequiredExtensions()
{
    uint32_t glfw_extension_count = 0;
    const char **glfw_extensions;

    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    std::vector<const char *> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

    if (enableValidationLayers)
    {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// STATIC METHODS //////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

VKAPI_ATTR VkBool32 VKAPI_CALL zh::Vulkan::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                         VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                         const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                         void *pUserData)
{
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
        std::cout << "[ DIAL ] (Validation Layer) " << pCallbackData->pMessage << std::endl;

    else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        std::cout << "[ INFO ] (Validation Layer) " << pCallbackData->pMessage << std::endl;

    else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        std::cout << "[ WARN ] (Validation Layer) " << pCallbackData->pMessage << std::endl;

    else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        std::cerr << "[ ERROR ] (Validation Layer) " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

VkResult zh::Vulkan::proxyCreateDebugUtilsMessengerEXT(VkInstance instance,
                                                       const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                                       const VkAllocationCallbacks *pAllocator,
                                                       VkDebugUtilsMessengerEXT *pDebugMessenger)
{
    auto fn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

    if (fn != nullptr)
    {
        return fn(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void zh::Vulkan::proxyDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                                    const VkAllocationCallbacks *pAllocator)
{
    auto fn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

    if (fn != nullptr)
    {
        fn(instance, debugMessenger, pAllocator);
    }
}
