#pragma once

#include "quantum/app/SimulationState.h"
#include "quantum/core/Logger.h"
#include "quantum/core/Paths.h"
#include "quantum/core/PerformanceTracker.h"
#include "quantum/physics/CloudGenerator.h"
#include "quantum/physics/ElementDatabase.h"
#include "quantum/physics/NumericalSolver.h"
#include "quantum/render/Camera.h"
#include "quantum/render/SceneRenderer.h"
#include "quantum/ui/AppUi.h"

#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>

#include <cstdint>
#include <filesystem>
#include <future>
#include <optional>
#include <string>

namespace quantum::app {

class Application {
public:
    explicit Application(std::filesystem::path projectRoot);
    ~Application();

    bool initialize();
    int run();

private:
    struct CloudBuildInput {
        quantum::physics::CloudRequest request;
        std::uint64_t generation = 0;
    };

    struct CloudBuildOutput {
        quantum::physics::CloudData cloud;
        std::uint64_t generation = 0;
    };

    bool initializeWindow();
    bool initializeImGui();
    void shutdown();
    void recomputeDerived();
    void startCloudBuild();
    void pumpCloudBuild();
    void applyCloudBuildResult(CloudBuildOutput&& result);
    void loadReferenceLines();
    void processSceneCameraInput(const quantum::ui::UiFrameResult& uiFrame);

    std::filesystem::path projectRoot_;
    GLFWwindow* window_ = nullptr;
    quantum::core::Paths paths_;
    quantum::core::Logger logger_;
    quantum::core::PerformanceTracker performance_;
    quantum::physics::ElementDatabase elementDatabase_;
    quantum::physics::ProbabilityCloudGenerator cloudGenerator_;
    quantum::physics::SchrodingerNumericalSolver numericalSolver_;
    quantum::render::OrbitCamera camera_;
    quantum::render::SceneRenderer sceneRenderer_;
    quantum::ui::AppUi appUi_;
    SimulationState state_;
    std::future<CloudBuildOutput> cloudBuildFuture_;
    std::optional<CloudBuildInput> queuedCloudBuild_;
    std::uint64_t cloudBuildGeneration_ = 0;
    bool cloudBuildInFlight_ = false;
};

} // namespace quantum::app
