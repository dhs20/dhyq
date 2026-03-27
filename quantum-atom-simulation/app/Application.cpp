#include "quantum/app/Application.h"

#include <GL/glew.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <nlohmann/json.hpp>
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

std::filesystem::path firstExistingPath(std::initializer_list<std::filesystem::path> candidates) {
    for (const auto& candidate : candidates) {
        if (!candidate.empty() && std::filesystem::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

std::filesystem::path detectChineseFontPath() {
    if (const char* windir = std::getenv("WINDIR"); windir != nullptr) {
        const std::filesystem::path fontDir = std::filesystem::path(windir) / "Fonts";
        const auto windowsFont = firstExistingPath(
            {fontDir / "msyh.ttc", fontDir / "msyh.ttf", fontDir / "simhei.ttf", fontDir / "simsun.ttc"});
        if (!windowsFont.empty()) {
            return windowsFont;
        }
    }

    return firstExistingPath({"/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
                              "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
                              "/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc",
                              "/System/Library/Fonts/PingFang.ttc",
                              "/System/Library/Fonts/Hiragino Sans GB.ttc"});
}

void configureUiFonts(ImGuiIO& io, quantum::core::Logger& logger) {
    io.Fonts->Clear();

    ImFontConfig config;
    config.OversampleH = 2;
    config.OversampleV = 2;
    config.PixelSnapH = false;

    const auto fontPath = detectChineseFontPath();
    if (!fontPath.empty()) {
        if (ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath.string().c_str(),
                                                        18.0f,
                                                        &config,
                                                        io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
            font != nullptr) {
            io.FontDefault = font;
            logger.info("已加载中文界面字体: " + fontPath.string());
            return;
        }
    }

    io.Fonts->AddFontDefault();
    logger.warn("未找到可用的中文字体，界面可能无法正确显示中文。");
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
            state.cloud.pointCount,
            state.cloud.volumeResolution,
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

template <typename T>
void hashCombine(std::uint64_t& seed, const T& value) {
    const std::uint64_t hash = static_cast<std::uint64_t>(std::hash<T>{}(value));
    seed ^= hash + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U);
}

std::int64_t quantizeDouble(double value, double scale = 1.0e9) {
    return static_cast<std::int64_t>(std::llround(value * scale));
}

std::uint64_t signatureForRequest(const quantum::physics::CloudRequest& request) {
    std::uint64_t seed = 1469598103934665603ULL;
    hashCombine(seed, request.nuclearCharge);
    hashCombine(seed, quantizeDouble(request.nuclearMassKg, 1.0e24));
    hashCombine(seed, request.useReducedMass);
    hashCombine(seed, request.pointCount);
    hashCombine(seed, request.targetPointCount);
    hashCombine(seed, request.volumeResolution);
    hashCombine(seed, request.targetVolumeResolution);
    hashCombine(seed, quantizeDouble(request.extentScale, 1.0e6));
    hashCombine(seed, request.buildVolume);
    hashCombine(seed, request.adaptiveSampling);
    hashCombine(seed, quantizeDouble(request.candidateMultiplier, 1.0e6));
    hashCombine(seed, request.radialCdfSamples);
    hashCombine(seed, request.angularScanResolution);
    hashCombine(seed, request.monteCarloSamples);

    hashCombine(seed, request.numericalRadial.valid);
    hashCombine(seed, request.numericalRadial.qn.n);
    hashCombine(seed, request.numericalRadial.qn.l);
    hashCombine(seed, request.numericalRadial.qn.m);
    hashCombine(seed, quantizeDouble(request.numericalRadial.zeff, 1.0e6));
    hashCombine(seed, request.numericalRadial.radiiMeters.size());
    if (!request.numericalRadial.radiiMeters.empty()) {
        const std::size_t step = std::max<std::size_t>(1, request.numericalRadial.radiiMeters.size() / 12);
        for (std::size_t index = 0; index < request.numericalRadial.radiiMeters.size(); index += step) {
            hashCombine(seed, quantizeDouble(request.numericalRadial.radiiMeters[index], 1.0e15));
            hashCombine(seed, quantizeDouble(request.numericalRadial.radialValues[index], 1.0e12));
        }
        hashCombine(seed, quantizeDouble(request.numericalRadial.radiiMeters.back(), 1.0e15));
        hashCombine(seed, quantizeDouble(request.numericalRadial.radialValues.back(), 1.0e12));
    }

    hashCombine(seed, request.components.size());
    for (const auto& component : request.components) {
        hashCombine(seed, component.qn.n);
        hashCombine(seed, component.qn.l);
        hashCombine(seed, component.qn.m);
        hashCombine(seed, quantizeDouble(component.amplitude, 1.0e6));
        hashCombine(seed, quantizeDouble(component.phaseRadians, 1.0e6));
        hashCombine(seed, quantizeDouble(component.zeff, 1.0e6));
    }
    return seed;
}

std::string approximationModeTag(quantum::physics::ApproximationMode mode) {
    return (mode == quantum::physics::ApproximationMode::MultiElectronApproximation) ? "multi_electron_approximation"
                                                                                      : "strict_hydrogenic";
}

std::string modelKindTag(ModelKind kind) {
    switch (kind) {
    case ModelKind::WaveParticle:
        return "wave_particle";
    case ModelKind::SchrodingerCloud:
        return "schrodinger_cloud";
    case ModelKind::Compare:
        return "compare";
    case ModelKind::Bohr:
    default:
        return "bohr";
    }
}

std::string cloudModeTag(CloudRenderMode mode) {
    switch (mode) {
    case CloudRenderMode::PointCloud:
        return "point_cloud";
    case CloudRenderMode::VolumeSlices:
        return "volume_slices";
    case CloudRenderMode::Hybrid:
    default:
        return "hybrid";
    }
}

quantum::physics::ApproximationMode parseApproximationMode(const std::string& value) {
    return (value == "multi_electron_approximation") ? quantum::physics::ApproximationMode::MultiElectronApproximation
                                                     : quantum::physics::ApproximationMode::StrictHydrogenic;
}

ModelKind parseModelKind(const std::string& value) {
    if (value == "wave_particle") {
        return ModelKind::WaveParticle;
    }
    if (value == "schrodinger_cloud") {
        return ModelKind::SchrodingerCloud;
    }
    if (value == "compare") {
        return ModelKind::Compare;
    }
    return ModelKind::Bohr;
}

CloudRenderMode parseCloudMode(const std::string& value) {
    if (value == "point_cloud") {
        return CloudRenderMode::PointCloud;
    }
    if (value == "volume_slices") {
        return CloudRenderMode::VolumeSlices;
    }
    return CloudRenderMode::Hybrid;
}

nlohmann::json quantumNumbersToJson(const quantum::physics::QuantumNumbers& qn) {
    return nlohmann::json::array({qn.n, qn.l, qn.m});
}

quantum::physics::QuantumNumbers quantumNumbersFromJson(const nlohmann::json& node) {
    quantum::physics::QuantumNumbers qn;
    if (node.is_array() && node.size() >= 2) {
        qn.n = node.at(0).get<int>();
        qn.l = node.at(1).get<int>();
        qn.m = (node.size() >= 3) ? node.at(2).get<int>() : 0;
    }
    return qn;
}

nlohmann::json poseToJson(const quantum::render::OrbitCamera::Pose& pose) {
    return {{"yaw", pose.yaw},
            {"pitch", pose.pitch},
            {"distance", pose.distance},
            {"target", {pose.target.x, pose.target.y, pose.target.z}}};
}

quantum::render::OrbitCamera::Pose poseFromJson(const nlohmann::json& node) {
    quantum::render::OrbitCamera::Pose pose;
    pose.yaw = node.value("yaw", pose.yaw);
    pose.pitch = node.value("pitch", pose.pitch);
    pose.distance = node.value("distance", pose.distance);
    if (node.contains("target") && node.at("target").is_array() && node.at("target").size() >= 3) {
        pose.target.x = node.at("target")[0].get<float>();
        pose.target.y = node.at("target")[1].get<float>();
        pose.target.z = node.at("target")[2].get<float>();
    }
    return pose;
}

nlohmann::json demoStepToJson(const DemoScriptStep& step) {
    nlohmann::json components = nlohmann::json::array();
    for (const auto& component : step.components) {
        components.push_back({{"qn", quantumNumbersToJson(component.qn)},
                              {"amplitude", component.amplitude},
                              {"phase", component.phaseRadians}});
    }
    return {{"name", step.name},
            {"atomic_number", step.atomicNumber},
            {"charge_state", step.chargeState},
            {"approximation", approximationModeTag(step.approximationMode)},
            {"model", modelKindTag(step.modelKind)},
            {"cloud_render_mode", cloudModeTag(step.cloudRenderMode)},
            {"solver_qn", quantumNumbersToJson(step.solverQn)},
            {"components", components},
            {"use_numerical_radial", step.useNumericalRadial},
            {"build_volume", step.buildVolume},
            {"principal_n", step.principalN},
            {"max_spectrum_n", step.maxSpectrumN},
            {"transition",
             {{"initial", quantumNumbersToJson(step.transition.initial)},
              {"final", quantumNumbersToJson(step.transition.final)},
              {"enforce_rules", step.transition.enforceRules},
              {"enforce_delta_m", step.transition.enforceDeltaM}}},
            {"camera", poseToJson(step.cameraPose)}};
}

DemoScriptStep demoStepFromJson(const nlohmann::json& node) {
    DemoScriptStep step;
    step.name = node.value("name", "脚本步骤");
    step.atomicNumber = node.value("atomic_number", 1);
    step.chargeState = node.value("charge_state", 0);
    step.approximationMode = parseApproximationMode(node.value("approximation", std::string("strict_hydrogenic")));
    step.modelKind = parseModelKind(node.value("model", std::string("bohr")));
    step.cloudRenderMode = parseCloudMode(node.value("cloud_render_mode", std::string("hybrid")));
    if (node.contains("solver_qn")) {
        step.solverQn = quantumNumbersFromJson(node.at("solver_qn"));
    }
    if (node.contains("components") && node.at("components").is_array()) {
        for (const auto& componentNode : node.at("components")) {
            quantum::physics::SuperpositionComponent component;
            component.qn = quantumNumbersFromJson(componentNode.at("qn"));
            component.amplitude = componentNode.value("amplitude", 1.0);
            component.phaseRadians = componentNode.value("phase", 0.0);
            step.components.push_back(component);
        }
    }
    step.useNumericalRadial = node.value("use_numerical_radial", false);
    step.buildVolume = node.value("build_volume", true);
    step.principalN = node.value("principal_n", 2);
    step.maxSpectrumN = node.value("max_spectrum_n", 6);
    if (node.contains("transition")) {
        const auto& transitionNode = node.at("transition");
        if (transitionNode.contains("initial")) {
            step.transition.initial = quantumNumbersFromJson(transitionNode.at("initial"));
        }
        if (transitionNode.contains("final")) {
            step.transition.final = quantumNumbersFromJson(transitionNode.at("final"));
        }
        step.transition.enforceRules = transitionNode.value("enforce_rules", true);
        step.transition.enforceDeltaM = transitionNode.value("enforce_delta_m", false);
    }
    if (node.contains("camera")) {
        step.cameraPose = poseFromJson(node.at("camera"));
    }
    return step;
}

const std::array<DemoScriptStep, 3>& builtInDemoSteps() {
    static const std::array<DemoScriptStep, 3> kSteps{{
        {"氢原子玻尔谱线",
         1,
         0,
         quantum::physics::ApproximationMode::StrictHydrogenic,
         ModelKind::Bohr,
         CloudRenderMode::PointCloud,
         {2, 1, 0},
         {{{2, 1, 0}, 1.0, 0.0, 1.0}},
         false,
         false,
         2,
         6,
         {{2, 1, 0}, {1, 0, 0}, true, false},
         {0.2f, 0.45f, 10.0f, glm::vec3(0.0f, 0.2f, 0.0f)}},
        {"氦离子相位云",
         2,
         1,
         quantum::physics::ApproximationMode::StrictHydrogenic,
         ModelKind::SchrodingerCloud,
         CloudRenderMode::Hybrid,
         {3, 2, 0},
         {{{3, 2, 0}, 1.0, 0.0, 2.0}},
         true,
         true,
         3,
         7,
         {{3, 2, 0}, {2, 1, 0}, true, false},
         {1.15f, 0.6f, 14.0f, glm::vec3(0.0f, 0.6f, 0.0f)}},
        {"氖原子多电子近似",
         10,
         0,
         quantum::physics::ApproximationMode::MultiElectronApproximation,
         ModelKind::Compare,
         CloudRenderMode::Hybrid,
         {2, 1, 0},
         {{{2, 1, 0}, 0.8, 0.0, 1.0}, {{2, 1, 1}, 0.6, 1.57, 1.0}},
         false,
         true,
         2,
         6,
         {{3, 1, 0}, {2, 0, 0}, true, false},
         {-0.75f, 0.4f, 16.0f, glm::vec3(0.0f, 0.3f, 0.0f)}}}};
    return kSteps;
}

} // namespace

Application::Application(std::filesystem::path projectRoot)
    : projectRoot_(resolveProjectRoot(std::move(projectRoot))),
      paths_(projectRoot_) {}

Application::~Application() {
    shutdown();
}

bool Application::initialize() {
    const auto logPath = projectRoot_ / "logs" / "runtime.log";
    logger_.setOutputFile(logPath);
    logger_.info("运行日志已写入: " + logPath.string());

    if (!initializeWindow()) {
        return false;
    }
    if (!initializeImGui()) {
        return false;
    }

    if (!elementDatabase_.loadFromJson(paths_.asset("data/elements.json"))) {
        elementDatabase_.loadBuiltInSubset();
        logger_.warn("加载 assets/data/elements.json 失败，已回退到内置元素子集。");
    } else {
        logger_.info("已从 assets/data/elements.json 加载元素数据库。");
    }

    std::string rendererError;
    if (!sceneRenderer_.initialize(rendererError)) {
        logger_.error(rendererError);
        return false;
    }

    loadReferenceLines();
    loadDefaultDemoScript();
    lastFrameSeconds_ = glfwGetTime();
    recomputeDerived();
    logger_.info("应用初始化完成。");
    return true;
}

int Application::run() {
    while (!glfwWindowShouldClose(window_)) {
        const double nowSeconds = glfwGetTime();
        const double deltaSeconds = std::max(0.0, nowSeconds - lastFrameSeconds_);
        lastFrameSeconds_ = nowSeconds;

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
        processDemoScriptRequests();
        updateAutoDemo(deltaSeconds);
        camera_.update(static_cast<float>(deltaSeconds));

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

    window_ = glfwCreateWindow(1600, 960, "量子原子可视化系统", nullptr, nullptr);
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
    configureUiFonts(io, logger_);

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
    if (activeCloudCancelFlag_ != nullptr) {
        activeCloudCancelFlag_->store(true, std::memory_order_relaxed);
    }
    if (cloudBuildFuture_.valid()) {
        cloudBuildFuture_.wait();
    }
    cloudBuildInFlight_ = false;
    queuedCloudBuild_.reset();
    activeCloudCancelFlag_.reset();

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
                    "严格氢样模式只对单电子离子严格成立。请调整电荷态，或切换到多电子近似模式。";
            }
        } else {
            state_.derived.activeZeff = state_.derived.slater.zeff;
            state_.derived.approximationWarning =
                "多电子模式使用 Aufbau 填充与 Slater Z_eff，并用 Z -> Z_eff 的氢样替代公式估算。这是教学近似，不是高精度光谱模型。";
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
            512,
            state_.bohr.useReducedMass);

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

void Application::processDemoScriptRequests() {
    const auto resolveScriptPath = [this]() {
        const std::filesystem::path configured(state_.demo.scriptPath);
        return configured.is_absolute() ? configured : (projectRoot_ / configured);
    };

    if (state_.demo.clearRecordedStepsRequested) {
        recordedDemoSteps_.clear();
        state_.demo.recordedStepCount = 0;
        state_.demo.scriptStatus = "已清空录制步骤";
        state_.demo.clearRecordedStepsRequested = false;
        logger_.info("已清空演示脚本录制缓存。");
    }

    if (state_.demo.captureStepRequested) {
        captureCurrentDemoStep();
        state_.demo.captureStepRequested = false;
    }

    if (state_.demo.loadScriptRequested) {
        (void)loadDemoScriptFromFile(resolveScriptPath());
        state_.demo.loadScriptRequested = false;
    }

    if (state_.demo.exportBuiltInScriptRequested) {
        const std::vector<DemoScriptStep> steps(builtInDemoSteps().begin(), builtInDemoSteps().end());
        saveDemoScriptToFile(resolveScriptPath(), steps, "内置自动演示");
        state_.demo.exportBuiltInScriptRequested = false;
    }

    if (state_.demo.saveScriptRequested) {
        const bool hasRecorded = !recordedDemoSteps_.empty();
        const auto& steps = hasRecorded ? recordedDemoSteps_ : currentDemoSteps();
        saveDemoScriptToFile(resolveScriptPath(), steps, hasRecorded ? "录制脚本" : state_.demo.activeScriptName);
        state_.demo.saveScriptRequested = false;
    }
}

void Application::loadDefaultDemoScript() {
    state_.demo.activeScriptName = "内置演示";
    state_.demo.scriptStatus = "使用内置自动演示";
    state_.demo.loadedStepCount = 0;
    state_.demo.recordedStepCount = 0;

    const auto scriptPath = projectRoot_ / state_.demo.scriptPath;
    if (std::filesystem::exists(scriptPath)) {
        (void)loadDemoScriptFromFile(scriptPath);
    } else {
        logger_.warn("未找到默认演示脚本文件，继续使用内置步骤。");
    }
}

bool Application::loadDemoScriptFromFile(const std::filesystem::path& scriptPath) {
    std::ifstream stream(scriptPath);
    if (!stream.is_open()) {
        state_.demo.scriptStatus = "脚本文件打开失败";
        logger_.warn("无法打开演示脚本文件: " + scriptPath.string());
        return false;
    }

    try {
        nlohmann::json root;
        stream >> root;
        std::vector<DemoScriptStep> steps;
        if (root.contains("steps") && root.at("steps").is_array()) {
            for (const auto& stepNode : root.at("steps")) {
                steps.push_back(demoStepFromJson(stepNode));
            }
        }
        if (steps.empty()) {
            state_.demo.scriptStatus = "脚本中没有可播放步骤";
            logger_.warn("演示脚本文件中没有有效步骤: " + scriptPath.string());
            return false;
        }

        loadedDemoSteps_ = std::move(steps);
        state_.demo.loadedStepCount = static_cast<int>(loadedDemoSteps_.size());
        state_.demo.activeScriptName = root.value("script_name", scriptPath.stem().string());
        state_.demo.scriptStatus = "已加载脚本文件";
        logger_.info("已加载演示脚本文件: " + scriptPath.string());
        return true;
    } catch (const std::exception& exception) {
        state_.demo.scriptStatus = "脚本解析失败";
        logger_.error("演示脚本解析失败: " + std::string(exception.what()));
        return false;
    }
}

bool Application::saveDemoScriptToFile(const std::filesystem::path& scriptPath,
                                       const std::vector<DemoScriptStep>& steps,
                                       const std::string& scriptName) {
    try {
        std::filesystem::create_directories(scriptPath.parent_path());
        nlohmann::json root;
        root["script_name"] = scriptName;
        root["version"] = 1;
        root["steps"] = nlohmann::json::array();
        for (const auto& step : steps) {
            root["steps"].push_back(demoStepToJson(step));
        }

        std::ofstream stream(scriptPath, std::ios::binary | std::ios::trunc);
        if (!stream.is_open()) {
            logger_.error("无法写入演示脚本文件: " + scriptPath.string());
            return false;
        }
        stream << root.dump(2);
        logger_.info("已保存演示脚本文件: " + scriptPath.string());
        return true;
    } catch (const std::exception& exception) {
        logger_.error("保存演示脚本失败: " + std::string(exception.what()));
        return false;
    }
}

void Application::captureCurrentDemoStep() {
    DemoScriptStep step;
    step.name = "录制步骤 " + std::to_string(recordedDemoSteps_.size() + 1);
    step.atomicNumber = state_.atomicNumber;
    step.chargeState = state_.chargeState;
    step.approximationMode = state_.approximationMode;
    step.modelKind = state_.modelKind;
    step.cloudRenderMode = state_.view.cloudRenderMode;
    step.solverQn = state_.solver.qn;
    step.components = state_.cloud.components;
    step.useNumericalRadial = state_.cloud.useNumericalRadial;
    step.buildVolume = state_.cloud.buildVolume;
    step.principalN = state_.bohr.principalN;
    step.maxSpectrumN = state_.bohr.maxSpectrumN;
    step.transition = state_.transition;
    step.cameraPose = camera_.pose();
    if (step.components.empty()) {
        step.components.push_back({state_.solver.qn, 1.0, 0.0, state_.derived.activeZeff});
    }
    recordedDemoSteps_.push_back(std::move(step));
    state_.demo.recordedStepCount = static_cast<int>(recordedDemoSteps_.size());
    state_.demo.scriptStatus = "已录制当前场景";
    logger_.info("已记录当前场景到演示脚本缓存。");
}

const std::vector<DemoScriptStep>& Application::currentDemoSteps() const {
    if (state_.demo.useScriptPlayback && !loadedDemoSteps_.empty()) {
        return loadedDemoSteps_;
    }
    static const std::vector<DemoScriptStep> kBuiltInSteps(builtInDemoSteps().begin(), builtInDemoSteps().end());
    return kBuiltInSteps;
}

Application::CloudBuildInput Application::makeCloudBuildInput(CloudBuildStage stage) const {
    CloudBuildInput input;
    input.request = makeCloudRequest(state_, state_.derived.element, state_.derived.solver);
    input.request.targetPointCount = input.request.pointCount;
    input.request.targetVolumeResolution = input.request.volumeResolution;
    input.targetSignature = signatureForRequest(input.request);
    input.stage = stage;
    input.signature = cloudRequestSignature(input.request, stage);
    return input;
}

Application::CloudBuildInput Application::makePreviewCloudBuildInput(const CloudBuildInput& fullInput) const {
    CloudBuildInput preview = fullInput;
    preview.stage = CloudBuildStage::Preview;
    preview.request.pointCount =
        std::min(fullInput.request.targetPointCount, std::max(4000, state_.cloud.previewPointCount));
    if (preview.request.buildVolume) {
        preview.request.volumeResolution =
            std::min(fullInput.request.targetVolumeResolution, std::max(8, state_.cloud.previewVolumeResolution));
    } else {
        preview.request.volumeResolution = 0;
    }
    preview.request.radialCdfSamples = std::max(512, fullInput.request.radialCdfSamples / 2);
    preview.request.angularScanResolution = std::max(16, fullInput.request.angularScanResolution / 2);
    preview.request.monteCarloSamples = std::max(512, fullInput.request.monteCarloSamples / 2);
    preview.request.candidateMultiplier = std::clamp(fullInput.request.candidateMultiplier * 0.75, 1.2, 2.0);
    preview.signature = cloudRequestSignature(preview.request, CloudBuildStage::Preview);
    return preview;
}

bool Application::shouldUsePreviewBuild(const CloudBuildInput& fullInput) const {
    return state_.cloud.progressiveBuild &&
           (fullInput.request.pointCount > std::max(4000, state_.cloud.previewPointCount) ||
            (fullInput.request.buildVolume &&
             fullInput.request.volumeResolution > std::max(8, state_.cloud.previewVolumeResolution)));
}

void Application::updateAutoDemo(double deltaSeconds) {
    const auto& steps = currentDemoSteps();
    if (steps.empty()) {
        return;
    }

    if (!state_.demo.enabled) {
        autoDemoElapsedSeconds_ = 0.0;
        if (autoDemoWasEnabled_) {
            state_.demo.currentStepName = "手动";
            state_.demo.stepProgress = 0.0;
        }
        autoDemoWasEnabled_ = false;
        autoDemoPrimed_ = false;
        state_.demo.restartRequested = false;
        state_.demo.advanceRequested = false;
        return;
    }

    autoDemoWasEnabled_ = true;
    if (state_.demo.restartRequested || !autoDemoPrimed_) {
        autoDemoElapsedSeconds_ = 0.0;
        applyDemoStep(0, !autoDemoPrimed_);
        state_.demo.restartRequested = false;
        state_.demo.advanceRequested = false;
        autoDemoPrimed_ = true;
        return;
    }

    if (state_.demo.advanceRequested) {
        autoDemoElapsedSeconds_ = 0.0;
        applyDemoStep(static_cast<std::size_t>(state_.demo.currentStepIndex + 1), false);
        state_.demo.advanceRequested = false;
        return;
    }

    autoDemoElapsedSeconds_ += deltaSeconds;
    const double duration = std::max(1.0, state_.demo.stepDurationSeconds);
    state_.demo.stepProgress = std::clamp(autoDemoElapsedSeconds_ / duration, 0.0, 1.0);
    if (autoDemoElapsedSeconds_ < duration) {
        return;
    }

    autoDemoElapsedSeconds_ = 0.0;
    const std::size_t nextIndex = static_cast<std::size_t>(state_.demo.currentStepIndex + 1);
    if (nextIndex < steps.size()) {
        applyDemoStep(nextIndex, false);
        return;
    }

    if (state_.demo.loop) {
        applyDemoStep(0, false);
    } else {
        state_.demo.enabled = false;
        state_.demo.currentStepName = "手动";
        state_.demo.stepProgress = 0.0;
        autoDemoPrimed_ = false;
        autoDemoWasEnabled_ = false;
    }
}

void Application::applyDemoStep(std::size_t stepIndex, bool immediateCamera) {
    const auto& steps = currentDemoSteps();
    if (steps.empty()) {
        return;
    }

    const std::size_t wrappedIndex = stepIndex % steps.size();
    const auto& step = steps[wrappedIndex];
    state_.atomicNumber = step.atomicNumber;
    state_.chargeState = step.chargeState;
    state_.approximationMode = step.approximationMode;
    state_.modelKind = step.modelKind;
    state_.view.cloudRenderMode = step.cloudRenderMode;
    state_.cloud.components = step.components;
    state_.cloud.useNumericalRadial = step.useNumericalRadial;
    state_.cloud.buildVolume = step.buildVolume;
    state_.solver.qn = step.solverQn;
    state_.bohr.principalN = step.principalN;
    state_.bohr.maxSpectrumN = step.maxSpectrumN;
    state_.transition = step.transition;
    state_.demo.currentStepIndex = static_cast<int>(wrappedIndex);
    state_.demo.currentStepName = step.name;
    state_.demo.activeScriptName =
        (state_.demo.useScriptPlayback && !loadedDemoSteps_.empty()) ? state_.demo.activeScriptName : "内置演示";
    state_.demo.stepProgress = 0.0;
    state_.dirty.physics = true;
    state_.dirty.cloud = true;
    state_.dirty.solver = true;
    state_.dirty.reference = true;

    if (immediateCamera) {
        camera_.jumpToPose(step.cameraPose);
    } else {
        camera_.animateToPose(step.cameraPose);
    }

    logger_.info("自动演示切换到场景: " + std::string(step.name));
}

std::uint64_t Application::cloudRequestSignature(const quantum::physics::CloudRequest& request,
                                                 CloudBuildStage stage) const {
    std::uint64_t seed = signatureForRequest(request);
    hashCombine(seed, static_cast<int>(stage));
    return seed;
}

void Application::startCloudBuild() {
    CloudBuildInput fullInput = makeCloudBuildInput(CloudBuildStage::Full);
    CloudBuildInput input = shouldUsePreviewBuild(fullInput) ? makePreviewCloudBuildInput(fullInput) : fullInput;
    input.generation = ++cloudBuildGeneration_;
    fullInput.generation = input.generation;

    if (appliedFullCloudTargetSignature_ == fullInput.targetSignature && !cloudBuildInFlight_ && !queuedCloudBuild_.has_value()) {
        performance_.setCloudBuildState(false, false);
        return;
    }

    if (cloudBuildInFlight_) {
        if (input.targetSignature == activeCloudTargetSignature_ ||
            (queuedCloudBuild_.has_value() && input.targetSignature == queuedCloudTargetSignature_)) {
            performance_.setCloudBuildState(true, queuedCloudBuild_.has_value());
            return;
        }

        const bool hadQueuedRequest = queuedCloudBuild_.has_value();
        if (activeCloudCancelFlag_ != nullptr) {
            activeCloudCancelFlag_->store(true, std::memory_order_relaxed);
        }
        queuedCloudBuild_ = std::move(input);
        queuedCloudSignature_ = queuedCloudBuild_->signature;
        queuedCloudTargetSignature_ = queuedCloudBuild_->targetSignature;
        performance_.setCloudBuildState(true, true);
        if (!hadQueuedRequest) {
            logger_.info("概率云重建请求已排队，并已请求取消当前后台任务。");
        }
        return;
    }

    launchCloudBuild(std::move(input));
}

void Application::launchCloudBuild(CloudBuildInput input) {
    auto request = input.request;
    const auto generation = input.generation;
    const auto signature = input.signature;
    const auto targetSignature = input.targetSignature;
    const auto stage = input.stage;
    auto cancelFlag = std::make_shared<std::atomic_bool>(false);
    activeCloudCancelFlag_ = cancelFlag;
    activeCloudSignature_ = signature;
    activeCloudTargetSignature_ = targetSignature;
    std::ostringstream stream;
    stream.setf(std::ios::fixed, std::ios::floatfield);
    stream.precision(2);
    stream << "开始后台生成概率云(" << ((stage == CloudBuildStage::Preview) ? "预览" : "全质量")
           << "): generation=" << generation << ", signature=" << signature << ", target=" << targetSignature
           << ", 点数=" << request.pointCount << ", 体分辨率=" << request.volumeResolution
           << ", 范围倍率=" << request.extentScale;
    logger_.info(stream.str());
    cloudBuildFuture_ = std::async(
        std::launch::async, [request = std::move(request), generation, signature, targetSignature, stage, cancelFlag]() mutable {
        quantum::physics::ProbabilityCloudGenerator generator;
        CloudBuildOutput output;
        output.generation = generation;
            output.signature = signature;
            output.targetSignature = targetSignature;
            output.stage = stage;
            const auto cloud = generator.generate(request, cancelFlag.get());
            if (!cloud.has_value()) {
                output.cancelled = true;
                return output;
            }
            output.cloud = std::move(*cloud);
        return output;
    });
    cloudBuildInFlight_ = true;
    performance_.setCloudBuildState(true, queuedCloudBuild_.has_value());
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
    activeCloudCancelFlag_.reset();
    performance_.setCloudBuildState(false, queuedCloudBuild_.has_value());
    const bool currentResult = result.signature == activeCloudSignature_;

    if (result.cancelled) {
        logger_.info("后台概率云任务已取消，跳过过期结果。");
    } else if (currentResult) {
        logger_.info(std::string("后台概率云") + ((result.stage == CloudBuildStage::Preview) ? "预览" : "全质量") +
                     "结果生成完成，正在上传到渲染器。");
        applyCloudBuildResult(std::move(result));
    } else {
        logger_.warn("后台概率云结果已过期，已丢弃 generation=" + std::to_string(result.generation) + "。");
    }

    if (!queuedCloudBuild_.has_value() && currentResult && !result.cancelled && result.stage == CloudBuildStage::Preview) {
        CloudBuildInput fullInput = makeCloudBuildInput(CloudBuildStage::Full);
        fullInput.generation = ++cloudBuildGeneration_;
        if (fullInput.targetSignature == result.targetSignature && fullInput.targetSignature != appliedFullCloudTargetSignature_) {
            launchCloudBuild(std::move(fullInput));
            return;
        }
    }

    if (queuedCloudBuild_.has_value()) {
        auto queuedInput = std::move(*queuedCloudBuild_);
        queuedCloudBuild_.reset();
        queuedCloudSignature_ = 0;
        queuedCloudTargetSignature_ = 0;
        logger_.info("开始处理排队中的概率云重建请求。");
        launchCloudBuild(std::move(queuedInput));
    }
}

void Application::applyCloudBuildResult(CloudBuildOutput&& result) {
    state_.derived.cloud = std::move(result.cloud);
    appliedCloudSignature_ = result.signature;
    if (result.stage == CloudBuildStage::Full) {
        appliedFullCloudTargetSignature_ = result.targetSignature;
    }
    hasAppliedCloud_ = true;
    sceneRenderer_.uploadCloud(state_.derived.cloud);
    performance_.setCloudBuildMs(state_.derived.cloud.stats.pointBuildMs);
    performance_.setVolumeBuildMs(state_.derived.cloud.stats.volumeBuildMs);
    performance_.setSamplingStats(state_.derived.cloud.stats.requestedPointCount,
                                  state_.derived.cloud.stats.targetPointCount,
                                  state_.derived.cloud.stats.targetVolumeResolution,
                                  state_.derived.cloud.stats.acceptedCount,
                                  state_.derived.cloud.stats.candidateCount,
                                  state_.derived.cloud.stats.candidateMultiplier,
                                  state_.derived.cloud.stats.radialCdfSamples,
                                  state_.derived.cloud.stats.angularScanResolution,
                                  state_.derived.cloud.stats.monteCarloSamples,
                                  state_.derived.cloud.stats.previewStage,
                                  state_.derived.cloud.stats.qualityRatio);

    std::ostringstream stream;
    stream.setf(std::ios::fixed, std::ios::floatfield);
    stream.precision(3);
    stream << "已应用概率云(" << ((result.stage == CloudBuildStage::Preview) ? "预览" : "全质量")
           << "): signature=" << result.signature << ", 接受点数=" << state_.derived.cloud.stats.acceptedCount
           << ", 目标点数=" << state_.derived.cloud.stats.targetPointCount
           << ", 候选点数=" << state_.derived.cloud.stats.candidateCount
           << ", 点云耗时=" << state_.derived.cloud.stats.pointBuildMs << " ms"
           << ", 体数据耗时=" << state_.derived.cloud.stats.volumeBuildMs << " ms";
    logger_.info(stream.str());
}

void Application::loadReferenceLines() {
    state_.derived.referenceLines.clear();
    const auto csvPath = projectRoot_ / state_.reference.csvPath;
    std::ifstream stream(csvPath);
    if (!stream.is_open()) {
        logger_.warn("未找到参考谱线 CSV: " + csvPath.string());
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
    if (!(uiFrame.sceneHovered || uiFrame.sceneActive)) {
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
