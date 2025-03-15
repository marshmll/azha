#pragma once

namespace zh
{
class AzhaCore
{
  public:
    AzhaCore();

    ~AzhaCore();

    void cleanup();

    const uint32_t getVulkanExtensionCount();

  private:
    VkInstance vkInstance;
    VkDebugUtilsMessengerEXT debugMessenger;

    std::vector<const char *> validationLayers;
    bool enableValidationLayers;

    bool cleaned;

    void initValidationLayers();

    void initGLFW();

    void initDebugMessenger();

    void initVulkanInstance();

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
