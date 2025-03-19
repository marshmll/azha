#pragma once

#include "System/Device.hpp"

namespace zh
{
class DescriptorSetLayout
{
  public:
    class Builder
    {
      public:
        Builder(Device &device);

        Builder &addBinding(const uint32_t binding, VkDescriptorType descriptor_type, VkShaderStageFlags stage_flags,
                            const uint32_t count = 1);
        std::unique_ptr<DescriptorSetLayout> build() const;

      private:
        Device &device;
        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;
    };

    DescriptorSetLayout(Device &device, const std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> &bindings);
    ~DescriptorSetLayout();
    DescriptorSetLayout(const DescriptorSetLayout &) = delete;
    DescriptorSetLayout &operator=(const DescriptorSetLayout &) = delete;

    VkDescriptorSetLayout getDescriptorSetLayout() const;

    const std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> &getBindings() const;

    VkDescriptorSetLayoutBinding &getBinding(const uint32_t binding);

  private:
    Device &device;
    VkDescriptorSetLayout descriptorSetLayout;
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;
};

class DescriptorPool
{
  public:
    class Builder
    {
      public:
        Builder(Device &device);

        Builder &addPoolSize(VkDescriptorType descriptor_type, const uint32_t count);
        Builder &setPoolFlags(VkDescriptorPoolCreateFlags flags);
        Builder &setMaxSets(const uint32_t count);
        std::unique_ptr<DescriptorPool> build() const;

      private:
        Device &device;
        std::vector<VkDescriptorPoolSize> poolSizes;
        uint32_t maxSets;
        VkDescriptorPoolCreateFlags poolFlags;
    };

    DescriptorPool(Device &device, const uint32_t max_sets, VkDescriptorPoolCreateFlags pool_flags,
                   const std::vector<VkDescriptorPoolSize> &pool_sizes);
    ~DescriptorPool();
    DescriptorPool(const DescriptorPool &) = delete;
    DescriptorPool &operator=(const DescriptorPool &) = delete;

    const bool allocateDescriptor(const VkDescriptorSetLayout descriptor_set_layout, VkDescriptorSet &descriptor) const;

    void freeDescriptors(std::vector<VkDescriptorSet> &descriptors) const;

    void resetPool();

  private:
    Device &device;
    VkDescriptorPool descriptorPool;
};

class DescriptorWriter
{
  public:
    DescriptorWriter(Device &device, DescriptorSetLayout &descriptor_set_layout, DescriptorPool &descriptor_pool);

    DescriptorWriter &writeBuffer(const uint32_t binding, VkDescriptorBufferInfo *buffer_info);
    DescriptorWriter &writeImage(const uint32_t binding, VkDescriptorImageInfo *image_info);

    const bool build(VkDescriptorSet &descriptor_set);
    void overwrite(VkDescriptorSet &set);

  private:
    Device &device;
    DescriptorSetLayout &descriptorSetLayout;
    DescriptorPool &descriptorPool;
    std::vector<VkWriteDescriptorSet> writes;
};

} // namespace zh
