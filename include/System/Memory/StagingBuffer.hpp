#pragma once

#include "System/Memory/Buffer.hpp"

namespace zh
{
class StagingBuffer : public Buffer
{
  public:
    StagingBuffer(VmaAllocator &allocator, VkDeviceSize buffer_size);

    // No default constructor, not copyable or movable.
    StagingBuffer() = delete;
    StagingBuffer(const Buffer &) = delete;
    StagingBuffer operator=(const Buffer &) = delete;

  private:
};

} // namespace zh
