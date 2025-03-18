#pragma once

#include "vk_mem_alloc.h"

namespace zh
{
class Buffer
{
  public:
    static void create(VmaAllocator &allocator, VkDeviceSize size, VkBufferUsageFlags usage,
                       VkMemoryPropertyFlags properties, VmaMemoryUsage memory_usage,
                       VmaAllocationCreateFlags allocation_flags, VkBuffer &buffer, VmaAllocation &buffer_memory);

    static void stagingBuffer(VmaAllocator &allocator, VkDeviceSize buffer_size, VkBuffer &buffer,
                              VmaAllocation &buffer_memory);

    static void copy(VkDevice &device, VkQueue &queue, VkCommandPool &command_pool, VkBuffer src, VkBuffer dst,
                     VkDeviceSize size);
};
} // namespace zh
