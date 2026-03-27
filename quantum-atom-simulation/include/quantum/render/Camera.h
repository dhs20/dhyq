#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace quantum::render {

class OrbitCamera {
public:
    struct Pose {
        float yaw = 0.5f;
        float pitch = 0.35f;
        float distance = 12.0f;
        glm::vec3 target{0.0f, 0.0f, 0.0f};
    };

    OrbitCamera();

    void resize(int width, int height);
    void orbit(const glm::vec2& delta);
    void pan(const glm::vec2& delta);
    void dolly(float delta);
    void update(float deltaTimeSeconds);
    void jumpToPose(const Pose& pose);
    void animateToPose(const Pose& pose);

    [[nodiscard]] glm::mat4 viewMatrix() const;
    [[nodiscard]] glm::mat4 projectionMatrix() const;
    [[nodiscard]] glm::vec3 position() const;
    [[nodiscard]] glm::vec3 offsetFromTarget() const;
    [[nodiscard]] float aspectRatio() const;
    [[nodiscard]] float distanceToTarget() const;
    [[nodiscard]] bool isTransitioning() const;
    [[nodiscard]] const Pose& pose() const;

private:
    Pose current_;
    Pose target_;
    bool transitioning_ = false;
    int width_ = 1280;
    int height_ = 720;
};

} // namespace quantum::render
