#pragma once

#include "quantum/app/SimulationState.h"
#include "quantum/render/Camera.h"
#include "quantum/render/GLObjects.h"

#include <GL/glew.h>
#include <glm/vec2.hpp>

#include <array>
#include <cstddef>
#include <string>

namespace quantum::render {

struct SceneRenderStats {
    bool gpuTimersSupported = false;
    double gpuFrameMs = 0.0;
    double gpuPointMs = 0.0;
    double gpuVolumeMs = 0.0;
    std::size_t pointBufferBytes = 0;
    std::size_t volumeTextureBytes = 0;
    std::size_t framebufferBytes = 0;
    std::size_t trackedGpuBytes = 0;
    int totalPointCount = 0;
    int renderedPointCount = 0;
    int volumeSliceCount = 0;
    int volumeSliceAxis = 2;
    int lodLevel = 0;
    bool interactionLod = false;
    float cameraDistance = 0.0f;
};

class SceneRenderer {
public:
    ~SceneRenderer();

    bool initialize(std::string& errorMessage);
    void shutdown();
    void resize(int width, int height);
    void uploadCloud(const quantum::physics::CloudData& cloud);
    [[nodiscard]] SceneRenderStats render(const quantum::app::SimulationState& state, OrbitCamera& camera);

    [[nodiscard]] GLuint sceneTexture() const;
    [[nodiscard]] glm::ivec2 size() const;

private:
    struct LODDecision {
        int level = 0;
        int renderedPointCount = 0;
        int volumeSliceCount = 0;
        bool interactionLod = false;
        float cameraDistance = 0.0f;
    };

    struct VolumeSlicePlan {
        int axis = 2;
        bool reverse = false;
    };

    struct TimerQuerySet {
        GLuint frameStart = 0;
        GLuint frameEnd = 0;
        GLuint pointStart = 0;
        GLuint pointEnd = 0;
        GLuint volumeStart = 0;
        GLuint volumeEnd = 0;
        bool frameIssued = false;
        bool pointIssued = false;
        bool volumeIssued = false;
    };

    void destroyPointBuffers();
    void initializeGpuTimers();
    void destroyGpuTimers();
    void resolveGpuTimers();
    [[nodiscard]] LODDecision decideLod(const quantum::app::SimulationState& state, const OrbitCamera& camera) const;
    [[nodiscard]] VolumeSlicePlan chooseVolumeSlicePlan(const OrbitCamera& camera) const;
    void renderGrid(const glm::mat4& view, const glm::mat4& projection);
    void renderNucleus(const glm::mat4& view, const glm::mat4& projection, float animationTimeSeconds);
    void renderOrbits(const quantum::app::SimulationState& state,
                      const glm::mat4& view,
                      const glm::mat4& projection,
                      float animationTimeSeconds);
    void renderPointCloud(const quantum::app::SimulationState& state,
                          const glm::mat4& view,
                          const glm::mat4& projection,
                          int renderedPointCount,
                          float animationTimeSeconds);
    void renderVolume(const quantum::app::SimulationState& state,
                      const glm::mat4& view,
                      const glm::mat4& projection,
                      int sliceCount,
                      const VolumeSlicePlan& slicePlan,
                      float animationTimeSeconds);

    Framebuffer framebuffer_;
    ShaderProgram meshShader_;
    ShaderProgram pointShader_;
    ShaderProgram volumeShader_;
    Mesh sphereMesh_;
    Mesh ringMesh_;
    Mesh quadMesh_;
    Mesh gridMesh_;
    Texture3D volumeTexture_;

    GLuint pointVao_ = 0;
    GLuint pointVbo_ = 0;
    GLsizei pointCount_ = 0;
    float volumeExtent_ = 0.0f;
    std::size_t pointBufferBytes_ = 0;
    std::size_t volumeTextureBytes_ = 0;

    static constexpr std::size_t kTimerLatency = 4;
    std::array<TimerQuerySet, kTimerLatency> timerQueries_{};
    std::size_t timerQueryIndex_ = 0;
    bool gpuTimersSupported_ = false;
    double lastGpuFrameMs_ = 0.0;
    double lastGpuPointMs_ = 0.0;
    double lastGpuVolumeMs_ = 0.0;
};

} // namespace quantum::render
