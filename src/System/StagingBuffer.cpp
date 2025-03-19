#include "stdafx.hpp"
#include "System/StagingBuffer.hpp"

zh::StagingBuffer::StagingBuffer(VmaAllocator &allocator, VkDeviceSize buffer_size)
    : Buffer(allocator, buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
             VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
             VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
                 VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT)
{
}
