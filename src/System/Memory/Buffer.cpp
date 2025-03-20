#include "stdafx.hpp"
#include "System/Memory/Buffer.hpp"

zh::Buffer::Buffer(VmaAllocator &allocator, VkDeviceSize size, VkBufferUsageFlags usage,
                   VkMemoryPropertyFlags properties, VmaMemoryUsage memory_usage,
                   VmaAllocationCreateFlags allocation_flags)
    : allocator(allocator), size(size), mapped(false), mappable(false), mmem(nullptr)
{
    if (properties & (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
        mappable = true;

    create(allocator, size, usage, properties, memory_usage, allocation_flags, buffer, memory);
}

VkBuffer &zh::Buffer::getBuffer()
{
    return buffer;
}

VmaAllocation &zh::Buffer::getMemory()
{
    return memory;
}

const VkDeviceSize &zh::Buffer::getSize() const
{
    return size;
}

const bool zh::Buffer::isMappable() const
{
    return mappable;
}

void zh::Buffer::map()
{
    assert(allocator != VK_NULL_HANDLE && "zh::Buffer::map: ALLOCATOR IS NOT INITIALIZED");
    assert(memory != VK_NULL_HANDLE && "zh::Buffer::map: MEMORY IS NOT INITIALIZED");
    assert(mmem == nullptr && "zh::Buffer::map: TRYING TO MAP ALREADY MAPPED BUFFER");
    assert(mappable == true && "zh::Buffer::map: MEMORY IS NOT MAPPABLE");

    VkResult result = vmaMapMemory(allocator, memory, &mmem);
    assert(result == VK_SUCCESS && "zh::Buffer::map: FAILED TO MAP MEMORY");
}

void zh::Buffer::map(void *&mmem)
{
    assert(allocator != VK_NULL_HANDLE && "zh::Buffer::map: ALLOCATOR IS NOT INITIALIZED");
    assert(memory != VK_NULL_HANDLE && "zh::Buffer::map: MEMORY IS NOT INITIALIZED");
    assert(this->mmem == nullptr && "zh::Buffer::map: TRYING TO MAP ALREADY MAPPED BUFFER");
    assert(mmem != nullptr && "zh::Buffer::map: TRYING TO MAP TO NULL POINTER");
    assert(mappable == true && "zh::Buffer::map: MEMORY IS NOT MAPPABLE");

    VkResult result = vmaMapMemory(allocator, memory, &mmem);
    assert(result == VK_SUCCESS && "zh::Buffer::map: FAILED TO MAP MEMORY");
    this->mmem = mmem;
}

void zh::Buffer::write(void *data, const size_t size)
{
    assert(mmem != nullptr && "zh::Buffer::write: TRYING TO WRITE TO UNMAPPED BUFFER");
    assert(data != nullptr && "zh::Buffer::write: DATA POINTER IS NULL");
    assert(size <= this->size && "zh::Buffer::write: SIZE EXCEEDS BUFFER CAPACITY");

    std::memcpy(mmem, data, size);
}

void zh::Buffer::unmap()
{
    assert(mmem != nullptr && "zh::Buffer::unmap: TRYING TO UNMAP UNMAPPED BUFFER");

    vmaUnmapMemory(allocator, memory);
    mmem = nullptr; // Reset pointer.
}

zh::Buffer::~Buffer()
{
    vmaDestroyBuffer(allocator, buffer, memory);
}

void zh::Buffer::create(VmaAllocator &allocator, VkDeviceSize size, VkBufferUsageFlags usage,
                        VkMemoryPropertyFlags properties, VmaMemoryUsage memory_usage,
                        VmaAllocationCreateFlags allocation_flags, VkBuffer &buffer, VmaAllocation &buffer_memory)
{
    VkBufferCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.size = size;
    create_info.usage = usage;
    create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo alloc_info{};
    alloc_info.usage = memory_usage;
    alloc_info.flags = allocation_flags;
    alloc_info.requiredFlags = properties;

    vmaCreateBuffer(allocator, &create_info, &alloc_info, &buffer, &buffer_memory, nullptr);
}

void zh::Buffer::copy(VkDevice &device, VkCommandPool &command_pool, VkQueue &queue, Buffer &src, Buffer &dst)
{
    // Allocate command buffer
    // TODO: (reuse if possible)
    VkCommandBuffer command_buffer;
    VkCommandBufferAllocateInfo alloc_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = command_pool;
    alloc_info.commandBufferCount = 1;
    vkAllocateCommandBuffers(device, &alloc_info, &command_buffer);

    // Begin command buffer
    VkCommandBufferBeginInfo begin_info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(command_buffer, &begin_info);

    // Single barrier to handle both host write visibility and transfer readiness
    VkBufferMemoryBarrier buf_mem_barrier = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
    buf_mem_barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    buf_mem_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    buf_mem_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    buf_mem_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    buf_mem_barrier.buffer = src.getBuffer();
    buf_mem_barrier.offset = 0;
    buf_mem_barrier.size = VK_WHOLE_SIZE;

    vkCmdPipelineBarrier(command_buffer,
                         VK_PIPELINE_STAGE_HOST_BIT,     // Host write is complete
                         VK_PIPELINE_STAGE_TRANSFER_BIT, // Ready for transfer
                         0, 0, nullptr, 1, &buf_mem_barrier, 0, nullptr);

    // Copy buffer
    VkBufferCopy copy_region{};
    copy_region.srcOffset = 0;
    copy_region.dstOffset = 0;
    copy_region.size = src.getSize();
    vkCmdCopyBuffer(command_buffer, src.getBuffer(), dst.getBuffer(), 1, &copy_region);

    // Single barrier to handle transfer completion and make buffer available for other operations
    VkBufferMemoryBarrier buf_mem_barrier_2 = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
    buf_mem_barrier_2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    buf_mem_barrier_2.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
    buf_mem_barrier_2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    buf_mem_barrier_2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    buf_mem_barrier_2.buffer = dst.getBuffer();
    buf_mem_barrier_2.offset = 0;
    buf_mem_barrier_2.size = VK_WHOLE_SIZE;

    vkCmdPipelineBarrier(command_buffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,     // Transfer is complete
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, // Make buffer available for all subsequent operations
                         0, 0, nullptr, 1, &buf_mem_barrier_2, 0, nullptr);

    // End command buffer
    vkEndCommandBuffer(command_buffer);

    // Submit command buffer
    VkSubmitInfo submit_info{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    // Use a fence only if synchronization is absolutely necessary
    VkFenceCreateInfo fence_info{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    VkFence fence;
    vkCreateFence(device, &fence_info, nullptr, &fence);

    vkQueueSubmit(queue, 1, &submit_info, fence);

    // Wait for the fence to ensure the copy is complete
    vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX);
    vkDestroyFence(device, fence, nullptr);

    // Free command buffer (consider reusing it instead of freeing)
    vkFreeCommandBuffers(device, command_pool, 1, &command_buffer);
}
