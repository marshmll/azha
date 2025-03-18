#include "stdafx.hpp"
#include "System/Device.hpp"

zh::Device::Device(Window &window) : window(window)
{
    nullifyHandles();
    initVulkanInstance();
    initDebugMessenger();
    initWindowSurface();
    pickAdequatePhysicalDevice();
    createLogicalDevice();
    initMemoryAllocator();
    createVertexBuffer(1);
    createIndexBuffer(1);
    createUniformBuffers(1);
}

zh::Device::~Device()
{
    for (size_t i = 0; i < Device::MAX_FRAMES_IN_FLIGHT; i++)
    {
        vmaUnmapMemory(allocator, uniformBuffersMemory[i]);
        vmaDestroyBuffer(allocator, uniformBuffers[i], uniformBuffersMemory[i]);
    }

    vmaDestroyBuffer(allocator, indexBuffer, indexBufferMemory);
    vmaDestroyBuffer(allocator, vertexBuffer, vertexBufferMemory);
    vmaDestroyAllocator(allocator);
    vkDestroySurfaceKHR(instance, window.getSurface(), nullptr);
    vkDestroyDevice(device, nullptr);
#ifndef NDEBUG
    destroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
#endif
    vkDestroyInstance(instance, nullptr);
}

VkPhysicalDevice &zh::Device::getPhysicalDevice()
{
    return physicalDevice;
}

VkDevice &zh::Device::getLogicalDevice()
{
    return device;
}

const bool zh::Device::checkValidationLayerSupport()
{
    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

    std::vector<VkLayerProperties> available_layers(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

    for (const char *layer_name : VALIDATION_LAYERS)
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

std::vector<const char *> zh::Device::getRequiredExtensions()
{
    uint32_t glfw_extension_count = 0;
    const char **glfw_extensions;

    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    std::vector<const char *> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

#ifndef NDEBUG
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
    extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    return extensions;
}

const zh::Device::QueueFamilyIndices zh::Device::findQueueFamilies()
{
    QueueFamilyIndices indices;

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queue_family_count, nullptr);

    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queue_family_count, queue_families.data());

    for (int i = 0; i < queue_families.size(); ++i)
    {
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            indices.graphicsFamily = i;

        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, window.getSurface(), &present_support);

        if (present_support)
            indices.presentFamily = i;

        if (indices.isComplete())
            break;
    }

    return indices;
}

const zh::Device::SwapchainSupportDetails zh::Device::querySwapchainSupport() const
{
    SwapchainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, window.getSurface(), &details.capabilities);

    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, window.getSurface(), &format_count, nullptr);

    if (format_count != 0)
    {
        details.formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, window.getSurface(), &format_count,
                                             details.formats.data());
    }

    uint32_t present_modes_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, window.getSurface(), &present_modes_count, nullptr);

    if (present_modes_count != 0)
    {
        details.presentModes.resize(format_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, window.getSurface(), &present_modes_count,
                                                  details.presentModes.data());
    }

    return details;
}

void zh::Device::nullifyHandles()
{
    instance = VK_NULL_HANDLE;
    debugMessenger = VK_NULL_HANDLE;
    physicalDevice = VK_NULL_HANDLE;
    device = VK_NULL_HANDLE;
    graphicsQueue = VK_NULL_HANDLE;
    presentQueue = VK_NULL_HANDLE;
}

void zh::Device::initVulkanInstance()
{
    // Check Validation Layers
#ifndef NDEBUG
    if (!checkValidationLayerSupport())
        throw std::runtime_error(
            "zh::Device::initVulkanInstance: SOME VALIDATION LAYERS WERE REQUESTED BUT ARE NOT AVAILABLE");
#endif

    // Aplication info
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Azha alpha v0.0.1";
    app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    app_info.pEngineName = "Azha";
    app_info.engineVersion = VK_MAKE_VERSION(0, 0, 1);
    app_info.apiVersion = VK_API_VERSION_1_4;

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
#endif
    create_info.enabledExtensionCount = static_cast<uint32_t>(glfw_extensions.size());
    create_info.ppEnabledExtensionNames = glfw_extensions.data();

    // Validation Layers
    VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
#ifndef NDEBUG
    create_info.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
    create_info.ppEnabledLayerNames = VALIDATION_LAYERS.data();

    populateDebugMessengerCreateInfo(debug_create_info);
    create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debug_create_info;
#else
    create_info.enabledLayerCount = 0;
    create_info.pNext = nullptr;
#endif

    if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS)
        throw std::runtime_error("zh::Device::initVulkanInstance: FAILED TO CREATE VULKAN INSTANCE");
}

