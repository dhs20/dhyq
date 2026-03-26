#include "quantum/render/Camera.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace quantum::render {

OrbitCamera::OrbitCamera() = default;

void OrbitCamera::resize(int width, int height) {
    width_ = std::max(width, 1);
    height_ = std::max(height, 1);
}

void OrbitCamera::orbit(const glm::vec2& delta) {
    yaw_ += delta.x * 0.01f;
    pitch_ = std::clamp(pitch_ + delta.y * 0.01f, -1.3f, 1.3f);
}

void OrbitCamera::pan(const glm::vec2& delta) {
    const glm::vec3 right = glm::normalize(glm::cross(position() - target_, glm::vec3(0.0f, 1.0f, 0.0f)));
    const glm::vec3 up = glm::normalize(glm::cross(right, position() - target_));
    target_ += (-delta.x * right + delta.y * up) * 0.01f * distance_;
}

void OrbitCamera::dolly(float delta) {
    distance_ = std::clamp(distance_ * std::exp(-delta * 0.1f), 1.0f, 120.0f);
}

glm::mat4 OrbitCamera::viewMatrix() const {
    return glm::lookAt(position(), target_, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 OrbitCamera::projectionMatrix() const {
    return glm::perspective(glm::radians(45.0f), aspectRatio(), 0.01f, 500.0f);
}

glm::vec3 OrbitCamera::position() const {
    const float cp = std::cos(pitch_);
    const float sp = std::sin(pitch_);
    const float cy = std::cos(yaw_);
    const float sy = std::sin(yaw_);
    return target_ + glm::vec3(distance_ * cp * cy, distance_ * sp, distance_ * cp * sy);
}

glm::vec3 OrbitCamera::offsetFromTarget() const {
    return position() - target_;
}

float OrbitCamera::aspectRatio() const {
    return static_cast<float>(width_) / static_cast<float>(height_);
}

float OrbitCamera::distanceToTarget() const {
    return distance_;
}

} // namespace quantum::render
