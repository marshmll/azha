#include "stdafx.hpp"
#include "Graphics/Models/Model.hpp"

zh::Model::Model(Device &device) : device(device)
{

}

zh::Model::Model(Device &device, const std::string &path) : device(device), hasIndexBuffer(false), loaded(false)
{
    loadFromFile(path);
}

zh::Model::Model(Device &device, const std::vector<Vertex> &vertices) : device(device), hasIndexBuffer(false), loaded(true)
{
    vertexCount = static_cast<uint32_t>(vertices.size());
    indexCount = 0;
}

zh::Model::Model(Device &device, std::vector<Vertex> vertices, std::vector<Index> indices)
    : device(device), hasIndexBuffer(true), loaded(true)
{
    vertexCount = static_cast<uint32_t>(vertices.size());
    indexCount = static_cast<uint32_t>(indices.size());

    createVertexBuffer(vertices);
    createIndexBuffer(indices);
}

zh::Model::~Model() = default;

const bool zh::Model::loadFromFile(const std::string &path)
{
    loaded = true;
    return loaded;
}

void zh::Model::draw(VkCommandBuffer &command_buffer)
{
    if (hasIndexBuffer)
        vkCmdDrawIndexed(command_buffer, indexCount, 1, 0, 0, 0);
    else
        vkCmdDraw(command_buffer, vertexCount, 1, 0, 0);
}

void zh::Model::bind(VkCommandBuffer &command_buffer)
{
    VkBuffer buffers[] = {vertexBuffer->getBuffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(command_buffer, 0, 1, buffers, offsets);

    if (hasIndexBuffer)
        vkCmdBindIndexBuffer(command_buffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
}
void zh::Model::createVertexBuffer(std::vector<Vertex> &vertices)
{
    StagingBuffer staging_buffer(device.getAllocator(), vertices.size() * sizeof(Vertex));

    staging_buffer.map();
    staging_buffer.write(vertices.data(), staging_buffer.getSize());
    staging_buffer.unmap();

    vertexBuffer = std::make_unique<Buffer>(device.getAllocator(), staging_buffer.getSize(),
                                            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
                                            VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT);

    Buffer::copy(device.getLogicalDevice(), device.getTransientCommandPool(), device.getTransferQueue(), staging_buffer,
                 *vertexBuffer);
}

void zh::Model::createIndexBuffer(std::vector<Index> &indices)
{
    StagingBuffer staging_buffer(device.getAllocator(), indices.size() * sizeof(Index));

    staging_buffer.map();
    staging_buffer.write(indices.data(), staging_buffer.getSize());
    staging_buffer.unmap();

    indexBuffer = std::make_unique<Buffer>(device.getAllocator(), staging_buffer.getSize(),
                                           VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
                                           VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT);

    Buffer::copy(device.getLogicalDevice(), device.getTransientCommandPool(), device.getTransferQueue(), staging_buffer,
                 *vertexBuffer);
}
