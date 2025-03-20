#pragma once

class Camera
{
  public:
    void setOrthographicProjection(const float left, const float right, const float top, const float bottom,
                                   const float near, const float far);

    void setPerspectiveProjection(const float fovy, const float aspect, const float near, const float far);

    void setViewDirection(const glm::vec3 &position, const glm::vec3 &direction,
                          const glm::vec3 &up = glm::vec3{0.f, -1.f, 0.f});

    void setViewTarget(const glm::vec3 &position, const glm::vec3 &target,
                       const glm::vec3 &up = glm::vec3{0.f, -1.f, 0.f});

    void setViewYXZ(glm::vec3 position, glm::vec3 rotation);

  private:
    glm::mat4 projectionMatrix;
    glm::mat4 viewMatrix;
    glm::mat4 inverseViewMatrix;
};
