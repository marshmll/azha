#pragma once

#include "Graphics/Window.hpp"

namespace zh
{
struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    inline const bool isComplete() const { return this->graphicsFamily.has_value() && this->presentFamily.has_value(); }

    inline const uint32_t getGraphicsFamily() const { return this->graphicsFamily.value_or(0); }

    inline const uint32_t getPresentFamily() const { return this->presentFamily.value_or(0); }
};

class Vulkan
{
  public:
    Vulkan();

    ~Vulkan();

    void cleanup();

    const uint32_t getExtensionCount();

    Window createWindow(const unsigned int width, const unsigned int height, const std::string &title);

  private:
    VkInstance vkInstance;
    VkDebugUtilsMessengerEXT debugMessenger;

    VkPhysicalDevice physicalDevice;
    VkDevice device;

    GLFWwindow *window;
    VkSurfaceKHR surface;

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    std::vector<const char *> validationLayers;
    std::vector<const char *> deviceExtensions;

    bool enableValidationLayers;
    bool cleaned;

    void initValidationLayers();

    void initGLFW();

    void initVulkanInstance();

    void initDebugMessenger();

    void pickPhysicalDevice();

    void createLogicalDevice();

    void initDeviceExtensions();

    const int rateDeviceSuitability(VkPhysicalDevice device) const;

    const QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;

    const bool checkDeviceExtensionSupport(VkPhysicalDevice device) const;

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &create_info);

    const bool checkValidationLayerSupport();

    std::vector<const char *> getRequiredExtensions();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                        VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                        void *pUserData);

    static VkResult proxyCreateDebugUtilsMessengerEXT(VkInstance instance,
                                                      const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                                      const VkAllocationCallbacks *pAllocator,
                                                      VkDebugUtilsMessengerEXT *pDebugMessenger);

    static void proxyDestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                                   const VkAllocationCallbacks *pAllocator);
};
} // namespace zh