void zh::Device::initDebugMessenger()
{
#ifndef NDEBUG
    VkDebugUtilsMessengerCreateInfoEXT create_info;
    populateDebugMessengerCreateInfo(create_info);

    if (createDebugUtilsMessengerEXT(instance, &create_info, nullptr, &debugMessenger) != VK_SUCCESS)
        throw std::runtime_error("zh::Device::initDebugMessenger: FAILED TO INITIALIZE DEBUG MESSENGER");
#endif
}

void zh::Device::initWindowSurface()
{
    window.createSurface(instance);
}

void zh::Device::pickAdequatePhysicalDevice()
{
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

    if (device_count == 0)
        throw std::runtime_error("zh::Device::pickAdequatePhysicalDevice: NO GPU(S) WITH VULKAN SUPPORT FOUND");

    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

    std::multimap<int, VkPhysicalDevice> candidate_devices;

    for (auto &device : devices)
    {
        int score = rateDeviceSuitability(device);
        candidate_devices.insert(std::make_pair(score, device));
    }

    if (candidate_devices.rbegin()->first >= 0)
        physicalDevice = candidate_devices.rbegin()->second;
    else
        throw std::runtime_error("zh::Device::pickAdequatePhysicalDevice: FAILED TO FIND A SUITABLE GPU");
}

void zh::Device::createLogicalDevice()
{
    QueueFamilyIndices indices = findQueueFamilies();

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
    create_info.enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size());
    create_info.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();

#ifndef NDEBUG
    create_info.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
    create_info.ppEnabledLayerNames = VALIDATION_LAYERS.data();
#else
    create_info.enabledLayerCount = 0;
#endif

    if (vkCreateDevice(physicalDevice, &create_info, nullptr, &device) != VK_SUCCESS)
        throw std::runtime_error("zh::Device::createLogicalDevice: FAILED TO CREATE A LOGICAL DEVICE");

    // Retrieve Device Queues
    vkGetDeviceQueue(device, indices.getGraphicsFamily(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.getPresentFamily(), 0, &presentQueue);
}

void zh::Device::initMemoryAllocator()
{
    VmaVulkanFunctions vulkan_functions{};
    vulkan_functions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
    vulkan_functions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocator_create_info{};
    allocator_create_info.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    allocator_create_info.vulkanApiVersion = VK_API_VERSION_1_2;
    allocator_create_info.physicalDevice = physicalDevice;
    allocator_create_info.device = device;
    allocator_create_info.instance = instance;
    allocator_create_info.pVulkanFunctions = &vulkan_functions;

    if (vmaCreateAllocator(&allocator_create_info, &allocator) != VK_SUCCESS)
        throw std::runtime_error("zh::Device::initMemoryAllocator: FAILED TO INITIALIZE VMA MEMORY ALLOCATOR");
}

void zh::Device::createVertexBuffer(VkDeviceSize buffer_size)
{
    VkBuffer staging_buffer;
    VmaAllocation staging_buffer_memory;

    Buffer::stagingBuffer(allocator, buffer_size, staging_buffer, staging_buffer_memory);

    // void *data;

    // vmaMapMemory(allocator, staging_buffer_memory, &data);
    // std::memcpy(data, vertices, static_cast<size_t>(buffer_size));
    // vmaUnmapMemory(allocator, staging_buffer_memory);

    Buffer::create(allocator, buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
                   VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT, vertexBuffer, vertexBufferMemory);

    // Buffer::copy(device, graphicsQueue, staging_buffer, vertexBuffer, buffer_size);

    vmaDestroyBuffer(allocator, staging_buffer, staging_buffer_memory);
}

