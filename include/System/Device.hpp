#pragma once

#include "System/Buffer.hpp"
#include "System/StagingBuffer.hpp"
#include "System/Window.hpp"

namespace zh
{
class Device
{
  public:
    inline static const std::vector<const char *> VALIDATION_LAYERS = {"VK_LAYER_KHRONOS_validation"};
    inline static const std::vector<const char *> DEVICE_EXTENSIONS = {VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                                                       VK_EXT_MEMORY_BUDGET_EXTENSION_NAME};
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    struct QueueFamilyIndices
    {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        inline const bool isComplete() const
        {
            return this->graphicsFamily.has_value() && this->presentFamily.has_value();
        }

        inline const uint32_t getGraphicsFamily() const { return this->graphicsFamily.value(); }

        inline const uint32_t getPresentFamily() const { return this->presentFamily.value(); }
    };

    struct SwapchainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    Device(Window &window);

    Device() = delete;

    Device(const Device &) = delete;

    Device operator=(const Device &) = delete;

    ~Device();

    VkPhysicalDevice &getPhysicalDevice();

    VkDevice &getLogicalDevice();

    VmaAllocator &getAllocator();

    VkQueue &getTransferQueue();

    VkCommandPool &getCommandPool();

    VkCommandPool &getTransientCommandPool();

    const bool checkValidationLayerSupport();

    std::vector<const char *> getRequiredExtensions();

    const QueueFamilyIndices findQueueFamilies(VkPhysicalDevice &physical_device);

    const SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice &physical_device) const;

  private:
    // clang-format off
    std::vector<char *>          validationLayers;
    std::vector<char *>          deviceExtensions;

    Window                       &window;

    VkInstance                   instance;
    VkDebugUtilsMessengerEXT     debugMessenger;

    VkPhysicalDevice             physicalDevice;
    VkDevice                     device;

    VmaAllocator                 allocator;

    VkQueue                      graphicsQueue;
    VkQueue                      presentQueue;

    VkCommandPool                commandPool;
    VkCommandPool                transientCommandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    VkBuffer                     vertexBuffer;
    VmaAllocation                vertexBufferMemory;
    VkBuffer                     indexBuffer;
    VmaAllocation                indexBufferMemory;

    std::vector<VkBuffer>        uniformBuffers;
    std::vector<VmaAllocation>   uniformBuffersMemory;
    std::vector<void *>          uniformBuffersMapped;

    VkDescriptorPool             descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
    // clang-format on

    void nullifyHandles();

    void initVulkanInstance();

    void initDebugMessenger();

    void initWindowSurface();

    void pickAdequatePhysicalDevice();

    void createLogicalDevice();

    void initMemoryAllocator();

    void createCommandPools();

    // void createUniformBuffers(VkDeviceSize buffer_size);

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &create_info);

    const int rateDeviceSuitability(VkPhysicalDevice physical_device);

    const bool checkDeviceExtensionSupport(VkPhysicalDevice device) const;

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
};
} // namespace zh
