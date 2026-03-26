#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace quantum::render {

class OrbitCamera {
public:
    OrbitCamera();

    void resize(int width, int height);
    void orbit(const glm::vec2& delta);
    void pan(const glm::vec2& delta);
    void dolly(float delta);

    [[nodiscard]] glm::mat4 viewMatrix() const;
    [[nodiscard]] glm::mat4 projectionMatrix() const;
    [[nodiscard]] glm::vec3 position() const;
    [[nodiscard]] glm::vec3 offsetFromTarget() const;
    [[nodiscard]] float aspectRatio() const;
    [[nodiscard]] float distanceToTarget() const;

private:
    float yaw_ = 0.5f;
    float pitch_ = 0.35f;
    float distance_ = 12.0f;
    glm::vec3 target_{0.0f, 0.0f, 0.0f};
    int width_ = 1280;
    int height_ = 720;
};

} // namespace quantum::render
