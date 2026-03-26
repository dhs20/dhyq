#include "quantum/app/Application.h"

#include <GL/glew.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <chrono>
#include <fstream>
#include <sstream>

namespace quantum::app {
namespace {

std::filesystem::path resolveProjectRoot(std::filesystem::path path) {
    return std::filesystem::weakly_canonical(std::move(path));
}

struct SamplingPresetValues {
    double candidateMultiplier = 2.5;
    int radialCdfSamples = 4096;
    int angularScanResolution = 48;
    int monteCarloSamples = 4096;
};

SamplingPresetValues samplingPresetValues(SamplingQualityPreset preset) {
    switch (preset) {
    case SamplingQualityPreset::Fast:
        return {1.8, 2048, 32, 2048};
    case SamplingQualityPreset::HighFidelity:
        return {3.6, 6144, 64, 8192};
    case SamplingQualityPreset::Balanced:
    default:
        return {2.5, 4096, 48, 4096};
    }
}

quantum::physics::CloudRequest makeCloudRequest(const SimulationState& state,
                                                const quantum::physics::ElementRecord& element,
                                                const quantum::physics::NumericalSolverResult& solver) {
    auto components = state.cloud.components;
    if (components.empty()) {
        components.push_back({state.solver.qn, 1.0, 0.0, state.derived.activeZeff});
    }
    for (auto& component : components) {
        component.zeff = state.derived.activeZeff;
    }

    quantum::physics::NumericalRadialCache numericalCache;
    if (state.cloud.useNumericalRadial && solver.converged && !components.empty()) {
        numericalCache.valid = true;
        numericalCache.qn = state.solver.qn;
        numericalCache.zeff = state.derived.activeZeff;
        numericalCache.radiiMeters = solver.radiiMeters;
        numericalCache.radialValues = solver.radialFunction;
    }

    const auto samplingPreset = samplingPresetValues(state.cloud.samplingQuality);
    return {state.atomicNumber,
            quantum::physics::nuclearMassKg(element),
            state.bohr.useReducedMass,
            state.cloud.pointCount,
            state.cloud.volumeResolution,
            state.cloud.extentScale,
            state.cloud.buildVolume,
            state.cloud.adaptiveSampling,
            samplingPreset.candidateMultiplier,
            samplingPreset.radialCdfSamples,
            samplingPreset.angularScanResolution,
            samplingPreset.monteCarloSamples,
            components,
            numericalCache};
}

} // namespace

Application::Application(std::filesystem::path projectRoot)
    : projectRoot_(resolveProjectRoot(std::move(projectRoot))),
      paths_(projectRoot_) {}

Application::~Application() {
    shutdown();
}

bool Application::initialize() {
    if (!initializeWindow()) {
        return false;
    }
    if (!initializeImGui()) {
        return false;
    }

    if (!elementDatabase_.loadFromJson(paths_.asset("data/elements.json"))) {
        elementDatabase_.loadBuiltInSubset();
        logger_.warn("Failed to load assets/data/elements.json; using built-in element subset.");
    } else {
        logger_.info("Loaded element database from assets/data/elements.json.");
    }

    std::string rendererError;
    if (!sceneRenderer_.initialize(rendererError)) {
        logger_.error(rendererError);
        return false;
    }

    loadReferenceLines();
    recomputeDerived();
    logger_.info("Application initialized.");
    return true;
}

int Application::run() {
    while (!glfwWindowShouldClose(window_)) {
        performance_.beginFrame();
        glfwPollEvents();
        pumpCloudBuild();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        const auto uiFrame =
            appUi_.draw(state_, elementDatabase_, performance_.snapshot(), logger_, sceneRenderer_.sceneTexture(), sceneRenderer_.size());

        sceneRenderer_.resize(static_cast<int>(uiFrame.sceneSize.x), static_cast<int>(uiFrame.sceneSize.y));
        camera_.resize(static_cast<int>(uiFrame.sceneSize.x), static_cast<int>(uiFrame.sceneSize.y));
        processSceneCameraInput(uiFrame);

        recomputeDerived();
        const auto renderStats = sceneRenderer_.render(state_, camera_);
        performance_.setRenderStats(renderStats.gpuTimersSupported,
                                    renderStats.gpuFrameMs,
                                    renderStats.gpuPointMs,
                                    renderStats.gpuVolumeMs,
                                    renderStats.renderedPointCount,
                                    renderStats.volumeSliceCount,
                                    renderStats.volumeSliceAxis,
                                    renderStats.lodLevel,
                                    renderStats.cameraDistance,
                                    renderStats.pointBufferBytes,
                                    renderStats.volumeTextureBytes,
                                    renderStats.framebufferBytes);

        ImGui::Render();
        int displayWidth = 0;
        int displayHeight = 0;
        glfwGetFramebufferSize(window_, &displayWidth, &displayHeight);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, displayWidth, displayHeight);
        glClearColor(0.02f, 0.02f, 0.02f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window_);
        performance_.endFrame();
    }

