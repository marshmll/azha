#include "stdafx.hpp"
#include "System/AzhaCore.hpp"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PUBLIC METHODS //////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

zh::AzhaCore::AzhaCore() : cleaned(false)
{
    initValidationLayers();
    initGLFW();
    initVulkanInstance();
    initDebugMessenger();
}

zh::AzhaCore::~AzhaCore()
{
    cleanup();
}

void zh::AzhaCore::cleanup()
{
    if (!cleaned)
    {
        if (enableValidationLayers)
            proxyDestroyDebugUtilsMessengerEXT(vkInstance, debugMessenger, nullptr);

        vkDestroyInstance(vkInstance, nullptr);
        glfwTerminate();
        cleaned = true;
    }
}

const uint32_t zh::AzhaCore::getVulkanExtensionCount()
{
    uint32_t extension_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);

    return extension_count;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PRIVATE METHODS /////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

void zh::AzhaCore::initValidationLayers()
{
#ifdef NDEBUG
    enableValidationLayers = false;
#else
    enableValidationLayers = true;
#endif

    validationLayers = {"VK_LAYER_KHRONOS_validation"};
}

void zh::AzhaCore::initGLFW()
{
    glfwInit();
}

void zh::AzhaCore::initDebugMessenger()
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

void zh::AzhaCore::initVulkanInstance()
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

#ifndef NDEBUG
    uint32_t extension_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);

    std::vector<VkExtensionProperties> extensions(extension_count);

    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

    std::cout << "Available Vulkan Extensions: " << "\n";

    for (const auto &extension : extensions)
        std::cout << "    - " << extension.extensionName << std::endl;
#endif
}

void zh::AzhaCore::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &create_info)
{
    create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    create_info.pfnUserCallback = zh::AzhaCore::debugCallback;
    create_info.pUserData = nullptr;
}

const bool zh::AzhaCore::checkValidationLayerSupport()
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

std::vector<const char *> zh::AzhaCore::getRequiredExtensions()
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

VKAPI_ATTR VkBool32 VKAPI_CALL zh::AzhaCore::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                           VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                           const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                           void *pUserData)
{
    if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
        std::cout << "[ DIAG ] Validation Layer: " << pCallbackData->pMessage << std::endl;

    else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
        std::cout << "[ INFO ] Validation Layer: " << pCallbackData->pMessage << std::endl;

    else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        std::cout << "[ WARN ] Validation Layer: " << pCallbackData->pMessage << std::endl;

    else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        std::cerr << "[ ERROR ] Validation Layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

VkResult zh::AzhaCore::proxyCreateDebugUtilsMessengerEXT(VkInstance instance,
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

void zh::AzhaCore::proxyDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                                      const VkAllocationCallbacks *pAllocator)
{
    auto fn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

    if (fn != nullptr)
    {
        fn(instance, debugMessenger, pAllocator);
    }
}
