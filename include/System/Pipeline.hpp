#pragma once

#include "Graphics/Vertex.hpp"
#include "Graphics/UniformBufferObject.hpp"
#include "System/Device.hpp"

namespace zh
{
class Pipeline
{
  public:
    Pipeline(Device &device, const std::string &vertex_shader_path, const std::string &fragment_shader_path);

    Pipeline() = delete;

    Pipeline(const Pipeline &) = delete;

    Pipeline operator=(const Pipeline &) = delete;

  private:
    Device &device;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;

    void createPipeline(const std::string &vertex_shader_path, const std::string &fragment_shader_path);

    const std::vector<uint8_t> readFile(const std::string &path);

    VkShaderModule createShaderModule(std::vector<uint8_t> &code);

};
} // namespace zh
