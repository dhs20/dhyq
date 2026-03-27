#include "quantum/render/SceneRenderer.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cmath>
#include <vector>

namespace quantum::render {
namespace {

constexpr float kSceneScale = 2.0e10f;
constexpr int kFullVolumeSlices = 64;
constexpr int kMediumVolumeSlices = 40;
constexpr int kFarVolumeSlices = 24;
constexpr int kInteractionVolumeSlices = 16;
constexpr float kBaseGridHalfExtent = 10.0f;

constexpr const char* kMeshVertexShader = R"(
#version 330 core
layout(location = 0) in vec3 aPosition;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;

out vec3 vNormal;

void main() {
    vec4 world = uModel * vec4(aPosition, 1.0);
    vNormal = normalize(uNormalMatrix * aPosition);
    gl_Position = uProjection * uView * world;
}
)";

constexpr const char* kMeshFragmentShader = R"(
#version 330 core
in vec3 vNormal;

uniform vec3 uColor;
uniform vec3 uLightDir;
uniform float uAlpha;

out vec4 FragColor;

void main() {
    float ndotl = clamp(dot(normalize(vNormal), normalize(uLightDir)), 0.2, 1.0);
    FragColor = vec4(uColor * ndotl, uAlpha);
}
)";

constexpr const char* kPointVertexShader = R"(
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in float aDensity;
layout(location = 2) in float aPhase;

uniform mat4 uView;
uniform mat4 uProjection;
uniform float uPointSize;

out float vDensity;
out float vPhase;

void main() {
    vDensity = aDensity;
    vPhase = aPhase;
    gl_Position = uProjection * uView * vec4(aPosition, 1.0);
    gl_PointSize = max(1.0, uPointSize * (0.5 + 1.5 * aDensity));
}
)";

constexpr const char* kPointFragmentShader = R"(
#version 330 core
in float vDensity;
in float vPhase;
uniform float uTime;
out vec4 FragColor;

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    vec2 coord = gl_PointCoord * 2.0 - 1.0;
    float radius = dot(coord, coord);
    if (radius > 1.0) {
        discard;
    }
    float hue = (vPhase + 3.14159265) / (2.0 * 3.14159265);
    float shimmer = 0.90 + 0.10 * sin(uTime * 1.7 + vPhase * 2.5);
    vec3 color = hsv2rgb(vec3(hue, 0.85, clamp((0.35 + 0.8 * vDensity) * shimmer, 0.0, 1.0)));
    float alpha = clamp(vDensity * (1.0 - radius), 0.0, 1.0);
    FragColor = vec4(color, alpha);
}
)";

constexpr const char* kVolumeVertexShader = R"(
#version 330 core
layout(location = 0) in vec3 aPosition;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform float uSlice;
uniform int uSliceAxis;

out vec3 vTexCoord;

void main() {
    float slicePosition = mix(-1.0, 1.0, uSlice);
    vec3 position;
    if (uSliceAxis == 0) {
        position = vec3(slicePosition, aPosition.x, aPosition.y);
    } else if (uSliceAxis == 1) {
        position = vec3(aPosition.x, slicePosition, aPosition.y);
    } else {
        position = vec3(aPosition.xy, slicePosition);
    }
    vTexCoord = position * 0.5 + 0.5;
    gl_Position = uProjection * uView * uModel * vec4(position, 1.0);
}
)";

constexpr const char* kVolumeFragmentShader = R"(
#version 330 core
in vec3 vTexCoord;
uniform sampler3D uVolumeTexture;
uniform float uExposure;
uniform float uTime;
out vec4 FragColor;

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    vec2 sampleValue = texture(uVolumeTexture, vTexCoord).rg;
    float density = sampleValue.r;
    float phase = sampleValue.g;
    if (density < 0.02) {
        discard;
    }
    float hue = (phase + 3.14159265) / (2.0 * 3.14159265);
    float shimmer = 0.92 + 0.08 * sin(uTime * 1.2 + phase * 1.3);
    vec3 color = hsv2rgb(vec3(hue, 0.8, clamp(density * uExposure * shimmer, 0.0, 1.0)));
    FragColor = vec4(color, density * 0.08);
}
)";

