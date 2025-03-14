#include "stdafx.hpp"
#include "System/AzhaCore.hpp"

zh::AzhaCore::AzhaCore() : cleaned(false)
{
    initValidationLayers();

    glfwInit();
    vulkanInit();
}

zh::AzhaCore::~AzhaCore()
{
    cleanup();
}

void zh::AzhaCore::initValidationLayers()
{
#ifdef NDEBUG
    enableValidationLayers = false;
#else
    enableValidationLayers = true;
#endif

    validationLayers = {"VK_LAYER_KHRONOS_validation"};
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

void zh::AzhaCore::vulkanInit()
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
    uint32_t glfw_extension_count = 0;
    const char **glfw_extensions;
    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

#ifdef __APPLE__
    // Avoid VK_ERROR_INCOMPATIBLE_DRIVER
    std::vector<const char *> required_extensions;

    for (uint32_t i = 0; i < glfw_extension_count; i++)
    {
        required_extensions.emplace_back(glfw_extensions[i]);
    }

    required_extensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

    create_info.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    create_info.enabledExtensionCount = static_cast<uint32_t>(required_extensions.size());
    create_info.ppEnabledExtensionNames = required_extensions.data();
#else
    create_info.enabledExtensionCount = glfw_extension_count;
    create_info.ppEnabledExtensionNames = glfw_extensions;

    if (enableValidationLayers)
    {
        create_info.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        create_info.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
        create_info.enabledLayerCount = 0;
    }
#endif

    if (vkCreateInstance(&create_info, nullptr, &vkInstance) != VK_SUCCESS)
        throw std::runtime_error("Failed to create a Vulkan Instance.");

    uint32_t extension_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);

    std::vector<VkExtensionProperties> extensions(extension_count);

    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, extensions.data());

    std::cout << "Available Vulkan Extensions: " << "\n";

    for (const auto &extension : extensions)
        std::cout << "    - " << extension.extensionName << std::endl;
}

void zh::AzhaCore::cleanup()
{
    if (!cleaned)
    {
        std::cout << "Cleaning up..." << std::endl;
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
