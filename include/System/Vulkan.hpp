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

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
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
    std::vector<const char *> validationLayers;
    std::vector<const char *> deviceExtensions;

    VkInstance vkInstance;
    VkDebugUtilsMessengerEXT debugMessenger;

    VkPhysicalDevice physicalDevice;
    VkDevice device;

    GLFWwindow *window;
    VkSurfaceKHR surface;

    VkQueue graphicsQueue;
    VkQueue presentQueue;

    VkSwapchainKHR swapchain;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;

    VkPipeline graphicsPipeline;

    bool enableValidationLayers;
    bool cleaned;

    void initValidationLayers();

    void initDeviceExtensions();

    void initGLFW();

    void initVulkanInstance();

    void initDebugMessenger();

    void pickPhysicalDevice();

    void createLogicalDevice();

    void createSwapChain();

    void createImageViews();

    void createRenderPass();

    void createGraphicsPipeline();

    const int rateDeviceSuitability(VkPhysicalDevice device) const;

    const QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;

    const SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) const;

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &available_formats);

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &available_modes);

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

    const bool checkDeviceExtensionSupport(VkPhysicalDevice device) const;

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &create_info);

    const bool checkValidationLayerSupport();

    VkShaderModule createShaderModule(std::vector<char> &code);

    std::vector<const char *> getRequiredExtensions();

    std::vector<char> readFile(const std::string &filename);

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
