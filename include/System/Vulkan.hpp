#pragma once

#include "Graphics/Window.hpp"

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

namespace zh
{
struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    inline const bool isComplete() const { return this->graphicsFamily.has_value() && this->presentFamily.has_value(); }

    inline const uint32_t getGraphicsFamily() const { return this->graphicsFamily.value(); }

    inline const uint32_t getPresentFamily() const { return this->presentFamily.value(); }
};

struct SwapchainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class Vulkan
{
  public:
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PUBLIC METHODS //////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////

    Vulkan();

    ~Vulkan();

    Window createWindow(const unsigned int width, const unsigned int height, const std::string &title);

    void drawFrameTemp();

    void setFramebufferResized(const bool resized);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PUBLIC STATIC METHODS ///////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////

    static const uint32_t getExtensionCount();

  private:
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PRIVATE ATTRIBUTES //////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // clang-format off

    std::vector<const char *>       validationLayers;
    std::vector<const char *>       deviceExtensions;

    VkInstance                      vkInstance;
    VkDebugUtilsMessengerEXT        debugMessenger;

    VkPhysicalDevice                physicalDevice;
    VkDevice                        device;

    GLFWwindow                      *window;
    VkSurfaceKHR                    surface;

    VkQueue                         graphicsQueue;
    VkQueue                         presentQueue;

    VkSwapchainKHR                  swapchain;
    VkFormat                        swapchainImageFormat;
    VkExtent2D                      swapchainExtent;
    std::vector<VkImage>            swapchainImages;
    std::vector<VkImageView>        swapchainImageViews;
    std::vector<VkFramebuffer>      swapchainFramebuffers;

    VkRenderPass                    renderPass;
    VkPipelineLayout                pipelineLayout;
    VkPipeline                      graphicsPipeline;

    VkCommandPool                   commandPool;
    std::vector<VkCommandBuffer>    commandBuffers;

    std::vector<VkSemaphore>        imageAvailableSemaphores;
    std::vector<VkSemaphore>        renderFinishedSemaphores;
    std::vector<VkFence>            inFlightFences;

    uint32_t                        currentFrame;

    bool                            framebufferResized;
    bool                            enableValidationLayers;
    bool                            cleaned;

    // clang-format on

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PRIVATE METHODS /////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void initValidationLayers();

    void initDeviceExtensions();

    void initGLFW();

    void initVulkanInstance();

    void initDebugMessenger();

    void pickPhysicalDevice();

    void createLogicalDevice();

    void createSwapchain();

    void createImageViews();

    void createRenderPass();

    void createGraphicsPipeline();

    void createFramebuffers();

    void createCommandPool();

    void createCommandBuffers();

    void createSyncObjects();

    void recreateSwapchain();

    void cleanup();

    void cleanupSwapchain();

    void cleanupPipeline();

    void cleanupRenderPass();

    void cleanupSyncObjects();

    void cleanupCommandPool();

    void cleanupDevice();

    void cleanupDebugUtils();

    void cleanupSurface();

    void cleanupWindow();

    void cleanupVulkanInstance();

    const int rateDeviceSuitability(VkPhysicalDevice device) const;

    const QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;

    const SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device) const;

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &available_formats);

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &available_modes);

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

    const bool checkDeviceExtensionSupport(VkPhysicalDevice device) const;

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &create_info);

    const bool checkValidationLayerSupport();

    VkShaderModule createShaderModule(std::vector<char> &code);

    std::vector<const char *> getRequiredExtensions();

    void recordCommandBuffer(VkCommandBuffer command_buffer, const uint32_t image_index);

    std::vector<char> readFile(const std::string &filename);

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PRIVATE STATIC METHODS //////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                        VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                        void *pUserData);

    static VkResult createDebugUtilsMessengerEXT(VkInstance instance,
                                                 const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                                 const VkAllocationCallbacks *pAllocator,
                                                 VkDebugUtilsMessengerEXT *pDebugMessenger);

    static void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
                                              const VkAllocationCallbacks *pAllocator);

    static void framebufferResizedCallback(GLFWwindow *window, int width, int height);
};
} // namespace zh
