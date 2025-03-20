#pragma once

#include "Graphics/Models/Model.hpp"

namespace zh
{
class Object
{
  public:
    struct TransformComponent
    {
        glm::vec3 translation{};
        glm::vec3 scale{1.f, 1.f, 1.f};
        glm::vec3 rotation{};
        glm::mat4 mat4();
        glm::mat3 normalMatrix();
    };

    Object(Device &device);

    ~Object();

    Object() = delete;

    const bool loadModelFromFile(const std::string &path);

    void loadModelFromData(const std::vector<Vertex> &vertices);

    void loadModelFromData(const std::vector<Vertex> &vertices, const std::vector<Index> &indices);

    void setModel(std::shared_ptr<Model> &model);

    void setTranslation(const glm::vec3 &translation);

    void setScale(const glm::vec3 &scale);

    void setRotation(const glm::vec3 &rotation);

  private:
    inline static uint32_t idCounter = 0;

    Device &device;
    uint32_t id;
    std::shared_ptr<Model> model;
    TransformComponent transform;
};

} // namespace zh
