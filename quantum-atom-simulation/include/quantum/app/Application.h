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

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace quantum::app {

struct DemoScriptStep {
    std::string name;
    int atomicNumber = 1;
    int chargeState = 0;
    quantum::physics::ApproximationMode approximationMode =
        quantum::physics::ApproximationMode::StrictHydrogenic;
    ModelKind modelKind = ModelKind::Bohr;
    CloudRenderMode cloudRenderMode = CloudRenderMode::Hybrid;
    quantum::physics::QuantumNumbers solverQn{2, 1, 0};
    std::vector<quantum::physics::SuperpositionComponent> components;
    bool useNumericalRadial = false;
    bool buildVolume = true;
    int principalN = 2;
    int maxSpectrumN = 6;
    TransitionSelection transition;
    quantum::render::OrbitCamera::Pose cameraPose;
};

class Application {
public:
    explicit Application(std::filesystem::path projectRoot);
    ~Application();

    bool initialize();
    int run();

private:
    enum class CloudBuildStage {
        Preview,
        Full
    };

    struct CloudBuildInput {
        quantum::physics::CloudRequest request;
        std::uint64_t generation = 0;
        std::uint64_t signature = 0;
        std::uint64_t targetSignature = 0;
        CloudBuildStage stage = CloudBuildStage::Full;
    };

    struct CloudBuildOutput {
        quantum::physics::CloudData cloud;
        std::uint64_t generation = 0;
        std::uint64_t signature = 0;
        std::uint64_t targetSignature = 0;
        CloudBuildStage stage = CloudBuildStage::Full;
        bool cancelled = false;
    };

    bool initializeWindow();
    bool initializeImGui();
    void shutdown();
    void recomputeDerived();
    void processDemoScriptRequests();
    void startCloudBuild();
    [[nodiscard]] CloudBuildInput makeCloudBuildInput(CloudBuildStage stage) const;
    [[nodiscard]] CloudBuildInput makePreviewCloudBuildInput(const CloudBuildInput& fullInput) const;
    [[nodiscard]] bool shouldUsePreviewBuild(const CloudBuildInput& fullInput) const;
    void launchCloudBuild(CloudBuildInput input);
    void pumpCloudBuild();
    void applyCloudBuildResult(CloudBuildOutput&& result);
    void loadReferenceLines();
    void processSceneCameraInput(const quantum::ui::UiFrameResult& uiFrame);
    void loadDefaultDemoScript();
    bool loadDemoScriptFromFile(const std::filesystem::path& scriptPath);
    bool saveDemoScriptToFile(const std::filesystem::path& scriptPath,
                              const std::vector<DemoScriptStep>& steps,
                              const std::string& scriptName);
    void captureCurrentDemoStep();
    void updateAutoDemo(double deltaSeconds);
    void applyDemoStep(std::size_t stepIndex, bool immediateCamera);
    [[nodiscard]] const std::vector<DemoScriptStep>& currentDemoSteps() const;
    [[nodiscard]] std::uint64_t cloudRequestSignature(const quantum::physics::CloudRequest& request,
                                                      CloudBuildStage stage) const;

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
    std::vector<DemoScriptStep> loadedDemoSteps_;
    std::vector<DemoScriptStep> recordedDemoSteps_;
    std::shared_ptr<std::atomic_bool> activeCloudCancelFlag_;
    std::uint64_t cloudBuildGeneration_ = 0;
    std::uint64_t activeCloudSignature_ = 0;
    std::uint64_t activeCloudTargetSignature_ = 0;
    std::uint64_t queuedCloudSignature_ = 0;
    std::uint64_t queuedCloudTargetSignature_ = 0;
    std::uint64_t appliedCloudSignature_ = 0;
    std::uint64_t appliedFullCloudTargetSignature_ = 0;
    bool cloudBuildInFlight_ = false;
    bool hasAppliedCloud_ = false;
    bool autoDemoPrimed_ = false;
    bool autoDemoWasEnabled_ = false;
    double autoDemoElapsedSeconds_ = 0.0;
    double lastFrameSeconds_ = 0.0;
};

} // namespace quantum::app
