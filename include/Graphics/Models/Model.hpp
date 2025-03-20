#pragma once

#include <vk_mem_alloc.h>

#include "Graphics/Vertex/Vertex.hpp"
#include "System/Core/Device.hpp"
#include "System/Memory/Buffer.hpp"
#include "System/Memory/StagingBuffer.hpp"

namespace zh
{
class Model
{
  public:
    Model(Device &device);

    Model(Device &device, const std::string &path);

    Model(Device &device, const std::vector<Vertex> &vertices);

    Model(Device &device, std::vector<Vertex> vertices, std::vector<Index> indices);

    ~Model();

    Model(const Model &) = delete;
    Model operator=(const Model &) = delete;

    const bool loadFromFile(const std::string &path);

    void draw(VkCommandBuffer &command_buffer);

    void bind(VkCommandBuffer &command_buffer);

  private:
    Device &device;

    std::unique_ptr<Buffer> vertexBuffer;
    std::unique_ptr<Buffer> indexBuffer;

    uint32_t vertexCount;
    uint32_t indexCount;

    bool hasIndexBuffer;
    bool loaded;

    void createVertexBuffer(std::vector<Vertex> &vertices);

    void createIndexBuffer(std::vector<Index> &indices);
};

} // namespace zh