void zh::Device::createIndexBuffer(VkDeviceSize buffer_size)
{
    VkBuffer staging_buffer;
    VmaAllocation staging_buffer_memory;

    Buffer::stagingBuffer(allocator, buffer_size, staging_buffer, staging_buffer_memory);

    void *data;

    // vmaMapMemory(allocator, staging_buffer_memory, &data);
    // std::memcpy(data, indices, static_cast<size_t>(buffer_size));
    // vmaUnmapMemory(allocator, staging_buffer_memory);

    Buffer::create(allocator, buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
                   VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT, indexBuffer, indexBufferMemory);

    // Buffer::copy(device, graphicsQueue, staging_buffer, indexBuffer, buffer_size);

    vmaDestroyBuffer(allocator, staging_buffer, staging_buffer_memory);
}

void zh::Device::createUniformBuffers(VkDeviceSize buffer_size)
{
    uniformBuffers.resize(buffer_size);
    uniformBuffersMemory.resize(buffer_size);
    uniformBuffersMapped.resize(buffer_size);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        Buffer::create(allocator, buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
                       VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
                           VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
                       uniformBuffers[i], uniformBuffersMemory[i]);

        vmaMapMemory(allocator, uniformBuffersMemory[i], &uniformBuffersMapped[i]);
    }
}

void zh::Device::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &create_info)
{
    create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    create_info.pfnUserCallback = zh::Device::debugCallback;
    create_info.pUserData = nullptr;
}

const int zh::Device::rateDeviceSuitability(VkPhysicalDevice physical_device)
{
    int score = 0;

    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceProperties(physical_device, &properties);
    vkGetPhysicalDeviceFeatures(physical_device, &features);
    bool extension_support = checkDeviceExtensionSupport(physical_device);

    // Prefer discrete GPU over integrated GPU.
    if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        score += 10;
    }
    else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
    {
        score += 1;
    }

    // Max possible texture size to max graphics quality.
    score += properties.limits.maxImageDimension2D;

    // Require geometry shader.
    if (!features.geometryShader)
        score = -1;

    // Require a device with a valid queue familiy.
    if (!findQueueFamilies().isComplete())
        score = -1;

    // Require that swap chain is adequate
    if (extension_support)
    {
        SwapchainSupportDetails details = querySwapchainSupport();
        if (details.formats.empty() || details.presentModes.empty())
            score = -1;
    }

    // Require a device with extensions support.
    if (!extension_support)
        score = -1;

    return score;
}

const bool zh::Device::checkDeviceExtensionSupport(VkPhysicalDevice device) const
{
    uint32_t extension_count = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

    std::vector<VkExtensionProperties> available_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

    std::set<std::string> required_extensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto &extension : available_extensions)
        required_extensions.erase(extension.extensionName);

    return required_extensions.empty();
}

VKAPI_ATTR VkBool32 VKAPI_CALL zh::Device::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                         VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                         const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                         void *pUserData)
{
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
        std::cout << "[ VL DIAL ] " << pCallbackData->pMessage << std::endl;

    else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        std::cout << "[ VL INFO ] " << pCallbackData->pMessage << std::endl;

    else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        std::cout << "[ VL WARN ] " << pCallbackData->pMessage << std::endl;

    else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        std::cerr << "[ VL ERROR ] " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

VkResult zh::Device::createDebugUtilsMessengerEXT(VkInstance instance,
                                                  const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                                  const VkAllocationCallbacks *pAllocator,
                                                  VkDebugUtilsMessengerEXT *pDebugMessenger)
{
    auto fn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

    if (fn != nullptr)
        return fn(instance, pCreateInfo, pAllocator, pDebugMessenger);

    else
        return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void zh::Device::destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                               const VkAllocationCallbacks *pAllocator)
{
    auto fn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

    if (fn != nullptr)
        fn(instance, debugMessenger, pAllocator);
}
