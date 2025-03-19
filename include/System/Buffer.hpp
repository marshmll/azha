#pragma once

#include "vk_mem_alloc.h"

namespace zh
{
class Buffer
{
  public:
    Buffer(VmaAllocator &allocator, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
           VmaMemoryUsage memory_usage, VmaAllocationCreateFlags allocation_flags);

    ~Buffer();

    // No default constructor, not copyable or movable.
    Buffer() = delete;
    Buffer(const Buffer &) = delete;
    Buffer operator=(const Buffer &) = delete;

    VkBuffer &getBuffer();

    VmaAllocation &getMemory();

    const VkDeviceSize &getSize() const;

    const bool isMappable() const;

    void map();

    void write(void *data, const size_t size);

    void unmap();

    static void copy(VkDevice &device, VkCommandPool &command_pool, VkQueue &queue, Buffer &src, Buffer &dst);

  protected:
    VmaAllocator &allocator;

    VkBuffer buffer;
    VmaAllocation memory;
    VkDeviceSize size;

    bool mappable;
    bool mapped;

    void *mmem;

    void create(VmaAllocator &allocator, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                VmaMemoryUsage memory_usage, VmaAllocationCreateFlags allocation_flags, VkBuffer &buffer,
                VmaAllocation &buffer_memory);
};
} // namespace zh