    return 0;
}

bool Application::initializeWindow() {
    if (glfwInit() == GLFW_FALSE) {
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

    window_ = glfwCreateWindow(1600, 960, "Quantum Atom Simulation", nullptr, nullptr);
    if (window_ == nullptr) {
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        return false;
    }
    return true;
}

bool Application::initializeImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;

    if (!ImGui_ImplGlfw_InitForOpenGL(window_, true)) {
        return false;
    }
    if (!ImGui_ImplOpenGL3_Init("#version 330")) {
        return false;
    }
    return true;
}

void Application::shutdown() {
    if (cloudBuildFuture_.valid()) {
        cloudBuildFuture_.wait();
    }
    cloudBuildInFlight_ = false;
    queuedCloudBuild_.reset();

    sceneRenderer_.shutdown();

    if (ImGui::GetCurrentContext() != nullptr) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    if (window_ != nullptr) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
    glfwTerminate();
}

void Application::recomputeDerived() {
    pumpCloudBuild();
    if (!state_.dirty.physics && !state_.dirty.cloud && !state_.dirty.solver && !state_.dirty.reference) {
        return;
    }

    try {
        state_.derived.element = elementDatabase_.elementByAtomicNumber(state_.atomicNumber);
    } catch (...) {
        elementDatabase_.loadBuiltInSubset();
        state_.derived.element = elementDatabase_.elementByAtomicNumber(1);
    }

    state_.derived.electronCount = std::max(0, state_.atomicNumber - state_.chargeState);
    state_.derived.strictHydrogenicValid = state_.derived.electronCount == 1;
    state_.derived.approximationWarning.clear();

    const double nuclearMass = quantum::physics::nuclearMassKg(state_.derived.element);
    if (state_.dirty.physics) {
        state_.derived.configuration = quantum::physics::buildElectronConfiguration(state_.atomicNumber, state_.chargeState);
        state_.derived.slater = quantum::physics::computeSlaterShielding(
            state_.atomicNumber, state_.derived.configuration, state_.transition.initial.n, state_.transition.initial.l);

        if (state_.approximationMode == quantum::physics::ApproximationMode::StrictHydrogenic) {
            state_.derived.activeZeff = static_cast<double>(state_.atomicNumber);
            if (!state_.derived.strictHydrogenicValid) {
                state_.derived.approximationWarning =
                    "Strict hydrogenic mode is only physically valid for one-electron ions. Adjust charge state or switch to multi-electron approximation.";
            }
        } else {
            state_.derived.activeZeff = state_.derived.slater.zeff;
            state_.derived.approximationWarning =
                "Multi-electron mode uses Aufbau + Slater Z_eff and a hydrogenic replacement Z -> Z_eff. This is a teaching approximation, not a spectroscopic-quality model.";
        }

        state_.derived.bohrMetrics = quantum::physics::computeHydrogenicMetrics(
            state_.atomicNumber, state_.bohr.principalN, state_.derived.activeZeff, nuclearMass, state_.bohr.useReducedMass);

        quantum::physics::TransitionRequest transitionRequest;
        transitionRequest.initial = state_.transition.initial;
        transitionRequest.final = state_.transition.final;
        transitionRequest.rules = state_.transition.enforceRules ? quantum::physics::TransitionRuleSet::ElectricDipole
                                                                 : quantum::physics::TransitionRuleSet::None;
        transitionRequest.enforceDeltaM = state_.transition.enforceDeltaM;
        transitionRequest.nuclearCharge = state_.atomicNumber;
        transitionRequest.initialZeff = state_.derived.activeZeff;
        transitionRequest.finalZeff = state_.derived.activeZeff;
        transitionRequest.nuclearMassKg = nuclearMass;
        transitionRequest.useReducedMass = state_.bohr.useReducedMass;
        state_.derived.transition = quantum::physics::computeTransition(transitionRequest);
        state_.derived.spectrumLines = quantum::physics::generateSpectrum(state_.atomicNumber,
                                                                          state_.bohr.maxSpectrumN,
                                                                          state_.derived.activeZeff,
                                                                          nuclearMass,
                                                                          transitionRequest.rules,
                                                                          state_.transition.enforceDeltaM,
                                                                          state_.bohr.useReducedMass);
        state_.derived.energyLevels = quantum::physics::generateEnergyLevels(
            state_.atomicNumber, state_.bohr.maxSpectrumN, state_.derived.activeZeff, nuclearMass, state_.bohr.useReducedMass);
        state_.derived.radialDistribution = quantum::physics::sampleRadialDistribution(
            state_.atomicNumber,
            state_.solver.qn,
            state_.derived.activeZeff,
            nuclearMass,
            std::max(5.0e-10, state_.derived.bohrMetrics.orbitalRadiusM * 8.0),
            512);

        state_.dirty.solver = true;
        state_.dirty.cloud = true;
    }

    if (state_.dirty.solver) {
        const auto solverResult = numericalSolver_.solve({state_.atomicNumber,
                                                          state_.derived.activeZeff,
                                                          nuclearMass,
                                                          state_.bohr.useReducedMass,
                                                          state_.solver.qn,
                                                          state_.solver.gridPoints,
                                                          state_.solver.convergencePasses,
                                                          state_.solver.maxScaledRadius});
        state_.derived.solver = solverResult;
        if (solverResult.converged && state_.cloud.useNumericalRadial && !state_.cloud.components.empty() &&
            state_.solver.qn.n == state_.cloud.components.front().qn.n &&
            state_.solver.qn.l == state_.cloud.components.front().qn.l) {
            state_.derived.radialDistribution = solverResult.radialDistribution;
            state_.dirty.cloud = true;
        }
    }

    if (state_.dirty.reference) {
        loadReferenceLines();
    }

    if (state_.dirty.cloud) {
        startCloudBuild();
    }

    state_.dirty = {};
}

