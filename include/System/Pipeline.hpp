#pragma once

#include "Graphics/Vertex.hpp"
#include "Graphics/UniformBufferObject.hpp"
#include "System/Swapchain.hpp"

namespace zh
{
class Pipeline
{
  public:
    Pipeline(Device &device, Swapchain &swapchain, const std::string &vertex_shader_path,
             const std::string &fragment_shader_path);

    Pipeline() = delete;

    Pipeline(const Pipeline &) = delete;

    Pipeline operator=(const Pipeline &) = delete;

  private:
    Device &device;
    Swapchain &swapchain;

    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

    void createPipeline(const std::string &vertex_shader_path, const std::string &fragment_shader_path);

    const std::vector<uint8_t> readFile(const std::string &path);

    VkShaderModule createShaderModule(std::vector<uint8_t> &code);
};
} // namespace zh
