#include "quantum/app/Application.h"

#include <GL/glew.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <functional>
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

std::string csvField(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size() + 8);
    escaped.push_back('"');
    for (const char ch : value) {
        if (ch == '"') {
            escaped.push_back('"');
        }
        escaped.push_back(ch);
    }
    escaped.push_back('"');
    return escaped;
}

std::string currentTimestampTag() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &nowTime);
#else
    localtime_r(&nowTime, &localTime);
#endif
    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", &localTime);
    return buffer;
}

std::filesystem::path appendStemSuffix(const std::filesystem::path& path, const std::string& suffix) {
    return path.parent_path() / (path.stem().string() + suffix + path.extension().string());
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

quantum::meta::SourceRecord sourceRecordFromJson(const nlohmann::json& node) {
    quantum::meta::SourceRecord source;
    if (!node.is_object()) {
        return source;
    }
    source.id = node.value("id", std::string{});
    source.title = node.value("title", std::string{});
    source.provider = node.value("provider", std::string{});
    source.revision = node.value("revision", std::string{});
    source.license = node.value("license", std::string{});
    source.url = node.value("url", std::string{});
    return source;
}

nlohmann::json sourceRecordToJson(const quantum::meta::SourceRecord& source) {
    return {{"id", source.id},
            {"title", source.title},
            {"provider", source.provider},
            {"revision", source.revision},
            {"license", source.license},
            {"url", source.url}};
}

quantum::meta::ProvenanceRecord provenanceRecordFromJson(const nlohmann::json& node) {
    quantum::meta::ProvenanceRecord provenance;
    if (!node.is_object()) {
        return provenance;
    }
    provenance.recordId = node.value("record_id", std::string{});
    provenance.sourceId = node.value("source_id", std::string{});
    provenance.createdBy = node.value("created_by", std::string{});
    provenance.notes = node.value("notes", std::string{});
    return provenance;
}

nlohmann::json provenanceRecordToJson(const quantum::meta::ProvenanceRecord& provenance) {
    return {{"record_id", provenance.recordId},
            {"source_id", provenance.sourceId},
            {"created_by", provenance.createdBy},
            {"notes", provenance.notes}};
}

quantum::meta::MethodStamp methodStampFromJson(const nlohmann::json& node) {
    quantum::meta::MethodStamp stamp;
    if (!node.is_object()) {
        return stamp;
    }
    stamp.methodName = node.value("method_name", std::string{});
    stamp.approximation = node.value("approximation", std::string{});
    stamp.dataSource = node.value("data_source", std::string{});
    stamp.validationSummary = node.value("validation_summary", std::string{});
    stamp.includesElectronCorrelation = node.value("includes_electron_correlation", false);
    stamp.enforcesAntisymmetry = node.value("enforces_antisymmetry", false);
    stamp.includesRelativity = node.value("includes_relativity", false);
    stamp.includesExternalField = node.value("includes_external_field", false);
    stamp.isTimeDependent = node.value("is_time_dependent", false);
    if (node.contains("caveats") && node.at("caveats").is_array()) {
        stamp.caveats = node.at("caveats").get<std::vector<std::string>>();
    }
    return stamp;
}

nlohmann::json methodStampToJson(const quantum::meta::MethodStamp& stamp) {
    return {{"method_name", stamp.methodName},
            {"approximation", stamp.approximation},
            {"data_source", stamp.dataSource},
            {"validation_summary", stamp.validationSummary},
            {"includes_electron_correlation", stamp.includesElectronCorrelation},
            {"enforces_antisymmetry", stamp.enforcesAntisymmetry},
            {"includes_relativity", stamp.includesRelativity},
            {"includes_external_field", stamp.includesExternalField},
            {"is_time_dependent", stamp.isTimeDependent},
            {"caveats", stamp.caveats}};
}

quantum::meta::SourceRecord builtInSceneSource() {
    return {"demo-scenes",
            "Built-in scene presets",
            "embedded",
            "1",
            "",
            "assets/scenarios/demo_scenes.json"};
}

quantum::meta::MethodStamp scenePresetMethodStamp(quantum::physics::ApproximationMode mode) {
    quantum::meta::MethodStamp stamp;
    stamp.methodName = "Scene preset selection";
    stamp.approximation = (mode == quantum::physics::ApproximationMode::StrictHydrogenic)
                              ? "strict hydrogenic teaching preset"
                              : "multi-electron teaching preset";
    stamp.dataSource = "Built-in scripted scene preset";
    stamp.validationSummary = "Preset configures a pedagogical visualization, not a new physics solver.";
    stamp.tier = quantum::meta::ModelTier::Tier0Teaching;
    stamp.animationKind = quantum::meta::AnimationKind::Illustrative;
    stamp.caveats.push_back("Scene presets control visualization and parameter defaults.");
    stamp.caveats.push_back("They do not imply research-grade validation.");
    return stamp;
}

std::string localizedStepName(const DemoScriptStep& step) {
    if (step.atomicNumber == 1 && step.chargeState == 0 && step.modelKind == ModelKind::Bohr) {
        return "氢原子玻尔轨道";
    }
    if (step.atomicNumber == 2 && step.chargeState == 1 &&
        step.modelKind == ModelKind::SchrodingerCloud) {
        return "氦离子相位云";
    }
    if (step.atomicNumber == 10 &&
        step.approximationMode == quantum::physics::ApproximationMode::MultiElectronApproximation) {
        return "氖原子多电子近似";
    }
    return step.name;
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
            {"camera", poseToJson(step.cameraPose)},
            {"method", methodStampToJson(step.method)},
            {"source", sourceRecordToJson(step.source)},
            {"provenance", provenanceRecordToJson(step.provenance)}};
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
    if (node.contains("method")) {
        step.method = methodStampFromJson(node.at("method"));
    }
    if (node.contains("source")) {
        step.source = sourceRecordFromJson(node.at("source"));
    }
    if (node.contains("provenance")) {
        step.provenance = provenanceRecordFromJson(node.at("provenance"));
    }
    if (step.method.methodName.empty()) {
        step.method = scenePresetMethodStamp(step.approximationMode);
    }
    if (step.source.id.empty()) {
        step.source = builtInSceneSource();
    }
    if (step.provenance.recordId.empty()) {
        step.provenance = {"demo-step-" + std::to_string(step.atomicNumber) + "-" + step.name,
                           step.source.id,
                           "app::demoStepFromJson",
                           "Parsed from demo script JSON."};
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

    if (!elementDatabase_.loadReferenceTransitions(paths_.asset("data/nist_reference_lines.csv"))) {
        logger_.warn("未能加载本地参考谱线映射，谱线对照将不可用。");
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
        if (uiFrame.fitSceneRequested || (uiFrame.sceneFocused && ImGui::IsKeyPressed(ImGuiKey_F))) {
            fitCameraToScene(uiFrame.fitSceneRequested ? uiFrame.fitSceneMode : quantum::app::SceneFitMode::Full, false);
        }
        processSceneCameraInput(uiFrame);
        processDemoScriptRequests();
        updateAutoDemo(deltaSeconds);
        camera_.update(static_cast<float>(deltaSeconds));

        recomputeDerived();
        processReportRequests();
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
        state_.derived.spectroscopy = quantum::spectroscopy::evaluateHydrogenicCorrections(
            transitionRequest, state_.derived.transition, state_.spectroscopy, nuclearMass);
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
        quantum::physics::CentralFieldParameters centralField;
        centralField.mode = state_.solver.useScreenedCentralField ? quantum::physics::CentralFieldMode::ScreenedCentralField
                                                                  : quantum::physics::CentralFieldMode::HydrogenicCoulomb;
        centralField.nuclearCharge = state_.atomicNumber;
        centralField.ionCharge = state_.chargeState;
        centralField.zeff = state_.derived.activeZeff;
        centralField.autoResidualCharge = state_.solver.autoResidualCharge;
        centralField.residualCharge = state_.solver.residualCharge;
        centralField.screeningLengthBohr = state_.solver.screeningLengthBohr;
        state_.derived.centralField =
            quantum::physics::sampleCentralFieldProfile(centralField, state_.solver.maxScaledRadius, 160);

        state_.dirty.solver = true;
        state_.dirty.cloud = true;
    }

    if (state_.dirty.solver) {
        quantum::physics::CentralFieldParameters centralField;
        centralField.mode = state_.solver.useScreenedCentralField ? quantum::physics::CentralFieldMode::ScreenedCentralField
                                                                  : quantum::physics::CentralFieldMode::HydrogenicCoulomb;
        centralField.nuclearCharge = state_.atomicNumber;
        centralField.ionCharge = state_.chargeState;
        centralField.zeff = state_.derived.activeZeff;
        centralField.autoResidualCharge = state_.solver.autoResidualCharge;
        centralField.residualCharge = state_.solver.residualCharge;
        centralField.screeningLengthBohr = state_.solver.screeningLengthBohr;

        quantum::physics::NumericalSolverRequest request;
        request.nuclearCharge = state_.atomicNumber;
        request.zeff = state_.derived.activeZeff;
        request.nuclearMassKg = nuclearMass;
        request.useReducedMass = state_.bohr.useReducedMass;
        request.centralField = centralField;
        request.qn = state_.solver.qn;
        request.gridPoints = state_.solver.gridPoints;
        request.convergencePasses = state_.solver.convergencePasses;
        request.maxScaledRadius = state_.solver.maxScaledRadius;

        const auto solverResult = numericalSolver_.solve(request);
        state_.derived.solver = solverResult;
        state_.derived.centralField = solverResult.centralFieldProfile;
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

    refreshMethodAndValidationMetadata();
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

void Application::processReportRequests() {
    if (state_.reporting.exportValidationReportRequested) {
        const std::filesystem::path outputPath = resolveReportingPath(state_.reporting.outputPath);
        auto reportInput = makeValidationReportInput();
        reportInput.outputStem = outputPath.stem().string();
        const auto result = validationReportWriter_.write(reportInput, outputPath);
        if (result.ok) {
            state_.reporting.exportStatus =
                "已导出: " + result.markdownPath.string() + " | " + result.jsonPath.filename().string();
            logger_.info("已导出验证报告: " + result.markdownPath.string());
        } else {
            state_.reporting.exportStatus = "导出失败: " + result.message;
            logger_.error("导出验证报告失败: " + result.message);
        }
        state_.reporting.exportValidationReportRequested = false;
    }

    if (state_.reporting.exportCurrentPlotCsvRequested) {
        const std::filesystem::path outputPath = resolveReportingPath(state_.reporting.csvOutputPath);
        const auto exportedPaths = exportCurrentPlotWindowCsv(outputPath);
        if (!exportedPaths.empty()) {
            if (exportedPaths.size() == 1) {
                state_.reporting.csvExportStatus = "已导出: " + exportedPaths.front().string();
                logger_.info("已导出当前窗口 CSV: " + exportedPaths.front().string());
            } else {
                state_.reporting.csvExportStatus =
                    "已导出 " + std::to_string(exportedPaths.size()) + " 个 CSV: " + exportedPaths.front().string();
                logger_.info("已导出当前窗口拆分 CSV，共 " + std::to_string(exportedPaths.size()) + " 个文件。");
            }
        } else {
            state_.reporting.csvExportStatus = "导出失败，请检查路径与日志";
            logger_.error("导出当前窗口 CSV 失败: " + outputPath.string());
        }
        state_.reporting.exportCurrentPlotCsvRequested = false;
    }
}

std::filesystem::path Application::resolveReportingPath(const std::string& configuredPath) const {
    const std::filesystem::path configured(configuredPath);
    if (configured.is_absolute()) {
        return configured;
    }
    if (configured.parent_path().empty()) {
        return paths_.report(configured.string());
    }
    return projectRoot_ / configured;
}

std::vector<std::filesystem::path> Application::exportCurrentPlotWindowCsv(const std::filesystem::path& outputPath) {
    try {
        const std::filesystem::path baseOutputPath =
            state_.reporting.csvAppendTimestamp ? appendStemSuffix(outputPath, "_" + currentTimestampTag()) : outputPath;
        std::filesystem::create_directories(baseOutputPath.parent_path());

        const auto writeCsvFile =
            [](const std::filesystem::path& path,
               const std::function<void(std::ofstream&)>& emitRows) -> bool {
            std::ofstream stream(path, std::ios::binary | std::ios::trunc);
            if (!stream.is_open()) {
                return false;
            }
            stream << "plot,source,index,label,x,y,extra_1,extra_2,extra_3\n";
            emitRows(stream);
            return true;
        };

        const auto writeRow = [](std::ofstream& stream,
                                 const std::string& plot,
                                 const std::string& source,
                                 std::size_t index,
                                 const std::string& label,
                                 double x,
                                 double y,
                                 const std::string& extra1 = {},
                                 const std::string& extra2 = {},
                                 const std::string& extra3 = {}) {
            stream << csvField(plot) << ',' << csvField(source) << ',' << index << ',' << csvField(label) << ','
                   << x << ',' << y << ',' << csvField(extra1) << ',' << csvField(extra2) << ',' << csvField(extra3) << '\n';
        };

        std::vector<std::filesystem::path> exportedPaths;

        const auto exportSingleFile = [&](const std::filesystem::path& path) -> bool {
            return writeCsvFile(path, [&](std::ofstream& stream) {
                for (std::size_t index = 0; index < state_.derived.energyLevels.size(); ++index) {
                    const auto& level = state_.derived.energyLevels[index];
                    if (level.energyEv < state_.reporting.energyPlotWindow.minY ||
                        level.energyEv > state_.reporting.energyPlotWindow.maxY) {
                        continue;
                    }
                    std::string stateTag = "可见";
                    if (level.n == state_.transition.initial.n) {
                        stateTag = "初态";
                    } else if (level.n == state_.transition.final.n) {
                        stateTag = "末态";
                    }
                    writeRow(stream, "能级图", "计算", index, "n=" + std::to_string(level.n), static_cast<double>(level.n), level.energyEv, stateTag);
                }

                for (std::size_t index = 0; index < state_.derived.spectrumLines.size(); ++index) {
                    const auto& line = state_.derived.spectrumLines[index];
                    if (state_.transition.enforceRules && !line.allowed) {
                        continue;
                    }
                    if (line.wavelengthNm < state_.reporting.spectrumPlotWindow.minX ||
                        line.wavelengthNm > state_.reporting.spectrumPlotWindow.maxX ||
                        line.photonEnergyEv < state_.reporting.spectrumPlotWindow.minY ||
                        line.photonEnergyEv > state_.reporting.spectrumPlotWindow.maxY) {
                        continue;
                    }
                    writeRow(stream,
                             "谱线图",
                             "计算",
                             index,
                             line.seriesName,
                             line.wavelengthNm,
                             line.photonEnergyEv,
                             "(" + std::to_string(line.initial.n) + "," + std::to_string(line.initial.l) + "," +
                                 std::to_string(line.initial.m) + ")->(" + std::to_string(line.final.n) + "," +
                                 std::to_string(line.final.l) + "," + std::to_string(line.final.m) + ")",
                             line.allowed ? "allowed" : "forbidden");
                }

                for (std::size_t index = 0; index < state_.derived.referenceLines.size(); ++index) {
                    const auto& line = state_.derived.referenceLines[index];
                    if (line.wavelengthNm < state_.reporting.spectrumPlotWindow.minX ||
                        line.wavelengthNm > state_.reporting.spectrumPlotWindow.maxX ||
                        line.photonEnergyEv < state_.reporting.spectrumPlotWindow.minY ||
                        line.photonEnergyEv > state_.reporting.spectrumPlotWindow.maxY) {
                        continue;
                    }
                    writeRow(stream,
                             "谱线图",
                             "参考",
                             index,
                             line.seriesName,
                             line.wavelengthNm,
                             line.photonEnergyEv,
                             "(" + std::to_string(line.initial.n) + "," + std::to_string(line.initial.l) + "," +
                                 std::to_string(line.initial.m) + ")->(" + std::to_string(line.final.n) + "," +
                                 std::to_string(line.final.l) + "," + std::to_string(line.final.m) + ")");
                }

                for (std::size_t index = 0; index < state_.derived.radialDistribution.size(); ++index) {
                    const double radiusNm = state_.derived.radialDistribution[index].first * 1.0e9;
                    const double probability = state_.derived.radialDistribution[index].second;
                    if (radiusNm < state_.reporting.radialPlotWindow.minX ||
                        radiusNm > state_.reporting.radialPlotWindow.maxX ||
                        probability < state_.reporting.radialPlotWindow.minY ||
                        probability > state_.reporting.radialPlotWindow.maxY) {
                        continue;
                    }
                    writeRow(stream, "径向分布", "计算", index, "P(r)", radiusNm, probability);
                }

                for (std::size_t index = 0; index < state_.derived.centralField.samples.size(); ++index) {
                    const auto& sample = state_.derived.centralField.samples[index];
                    if (sample.radiusBohr < state_.reporting.centralFieldPlotWindow.minX ||
                        sample.radiusBohr > state_.reporting.centralFieldPlotWindow.maxX ||
                        sample.potentialHartree < state_.reporting.centralFieldPlotWindow.minY ||
                        sample.potentialHartree > state_.reporting.centralFieldPlotWindow.maxY) {
                        continue;
                    }
                    writeRow(stream,
                             "中心场势",
                             "计算",
                             index,
                             "V(r)",
                             sample.radiusBohr,
                             sample.potentialHartree,
                             "Z_eff=" + std::to_string(sample.effectiveCharge));
                }

                for (std::size_t index = 0; index < state_.derived.solver.convergence.size(); ++index) {
                    const auto& sample = state_.derived.solver.convergence[index];
                    const double stepPm = sample.gridStepMeters * 1.0e12;
                    if (stepPm < state_.reporting.convergencePlotWindow.minX ||
                        stepPm > state_.reporting.convergencePlotWindow.maxX ||
                        sample.errorEv < state_.reporting.convergencePlotWindow.minY ||
                        sample.errorEv > state_.reporting.convergencePlotWindow.maxY) {
                        continue;
                    }
                    writeRow(stream, "求解收敛曲线", "计算", index, "误差", stepPm, sample.errorEv);
                }
            });
        };

        const auto exportSplitFile = [&](const std::filesystem::path& path,
                                         const std::function<void(std::ofstream&)>& emitRows) -> bool {
            return writeCsvFile(path, emitRows);
        };

        if (!state_.reporting.csvSplitByPlot) {
            if (exportSingleFile(baseOutputPath)) {
                exportedPaths.push_back(baseOutputPath);
            }
        } else {
            const std::filesystem::path energyPath = appendStemSuffix(baseOutputPath, "_energy");
            if (exportSplitFile(energyPath, [&](std::ofstream& stream) {
                    for (std::size_t index = 0; index < state_.derived.energyLevels.size(); ++index) {
                        const auto& level = state_.derived.energyLevels[index];
                        if (level.energyEv < state_.reporting.energyPlotWindow.minY ||
                            level.energyEv > state_.reporting.energyPlotWindow.maxY) {
                            continue;
                        }
                        std::string stateTag = "可见";
                        if (level.n == state_.transition.initial.n) {
                            stateTag = "初态";
                        } else if (level.n == state_.transition.final.n) {
                            stateTag = "末态";
                        }
                        writeRow(stream, "能级图", "计算", index, "n=" + std::to_string(level.n), static_cast<double>(level.n), level.energyEv, stateTag);
                    }
                })) {
                exportedPaths.push_back(energyPath);
            }

            const std::filesystem::path spectrumPath = appendStemSuffix(baseOutputPath, "_spectrum");
            if (exportSplitFile(spectrumPath, [&](std::ofstream& stream) {
                    for (std::size_t index = 0; index < state_.derived.spectrumLines.size(); ++index) {
                        const auto& line = state_.derived.spectrumLines[index];
                        if (state_.transition.enforceRules && !line.allowed) {
                            continue;
                        }
                        if (line.wavelengthNm < state_.reporting.spectrumPlotWindow.minX ||
                            line.wavelengthNm > state_.reporting.spectrumPlotWindow.maxX ||
                            line.photonEnergyEv < state_.reporting.spectrumPlotWindow.minY ||
                            line.photonEnergyEv > state_.reporting.spectrumPlotWindow.maxY) {
                            continue;
                        }
                        writeRow(stream,
                                 "谱线图",
                                 "计算",
                                 index,
                                 line.seriesName,
                                 line.wavelengthNm,
                                 line.photonEnergyEv,
                                 "(" + std::to_string(line.initial.n) + "," + std::to_string(line.initial.l) + "," +
                                     std::to_string(line.initial.m) + ")->(" + std::to_string(line.final.n) + "," +
                                     std::to_string(line.final.l) + "," + std::to_string(line.final.m) + ")",
                                 line.allowed ? "allowed" : "forbidden");
                    }
                    for (std::size_t index = 0; index < state_.derived.referenceLines.size(); ++index) {
                        const auto& line = state_.derived.referenceLines[index];
                        if (line.wavelengthNm < state_.reporting.spectrumPlotWindow.minX ||
                            line.wavelengthNm > state_.reporting.spectrumPlotWindow.maxX ||
                            line.photonEnergyEv < state_.reporting.spectrumPlotWindow.minY ||
                            line.photonEnergyEv > state_.reporting.spectrumPlotWindow.maxY) {
                            continue;
                        }
                        writeRow(stream,
                                 "谱线图",
                                 "参考",
                                 index,
                                 line.seriesName,
                                 line.wavelengthNm,
                                 line.photonEnergyEv,
                                 "(" + std::to_string(line.initial.n) + "," + std::to_string(line.initial.l) + "," +
                                     std::to_string(line.initial.m) + ")->(" + std::to_string(line.final.n) + "," +
                                     std::to_string(line.final.l) + "," + std::to_string(line.final.m) + ")");
                    }
                })) {
                exportedPaths.push_back(spectrumPath);
            }

            const std::filesystem::path radialPath = appendStemSuffix(baseOutputPath, "_radial");
            if (exportSplitFile(radialPath, [&](std::ofstream& stream) {
                    for (std::size_t index = 0; index < state_.derived.radialDistribution.size(); ++index) {
                        const double radiusNm = state_.derived.radialDistribution[index].first * 1.0e9;
                        const double probability = state_.derived.radialDistribution[index].second;
                        if (radiusNm < state_.reporting.radialPlotWindow.minX ||
                            radiusNm > state_.reporting.radialPlotWindow.maxX ||
                            probability < state_.reporting.radialPlotWindow.minY ||
                            probability > state_.reporting.radialPlotWindow.maxY) {
                            continue;
                        }
                        writeRow(stream, "径向分布", "计算", index, "P(r)", radiusNm, probability);
                    }
                })) {
                exportedPaths.push_back(radialPath);
            }

            const std::filesystem::path centralPath = appendStemSuffix(baseOutputPath, "_central_field");
            if (exportSplitFile(centralPath, [&](std::ofstream& stream) {
                    for (std::size_t index = 0; index < state_.derived.centralField.samples.size(); ++index) {
                        const auto& sample = state_.derived.centralField.samples[index];
                        if (sample.radiusBohr < state_.reporting.centralFieldPlotWindow.minX ||
                            sample.radiusBohr > state_.reporting.centralFieldPlotWindow.maxX ||
                            sample.potentialHartree < state_.reporting.centralFieldPlotWindow.minY ||
                            sample.potentialHartree > state_.reporting.centralFieldPlotWindow.maxY) {
                            continue;
                        }
                        writeRow(stream,
                                 "中心场势",
                                 "计算",
                                 index,
                                 "V(r)",
                                 sample.radiusBohr,
                                 sample.potentialHartree,
                                 "Z_eff=" + std::to_string(sample.effectiveCharge));
                    }
                })) {
                exportedPaths.push_back(centralPath);
            }

            const std::filesystem::path convergencePath = appendStemSuffix(baseOutputPath, "_convergence");
            if (exportSplitFile(convergencePath, [&](std::ofstream& stream) {
                    for (std::size_t index = 0; index < state_.derived.solver.convergence.size(); ++index) {
                        const auto& sample = state_.derived.solver.convergence[index];
                        const double stepPm = sample.gridStepMeters * 1.0e12;
                        if (stepPm < state_.reporting.convergencePlotWindow.minX ||
                            stepPm > state_.reporting.convergencePlotWindow.maxX ||
                            sample.errorEv < state_.reporting.convergencePlotWindow.minY ||
                            sample.errorEv > state_.reporting.convergencePlotWindow.maxY) {
                            continue;
                        }
                        writeRow(stream, "求解收敛曲线", "计算", index, "误差", stepPm, sample.errorEv);
                    }
                })) {
                exportedPaths.push_back(convergencePath);
            }
        }

        return exportedPaths;
    } catch (const std::exception& exception) {
        logger_.error("写入当前窗口 CSV 失败: " + std::string(exception.what()));
        return {};
    }
}

void Application::refreshMethodAndValidationMetadata() {
    state_.derived.methodStamps.clear();
    state_.derived.validationRecords.clear();

    const auto appendMethod = [this](const quantum::meta::MethodStamp& stamp) {
        if (stamp.methodName.empty()) {
            return;
        }
        const auto duplicate = std::find_if(state_.derived.methodStamps.begin(),
                                            state_.derived.methodStamps.end(),
                                            [&stamp](const quantum::meta::MethodStamp& existing) {
                                                return existing.methodName == stamp.methodName &&
                                                       existing.approximation == stamp.approximation;
                                            });
        if (duplicate == state_.derived.methodStamps.end()) {
            state_.derived.methodStamps.push_back(stamp);
        }
    };

    appendMethod(state_.derived.configuration.method);
    appendMethod(state_.derived.slater.method);
    appendMethod(state_.derived.bohrMetrics.method);
    appendMethod(state_.derived.transition.method);
    appendMethod(state_.derived.spectroscopy.method);
    appendMethod(state_.derived.centralField.method);
    appendMethod(state_.derived.solver.method);
    if (const auto* elementMetadata = elementDatabase_.elementMetadataByAtomicNumber(state_.atomicNumber);
        elementMetadata != nullptr) {
        appendMethod(elementMetadata->method);
    }
    if ((state_.demo.enabled || state_.demo.useScriptPlayback) &&
        (currentDemoStep() != nullptr)) {
        const auto* sceneStep = currentDemoStep();
        appendMethod(sceneStep->method.methodName.empty() ? scenePresetMethodStamp(sceneStep->approximationMode)
                                                          : sceneStep->method);
    }
    for (const auto& line : state_.derived.referenceLines) {
        appendMethod(line.method);
    }
    for (const auto& level : state_.derived.energyLevels) {
        appendMethod(level.method);
    }

    const auto appendRecords = [this](const auto& records) {
        state_.derived.validationRecords.insert(state_.derived.validationRecords.end(), records.begin(), records.end());
    };

    appendRecords(state_.derived.bohrMetrics.validation);
    appendRecords(state_.derived.slater.validation);
    appendRecords(state_.derived.transition.validation);
    appendRecords(state_.derived.spectroscopy.validation);
    appendRecords(state_.derived.centralField.validation);
    appendRecords(state_.derived.solver.validation);
    for (const auto& line : state_.derived.referenceLines) {
        appendRecords(line.validation);
    }
    for (const auto& level : state_.derived.energyLevels) {
        if (!level.validation.caseName.empty()) {
            state_.derived.validationRecords.push_back(level.validation);
        }
    }
    for (const auto& sample : state_.derived.solver.convergence) {
        if (!sample.validation.caseName.empty()) {
            state_.derived.validationRecords.push_back(sample.validation);
        }
    }

    if (state_.derived.cloud.stats.acceptedCount > 0) {
        quantum::meta::ValidationRecord record;
        record.status = (std::abs(state_.derived.cloud.stats.normalizationError) <= 5.0e-2)
                            ? quantum::meta::ValidationStatus::ReferenceMatched
                            : quantum::meta::ValidationStatus::SanityChecked;
        record.caseName = "Cloud normalization estimate";
        record.referenceName = "Unity normalization";
        record.units = "";
        record.referenceValue = 1.0;
        record.measuredValue = state_.derived.cloud.stats.normalizationEstimate;
        record.absoluteError = std::abs(state_.derived.cloud.stats.normalizationError);
        record.relativeError = std::abs(state_.derived.cloud.stats.normalizationError);
        record.notes = state_.derived.cloud.stats.usedNumericalRadial
                           ? "Monte Carlo estimate with numerical radial override when available."
                           : "Monte Carlo estimate from analytic or effective-charge sampling.";
        record.source = {"cloud-normalization", "Internal cloud normalization estimate", "runtime", "1", "", ""};
        state_.derived.validationRecords.push_back(std::move(record));
    }

    if (state_.reference.enabled) {
        const auto referenceIt =
            std::find_if(state_.derived.referenceLines.begin(),
                         state_.derived.referenceLines.end(),
                         [this](const quantum::physics::SpectrumLine& line) {
                             return line.initial.n == state_.transition.initial.n &&
                                    line.initial.l == state_.transition.initial.l &&
                                    line.final.n == state_.transition.final.n &&
                                    line.final.l == state_.transition.final.l;
                         });
        if (referenceIt != state_.derived.referenceLines.end()) {
            quantum::meta::ValidationRecord record;
            const double error = std::abs(state_.derived.transition.wavelengthNm - referenceIt->wavelengthNm);
            record.status = (error <= 0.5) ? quantum::meta::ValidationStatus::ReferenceMatched
                                           : quantum::meta::ValidationStatus::SanityChecked;
            record.caseName = "Local reference line comparison";
            record.referenceName = "assets/data/nist_reference_lines.csv";
            record.units = "nm";
            record.referenceValue = referenceIt->wavelengthNm;
            record.measuredValue = state_.derived.transition.wavelengthNm;
            record.absoluteError = error;
            record.relativeError =
                (std::abs(referenceIt->wavelengthNm) > 1.0e-30) ? (error / std::abs(referenceIt->wavelengthNm)) : 0.0;
            record.notes = "Reference line matched by n and l quantum numbers only.";
            record.source = {"local-reference-csv",
                             "Local reference line CSV",
                             "assets/data/nist_reference_lines.csv",
                             "1",
                             "",
                             ""};
            state_.derived.validationRecords.push_back(std::move(record));
        }
    }
}

quantum::validation::ValidationReportInput Application::makeValidationReportInput() const {
    quantum::validation::ValidationReportInput input;
    input.title = "Quantum Atom Simulation Validation Report";
    input.subject = state_.derived.element.name + " (" + state_.derived.element.symbol + "), Z=" +
                    std::to_string(state_.atomicNumber) + ", q=" + std::to_string(state_.chargeState);

    std::ostringstream summary;
    summary << "model=" << modelKindTag(state_.modelKind) << ", approximation=" << approximationModeTag(state_.approximationMode)
            << ", active_zeff=" << state_.derived.activeZeff << ", transition=(" << state_.transition.initial.n << ','
            << state_.transition.initial.l << ',' << state_.transition.initial.m << ")->(" << state_.transition.final.n
            << ',' << state_.transition.final.l << ',' << state_.transition.final.m << "), solver=(" << state_.solver.qn.n
            << ',' << state_.solver.qn.l << ',' << state_.solver.qn.m << ')'
            << ", central_field=" << (state_.solver.useScreenedCentralField ? "screened" : "hydrogenic")
            << ", fine=" << (state_.spectroscopy.applyFineStructure ? "on" : "off")
            << ", zeeman=" << (state_.spectroscopy.applyZeeman ? "on" : "off")
            << ", stark=" << (state_.spectroscopy.applyStark ? "on" : "off");
    input.summary = summary.str();
    input.approximationWarning = state_.derived.approximationWarning;

    const auto appendNamedMethod = [&input](const std::string& label, const quantum::meta::MethodStamp& stamp) {
        if (!stamp.methodName.empty()) {
            input.methods.push_back({label, stamp});
        }
    };

    appendNamedMethod("Configuration builder", state_.derived.configuration.method);
    appendNamedMethod("Shielding model", state_.derived.slater.method);
    appendNamedMethod("Hydrogenic metrics", state_.derived.bohrMetrics.method);
    appendNamedMethod("Transition model", state_.derived.transition.method);
    appendNamedMethod("Tier 3 spectroscopy", state_.derived.spectroscopy.method);
    appendNamedMethod("Tier 1 central field", state_.derived.centralField.method);
    appendNamedMethod("Numerical solver", state_.derived.solver.method);
    if (const auto* elementMetadata = elementDatabase_.elementMetadataByAtomicNumber(state_.atomicNumber);
        elementMetadata != nullptr) {
        appendNamedMethod("Element metadata", elementMetadata->method);
    }
    if ((state_.demo.enabled || state_.demo.useScriptPlayback) &&
        (currentDemoStep() != nullptr)) {
        const auto* sceneStep = currentDemoStep();
        appendNamedMethod("Scene preset",
                          sceneStep->method.methodName.empty() ? scenePresetMethodStamp(sceneStep->approximationMode)
                                                               : sceneStep->method);
    }
    input.records = state_.derived.validationRecords;
    return input;
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
            DemoScriptStep enriched = step;
            if (enriched.method.methodName.empty()) {
                enriched.method = scenePresetMethodStamp(enriched.approximationMode);
            }
            if (enriched.source.id.empty()) {
                enriched.source = builtInSceneSource();
            }
            if (enriched.provenance.recordId.empty()) {
                enriched.provenance = {"demo-step-" + std::to_string(enriched.atomicNumber) + "-" + enriched.name,
                                       enriched.source.id,
                                       "app::saveDemoScriptToFile",
                                       "Serialized demo step with fallback metadata."};
            }
            root["steps"].push_back(demoStepToJson(enriched));
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
    step.method = scenePresetMethodStamp(step.approximationMode);
    step.source = {"recorded-scene",
                   "Recorded scene preset",
                   "runtime",
                   "1",
                   "",
                   state_.demo.scriptPath};
    step.provenance = {"recorded-demo-step-" + std::to_string(recordedDemoSteps_.size() + 1),
                       step.source.id,
                       "app::captureCurrentDemoStep",
                       "Captured from the current UI state."};
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

const DemoScriptStep* Application::currentDemoStep() const {
    const auto& steps = currentDemoSteps();
    if (steps.empty()) {
        return nullptr;
    }
    const std::size_t index = static_cast<std::size_t>(
        std::clamp(state_.demo.currentStepIndex, 0, static_cast<int>(steps.size() - 1)));
    return &steps[index];
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
    state_.demo.currentStepName = localizedStepName(step);
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
    refreshMethodAndValidationMetadata();

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
    for (const auto& record : elementDatabase_.referenceTransitions(state_.atomicNumber)) {
        quantum::physics::SpectrumLine referenceLine;
        referenceLine.seriesName = record.label;
        referenceLine.initial.n = record.upper.n;
        referenceLine.initial.l = record.upper.l;
        referenceLine.initial.m = record.upper.m;
        referenceLine.final.n = record.lower.n;
        referenceLine.final.l = record.lower.l;
        referenceLine.final.m = record.lower.m;
        referenceLine.wavelengthNm = record.wavelengthNm;
        referenceLine.photonEnergyEv = record.photonEnergyEv;
        referenceLine.allowed = record.allowed;
        referenceLine.method = record.method;
        referenceLine.validation = {record.validation};
        referenceLine.source = record.validation.source;
        referenceLine.provenance = record.provenance;
        state_.derived.referenceLines.push_back(std::move(referenceLine));
    }
    if (!state_.derived.referenceLines.empty()) {
        return;
    }
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

void Application::fitCameraToScene(quantum::app::SceneFitMode mode, bool immediate) {
    sceneRenderer_.configureCameraForScene(state_, camera_);
    camera_.setTransitionDuration(static_cast<float>(state_.view.fitTransitionSeconds));
    auto pose = sceneRenderer_.fitScenePose(state_, camera_, mode);
    if (state_.view.fitLockTarget) {
        pose.target = camera_.pose().target;
    }
    if (immediate) {
        camera_.jumpToPose(pose);
    } else {
        camera_.animateToPose(pose);
    }
}

} // namespace quantum::app
