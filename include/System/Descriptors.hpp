#pragma once

#include "System/Device.hpp"

namespace zh
{
class DescriptorSetLayout
{
  public:
  private:
    Device &device;
    VkDescriptorSetLayout descriptorSetLayout;
};

class DescriptorPool
{
  public:
  private:
    Device &device;
    VkDescriptorPool descriptorSetPool;
};

class DescriptorWriter
{
  public:
  private:
    DescriptorSetLayout &layout;
    DescriptorPool pool;
    std::vector<VkWriteDescriptorSet> writes;
};
} // namespace zh