std::vector<float> makeSphereVertices(int latitudeSegments, int longitudeSegments) {
    std::vector<float> vertices;
    for (int latitude = 0; latitude <= latitudeSegments; ++latitude) {
        const float v = static_cast<float>(latitude) / static_cast<float>(latitudeSegments);
        const float theta = v * static_cast<float>(quantum::physics::constants::pi);
        const float sinTheta = std::sin(theta);
        const float cosTheta = std::cos(theta);
        for (int longitude = 0; longitude <= longitudeSegments; ++longitude) {
            const float u = static_cast<float>(longitude) / static_cast<float>(longitudeSegments);
            const float phi = u * 2.0f * static_cast<float>(quantum::physics::constants::pi);
            vertices.push_back(sinTheta * std::cos(phi));
            vertices.push_back(cosTheta);
            vertices.push_back(sinTheta * std::sin(phi));
        }
    }
    return vertices;
}

std::vector<unsigned int> makeSphereIndices(int latitudeSegments, int longitudeSegments) {
    std::vector<unsigned int> indices;
    for (int latitude = 0; latitude < latitudeSegments; ++latitude) {
        for (int longitude = 0; longitude < longitudeSegments; ++longitude) {
            const unsigned int first = latitude * (longitudeSegments + 1) + longitude;
            const unsigned int second = first + longitudeSegments + 1;
            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);
            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }
    return indices;
}

std::vector<float> makeRingVertices(int segments) {
    std::vector<float> vertices;
    vertices.reserve(static_cast<std::size_t>(segments + 1) * 3ULL);
    for (int index = 0; index <= segments; ++index) {
        const float angle = 2.0f * static_cast<float>(quantum::physics::constants::pi) *
                            static_cast<float>(index) / static_cast<float>(segments);
        vertices.push_back(std::cos(angle));
        vertices.push_back(0.0f);
        vertices.push_back(std::sin(angle));
    }
    return vertices;
}

std::vector<float> makeQuadVertices() {
    return {-1.0f, -1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, -1.0f, -1.0f, 0.0f,
            1.0f, 1.0f, 0.0f,  -1.0f, 1.0f, 0.0f};
}

std::vector<float> makeGridVertices(int halfExtent, float spacing) {
    std::vector<float> vertices;
    for (int line = -halfExtent; line <= halfExtent; ++line) {
        const float x = static_cast<float>(line) * spacing;
        vertices.insert(vertices.end(), {x, 0.0f, -halfExtent * spacing, x, 0.0f, halfExtent * spacing});
        const float z = static_cast<float>(line) * spacing;
        vertices.insert(vertices.end(), {-halfExtent * spacing, 0.0f, z, halfExtent * spacing, 0.0f, z});
    }
    return vertices;
}

double nanosecondsToMilliseconds(GLuint64 value) {
    return static_cast<double>(value) * 1.0e-6;
}

float animationTimeSeconds() {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<float>(std::chrono::duration<double>(now).count());
}

glm::mat3 normalMatrixFromModel(const glm::mat4& model) {
    return glm::transpose(glm::inverse(glm::mat3(model)));
}

} // namespace

SceneRenderer::~SceneRenderer() {
    shutdown();
}

bool SceneRenderer::initialize(std::string& errorMessage) {
    if (!framebuffer_.create(1280, 720)) {
        errorMessage = "Failed to create scene framebuffer";
        return false;
    }

    if (!meshShader_.create(kMeshVertexShader, kMeshFragmentShader, errorMessage) ||
        !pointShader_.create(kPointVertexShader, kPointFragmentShader, errorMessage) ||
        !volumeShader_.create(kVolumeVertexShader, kVolumeFragmentShader, errorMessage)) {
        return false;
    }

    sphereMesh_.upload(makeSphereVertices(20, 20), 3, GL_TRIANGLES, makeSphereIndices(20, 20));
    ringMesh_.upload(makeRingVertices(256), 3, GL_LINE_STRIP);
    quadMesh_.upload(makeQuadVertices(), 3, GL_TRIANGLES);
    gridMesh_.upload(makeGridVertices(20, 0.5f), 3, GL_LINES);
    initializeGpuTimers();

    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    return true;
}

void SceneRenderer::shutdown() {
    destroyPointBuffers();
    volumeTexture_.destroy();
    volumeTextureBytes_ = 0;
    destroyGpuTimers();
    framebuffer_.destroy();
    meshShader_.destroy();
    pointShader_.destroy();
    volumeShader_.destroy();
    sphereMesh_.destroy();
    ringMesh_.destroy();
    quadMesh_.destroy();
    gridMesh_.destroy();
}

void SceneRenderer::resize(int width, int height) {
    framebuffer_.resize(std::max(width, 1), std::max(height, 1));
}

