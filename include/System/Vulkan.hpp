#pragma once

#include "vk_mem_alloc.h"

#include "System/Window.hpp"
#include "Graphics/Vertex.hpp"
#include "Graphics/UniformBufferObject.hpp"

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

// clang-format off

const Vertex vertices[] = {
    {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f, 1.0f}},
    {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f, 1.0f}},
    {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f, 1.0f}},
    {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f, 1.0f}},
};

const uint32_t indices[] = {0, 1, 2, 2, 3, 0};

// clang-format on

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

    // Window createWindow(const unsigned int width, const unsigned int height, const std::string &title);

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

    VmaAllocator                    allocator;

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
    VkDescriptorSetLayout           descriptorSetLayout;
    VkPipelineLayout                pipelineLayout;
    VkPipeline                      graphicsPipeline;

    VkCommandPool                   commandPool;
    VkCommandPool                   transientCommandPool;
    std::vector<VkCommandBuffer>    commandBuffers;

    std::vector<VkSemaphore>        imageAvailableSemaphores;
    std::vector<VkSemaphore>        renderFinishedSemaphores;
    std::vector<VkFence>            inFlightFences;
    uint32_t                        currentFrame;

    VkBuffer                        vertexBuffer;
    VmaAllocation                   vertexBufferMemory;
    VkBuffer                        indexBuffer;
    VmaAllocation                   indexBufferMemory;

    std::vector<VkBuffer>           uniformBuffers;
    std::vector<VmaAllocation>      uniformBuffersMemory;
    std::vector<void *>             uniformBuffersMapped;

    VkDescriptorPool                descriptorPool;
    std::vector<VkDescriptorSet>    descriptorSets;

    bool                            framebufferResized;
    bool                            enableValidationLayers;
    bool                            cleaned;

    // clang-format on

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PRIVATE METHODS /////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void initMemoryAllocator();

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

    void createDescriptorSetLayout();

    void createGraphicsPipeline();

    void createFramebuffers();

    void createCommandPools();

    void createVertexBuffer();

    void createIndexBuffer();

    void createUniformBuffers();

    void createDescriptorPool();

    void createDescriptorSets();

    void createCommandBuffers();

    void createSyncObjects();

    void recreateSwapchain();

    void updateUniformBuffer(const uint32_t current_image);

    void cleanup();

    void cleanupMemoryAllocator();

    void cleanupSwapchain();

    void cleanupIndexBuffer();

    void cleanupVertexBuffer();

    void cleanupUniformBuffers();

    void cleanupDescriptorPool();

    void cleanupDescriptorSetLayout();

    void cleanupPipeline();

    void cleanupRenderPass();

    void cleanupSyncObjects();

    void cleanupCommandPools();

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

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                      VmaMemoryUsage memory_usage, VmaAllocationCreateFlags allocation_flags, VkBuffer &buffer,
                      VmaAllocation &buffer_memory);

    void createStagingBuffer(VkDeviceSize buffer_size, VkBuffer &buffer, VmaAllocation &buffer_memory);

    void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);

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