void Application::startCloudBuild() {
    CloudBuildInput input;
    input.request = makeCloudRequest(state_, state_.derived.element, state_.derived.solver);
    input.generation = ++cloudBuildGeneration_;

    if (cloudBuildInFlight_) {
        queuedCloudBuild_ = std::move(input);
        performance_.setCloudBuildState(true, true);
        return;
    }

    auto request = input.request;
    const auto generation = input.generation;
    cloudBuildFuture_ = std::async(std::launch::async, [request = std::move(request), generation]() mutable {
        quantum::physics::ProbabilityCloudGenerator generator;
        CloudBuildOutput output;
        output.generation = generation;
        output.cloud = generator.generate(request);
        return output;
    });
    cloudBuildInFlight_ = true;
    performance_.setCloudBuildState(true, false);
}

void Application::pumpCloudBuild() {
    if (!cloudBuildInFlight_ || !cloudBuildFuture_.valid()) {
        performance_.setCloudBuildState(false, queuedCloudBuild_.has_value());
        return;
    }

    if (cloudBuildFuture_.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) {
        performance_.setCloudBuildState(true, queuedCloudBuild_.has_value());
        return;
    }

    auto result = cloudBuildFuture_.get();
    cloudBuildInFlight_ = false;
    performance_.setCloudBuildState(false, queuedCloudBuild_.has_value());

    if (result.generation == cloudBuildGeneration_) {
        applyCloudBuildResult(std::move(result));
    }

    if (queuedCloudBuild_.has_value()) {
        const auto queuedGeneration = queuedCloudBuild_->generation;
        auto request = queuedCloudBuild_->request;
        queuedCloudBuild_.reset();
        cloudBuildFuture_ = std::async(std::launch::async, [request = std::move(request), queuedGeneration]() mutable {
            quantum::physics::ProbabilityCloudGenerator generator;
            CloudBuildOutput output;
            output.generation = queuedGeneration;
            output.cloud = generator.generate(request);
            return output;
        });
        cloudBuildInFlight_ = true;
        performance_.setCloudBuildState(true, false);
    }
}