void SceneRenderer::uploadCloud(const quantum::physics::CloudData& cloud) {
    destroyPointBuffers();
    pointCount_ = static_cast<GLsizei>(cloud.points.size());
    volumeExtent_ = static_cast<float>(cloud.extentMeters);
    volumeTextureBytes_ = 0;

    if (!cloud.points.empty()) {
        glGenVertexArrays(1, &pointVao_);
        glGenBuffers(1, &pointVbo_);
        glBindVertexArray(pointVao_);
        glBindBuffer(GL_ARRAY_BUFFER, pointVbo_);
        std::vector<quantum::physics::CloudPoint> scaledPoints = cloud.points;
        for (auto& point : scaledPoints) {
            point.x *= kSceneScale;
            point.y *= kSceneScale;
            point.z *= kSceneScale;
        }
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(scaledPoints.size() * sizeof(quantum::physics::CloudPoint)),
                     scaledPoints.data(),
                     GL_STATIC_DRAW);
        pointBufferBytes_ = scaledPoints.size() * sizeof(quantum::physics::CloudPoint);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(quantum::physics::CloudPoint), nullptr);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1,
                              1,
                              GL_FLOAT,
                              GL_FALSE,
                              sizeof(quantum::physics::CloudPoint),
                              reinterpret_cast<void*>(offsetof(quantum::physics::CloudPoint, density)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2,
                              1,
                              GL_FLOAT,
                              GL_FALSE,
                              sizeof(quantum::physics::CloudPoint),
                              reinterpret_cast<void*>(offsetof(quantum::physics::CloudPoint, phase)));
        glBindVertexArray(0);
    }

    if (cloud.volume.resolution > 0 && !cloud.volume.voxels.empty()) {
        volumeTexture_.uploadDensityPhase(cloud.volume.resolution, cloud.volume.voxels);
        volumeTextureBytes_ = cloud.volume.voxels.size() * sizeof(float);
    } else {
        volumeTexture_.destroy();
    }
}

