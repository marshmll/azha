#pragma once

namespace zh
{
class AzhaCore
{
  private:
    VkInstance vkInstance;

    std::vector<const char *> validationLayers;
    bool enableValidationLayers;

    bool cleaned;

    void initValidationLayers();

    const bool checkValidationLayerSupport();

    void vulkanInit();

  public:
    AzhaCore();

    ~AzhaCore();

    void cleanup();

    static const uint32_t getVulkanExtensionCount();
};

} // namespace zh