void Application::applyCloudBuildResult(CloudBuildOutput&& result) {
    state_.derived.cloud = std::move(result.cloud);
    sceneRenderer_.uploadCloud(state_.derived.cloud);
    performance_.setCloudBuildMs(state_.derived.cloud.stats.pointBuildMs);
    performance_.setVolumeBuildMs(state_.derived.cloud.stats.volumeBuildMs);
    performance_.setSamplingStats(state_.derived.cloud.stats.requestedPointCount,
                                  state_.derived.cloud.stats.acceptedCount,
                                  state_.derived.cloud.stats.candidateCount,
                                  state_.derived.cloud.stats.candidateMultiplier,
                                  state_.derived.cloud.stats.radialCdfSamples,
                                  state_.derived.cloud.stats.angularScanResolution,
                                  state_.derived.cloud.stats.monteCarloSamples);
}

void Application::loadReferenceLines() {
    state_.derived.referenceLines.clear();
    const auto csvPath = projectRoot_ / state_.reference.csvPath;
    std::ifstream stream(csvPath);
    if (!stream.is_open()) {
        logger_.warn("Reference line CSV not found: " + csvPath.string());
        return;
    }

    std::string line;
    std::getline(stream, line);
    while (std::getline(stream, line)) {
        if (line.empty()) {
            continue;
        }
        std::stringstream row(line);
        std::string token;
        std::vector<std::string> tokens;
        while (std::getline(row, token, ',')) {
            tokens.push_back(token);
        }
        if (tokens.size() < 7) {
            continue;
        }

        quantum::physics::SpectrumLine referenceLine;
        referenceLine.seriesName = tokens[0];
        referenceLine.initial.n = std::stoi(tokens[1]);
        referenceLine.initial.l = std::stoi(tokens[2]);
        referenceLine.final.n = std::stoi(tokens[3]);
        referenceLine.final.l = std::stoi(tokens[4]);
        referenceLine.wavelengthNm = std::stod(tokens[5]);
        referenceLine.photonEnergyEv = std::stod(tokens[6]);
        referenceLine.allowed = true;
        state_.derived.referenceLines.push_back(referenceLine);
    }
}

void Application::processSceneCameraInput(const quantum::ui::UiFrameResult& uiFrame) {
    ImGuiIO& io = ImGui::GetIO();
    state_.view.interactionActive = false;
    if (!uiFrame.sceneHovered || io.WantCaptureMouse) {
        return;
    }

    const bool orbiting = ImGui::IsMouseDragging(ImGuiMouseButton_Left);
    const bool panning = ImGui::IsMouseDragging(ImGuiMouseButton_Middle);
    const bool dollying = std::abs(io.MouseWheel) > 1e-6f;
    state_.view.interactionActive = orbiting || panning || dollying;

    if (orbiting) {
        camera_.orbit({io.MouseDelta.x, -io.MouseDelta.y});
    }
    if (panning) {
        camera_.pan({io.MouseDelta.x, io.MouseDelta.y});
    }
    if (dollying) {
        camera_.dolly(io.MouseWheel);
    }
}

} // namespace quantum::app
