#include "quantum/render/Camera.h"

#include <glm/geometric.hpp>
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
    current_.yaw += delta.x * 0.01f;
    current_.pitch = std::clamp(current_.pitch + delta.y * 0.01f, -1.3f, 1.3f);
    target_ = current_;
    transitioning_ = false;
}

void OrbitCamera::pan(const glm::vec2& delta) {
    const glm::vec3 right = glm::normalize(glm::cross(position() - current_.target, glm::vec3(0.0f, 1.0f, 0.0f)));
    const glm::vec3 up = glm::normalize(glm::cross(right, position() - current_.target));
    current_.target += (-delta.x * right + delta.y * up) * 0.01f * current_.distance;
    target_ = current_;
    transitioning_ = false;
}

void OrbitCamera::dolly(float delta) {
    current_.distance = std::clamp(current_.distance * std::exp(-delta * 0.1f), minDistance_, maxDistance_);
    target_ = current_;
    transitioning_ = false;
}

void OrbitCamera::update(float deltaTimeSeconds) {
    if (!transitioning_) {
        return;
    }

    const float effectiveDuration = std::max(transitionDurationSeconds_, 0.02f);
    const float response = 1.0f - std::exp(-std::max(deltaTimeSeconds, 0.0f) * (4.0f / effectiveDuration));
    current_.yaw += (target_.yaw - current_.yaw) * response;
    current_.pitch += (target_.pitch - current_.pitch) * response;
    current_.distance += (target_.distance - current_.distance) * response;
    current_.target += (target_.target - current_.target) * response;

    const bool settled = std::abs(current_.yaw - target_.yaw) < 1.0e-3f &&
                         std::abs(current_.pitch - target_.pitch) < 1.0e-3f &&
                         std::abs(current_.distance - target_.distance) < 1.0e-3f &&
                         glm::length(current_.target - target_.target) < 1.0e-3f;
    if (settled) {
        current_ = target_;
        transitioning_ = false;
    }
}

void OrbitCamera::jumpToPose(const Pose& pose) {
    current_ = pose;
    current_.pitch = std::clamp(current_.pitch, -1.3f, 1.3f);
    current_.distance = std::clamp(current_.distance, minDistance_, maxDistance_);
    target_ = pose;
    target_.pitch = current_.pitch;
    target_.distance = current_.distance;
    transitioning_ = false;
}

void OrbitCamera::animateToPose(const Pose& pose) {
    target_ = pose;
    target_.pitch = std::clamp(target_.pitch, -1.3f, 1.3f);
    target_.distance = std::clamp(target_.distance, minDistance_, maxDistance_);
    transitioning_ = true;
}

void OrbitCamera::setTransitionDuration(float seconds) {
    transitionDurationSeconds_ = std::clamp(seconds, 0.02f, 5.0f);
}

void OrbitCamera::setDistanceLimits(float minDistance, float maxDistance) {
    minDistance_ = std::max(0.05f, minDistance);
    maxDistance_ = std::max(minDistance_ + 0.05f, maxDistance);
    current_.distance = std::clamp(current_.distance, minDistance_, maxDistance_);
    target_.distance = std::clamp(target_.distance, minDistance_, maxDistance_);
}

void OrbitCamera::setClipPlanes(float nearPlane, float farPlane) {
    nearPlane_ = std::clamp(nearPlane, 0.001f, 5.0f);
    farPlane_ = std::max(nearPlane_ + 1.0f, farPlane);
}

glm::mat4 OrbitCamera::viewMatrix() const {
    return glm::lookAt(position(), current_.target, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 OrbitCamera::projectionMatrix() const {
    return glm::perspective(glm::radians(45.0f), aspectRatio(), nearPlane_, farPlane_);
}

glm::vec3 OrbitCamera::position() const {
    const float cp = std::cos(current_.pitch);
    const float sp = std::sin(current_.pitch);
    const float cy = std::cos(current_.yaw);
    const float sy = std::sin(current_.yaw);
    return current_.target + glm::vec3(current_.distance * cp * cy, current_.distance * sp, current_.distance * cp * sy);
}

glm::vec3 OrbitCamera::offsetFromTarget() const {
    return position() - current_.target;
}

float OrbitCamera::aspectRatio() const {
    return static_cast<float>(width_) / static_cast<float>(height_);
}

float OrbitCamera::distanceToTarget() const {
    return current_.distance;
}

bool OrbitCamera::isTransitioning() const {
    return transitioning_;
}

const OrbitCamera::Pose& OrbitCamera::pose() const {
    return current_;
}

} // namespace quantum::render