SceneRenderStats SceneRenderer::render(const quantum::app::SimulationState& state, OrbitCamera& camera) {
    resolveGpuTimers();
    auto& timerSet = timerQueries_[timerQueryIndex_];
    timerSet.frameIssued = false;
    timerSet.pointIssued = false;
    timerSet.volumeIssued = false;

    configureCameraForScene(state, camera);

    const LODDecision lod = decideLod(state, camera);
    const VolumeSlicePlan slicePlan = chooseVolumeSlicePlan(camera);
    const float animationTime = animationTimeSeconds();
    framebuffer_.bind();
    glViewport(0, 0, framebuffer_.width(), framebuffer_.height());
    const bool schrodingerCloudMode = state.modelKind == quantum::app::ModelKind::SchrodingerCloud;
    const bool compareMode = state.modelKind == quantum::app::ModelKind::Compare;
    const float backgroundPulse = 0.008f * std::sin(animationTime * 0.23f);
    const float backgroundBlueBias = schrodingerCloudMode ? 0.018f : (compareMode ? 0.010f : 0.0f);
    glClearColor(0.04f + backgroundPulse * 0.5f,
                 0.06f + backgroundPulse * 0.35f,
                 0.09f + backgroundBlueBias + backgroundPulse,
                 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const glm::mat4 view = camera.viewMatrix();
    const glm::mat4 projection = camera.projectionMatrix();
    const bool bohrMode = state.modelKind == quantum::app::ModelKind::Bohr;
    const bool waveParticleMode = state.modelKind == quantum::app::ModelKind::WaveParticle;
    const bool volumeModeSelected = state.view.cloudRenderMode == quantum::app::CloudRenderMode::VolumeSlices ||
                                    state.view.cloudRenderMode == quantum::app::CloudRenderMode::Hybrid;
    const bool hasVolumeData = state.cloud.buildVolume && volumeTexture_.id() != 0;
    const bool orbitPassEnabled =
        bohrMode || compareMode || (waveParticleMode && state.view.showOrbitRings);
    const bool pointPassEnabled = waveParticleMode ||
                                  ((schrodingerCloudMode || compareMode) &&
                                   (state.view.cloudRenderMode == quantum::app::CloudRenderMode::PointCloud ||
                                    state.view.cloudRenderMode == quantum::app::CloudRenderMode::Hybrid ||
                                    (state.view.cloudRenderMode == quantum::app::CloudRenderMode::VolumeSlices &&
                                     !hasVolumeData)));
    const bool volumePassEnabled =
        (schrodingerCloudMode || compareMode) && volumeModeSelected && hasVolumeData;

    if (gpuTimersSupported_) {
        glQueryCounter(timerSet.frameStart, GL_TIMESTAMP);
    }

    if (state.view.showGrid) {
        renderGrid(state, camera, view, projection);
    }
    renderNucleus(view, projection, animationTime);
    if (orbitPassEnabled) {
        renderOrbits(state, view, projection, animationTime);
    }

    if (pointPassEnabled) {
        if (gpuTimersSupported_) {
            glQueryCounter(timerSet.pointStart, GL_TIMESTAMP);
        }
        renderPointCloud(state, view, projection, lod.renderedPointCount, animationTime);
        if (gpuTimersSupported_) {
            glQueryCounter(timerSet.pointEnd, GL_TIMESTAMP);
            timerSet.pointIssued = true;
        }
    }
    if (volumePassEnabled) {
        if (gpuTimersSupported_) {
            glQueryCounter(timerSet.volumeStart, GL_TIMESTAMP);
        }
        renderVolume(state, view, projection, lod.volumeSliceCount, slicePlan, animationTime);
        if (gpuTimersSupported_) {
            glQueryCounter(timerSet.volumeEnd, GL_TIMESTAMP);
            timerSet.volumeIssued = true;
        }
    }

    if (gpuTimersSupported_) {
        glQueryCounter(timerSet.frameEnd, GL_TIMESTAMP);
        timerSet.frameIssued = true;
    }

    Framebuffer::bindDefault();
    timerQueryIndex_ = (timerQueryIndex_ + 1) % kTimerLatency;

    SceneRenderStats stats;
    stats.gpuTimersSupported = gpuTimersSupported_;
    stats.gpuFrameMs = lastGpuFrameMs_;
    stats.gpuPointMs = lastGpuPointMs_;
    stats.gpuVolumeMs = lastGpuVolumeMs_;
    stats.pointBufferBytes = pointBufferBytes_;
    stats.volumeTextureBytes = volumeTextureBytes_;
    stats.framebufferBytes = static_cast<std::size_t>(framebuffer_.width()) *
                             static_cast<std::size_t>(framebuffer_.height()) * 8ULL;
    stats.trackedGpuBytes = stats.pointBufferBytes + stats.volumeTextureBytes + stats.framebufferBytes;
    stats.totalPointCount = static_cast<int>(pointCount_);
    stats.renderedPointCount = pointPassEnabled ? lod.renderedPointCount : 0;
    stats.volumeSliceCount = volumePassEnabled ? lod.volumeSliceCount : 0;
    stats.volumeSliceAxis = slicePlan.axis;
    stats.lodLevel = lod.level;
    stats.interactionLod = lod.interactionLod;
    stats.cameraDistance = lod.cameraDistance;
    return stats;
}

GLuint SceneRenderer::sceneTexture() const {
    return framebuffer_.colorTexture();
}

glm::ivec2 SceneRenderer::size() const {
    return {framebuffer_.width(), framebuffer_.height()};
}

void SceneRenderer::destroyPointBuffers() {
    if (pointVbo_ != 0) {
        glDeleteBuffers(1, &pointVbo_);
        pointVbo_ = 0;
    }
    if (pointVao_ != 0) {
        glDeleteVertexArrays(1, &pointVao_);
        pointVao_ = 0;
    }
    pointCount_ = 0;
    pointBufferBytes_ = 0;
}

void SceneRenderer::initializeGpuTimers() {
    gpuTimersSupported_ = (GLEW_ARB_timer_query == GL_TRUE) || (GLEW_VERSION_3_3 == GL_TRUE);
    if (!gpuTimersSupported_) {
        return;
    }

    for (auto& timerSet : timerQueries_) {
        glGenQueries(1, &timerSet.frameStart);
        glGenQueries(1, &timerSet.frameEnd);
        glGenQueries(1, &timerSet.pointStart);
        glGenQueries(1, &timerSet.pointEnd);
        glGenQueries(1, &timerSet.volumeStart);
        glGenQueries(1, &timerSet.volumeEnd);
    }
}

void SceneRenderer::destroyGpuTimers() {
    if (!gpuTimersSupported_) {
        return;
    }

    for (auto& timerSet : timerQueries_) {
        if (timerSet.frameStart != 0) {
            glDeleteQueries(1, &timerSet.frameStart);
            timerSet.frameStart = 0;
        }
        if (timerSet.frameEnd != 0) {
            glDeleteQueries(1, &timerSet.frameEnd);
            timerSet.frameEnd = 0;
        }
        if (timerSet.pointStart != 0) {
            glDeleteQueries(1, &timerSet.pointStart);
            timerSet.pointStart = 0;
        }
        if (timerSet.pointEnd != 0) {
            glDeleteQueries(1, &timerSet.pointEnd);
            timerSet.pointEnd = 0;
        }
        if (timerSet.volumeStart != 0) {
            glDeleteQueries(1, &timerSet.volumeStart);
            timerSet.volumeStart = 0;
        }
        if (timerSet.volumeEnd != 0) {
            glDeleteQueries(1, &timerSet.volumeEnd);
            timerSet.volumeEnd = 0;
        }
        timerSet.frameIssued = false;
        timerSet.pointIssued = false;
        timerSet.volumeIssued = false;
    }
    timerQueryIndex_ = 0;
    gpuTimersSupported_ = false;
    lastGpuFrameMs_ = 0.0;
    lastGpuPointMs_ = 0.0;
    lastGpuVolumeMs_ = 0.0;
}

void SceneRenderer::resolveGpuTimers() {
    if (!gpuTimersSupported_) {
        return;
    }

    auto& timerSet = timerQueries_[timerQueryIndex_];
    if (timerSet.frameIssued) {
        GLuint64 start = 0;
        GLuint64 end = 0;
        glGetQueryObjectui64v(timerSet.frameStart, GL_QUERY_RESULT, &start);
        glGetQueryObjectui64v(timerSet.frameEnd, GL_QUERY_RESULT, &end);
        lastGpuFrameMs_ = nanosecondsToMilliseconds(end - start);
    }
    if (timerSet.pointIssued) {
        GLuint64 start = 0;
        GLuint64 end = 0;
        glGetQueryObjectui64v(timerSet.pointStart, GL_QUERY_RESULT, &start);
        glGetQueryObjectui64v(timerSet.pointEnd, GL_QUERY_RESULT, &end);
        lastGpuPointMs_ = nanosecondsToMilliseconds(end - start);
    } else {
        lastGpuPointMs_ = 0.0;
    }
    if (timerSet.volumeIssued) {
        GLuint64 start = 0;
        GLuint64 end = 0;
        glGetQueryObjectui64v(timerSet.volumeStart, GL_QUERY_RESULT, &start);
        glGetQueryObjectui64v(timerSet.volumeEnd, GL_QUERY_RESULT, &end);
        lastGpuVolumeMs_ = nanosecondsToMilliseconds(end - start);
    } else {
        lastGpuVolumeMs_ = 0.0;
    }
}

SceneRenderer::LODDecision SceneRenderer::decideLod(const quantum::app::SimulationState& state,
                                                    const OrbitCamera& camera) const {
    LODDecision decision;
    decision.cameraDistance = camera.distanceToTarget();
    decision.renderedPointCount = static_cast<int>(pointCount_);
    decision.volumeSliceCount = (state.cloud.buildVolume && volumeTexture_.id() != 0) ? kFullVolumeSlices : 0;

    if (pointCount_ == 0) {
        decision.renderedPointCount = 0;
        decision.volumeSliceCount = 0;
        return decision;
    }

    if (!state.cloud.enableLod) {
        return decision;
    }

    const float mediumDistance = std::min(state.cloud.mediumLodDistance, state.cloud.farLodDistance);
    const float farDistance = std::max(state.cloud.mediumLodDistance, state.cloud.farLodDistance);

    if (state.view.interactionActive && state.cloud.reduceDuringInteraction) {
        decision.level = 3;
        decision.interactionLod = true;
        decision.renderedPointCount = std::min<int>(pointCount_, std::max(1000, state.cloud.interactionPointCount));
        decision.volumeSliceCount = (state.cloud.buildVolume && volumeTexture_.id() != 0) ? kInteractionVolumeSlices : 0;
        return decision;
    }

    if (decision.cameraDistance >= farDistance) {
        decision.level = 2;
        decision.renderedPointCount = std::min<int>(pointCount_, std::max(1000, state.cloud.farLodPointCount));
        decision.volumeSliceCount = (state.cloud.buildVolume && volumeTexture_.id() != 0) ? kFarVolumeSlices : 0;
        return decision;
    }

    if (decision.cameraDistance >= mediumDistance) {
        decision.level = 1;
        decision.renderedPointCount = std::min<int>(pointCount_, std::max(1000, state.cloud.mediumLodPointCount));
        decision.volumeSliceCount = (state.cloud.buildVolume && volumeTexture_.id() != 0) ? kMediumVolumeSlices : 0;
    }
    return decision;
}

SceneRenderer::VolumeSlicePlan SceneRenderer::chooseVolumeSlicePlan(const OrbitCamera& camera) const {
    VolumeSlicePlan plan;
    const glm::vec3 offset = camera.offsetFromTarget();
    const glm::vec3 absolute = glm::abs(offset);
    if (absolute.x >= absolute.y && absolute.x >= absolute.z) {
        plan.axis = 0;
        plan.reverse = offset.x < 0.0f;
    } else if (absolute.y >= absolute.z) {
        plan.axis = 1;
        plan.reverse = offset.y < 0.0f;
    } else {
        plan.axis = 2;
        plan.reverse = offset.z < 0.0f;
    }
    return plan;
}

float SceneRenderer::estimateSceneExtent(const quantum::app::SimulationState& state) const {
    const int principalN = std::max(state.bohr.principalN, 1);
    const int maxOrbitN = std::max({principalN, state.transition.initial.n, state.transition.final.n, 1});
    const double orbitScale = static_cast<double>(maxOrbitN * maxOrbitN) /
                              static_cast<double>(principalN * principalN);
    const float orbitExtent = static_cast<float>(
        std::max(0.35, state.derived.bohrMetrics.orbitalRadiusM * kSceneScale * orbitScale));
    const float cloudExtent = std::max(0.0f, volumeExtent_ * kSceneScale);
    return std::max({1.5f, orbitExtent * 1.8f, cloudExtent * 1.15f});
}

void SceneRenderer::configureCameraForScene(const quantum::app::SimulationState& state, OrbitCamera& camera) const {
    const float sceneExtent = estimateSceneExtent(state);
    const float recommendedMinDistance = std::max(0.12f, sceneExtent * 0.025f);
    const float recommendedMaxDistance = std::max(220.0f, sceneExtent * 30.0f);
    const float nearPlane = std::clamp(sceneExtent * 0.0008f, 0.0015f, 0.08f);
    const float farPlane = std::max(900.0f, sceneExtent * 65.0f + camera.distanceToTarget() * 8.0f);
    camera.setDistanceLimits(recommendedMinDistance, recommendedMaxDistance);
    camera.setClipPlanes(nearPlane, farPlane);
}

OrbitCamera::Pose SceneRenderer::fitScenePose(const quantum::app::SimulationState& state,
                                              const OrbitCamera& camera,
                                              quantum::app::SceneFitMode mode) const {
    const int principalN = std::max(state.bohr.principalN, 1);
    const int maxOrbitN = std::max({principalN, state.transition.initial.n, state.transition.final.n, 1});
    const double orbitScale = static_cast<double>(maxOrbitN * maxOrbitN) /
                              static_cast<double>(principalN * principalN);
    const float orbitExtent = static_cast<float>(
        std::max(0.35, state.derived.bohrMetrics.orbitalRadiusM * kSceneScale * orbitScale));
    const float cloudExtent = std::max(0.0f, volumeExtent_ * kSceneScale);
    float sceneExtent = estimateSceneExtent(state);
    switch (mode) {
    case quantum::app::SceneFitMode::Nucleus:
        sceneExtent = 0.6f;
        break;
    case quantum::app::SceneFitMode::Orbit:
        sceneExtent = std::max(1.4f, orbitExtent * 1.35f);
        break;
    case quantum::app::SceneFitMode::Cloud:
        sceneExtent = std::max(1.8f, cloudExtent * 1.18f);
        break;
    case quantum::app::SceneFitMode::Full:
    default:
        break;
    }
    OrbitCamera::Pose pose = camera.pose();
    const float targetYOffset = (mode == quantum::app::SceneFitMode::Nucleus) ? 0.0f : std::clamp(-sceneExtent * 0.05f, -1.0f, 0.0f);
    pose.target = glm::vec3(0.0f, targetYOffset, 0.0f);
    pose.distance = std::max(1.2f, sceneExtent * 2.6f);
    return pose;
}

void SceneRenderer::renderGrid(const quantum::app::SimulationState& state,
                               const OrbitCamera& camera,
                               const glm::mat4& view,
                               const glm::mat4& projection) {
    const float sceneExtent = estimateSceneExtent(state);
    const float halfExtent = std::max({6.0f, camera.distanceToTarget() * 1.35f, sceneExtent});
    const float gridScale = halfExtent / kBaseGridHalfExtent;
    const glm::vec3 target = camera.pose().target;
    const float gridY = std::min(0.0f, target.y - sceneExtent * 0.22f);

    meshShader_.use();
    const glm::mat4 model =
        glm::translate(glm::mat4(1.0f), glm::vec3(target.x, gridY, target.z)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(gridScale, 1.0f, gridScale));
    glUniformMatrix4fv(glGetUniformLocation(meshShader_.id(), "uModel"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(meshShader_.id(), "uView"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(meshShader_.id(), "uProjection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix3fv(glGetUniformLocation(meshShader_.id(), "uNormalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrixFromModel(model)));
    glUniform3f(glGetUniformLocation(meshShader_.id(), "uColor"), 0.24f, 0.31f, 0.38f);
    glUniform3f(glGetUniformLocation(meshShader_.id(), "uLightDir"), -0.4f, 1.0f, 0.2f);
    glUniform1f(glGetUniformLocation(meshShader_.id(), "uAlpha"), 0.68f);
    gridMesh_.draw();
}

void SceneRenderer::renderNucleus(const glm::mat4& view, const glm::mat4& projection, float animationTimeSeconds) {
    meshShader_.use();
    const float pulse = 1.0f + 0.08f * std::sin(animationTimeSeconds * 2.2f);
    glUniformMatrix4fv(glGetUniformLocation(meshShader_.id(), "uView"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(meshShader_.id(), "uProjection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3f(glGetUniformLocation(meshShader_.id(), "uLightDir"), -0.4f, 1.0f, 0.2f);

    const glm::mat4 haloModel = glm::scale(glm::mat4(1.0f), glm::vec3(0.30f * pulse));
    glUniformMatrix4fv(glGetUniformLocation(meshShader_.id(), "uModel"), 1, GL_FALSE, glm::value_ptr(haloModel));
    glUniformMatrix3fv(glGetUniformLocation(meshShader_.id(), "uNormalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrixFromModel(haloModel)));
    glUniform3f(glGetUniformLocation(meshShader_.id(), "uColor"), 1.00f, 0.52f, 0.24f);
    glUniform1f(glGetUniformLocation(meshShader_.id(), "uAlpha"), 0.18f);
    sphereMesh_.draw();

    const glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(0.18f * pulse));
    glUniformMatrix4fv(glGetUniformLocation(meshShader_.id(), "uModel"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix3fv(glGetUniformLocation(meshShader_.id(), "uNormalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrixFromModel(model)));
    glUniform3f(glGetUniformLocation(meshShader_.id(), "uColor"), 0.95f, 0.26f, 0.21f);
    glUniform1f(glGetUniformLocation(meshShader_.id(), "uAlpha"), 1.0f);
    sphereMesh_.draw();
}

void SceneRenderer::renderOrbits(const quantum::app::SimulationState& state,
                                 const glm::mat4& view,
                                 const glm::mat4& projection,
                                 float animationTimeSeconds) {
    meshShader_.use();
    glUniformMatrix4fv(glGetUniformLocation(meshShader_.id(), "uView"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(meshShader_.id(), "uProjection"), 1, GL_FALSE, glm::value_ptr(projection));
    const float glow = 0.90f + 0.10f * std::sin(animationTimeSeconds * 1.4f);
    glUniform3f(glGetUniformLocation(meshShader_.id(), "uColor"), 0.58f * glow, 0.67f * glow, 0.74f * glow);
    glUniform3f(glGetUniformLocation(meshShader_.id(), "uLightDir"), -0.4f, 1.0f, 0.2f);

    const int maxN = std::max({state.bohr.principalN, state.transition.initial.n, state.transition.final.n, 1});
    for (int n = 1; n <= maxN; ++n) {
        const float radius = static_cast<float>(state.derived.bohrMetrics.orbitalRadiusM * kSceneScale *
                                                static_cast<double>(n * n) /
                                                static_cast<double>(std::max(1, state.bohr.principalN * state.bohr.principalN)));
        const glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(std::max(0.35f, radius)));
        glm::vec3 orbitColor(0.50f, 0.58f, 0.66f);
        float alpha = 0.52f;
        if (n == state.bohr.principalN) {
            orbitColor = glm::vec3(0.95f, 0.82f, 0.38f);
            alpha = 0.92f;
        } else if (n == state.transition.initial.n) {
            orbitColor = glm::vec3(0.92f, 0.45f, 0.34f);
            alpha = 0.78f;
        } else if (n == state.transition.final.n) {
            orbitColor = glm::vec3(0.37f, 0.78f, 0.95f);
            alpha = 0.78f;
        }
        glUniformMatrix4fv(glGetUniformLocation(meshShader_.id(), "uModel"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix3fv(glGetUniformLocation(meshShader_.id(), "uNormalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrixFromModel(model)));
        glUniform3f(glGetUniformLocation(meshShader_.id(), "uColor"), orbitColor.r, orbitColor.g, orbitColor.b);
        glUniform1f(glGetUniformLocation(meshShader_.id(), "uAlpha"), alpha);
        ringMesh_.draw();

        if (n == state.bohr.principalN || n == state.transition.initial.n || n == state.transition.final.n) {
            const float speed = 0.55f + 0.35f / static_cast<float>(n);
            const float angle = animationTimeSeconds * speed + static_cast<float>(n) * 0.75f;
            const glm::vec3 orbitPosition(std::cos(angle) * std::max(0.35f, radius),
                                          0.0f,
                                          std::sin(angle) * std::max(0.35f, radius));
            const glm::mat4 markerModel =
                glm::translate(glm::mat4(1.0f), orbitPosition) *
                glm::scale(glm::mat4(1.0f), glm::vec3(0.05f + 0.01f * static_cast<float>(n)));
            glUniformMatrix4fv(glGetUniformLocation(meshShader_.id(), "uModel"), 1, GL_FALSE, glm::value_ptr(markerModel));
            glUniformMatrix3fv(glGetUniformLocation(meshShader_.id(), "uNormalMatrix"), 1, GL_FALSE, glm::value_ptr(normalMatrixFromModel(markerModel)));
            glUniform3f(glGetUniformLocation(meshShader_.id(), "uColor"),
                        orbitColor.r * 1.10f,
                        orbitColor.g * 1.10f,
                        orbitColor.b * 1.10f);
            glUniform1f(glGetUniformLocation(meshShader_.id(), "uAlpha"), 0.95f);
            sphereMesh_.draw();
        }
    }
}

void SceneRenderer::renderPointCloud(const quantum::app::SimulationState& state,
                                     const glm::mat4& view,
                                     const glm::mat4& projection,
                                     int renderedPointCount,
                                     float animationTimeSeconds) {
    if (pointVao_ == 0 || pointCount_ == 0 || renderedPointCount <= 0) {
        return;
    }
    pointShader_.use();
    glUniformMatrix4fv(glGetUniformLocation(pointShader_.id(), "uView"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(pointShader_.id(), "uProjection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform1f(glGetUniformLocation(pointShader_.id(), "uPointSize"), state.view.pointSize);
    glUniform1f(glGetUniformLocation(pointShader_.id(), "uTime"), animationTimeSeconds);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glBindVertexArray(pointVao_);
    glDrawArrays(GL_POINTS, 0, std::min<GLsizei>(pointCount_, static_cast<GLsizei>(renderedPointCount)));
    glBindVertexArray(0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void SceneRenderer::renderVolume(const quantum::app::SimulationState& state,
                                 const glm::mat4& view,
                                 const glm::mat4& projection,
                                 int sliceCount,
                                 const VolumeSlicePlan& slicePlan,
                                 float animationTimeSeconds) {
    if (volumeTexture_.id() == 0 || volumeExtent_ <= 0.0f || sliceCount <= 0) {
        return;
    }
    volumeShader_.use();
    const glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(std::max(1.0f, volumeExtent_ * kSceneScale)));
    glUniformMatrix4fv(glGetUniformLocation(volumeShader_.id(), "uModel"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(volumeShader_.id(), "uView"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(volumeShader_.id(), "uProjection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform1f(glGetUniformLocation(volumeShader_.id(), "uExposure"), state.view.exposure);
    glUniform1f(glGetUniformLocation(volumeShader_.id(), "uTime"), animationTimeSeconds);
    glUniform1i(glGetUniformLocation(volumeShader_.id(), "uVolumeTexture"), 0);
    glUniform1i(glGetUniformLocation(volumeShader_.id(), "uSliceAxis"), slicePlan.axis);
    volumeTexture_.bind(GL_TEXTURE0);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    for (int slice = 0; slice < sliceCount; ++slice) {
        const int sliceIndex = slicePlan.reverse ? (sliceCount - 1 - slice) : slice;
        glUniform1f(glGetUniformLocation(volumeShader_.id(), "uSlice"), static_cast<float>(sliceIndex) /
                                                                    static_cast<float>(std::max(1, sliceCount - 1)));
        quadMesh_.draw();
    }
    glDepthMask(GL_TRUE);
}

} // namespace quantum::render
