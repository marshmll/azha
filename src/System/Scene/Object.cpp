#include "stdafx.hpp"
#include "System/Scene/Object.hpp"

zh::Object::Object(Device &device) : device(device)
{
    id = idCounter++;
}

zh::Object::~Object()
{
}

const bool zh::Object::loadModelFromFile(const std::string &path)
{
    model = std::make_shared<Model>(device);

    return model->loadFromFile(path);
}

void zh::Object::loadModelFromData(const std::vector<Vertex> &vertices)
{
    model = std::make_shared<Model>(device, vertices);
}

void zh::Object::loadModelFromData(const std::vector<Vertex> &vertices, const std::vector<Index> &indices)
{
    model = std::make_shared<Model>(device, vertices, indices);
}

void zh::Object::setModel(std::shared_ptr<Model> &model)
{
    this->model = model;
}

void zh::Object::setTranslation(const glm::vec3 &translation)
{
    transform.translation = translation;
}

void zh::Object::setScale(const glm::vec3 &scale)
{
    transform.scale = scale;
}

void zh::Object::setRotation(const glm::vec3 &rotation)
{
    transform.rotation = rotation;
}
