#include "quantum/ui/AppUi.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <glm/common.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace quantum::ui {
namespace {

constexpr const char* kPanelScene = "场景";
constexpr const char* kPanelInspector = "控制面板";
constexpr const char* kPanelPhysics = "物理信息";
constexpr const char* kPanelPlots = "图表";
constexpr const char* kPanelPerformance = "性能监视";
constexpr const char* kPanelHelp = "帮助";
constexpr const char* kPanelLog = "日志";

const char* modelLabel(quantum::app::ModelKind kind) {
    switch (kind) {
    case quantum::app::ModelKind::Bohr:
        return "玻尔模型";
    case quantum::app::ModelKind::WaveParticle:
        return "波粒演示";
    case quantum::app::ModelKind::SchrodingerCloud:
        return "薛定谔概率云";
    case quantum::app::ModelKind::Compare:
        return "对比模式";
    default:
        return "未知";
    }
}

const char* approximationLabel(quantum::physics::ApproximationMode mode) {
    switch (mode) {
    case quantum::physics::ApproximationMode::StrictHydrogenic:
        return "严格氢样";
    case quantum::physics::ApproximationMode::MultiElectronApproximation:
        return "多电子近似";
    default:
        return "未知";
    }
}

const char* cloudModeLabel(quantum::app::CloudRenderMode mode) {
    switch (mode) {
    case quantum::app::CloudRenderMode::PointCloud:
        return "点云";
    case quantum::app::CloudRenderMode::VolumeSlices:
        return "体切片";
    case quantum::app::CloudRenderMode::Hybrid:
        return "混合";
    default:
        return "未知";
    }
}

const char* samplingQualityLabel(quantum::app::SamplingQualityPreset preset) {
    switch (preset) {
    case quantum::app::SamplingQualityPreset::Fast:
        return "快速";
    case quantum::app::SamplingQualityPreset::Balanced:
        return "均衡";
    case quantum::app::SamplingQualityPreset::HighFidelity:
        return "高精度";
    default:
        return "未知";
    }
}

const char* lodLabel(int lodLevel) {
    switch (lodLevel) {
    case 0:
        return "完整";
    case 1:
        return "中等";
    case 2:
        return "远距";
    case 3:
        return "交互降级";
    default:
        return "未知";
    }
}

const char* sliceAxisLabel(int axis) {
    switch (axis) {
    case 0:
        return "X";
    case 1:
        return "Y";
    case 2:
        return "Z";
    default:
        return "?";
    }
}

const char* yesNoLabel(bool value) {
    return value ? "是" : "否";
}

const char* logLevelLabel(quantum::core::LogLevel level) {
    switch (level) {
    case quantum::core::LogLevel::Info:
        return "INFO";
    case quantum::core::LogLevel::Warning:
        return "WARN";
    case quantum::core::LogLevel::Error:
        return "ERROR";
    default:
        return "LOG";
    }
}

std::string formatLogEntry(const quantum::core::LogEntry& entry) {
    return "[" + entry.timestamp + "][" + logLevelLabel(entry.level) + "] " + entry.message;
}

std::string localizedElementName(const quantum::physics::ElementRecord& element) {
    if (!element.localizedNameZh.empty()) {
        return element.localizedNameZh;
    }
    if (element.symbol == "H") return "氢";
    if (element.symbol == "He") return "氦";
    if (element.symbol == "Li") return "锂";
    if (element.symbol == "Be") return "铍";
    if (element.symbol == "B") return "硼";
    if (element.symbol == "C") return "碳";
    if (element.symbol == "N") return "氮";
    if (element.symbol == "O") return "氧";
    if (element.symbol == "F") return "氟";
    if (element.symbol == "Ne") return "氖";
    if (element.symbol == "Na") return "钠";
    if (element.symbol == "Mg") return "镁";
    if (element.symbol == "Al") return "铝";
    if (element.symbol == "Si") return "硅";
    if (element.symbol == "P") return "磷";
    if (element.symbol == "S") return "硫";
    if (element.symbol == "Cl") return "氯";
    if (element.symbol == "Ar") return "氩";
    return element.name;
}

std::string localizeMethodPhrase(std::string_view phrase) {
    if (phrase == "Reduced-mass corrected hydrogenic analytic model") return "约化质量修正的氢样解析模型";
    if (phrase == "Hydrogenic analytic model") return "氢样解析模型";
    if (phrase == "Hydrogenic transition and spectral observable evaluation") return "氢样跃迁与谱学观测量评估";
    if (phrase == "Aufbau-Pauli electron configuration builder") return "Aufbau-Pauli 电子组态构建器";
    if (phrase == "Slater-rules effective charge estimate") return "Slater 规则有效核电荷估计";
    if (phrase == "Finite-difference radial central-field eigen solver") return "有限差分径向中心场本征求解器";
    if (phrase == "Static element metadata import") return "静态元素元数据导入";
    if (phrase == "Reference transition import") return "参考跃迁数据导入";
    if (phrase == "Tier 1 screened central field") return "第一层屏蔽中心场";
    if (phrase == "Hydrogenic Coulomb central field") return "氢样库仑中心场";
    if (phrase == "phenomenological screened single-electron central field") return "现象学屏蔽单电子中心场";
    if (phrase == "analytic Coulomb central field") return "解析库仑中心场";
    if (phrase == "Tier 3 hydrogenic spectroscopy corrections") return "第三层氢样谱学修正";
    if (phrase == "metadata import") return "元数据导入";
    if (phrase == "local reference mapping") return "本地参考映射";
    if (phrase == "single-electron analytic model") return "单电子解析模型";
    if (phrase == "effective-charge hydrogenic estimate") return "有效核电荷氢样估计";
    if (phrase == "teaching configuration builder") return "教学型组态构建";
    if (phrase == "Z_eff teaching approximation") return "Z_eff 教学近似";
    if (phrase == "fine-structure + optional Zeeman + limited Stark estimate") {
        return "精细结构 + 可选 Zeeman + 受限 Stark 估计";
    }
    if (phrase == "single-electron radial central-field solver on a uniform grid") {
        return "均匀网格上的单电子径向中心场求解";
    }
    if (phrase == "Screened phenomenological single-electron central field") {
        return "屏蔽的现象学单电子中心场";
    }
    if (phrase == "Pure Coulomb single-electron central field") {
        return "纯库仑单电子中心场";
    }
    if (phrase == "Analytic closed-form expressions with H/He+ regression checks") {
        return "闭式解析表达式，已做 H/He+ 回归检查";
    }
    if (phrase == "Canonical hydrogen lines are regression-tested locally") {
        return "典型氢谱线已做本地回归测试";
    }
    if (phrase == "Selected light-element configurations are regression-tested locally") {
        return "选定轻元素组态已做本地回归测试";
    }
    if (phrase == "Selected subshell benchmarks are regression-tested locally") {
        return "选定亚层基准已做本地回归测试";
    }
    if (phrase == "Schema-level import without external online validation.") {
        return "仅做模式级导入，尚未接入外部在线校验。";
    }
    if (phrase == "Imported reference lines are used for local comparisons only.") {
        return "导入的参考谱线仅用于本地对照。";
    }
    if (phrase == "Fine structure and Zeeman terms are perturbative teaching-level corrections.") {
        return "精细结构与 Zeeman 项目前仅作为微扰教学修正。";
    }
    if (phrase == "Phenomenological model used for qualitative central-field trends.") {
        return "该现象学模型仅用于定性展示中心场趋势。";
    }
    if (phrase == "Matches the hydrogenic Coulomb model exactly.") {
        return "与氢样库仑模型一致。";
    }
    if (phrase == "Hydrogenic Coulomb mode is benchmarked against analytic energies; screened mode is a qualitative Tier 1 extension.") {
        return "氢样库仑模式已对解析能级做基准验证；屏蔽模式是定性第一层扩展。";
    }
    if (phrase == "Reference lines are compared to local curated examples, not a live external database.") {
        return "参考谱线仅对照本地整理样例，不是实时外部数据库。";
    }
    return std::string(phrase);
}

std::string localizeMethodTier(quantum::meta::ModelTier tier) {
    switch (tier) {
    case quantum::meta::ModelTier::Tier0Teaching:
        return "第 0 层：教学与演示";
    case quantum::meta::ModelTier::Tier1CentralField:
        return "第 1 层：中心场";
    case quantum::meta::ModelTier::Tier2MeanField:
        return "第 2 层：平均场";
    case quantum::meta::ModelTier::Tier3SpectroscopyCorrections:
        return "第 3 层：谱学修正";
    case quantum::meta::ModelTier::Tier4Correlation:
        return "第 4 层：电子关联";
    case quantum::meta::ModelTier::Tier5Dynamics:
        return "第 5 层：真实动力学";
    default:
        return "未知层级";
    }
}

std::string formatBytes(std::size_t bytes) {
    constexpr double kKilobyte = 1024.0;
    constexpr double kMegabyte = 1024.0 * 1024.0;
    std::ostringstream stream;
    stream.setf(std::ios::fixed, std::ios::floatfield);
    stream.precision(2);
    if (bytes >= static_cast<std::size_t>(kMegabyte)) {
        stream << (static_cast<double>(bytes) / kMegabyte) << " MB";
        return stream.str();
    }
    if (bytes >= static_cast<std::size_t>(kKilobyte)) {
        stream << (static_cast<double>(bytes) / kKilobyte) << " KB";
        return stream.str();
    }
    return std::to_string(bytes) + " B";
}

const char* validationStatusLabel(quantum::meta::ValidationStatus status) {
    switch (status) {
    case quantum::meta::ValidationStatus::NotValidated:
        return "未验证";
    case quantum::meta::ValidationStatus::SanityChecked:
        return "基础检查";
    case quantum::meta::ValidationStatus::Benchmarked:
        return "基准对比";
    case quantum::meta::ValidationStatus::ReferenceMatched:
        return "参考吻合";
    case quantum::meta::ValidationStatus::ExperimentalComparison:
        return "实验对照";
    default:
        return "未知";
    }
}

const char* currentAnimationLabel(const quantum::app::SimulationState& state) {
    if (state.modelKind == quantum::app::ModelKind::WaveParticle) {
        return "教学演示动画";
    }
    if (state.modelKind == quantum::app::ModelKind::SchrodingerCloud && state.cloud.components.size() > 1) {
        return "基组叠加可视化";
    }
    if (state.demo.enabled) {
        return "脚本化示意相机动画";
    }
    return "静态可视化";
}

void drawMethodTags(const quantum::app::SimulationState& state) {
    ImGui::SeparatorText("方法标签");
    const bool strictHydrogenic = state.approximationMode == quantum::physics::ApproximationMode::StrictHydrogenic;
    const bool numericalRadial = state.cloud.useNumericalRadial && state.derived.solver.converged;

    std::string method = strictHydrogenic ? "严格氢样解析模型" : "教学近似：Z_eff + Slater";
    if (numericalRadial) {
        method += " + 有限差分径向求解器";
    }

    ImGui::TextWrapped("方法: %s", method.c_str());
    ImGui::TextWrapped("近似层级: %s",
                       strictHydrogenic ? "单电子氢样模型"
                                        : "多电子教学近似");
    ImGui::TextWrapped("电子关联: 未实现");
    ImGui::TextWrapped("反对称性: %s",
                       strictHydrogenic ? "单电子体系，不涉及多电子反对称波函数"
                                        : "未显式处理多电子反对称性");
    ImGui::TextWrapped("相对论修正: 未实现");
    ImGui::TextWrapped("外场耦合: 未实现");
    ImGui::TextWrapped("时间依赖: 否，当前不是真实 TDSE/TD-CI 动力学");
    ImGui::TextWrapped("数据来源: elements.json + 本地参考谱线 CSV");
    ImGui::TextWrapped("验证状态: %s",
                       strictHydrogenic ? "已做 H/He+ 解析值、跃迁和数值收敛验证"
                                        : "教学近似，仅做一致性与数值链路验证");
    if (!state.derived.methodStamps.empty()) {
        ImGui::SeparatorText("当前激活方法");
        for (const auto& stamp : state.derived.methodStamps) {
            const std::string localizedName = localizeMethodPhrase(stamp.methodName);
            ImGui::BulletText("%s", localizedName.c_str());
            if (!stamp.approximation.empty()) {
                const std::string approximation = localizeMethodPhrase(stamp.approximation);
                ImGui::TextWrapped("    近似: %s", approximation.c_str());
            }
            ImGui::TextWrapped("    层级: %s", localizeMethodTier(stamp.tier).c_str());
            if (!stamp.validationSummary.empty()) {
                const std::string summary = localizeMethodPhrase(stamp.validationSummary);
                ImGui::TextWrapped("    验证: %s", summary.c_str());
            }
        }
    }
}

void drawAnimationDisclosure(const quantum::app::SimulationState& state) {
    ImGui::SeparatorText("动画分类");
    ImGui::TextWrapped("当前动画: %s", currentAnimationLabel(state));
    ImGui::TextWrapped("是否直接求解 TDSE/TD-CI: 否");
    ImGui::TextWrapped("是否存在外场驱动: 否");
    ImGui::TextWrapped("是否包含耗散: 否");
    ImGui::TextWrapped("说明: 当前动画主要用于教学展示、相机过渡和基态/叠加态可视化，不应解释为真实时间依赖量子动力学。");
}

void drawValidationTable(const std::vector<quantum::meta::ValidationRecord>& records) {
    ImGui::SeparatorText("验证记录");
    if (records.empty()) {
        ImGui::TextUnformatted("当前没有可展示的验证记录。");
        return;
    }

    if (ImGui::BeginTable("##validation_records",
                          5,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp |
                              ImGuiTableFlags_ScrollY,
                          ImVec2(0.0f, 180.0f))) {
        ImGui::TableSetupColumn("案例");
        ImGui::TableSetupColumn("状态");
        ImGui::TableSetupColumn("参考值");
        ImGui::TableSetupColumn("结果");
        ImGui::TableSetupColumn("误差/说明");
        ImGui::TableHeadersRow();
        for (const auto& record : records) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextWrapped("%s", record.caseName.c_str());
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(validationStatusLabel(record.status));
            ImGui::TableNextColumn();
            ImGui::Text("%.6g %s", record.referenceValue, record.units.c_str());
            ImGui::TableNextColumn();
            ImGui::Text("%.6g %s", record.measuredValue, record.units.c_str());
            ImGui::TableNextColumn();
            ImGui::TextWrapped("%.3e | %s", record.absoluteError, record.notes.c_str());
        }
        ImGui::EndTable();
    }
}

void ensureDefaultDockLayout(ImGuiID dockspaceId, bool& layoutInitialized) {
    if (layoutInitialized && ImGui::DockBuilderGetNode(dockspaceId) != nullptr) {
        return;
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::DockBuilderRemoveNode(dockspaceId);
    ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspaceId, viewport->WorkSize);

    ImGuiID dockMain = dockspaceId;
    ImGuiID dockLeft = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Left, 0.23f, nullptr, &dockMain);
    ImGuiID dockRight = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Right, 0.24f, nullptr, &dockMain);
    ImGuiID dockBottom = ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Down, 0.34f, nullptr, &dockMain);
    ImGuiID dockRightBottom = ImGui::DockBuilderSplitNode(dockRight, ImGuiDir_Down, 0.43f, nullptr, &dockRight);
    ImGuiID dockRightTabs = ImGui::DockBuilderSplitNode(dockRightBottom, ImGuiDir_Down, 0.52f, nullptr, &dockRightBottom);

    ImGui::DockBuilderDockWindow(kPanelScene, dockMain);
    ImGui::DockBuilderDockWindow(kPanelInspector, dockLeft);
    ImGui::DockBuilderDockWindow(kPanelPhysics, dockRight);
    ImGui::DockBuilderDockWindow(kPanelPlots, dockBottom);
    ImGui::DockBuilderDockWindow(kPanelPerformance, dockRightBottom);
    ImGui::DockBuilderDockWindow(kPanelHelp, dockRightTabs);
    ImGui::DockBuilderDockWindow(kPanelLog, dockRightTabs);
    ImGui::DockBuilderFinish(dockspaceId);
    layoutInitialized = true;
}

void markAllDirty(quantum::app::SimulationState& state) {
    state.dirty.physics = true;
    state.dirty.cloud = true;
    state.dirty.solver = true;
}

void applyModelPreset(quantum::app::SimulationState& state, quantum::app::ModelKind kind) {
    state.modelKind = kind;
    switch (kind) {
    case quantum::app::ModelKind::Bohr:
        state.view.showOrbitRings = true;
        break;
    case quantum::app::ModelKind::WaveParticle:
        state.view.cloudRenderMode = quantum::app::CloudRenderMode::PointCloud;
        state.view.showOrbitRings = true;
        break;
    case quantum::app::ModelKind::SchrodingerCloud:
        state.view.cloudRenderMode = state.cloud.buildVolume ? quantum::app::CloudRenderMode::Hybrid
                                                             : quantum::app::CloudRenderMode::PointCloud;
        state.view.showOrbitRings = false;
        break;
    case quantum::app::ModelKind::Compare:
        state.view.cloudRenderMode = state.cloud.buildVolume ? quantum::app::CloudRenderMode::Hybrid
                                                             : quantum::app::CloudRenderMode::PointCloud;
        state.view.showOrbitRings = true;
        break;
    }
}

void drawScenarioButtons(quantum::app::SimulationState& state) {
    if (ImGui::Button("H")) {
        state.demo.enabled = false;
        state.demo.currentStepName = "手动";
        state.demo.stepProgress = 0.0;
        state.atomicNumber = 1;
        state.chargeState = 0;
        state.approximationMode = quantum::physics::ApproximationMode::StrictHydrogenic;
        state.cloud.components = {{{2, 1, 0}, 1.0, 0.0, 1.0}};
        state.solver.qn = {2, 1, 0};
        markAllDirty(state);
    }
    ImGui::SameLine();
    if (ImGui::Button("He+")) {
        state.demo.enabled = false;
        state.demo.currentStepName = "手动";
        state.demo.stepProgress = 0.0;
        state.atomicNumber = 2;
        state.chargeState = 1;
        state.approximationMode = quantum::physics::ApproximationMode::StrictHydrogenic;
        state.cloud.components = {{{3, 2, 0}, 1.0, 0.0, 2.0}};
        state.solver.qn = {3, 2, 0};
        markAllDirty(state);
    }
    ImGui::SameLine();
    if (ImGui::Button("Ne 近似")) {
        state.demo.enabled = false;
        state.demo.currentStepName = "手动";
        state.demo.stepProgress = 0.0;
        state.atomicNumber = 10;
        state.chargeState = 0;
        state.approximationMode = quantum::physics::ApproximationMode::MultiElectronApproximation;
        state.cloud.components = {{{2, 1, 0}, 0.8, 0.0, 1.0}, {{2, 1, 1}, 0.6, 1.57, 1.0}};
        state.solver.qn = {2, 1, 0};
        markAllDirty(state);
    }
}

struct PlotCanvas {
    ImRect outer;
    ImRect inner;
    bool hovered = false;
    bool clicked = false;
    bool active = false;
    ImVec2 mousePosition{};
    ImVec2 mouseDelta{};
    float mouseWheel = 0.0f;
};

struct PlotRange {
    double minX = 0.0;
    double maxX = 1.0;
    double minY = 0.0;
    double maxY = 1.0;
};

struct PlotViewState {
    bool hasOverride = false;
    PlotRange range{};
    bool boxSelecting = false;
    bool panning = false;
    ImVec2 dragStart{};
    ImVec2 dragCurrent{};
};

std::string formatPlotValue(double value, int precision = 4) {
    std::ostringstream stream;
    stream.setf(std::ios::fixed, std::ios::floatfield);
    stream.precision(precision);
    stream << value;
    return stream.str();
}

double niceTickStep(double rawStep) {
    if (!(rawStep > 0.0) || !std::isfinite(rawStep)) {
        return 1.0;
    }
    const double exponent = std::floor(std::log10(rawStep));
    const double power = std::pow(10.0, exponent);
    const double fraction = rawStep / power;
    double niceFraction = 1.0;
    if (fraction <= 1.0) {
        niceFraction = 1.0;
    } else if (fraction <= 2.0) {
        niceFraction = 2.0;
    } else if (fraction <= 5.0) {
        niceFraction = 5.0;
    } else {
        niceFraction = 10.0;
    }
    return niceFraction * power;
}

int tickPrecision(double step) {
    const double absStep = std::abs(step);
    if (!(absStep > 0.0) || !std::isfinite(absStep)) {
        return 2;
    }
    int precision = 0;
    if (absStep < 1.0) {
        precision = static_cast<int>(std::ceil(-std::log10(absStep) - 1.0e-9));
    }
    return std::clamp(precision, 0, 6);
}

std::string formatTickValue(double value, double step) {
    const double absValue = std::abs(value);
    const double absStep = std::abs(step);
    std::ostringstream stream;
    if ((absValue >= 1.0e5 || (absValue > 0.0 && absValue < 1.0e-4)) && absStep < 1.0e4) {
        stream.setf(std::ios::scientific, std::ios::floatfield);
        stream.precision(2);
    } else {
        stream.setf(std::ios::fixed, std::ios::floatfield);
        stream.precision(tickPrecision(step));
    }
    const double snappedValue = (absValue < absStep * 1.0e-6) ? 0.0 : value;
    stream << snappedValue;
    return stream.str();
}

std::vector<double> buildNiceTicks(double minValue, double maxValue, int maxTickCount) {
    maxTickCount = std::clamp(maxTickCount, 2, 10);
    if (!(maxValue > minValue)) {
        return {minValue, maxValue + 1.0};
    }

    double step = niceTickStep((maxValue - minValue) / static_cast<double>(maxTickCount - 1));
    auto makeTicks = [&](double tickStep) {
        std::vector<double> ticks;
        const double epsilon = tickStep * 1.0e-6;
        const double first = std::ceil((minValue - epsilon) / tickStep) * tickStep;
        for (double value = first; value <= maxValue + epsilon; value += tickStep) {
            ticks.push_back(value);
        }
        if (ticks.empty()) {
            ticks.push_back(minValue);
            ticks.push_back(maxValue);
        }
        return ticks;
    };

    std::vector<double> ticks = makeTicks(step);
    while (ticks.size() > static_cast<std::size_t>(maxTickCount) && std::isfinite(step)) {
        step = niceTickStep(step * 1.5);
        ticks = makeTicks(step);
    }
    if (ticks.size() < 2) {
        ticks = {minValue, maxValue};
    }
    return ticks;
}

bool pointInRect(const ImRect& rect, const ImVec2& point) {
    return point.x >= rect.Min.x && point.x <= rect.Max.x && point.y >= rect.Min.y && point.y <= rect.Max.y;
}

ImVec2 clampToRect(const ImRect& rect, const ImVec2& point) {
    return {std::clamp(point.x, rect.Min.x, rect.Max.x), std::clamp(point.y, rect.Min.y, rect.Max.y)};
}

PlotViewState& plotViewState(const char* label) {
    static std::unordered_map<std::string, PlotViewState> states;
    return states[std::string(label)];
}

PlotRange sanitizePlotRange(PlotRange range) {
    if (range.maxX <= range.minX) {
        const double center = 0.5 * (range.minX + range.maxX);
        range.minX = center - 0.5;
        range.maxX = center + 0.5;
    }
    if (range.maxY <= range.minY) {
        const double center = 0.5 * (range.minY + range.maxY);
        range.minY = center - 0.5;
        range.maxY = center + 0.5;
    }
    return range;
}

PlotRange resolvePlotRange(PlotViewState& viewState, PlotRange dataRange) {
    dataRange = sanitizePlotRange(dataRange);
    if (!viewState.hasOverride) {
        viewState.range = dataRange;
        return dataRange;
    }
    return sanitizePlotRange(viewState.range);
}

double pixelToPlotX(const ImRect& rect, const PlotRange& range, float pixelX) {
    const float t = (pixelX - rect.Min.x) / std::max(rect.GetWidth(), 1.0f);
    return range.minX + static_cast<double>(t) * (range.maxX - range.minX);
}

double pixelToPlotY(const ImRect& rect, const PlotRange& range, float pixelY) {
    const float t = (rect.Max.y - pixelY) / std::max(rect.GetHeight(), 1.0f);
    return range.minY + static_cast<double>(t) * (range.maxY - range.minY);
}

void clearPlotView(PlotViewState& viewState) {
    viewState.hasOverride = false;
    viewState.boxSelecting = false;
    viewState.panning = false;
}

bool valueWithin(double value, double minValue, double maxValue) {
    return value >= minValue && value <= maxValue;
}

bool pointWithinPlotRange(const PlotRange& range, double x, double y) {
    return valueWithin(x, range.minX, range.maxX) && valueWithin(y, range.minY, range.maxY);
}

void drawPlotRangeSummary(const char* label,
                          const PlotViewState& viewState,
                          const char* xAxisLabel,
                          const char* yAxisLabel) {
    const char* windowMode = viewState.hasOverride ? "局部窗口" : "全局窗口";
    ImGui::Text("%s：%s", label, windowMode);
    if (xAxisLabel != nullptr) {
        ImGui::Text("%s = [%s, %s]",
                    xAxisLabel,
                    formatPlotValue(viewState.range.minX).c_str(),
                    formatPlotValue(viewState.range.maxX).c_str());
    }
    if (yAxisLabel != nullptr) {
        ImGui::Text("%s = [%s, %s]",
                    yAxisLabel,
                    formatPlotValue(viewState.range.minY).c_str(),
                    formatPlotValue(viewState.range.maxY).c_str());
    }
}

void syncReportingPlotWindow(quantum::app::PlotWindowRange& destination, const PlotViewState& source) {
    destination.hasOverride = source.hasOverride;
    destination.minX = source.range.minX;
    destination.maxX = source.range.maxX;
    destination.minY = source.range.minY;
    destination.maxY = source.range.maxY;
}

int nearestPointIndex(const std::vector<ImVec2>& points, const ImVec2& mousePosition, float maxDistancePixels) {
    int bestIndex = -1;
    float bestDistanceSquared = maxDistancePixels * maxDistancePixels;
    for (std::size_t index = 0; index < points.size(); ++index) {
        const float dx = points[index].x - mousePosition.x;
        const float dy = points[index].y - mousePosition.y;
        const float distanceSquared = dx * dx + dy * dy;
        if (distanceSquared <= bestDistanceSquared) {
            bestDistanceSquared = distanceSquared;
            bestIndex = static_cast<int>(index);
        }
    }
    return bestIndex;
}

void drawHighlightedPoint(ImDrawList* drawList, const ImVec2& point, ImU32 color) {
    drawList->AddCircleFilled(point, 4.5f, IM_COL32(20, 24, 31, 255));
    drawList->AddCircle(point, 6.5f, color, 0, 2.0f);
}

std::string animatedEllipsis() {
    const int phase = static_cast<int>(ImGui::GetTime() * 3.0) % 4;
    return std::string(static_cast<std::size_t>(phase), '.');
}

PlotCanvas beginPlotCanvas(const char* label,
                           float height,
                           const char* xAxisLabel,
                           const char* yAxisLabel,
                           PlotViewState* viewState = nullptr) {
    ImGui::PushID(label);
    ImGui::TextUnformatted(label);
    if (viewState != nullptr) {
        ImGui::SameLine();
        if (ImGui::SmallButton("重置视图")) {
            clearPlotView(*viewState);
        }
        if (viewState->hasOverride) {
            ImGui::SameLine();
            ImGui::TextDisabled("已缩放/平移");
        }
    }
    const ImVec2 canvasSize(ImGui::GetContentRegionAvail().x, height);
    const std::string buttonId = std::string("##plot_") + label;
    ImGui::InvisibleButton(buttonId.c_str(), canvasSize);
    const ImRect outer(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    const float leftMargin = std::clamp(canvasSize.x * 0.14f, 54.0f, 78.0f);
    const float rightMargin = std::clamp(canvasSize.x * 0.05f, 18.0f, 28.0f);
    const float topMargin = 56.0f;
    const float bottomMargin = 38.0f;
    const ImRect inner(ImVec2(outer.Min.x + leftMargin, outer.Min.y + topMargin),
                       ImVec2(outer.Max.x - rightMargin, outer.Max.y - bottomMargin));
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(outer.Min, outer.Max, IM_COL32(20, 24, 31, 255), 4.0f);
    drawList->AddRect(outer.Min, outer.Max, IM_COL32(70, 78, 88, 255), 4.0f);
    drawList->AddRectFilled(inner.Min, inner.Max, IM_COL32(14, 18, 24, 255), 2.0f);
    drawList->AddRect(inner.Min, inner.Max, IM_COL32(62, 71, 82, 255), 2.0f);

    for (int index = 1; index < 5; ++index) {
        const float t = static_cast<float>(index) / 5.0f;
        const float x = inner.Min.x + inner.GetWidth() * t;
        const float y = inner.Min.y + inner.GetHeight() * t;
        drawList->AddLine(ImVec2(x, inner.Min.y), ImVec2(x, inner.Max.y), IM_COL32(44, 52, 62, 180), 1.0f);
        drawList->AddLine(ImVec2(inner.Min.x, y), ImVec2(inner.Max.x, y), IM_COL32(44, 52, 62, 180), 1.0f);
    }

    if (xAxisLabel != nullptr) {
        const ImVec2 textSize = ImGui::CalcTextSize(xAxisLabel);
        drawList->AddText(ImVec2(inner.Max.x - textSize.x, outer.Max.y - textSize.y - 8.0f),
                          IM_COL32(180, 188, 196, 255),
                          xAxisLabel);
    }
    if (yAxisLabel != nullptr) {
        drawList->AddText(ImVec2(outer.Min.x + 10.0f, outer.Min.y + 8.0f), IM_COL32(180, 188, 196, 255), yAxisLabel);
    }
    const PlotCanvas canvas{outer,
                            inner,
                            ImGui::IsItemHovered(),
                            ImGui::IsItemClicked(ImGuiMouseButton_Left),
                            ImGui::IsItemActive(),
                            ImGui::GetIO().MousePos,
                            ImGui::GetIO().MouseDelta,
                            ImGui::GetIO().MouseWheel};
    ImGui::PopID();
    return canvas;
}

void drawPlotBounds(const PlotCanvas& canvas, double minX, double maxX, double minY, double maxY, bool showX = true, bool showY = true) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const int xTickBudget = std::clamp(static_cast<int>(canvas.inner.GetWidth() / 110.0f), 3, 7);
    const int yTickBudget = std::clamp(static_cast<int>(canvas.inner.GetHeight() / 42.0f), 3, 7);
    const auto xTicks = buildNiceTicks(minX, maxX, xTickBudget);
    const auto yTicks = buildNiceTicks(minY, maxY, yTickBudget);
    const double xStep = (xTicks.size() >= 2) ? (xTicks[1] - xTicks[0]) : (maxX - minX);
    const double yStep = (yTicks.size() >= 2) ? (yTicks[1] - yTicks[0]) : (maxY - minY);

    if (showX) {
        float lastMaxX = -1.0e9f;
        for (std::size_t index = 0; index < xTicks.size(); ++index) {
            const std::string text = formatTickValue(xTicks[index], xStep);
            const ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
            const float t = static_cast<float>((xTicks[index] - minX) / std::max(maxX - minX, 1.0e-9));
            float x = canvas.inner.Min.x + canvas.inner.GetWidth() * t - textSize.x * 0.5f;
            x = std::clamp(x, canvas.outer.Min.x + 8.0f, canvas.outer.Max.x - textSize.x - 8.0f);
            if (x < lastMaxX + 10.0f) {
                continue;
            }
            drawList->AddText(ImVec2(x, canvas.outer.Max.y - 24.0f), IM_COL32(160, 170, 180, 255), text.c_str());
            lastMaxX = x + textSize.x;
        }
    }
    if (showY) {
        float lastY = 1.0e9f;
        for (std::size_t index = 0; index < yTicks.size(); ++index) {
            const std::string text = formatTickValue(yTicks[index], yStep);
            const ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
            const float t = static_cast<float>((yTicks[index] - minY) / std::max(maxY - minY, 1.0e-9));
            const float y = canvas.inner.Max.y - canvas.inner.GetHeight() * t - textSize.y * 0.5f;
            if (std::abs(y - lastY) < textSize.y + 3.0f) {
                continue;
            }
            drawList->AddText(ImVec2(canvas.outer.Min.x + 10.0f, y), IM_COL32(160, 170, 180, 255), text.c_str());
            lastY = y;
        }
    }
}

void applyPlotInteractions(const PlotCanvas& canvas, PlotViewState& viewState, PlotRange& range) {
    range = sanitizePlotRange(range);
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const bool mouseInside = canvas.hovered && pointInRect(canvas.inner, canvas.mousePosition);

    if (mouseInside && std::abs(canvas.mouseWheel) > 1.0e-6f) {
        const double anchorX = pixelToPlotX(canvas.inner, range, canvas.mousePosition.x);
        const double anchorY = pixelToPlotY(canvas.inner, range, canvas.mousePosition.y);
        const double zoomFactor = std::exp(-static_cast<double>(canvas.mouseWheel) * 0.18);
        const double newWidth = std::clamp((range.maxX - range.minX) * zoomFactor, 1.0e-9, 1.0e12);
        const double newHeight = std::clamp((range.maxY - range.minY) * zoomFactor, 1.0e-9, 1.0e12);
        const double tx = (anchorX - range.minX) / std::max(range.maxX - range.minX, 1.0e-12);
        const double ty = (anchorY - range.minY) / std::max(range.maxY - range.minY, 1.0e-12);
        range.minX = anchorX - newWidth * tx;
        range.maxX = range.minX + newWidth;
        range.minY = anchorY - newHeight * ty;
        range.maxY = range.minY + newHeight;
        viewState.hasOverride = true;
    }

    if (mouseInside && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        viewState.boxSelecting = true;
        viewState.dragStart = clampToRect(canvas.inner, canvas.mousePosition);
        viewState.dragCurrent = viewState.dragStart;
    }
    if (viewState.boxSelecting) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            viewState.dragCurrent = clampToRect(canvas.inner, canvas.mousePosition);
            drawList->AddRectFilled(viewState.dragStart, viewState.dragCurrent, IM_COL32(90, 150, 255, 36));
            drawList->AddRect(viewState.dragStart, viewState.dragCurrent, IM_COL32(114, 182, 255, 200), 0.0f, 0, 1.5f);
        } else {
            const ImRect selection(ImMin(viewState.dragStart, viewState.dragCurrent),
                                   ImMax(viewState.dragStart, viewState.dragCurrent));
            const float selectionWidth = selection.GetWidth();
            const float selectionHeight = selection.GetHeight();
            if (selectionWidth >= 10.0f && selectionHeight >= 10.0f) {
                range.minX = pixelToPlotX(canvas.inner, range, selection.Min.x);
                range.maxX = pixelToPlotX(canvas.inner, range, selection.Max.x);
                range.minY = pixelToPlotY(canvas.inner, range, selection.Max.y);
                range.maxY = pixelToPlotY(canvas.inner, range, selection.Min.y);
                range = sanitizePlotRange(range);
                viewState.hasOverride = true;
            }
            viewState.boxSelecting = false;
        }
    }

    if (mouseInside && (ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseClicked(ImGuiMouseButton_Middle))) {
        viewState.panning = true;
    }
    if (viewState.panning) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Right) || ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
            const double dx = -static_cast<double>(canvas.mouseDelta.x) * (range.maxX - range.minX) /
                              std::max(canvas.inner.GetWidth(), 1.0f);
            const double dy = static_cast<double>(canvas.mouseDelta.y) * (range.maxY - range.minY) /
                              std::max(canvas.inner.GetHeight(), 1.0f);
            range.minX += dx;
            range.maxX += dx;
            range.minY += dy;
            range.maxY += dy;
            viewState.hasOverride = true;
        } else {
            viewState.panning = false;
        }
    }

    if (mouseInside && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Right)) {
        clearPlotView(viewState);
    }

    if (viewState.hasOverride) {
        viewState.range = sanitizePlotRange(range);
        range = viewState.range;
    }
}

void drawSceneStatusOverlay(const ImRect& rect,
                            const quantum::app::SimulationState& state,
                            const quantum::core::PerformanceSnapshot& performance) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const float overlayHeight = 30.0f;
    const ImVec2 min(rect.Min.x + 12.0f, rect.Max.y - overlayHeight - 12.0f);
    const ImVec2 max(rect.Max.x - 12.0f, rect.Max.y - 12.0f);
    drawList->AddRectFilled(min, max, IM_COL32(10, 14, 18, 180), 6.0f);

    std::ostringstream stream;
    stream.setf(std::ios::fixed, std::ios::floatfield);
    stream.precision(1);
    stream << modelLabel(state.modelKind) << " | " << cloudModeLabel(state.view.cloudRenderMode)
           << " | 帧率 " << performance.fps
           << " | 中心场 " << (state.solver.useScreenedCentralField ? "屏蔽" : "库仑");
    if (state.demo.enabled) {
        stream << " | 演示 " << state.demo.currentStepName << " (" << static_cast<int>(state.demo.stepProgress * 100.0) << "%)";
    }
    if (state.derived.spectroscopy.applied) {
        stream << " | 谱学修正开启";
    }
    if (performance.cloudBuildInFlight) {
        stream << " | 云数据生成中" << animatedEllipsis();
    } else if (performance.cloudBuildQueued) {
        stream << " | 已排队等待更新";
    }

    drawList->AddText(ImVec2(min.x + 10.0f, min.y + 7.0f), IM_COL32(228, 234, 241, 255), stream.str().c_str());
}

ImVec2 toPlotPoint(const ImRect& rect,
                   double x,
                   double y,
                   double minX,
                   double maxX,
                   double minY,
                   double maxY) {
    const float px = rect.Min.x + static_cast<float>((x - minX) / std::max(1e-9, maxX - minX)) * rect.GetWidth();
    const float py = rect.Max.y - static_cast<float>((y - minY) / std::max(1e-9, maxY - minY)) * rect.GetHeight();
    return {px, py};
}

void drawPolylinePlot(const char* label,
                      const std::vector<double>& xs,
                      const std::vector<double>& ys,
                      int& selectedIndex,
                      const ImU32 color,
                      float height = 220.0f,
                      const char* xAxisLabel = nullptr,
                      const char* yAxisLabel = nullptr) {
    PlotViewState& viewState = plotViewState(label);
    const PlotCanvas canvas = beginPlotCanvas(label, height, xAxisLabel, yAxisLabel, &viewState);
    if (xs.size() < 2 || ys.size() < 2) {
        return;
    }
    const auto [minXIt, maxXIt] = std::minmax_element(xs.begin(), xs.end());
    const auto [minYIt, maxYIt] = std::minmax_element(ys.begin(), ys.end());
    PlotRange range{*minXIt, *maxXIt, *minYIt, *maxYIt};
    range = resolvePlotRange(viewState, range);
    applyPlotInteractions(canvas, viewState, range);
    drawPlotBounds(canvas, range.minX, range.maxX, range.minY, range.maxY);
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    std::vector<ImVec2> points;
    points.reserve(xs.size());
    for (std::size_t index = 1; index < xs.size(); ++index) {
        const ImVec2 p0 =
            toPlotPoint(canvas.inner, xs[index - 1], ys[index - 1], range.minX, range.maxX, range.minY, range.maxY);
        const ImVec2 p1 = toPlotPoint(canvas.inner, xs[index], ys[index], range.minX, range.maxX, range.minY, range.maxY);
        drawList->AddLine(p0, p1, color, 2.0f);
        drawList->AddCircleFilled(p1, 2.5f, color);
        if (index == 1) {
            points.push_back(p0);
        }
        points.push_back(p1);
    }

    const bool mouseInside = canvas.hovered && pointInRect(canvas.inner, canvas.mousePosition);
    const int hoveredIndex = mouseInside ? nearestPointIndex(points, canvas.mousePosition, 10.0f) : -1;
    if (mouseInside && hoveredIndex >= 0) {
        const ImVec2 point = points[static_cast<std::size_t>(hoveredIndex)];
        drawList->AddLine(ImVec2(point.x, canvas.inner.Min.y), ImVec2(point.x, canvas.inner.Max.y), IM_COL32(90, 100, 112, 180), 1.0f);
        drawList->AddLine(ImVec2(canvas.inner.Min.x, point.y), ImVec2(canvas.inner.Max.x, point.y), IM_COL32(90, 100, 112, 180), 1.0f);
        drawHighlightedPoint(drawList, point, color);
        ImGui::BeginTooltip();
        ImGui::Text("%s", label);
        if (xAxisLabel != nullptr) {
            ImGui::Text("%s: %s", xAxisLabel, formatPlotValue(xs[static_cast<std::size_t>(hoveredIndex)]).c_str());
        }
        if (yAxisLabel != nullptr) {
            ImGui::Text("%s: %s", yAxisLabel, formatPlotValue(ys[static_cast<std::size_t>(hoveredIndex)]).c_str());
        }
        ImGui::EndTooltip();
    }
    if (canvas.clicked && hoveredIndex >= 0) {
        selectedIndex = hoveredIndex;
    }
    if (selectedIndex >= 0 && static_cast<std::size_t>(selectedIndex) < points.size()) {
        drawHighlightedPoint(drawList, points[static_cast<std::size_t>(selectedIndex)], IM_COL32(255, 208, 92, 255));
    }
}

void drawPolylinePlot(const char* label,
                      const std::vector<double>& xs,
                      const std::vector<double>& ys,
                      const ImU32 color,
                      float height = 220.0f,
                      const char* xAxisLabel = nullptr,
                      const char* yAxisLabel = nullptr) {
    static int fallbackSelection = -1;
    drawPolylinePlot(label, xs, ys, fallbackSelection, color, height, xAxisLabel, yAxisLabel);
}

void drawLegendItem(ImDrawList* drawList,
                    const ImVec2& origin,
                    ImU32 color,
                    const char* label,
                    bool dashed = false,
                    bool filledMarker = false) {
    const ImVec2 p0(origin.x, origin.y + 8.0f);
    const ImVec2 p1(origin.x + 22.0f, origin.y + 8.0f);
    if (dashed) {
        constexpr float dashLength = 4.0f;
        constexpr float gapLength = 3.0f;
        float x = p0.x;
        while (x < p1.x) {
            const float nextX = std::min(x + dashLength, p1.x);
            drawList->AddLine(ImVec2(x, p0.y), ImVec2(nextX, p1.y), color, 2.0f);
            x = nextX + gapLength;
        }
    } else {
        drawList->AddLine(p0, p1, color, 2.0f);
    }
    if (filledMarker) {
        drawList->AddCircleFilled(ImVec2(origin.x + 11.0f, origin.y + 8.0f), 3.0f, color);
    }
    drawList->AddText(ImVec2(origin.x + 28.0f, origin.y), IM_COL32(210, 218, 226, 255), label);
}

struct LegendEntry {
    ImU32 color = 0;
    const char* label = "";
    bool dashed = false;
    bool filledMarker = false;
};

void drawLegendWrapped(ImDrawList* drawList,
                       const ImVec2& origin,
                       float maxWidth,
                       std::initializer_list<LegendEntry> items) {
    float cursorX = origin.x;
    float cursorY = origin.y;
    constexpr float itemSpacing = 14.0f;
    constexpr float rowSpacing = 18.0f;
    for (const auto& item : items) {
        const ImVec2 textSize = ImGui::CalcTextSize(item.label);
        const float itemWidth = 28.0f + textSize.x + itemSpacing;
        if (cursorX > origin.x && cursorX + itemWidth > origin.x + maxWidth) {
            cursorX = origin.x;
            cursorY += rowSpacing;
        }
        drawLegendItem(drawList, ImVec2(cursorX, cursorY), item.color, item.label, item.dashed, item.filledMarker);
        cursorX += itemWidth;
    }
}

bool matchesTransitionLine(const quantum::physics::SpectrumLine& line, const quantum::app::TransitionSelection& transition) {
    return line.initial.n == transition.initial.n && line.initial.l == transition.initial.l &&
           line.initial.m == transition.initial.m && line.final.n == transition.final.n &&
           line.final.l == transition.final.l && line.final.m == transition.final.m;
}

void drawSpectrumPlot(const std::vector<quantum::physics::SpectrumLine>& calculated,
                      const std::vector<quantum::physics::SpectrumLine>& references,
                      const quantum::app::TransitionSelection& currentTransition,
                      const quantum::physics::TransitionResult& currentTransitionBase,
                      const quantum::spectroscopy::SpectroscopyCorrectionResult& currentTransitionCorrection,
                      bool filterAllowed,
                      int& selectedIndex,
                      bool& selectedReference) {
    PlotViewState& viewState = plotViewState("谱线图");
    const PlotCanvas canvas = beginPlotCanvas("谱线图", 220.0f, "波长 (nm)", "能量 (eV)", &viewState);
    double minX = 1e9;
    double maxX = 0.0;
    double maxY = 0.0;
    auto expandRange = [&](const quantum::physics::SpectrumLine& line) {
        if (filterAllowed && !line.allowed) {
            return;
        }
        minX = std::min(minX, line.wavelengthNm);
        maxX = std::max(maxX, line.wavelengthNm);
        maxY = std::max(maxY, line.photonEnergyEv);
    };
    for (const auto& line : calculated) {
        expandRange(line);
    }
    for (const auto& line : references) {
        expandRange(line);
    }
    if (currentTransitionBase.allowed) {
        minX = std::min(minX, currentTransitionBase.wavelengthNm);
        maxX = std::max(maxX, currentTransitionBase.wavelengthNm);
        maxY = std::max(maxY, currentTransitionBase.deltaEnergyEv);
    }
    if (currentTransitionCorrection.applied && currentTransitionBase.allowed) {
        minX = std::min(minX, currentTransitionCorrection.correctedWavelengthNm);
        maxX = std::max(maxX, currentTransitionCorrection.correctedWavelengthNm);
        maxY = std::max(maxY, currentTransitionCorrection.correctedDeltaEnergyEv);
    }
    if (maxX <= minX) {
        minX -= 1.0;
        maxX += 1.0;
    }
    maxY = std::max(maxY * 1.10, 1.0);
    const double xPadding = std::max(0.5, (maxX - minX) * 0.04);
    minX -= xPadding;
    maxX += xPadding;

    PlotRange range{minX, maxX, 0.0, maxY};
    range = resolvePlotRange(viewState, range);
    applyPlotInteractions(canvas, viewState, range);
    drawPlotBounds(canvas, range.minX, range.maxX, range.minY, range.maxY);
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    std::vector<ImVec2> calculatedAnchors;
    std::vector<const quantum::physics::SpectrumLine*> calculatedLines;
    std::vector<int> calculatedIndices;
    std::vector<ImVec2> referenceAnchors;
    std::vector<const quantum::physics::SpectrumLine*> referenceLines;
    std::vector<int> referenceIndices;

    drawLegendWrapped(drawList,
                      ImVec2(canvas.inner.Min.x + 10.0f, canvas.outer.Min.y + 2.0f),
                      canvas.inner.GetWidth() - 20.0f,
                      {{IM_COL32(72, 176, 255, 220), "计算谱线", false, false},
                       {IM_COL32(237, 184, 56, 220), "参考谱线", false, false},
                       {IM_COL32(94, 222, 196, 235), "当前基线", false, true},
                       {IM_COL32(255, 108, 196, 240), "修正后", true, true}});

    for (std::size_t lineIndex = 0; lineIndex < calculated.size(); ++lineIndex) {
        const auto& line = calculated[lineIndex];
        if (filterAllowed && !line.allowed) {
            continue;
        }
        const ImVec2 p0 = toPlotPoint(canvas.inner, line.wavelengthNm, 0.0, range.minX, range.maxX, range.minY, range.maxY);
        const ImVec2 p1 = toPlotPoint(canvas.inner,
                                      line.wavelengthNm,
                                      line.photonEnergyEv,
                                      range.minX,
                                      range.maxX,
                                      range.minY,
                                      range.maxY);
        const bool isCurrent = matchesTransitionLine(line, currentTransition);
        const ImU32 color = isCurrent ? IM_COL32(94, 222, 196, 235) : IM_COL32(72, 176, 255, 220);
        drawList->AddLine(p0, p1, color, isCurrent ? 2.5f : 1.5f);
        drawList->AddCircleFilled(p1, isCurrent ? 3.8f : 2.8f, color);
        calculatedAnchors.push_back(p1);
        calculatedLines.push_back(&line);
        calculatedIndices.push_back(static_cast<int>(lineIndex));
    }
    for (std::size_t lineIndex = 0; lineIndex < references.size(); ++lineIndex) {
        const auto& line = references[lineIndex];
        const ImVec2 p0 = toPlotPoint(canvas.inner, line.wavelengthNm, 0.0, range.minX, range.maxX, range.minY, range.maxY);
        const ImVec2 p1 = toPlotPoint(canvas.inner,
                                      line.wavelengthNm,
                                      line.photonEnergyEv,
                                      range.minX,
                                      range.maxX,
                                      range.minY,
                                      range.maxY);
        drawList->AddLine(p0, p1, IM_COL32(237, 184, 56, 220), 2.0f);
        drawList->AddCircleFilled(p1, 3.0f, IM_COL32(237, 184, 56, 220));
        referenceAnchors.push_back(p1);
        referenceLines.push_back(&line);
        referenceIndices.push_back(static_cast<int>(lineIndex));
    }

    bool hasCorrectedOverlay = currentTransitionCorrection.applied && currentTransitionBase.allowed &&
                               currentTransitionCorrection.correctedWavelengthNm > 0.0 &&
                               currentTransitionCorrection.correctedDeltaEnergyEv > 0.0;
    ImVec2 baselinePoint{};
    ImVec2 correctedPoint{};
    if (currentTransitionBase.allowed) {
        const ImVec2 p0 =
            toPlotPoint(canvas.inner, currentTransitionBase.wavelengthNm, 0.0, range.minX, range.maxX, range.minY, range.maxY);
        baselinePoint = toPlotPoint(canvas.inner,
                                    currentTransitionBase.wavelengthNm,
                                    currentTransitionBase.deltaEnergyEv,
                                    range.minX,
                                    range.maxX,
                                    range.minY,
                                    range.maxY);
        drawList->AddLine(p0, baselinePoint, IM_COL32(94, 222, 196, 160), 2.4f);
        drawList->AddCircleFilled(baselinePoint, 4.0f, IM_COL32(94, 222, 196, 235));
    }
    if (hasCorrectedOverlay) {
        const ImVec2 correctedBase =
            toPlotPoint(canvas.inner,
                        currentTransitionCorrection.correctedWavelengthNm,
                        0.0,
                        range.minX,
                        range.maxX,
                        range.minY,
                        range.maxY);
        correctedPoint = toPlotPoint(canvas.inner,
                                     currentTransitionCorrection.correctedWavelengthNm,
                                     currentTransitionCorrection.correctedDeltaEnergyEv,
                                     range.minX,
                                     range.maxX,
                                     range.minY,
                                     range.maxY);
        drawList->AddLine(baselinePoint, correctedPoint, IM_COL32(196, 204, 214, 160), 1.0f);
        constexpr float dash = 5.0f;
        constexpr float gap = 3.0f;
        float y = correctedBase.y;
        while (y > correctedPoint.y) {
            const float nextY = std::max(correctedPoint.y, y - dash);
            drawList->AddLine(ImVec2(correctedBase.x, y), ImVec2(correctedBase.x, nextY), IM_COL32(255, 108, 196, 240), 2.0f);
            y = nextY - gap;
        }
        drawList->AddCircleFilled(correctedPoint, 4.2f, IM_COL32(255, 108, 196, 240));
    }

    const bool mouseInside = canvas.hovered && pointInRect(canvas.inner, canvas.mousePosition);
    const int hoveredCalculated = mouseInside ? nearestPointIndex(calculatedAnchors, canvas.mousePosition, 9.0f) : -1;
    const int hoveredReference = mouseInside ? nearestPointIndex(referenceAnchors, canvas.mousePosition, 9.0f) : -1;
    const int hoveredCorrected =
        (mouseInside && hasCorrectedOverlay)
            ? nearestPointIndex(std::vector<ImVec2>{baselinePoint, correctedPoint}, canvas.mousePosition, 11.0f)
            : -1;
    bool hoveredIsReference = false;
    int hoveredIndex = hoveredCalculated;
    bool hoveredIsCorrection = false;
    if (hoveredReference >= 0) {
        hoveredIndex = hoveredReference;
        hoveredIsReference = true;
    }
    if (hoveredCalculated >= 0 && hoveredReference >= 0) {
        const ImVec2 calcPoint = calculatedAnchors[static_cast<std::size_t>(hoveredCalculated)];
        const ImVec2 refPoint = referenceAnchors[static_cast<std::size_t>(hoveredReference)];
        const float calcDx = calcPoint.x - canvas.mousePosition.x;
        const float calcDy = calcPoint.y - canvas.mousePosition.y;
        const float refDx = refPoint.x - canvas.mousePosition.x;
        const float refDy = refPoint.y - canvas.mousePosition.y;
        hoveredIsReference = (refDx * refDx + refDy * refDy) < (calcDx * calcDx + calcDy * calcDy);
        hoveredIndex = hoveredIsReference ? hoveredReference : hoveredCalculated;
    }
    if (hoveredCorrected >= 0 && hasCorrectedOverlay) {
        const ImVec2 candidatePoint = (hoveredCorrected == 0) ? baselinePoint : correctedPoint;
        const float dx = candidatePoint.x - canvas.mousePosition.x;
        const float dy = candidatePoint.y - canvas.mousePosition.y;
        const float correctedDistanceSquared = dx * dx + dy * dy;
        float currentBest = std::numeric_limits<float>::max();
        if (hoveredCalculated >= 0) {
            const ImVec2 calcPoint = calculatedAnchors[static_cast<std::size_t>(hoveredCalculated)];
            const float calcDx = calcPoint.x - canvas.mousePosition.x;
            const float calcDy = calcPoint.y - canvas.mousePosition.y;
            currentBest = std::min(currentBest, calcDx * calcDx + calcDy * calcDy);
        }
        if (hoveredReference >= 0) {
            const ImVec2 refPoint = referenceAnchors[static_cast<std::size_t>(hoveredReference)];
            const float refDx = refPoint.x - canvas.mousePosition.x;
            const float refDy = refPoint.y - canvas.mousePosition.y;
            currentBest = std::min(currentBest, refDx * refDx + refDy * refDy);
        }
        if (correctedDistanceSquared <= currentBest) {
            hoveredIsCorrection = true;
            hoveredIsReference = false;
        }
    }
    if (mouseInside && hoveredIsCorrection) {
        const ImVec2 point = (hoveredCorrected == 0) ? baselinePoint : correctedPoint;
        drawList->AddLine(ImVec2(point.x, canvas.inner.Min.y), ImVec2(point.x, canvas.inner.Max.y), IM_COL32(90, 100, 112, 180), 1.0f);
        drawHighlightedPoint(drawList, point, hoveredCorrected == 0 ? IM_COL32(94, 222, 196, 255) : IM_COL32(255, 108, 196, 255));
        ImGui::BeginTooltip();
        ImGui::Text("%s", hoveredCorrected == 0 ? "当前跃迁基线" : "当前跃迁修正后");
        ImGui::Text("跃迁: (%d,%d,%d)->(%d,%d,%d)",
                    currentTransition.initial.n,
                    currentTransition.initial.l,
                    currentTransition.initial.m,
                    currentTransition.final.n,
                    currentTransition.final.l,
                    currentTransition.final.m);
        ImGui::Text("波长: %s nm",
                    formatPlotValue(hoveredCorrected == 0 ? currentTransitionBase.wavelengthNm
                                                          : currentTransitionCorrection.correctedWavelengthNm)
                        .c_str());
        ImGui::Text("光子能量: %s eV",
                    formatPlotValue(hoveredCorrected == 0 ? currentTransitionBase.deltaEnergyEv
                                                          : currentTransitionCorrection.correctedDeltaEnergyEv)
                        .c_str());
        if (hasCorrectedOverlay) {
            ImGui::Separator();
            ImGui::Text("波长改变量: %s nm",
                        formatPlotValue(currentTransitionCorrection.correctedWavelengthNm - currentTransitionBase.wavelengthNm, 6)
                            .c_str());
            ImGui::Text("能量改变量: %s eV",
                        formatPlotValue(currentTransitionCorrection.correctedDeltaEnergyEv - currentTransitionBase.deltaEnergyEv, 6)
                            .c_str());
        }
        ImGui::EndTooltip();
    } else if (mouseInside && hoveredIndex >= 0) {
        const auto* line =
            hoveredIsReference ? referenceLines[static_cast<std::size_t>(hoveredIndex)]
                               : calculatedLines[static_cast<std::size_t>(hoveredIndex)];
        const ImVec2 point =
            hoveredIsReference ? referenceAnchors[static_cast<std::size_t>(hoveredIndex)]
                               : calculatedAnchors[static_cast<std::size_t>(hoveredIndex)];
        drawList->AddLine(ImVec2(point.x, canvas.inner.Min.y), ImVec2(point.x, canvas.inner.Max.y), IM_COL32(90, 100, 112, 180), 1.0f);
        drawHighlightedPoint(drawList, point, hoveredIsReference ? IM_COL32(237, 184, 56, 255) : IM_COL32(72, 176, 255, 255));
        ImGui::BeginTooltip();
        ImGui::Text("%s", hoveredIsReference ? "参考谱线" : "计算谱线");
        ImGui::Text("系列: %s", line->seriesName.c_str());
        ImGui::Text("跃迁: (%d,%d,%d)->(%d,%d,%d)",
                    line->initial.n,
                    line->initial.l,
                    line->initial.m,
                    line->final.n,
                    line->final.l,
                    line->final.m);
        ImGui::Text("波长: %s nm", formatPlotValue(line->wavelengthNm).c_str());
        ImGui::Text("光子能量: %s eV", formatPlotValue(line->photonEnergyEv).c_str());
        if (!hoveredIsReference && matchesTransitionLine(*line, currentTransition) && hasCorrectedOverlay) {
            ImGui::Separator();
            ImGui::Text("修正后波长: %s nm", formatPlotValue(currentTransitionCorrection.correctedWavelengthNm).c_str());
            ImGui::Text("修正后能量: %s eV", formatPlotValue(currentTransitionCorrection.correctedDeltaEnergyEv).c_str());
        }
        ImGui::EndTooltip();
    }
    if (canvas.clicked) {
        if (hoveredIsCorrection) {
            auto it = std::find_if(calculated.begin(), calculated.end(), [&](const quantum::physics::SpectrumLine& line) {
                return matchesTransitionLine(line, currentTransition);
            });
            if (it != calculated.end()) {
                selectedIndex = static_cast<int>(std::distance(calculated.begin(), it));
                selectedReference = false;
            }
        } else if (hoveredIndex >= 0) {
            selectedIndex = hoveredIsReference ? referenceIndices[static_cast<std::size_t>(hoveredIndex)]
                                               : calculatedIndices[static_cast<std::size_t>(hoveredIndex)];
            selectedReference = hoveredIsReference;
        }
    }
    if (selectedIndex >= 0) {
        const auto& indices = selectedReference ? referenceIndices : calculatedIndices;
        const auto& anchors = selectedReference ? referenceAnchors : calculatedAnchors;
        const auto it = std::find(indices.begin(), indices.end(), selectedIndex);
        if (it != indices.end()) {
            const std::size_t anchorIndex = static_cast<std::size_t>(std::distance(indices.begin(), it));
            if (anchorIndex < anchors.size()) {
                drawHighlightedPoint(drawList, anchors[anchorIndex], IM_COL32(255, 208, 92, 255));
            }
        }
    }
}

void drawEnergyLevelsPlot(const std::vector<quantum::physics::EnergyLevelSample>& levels,
                          const quantum::app::TransitionSelection& transition,
                          const quantum::spectroscopy::SpectroscopyCorrectionResult& correction,
                          int& selectedIndex) {
    PlotViewState& viewState = plotViewState("能级图");
    const PlotCanvas canvas = beginPlotCanvas("能级图", 220.0f, nullptr, "能量 (eV)", &viewState);
    if (levels.empty()) {
        return;
    }
    double minY = levels.front().energyEv;
    double maxY = levels.front().energyEv;
    for (const auto& level : levels) {
        minY = std::min(minY, level.energyEv);
        maxY = std::max(maxY, level.energyEv);
    }
    if (correction.applied) {
        minY = std::min(minY, std::min(correction.initial.correctedEnergyEv, correction.final.correctedEnergyEv));
        maxY = std::max(maxY, std::max(correction.initial.correctedEnergyEv, correction.final.correctedEnergyEv));
    }
    if (std::abs(maxY - minY) < 1.0e-9) {
        minY -= 1.0;
        maxY += 1.0;
    }
    const double yPadding = std::max(0.2, (maxY - minY) * 0.06);
    minY -= yPadding;
    maxY += yPadding;
    PlotRange range{0.0, 1.0, minY, maxY};
    range = resolvePlotRange(viewState, range);
    applyPlotInteractions(canvas, viewState, range);
    drawPlotBounds(canvas, range.minX, range.maxX, range.minY, range.maxY, false, true);

    struct LabelPlacement {
        const quantum::physics::EnergyLevelSample* level = nullptr;
        int sourceIndex = -1;
        float actualY = 0.0f;
        float labelY = 0.0f;
        bool drawLabel = false;
        bool initialState = false;
        bool finalState = false;
    };

    std::vector<LabelPlacement> placements;
    placements.reserve(levels.size());
    const std::size_t labelStride = (levels.size() <= 6) ? 1 : ((levels.size() <= 10) ? 2 : 3);
    for (std::size_t index = 0; index < levels.size(); ++index) {
        const auto& level = levels[index];
        const ImVec2 anchor = toPlotPoint(canvas.inner, 0.5, level.energyEv, range.minX, range.maxX, range.minY, range.maxY);
        const bool isInitial = level.n == transition.initial.n;
        const bool isFinal = level.n == transition.final.n;
        const bool isSelected = static_cast<int>(index) == selectedIndex;
        const bool drawLabel = isInitial || isFinal || isSelected || index == 0 || index + 1 == levels.size() ||
                               (index % labelStride == 0);
        placements.push_back({&level, static_cast<int>(index), anchor.y, anchor.y, drawLabel, isInitial, isFinal});
    }

    std::vector<LabelPlacement> labelPlacements;
    for (const auto& placement : placements) {
        if (placement.drawLabel) {
            labelPlacements.push_back(placement);
        }
    }
    std::sort(labelPlacements.begin(), labelPlacements.end(), [](const LabelPlacement& lhs, const LabelPlacement& rhs) {
        return lhs.actualY < rhs.actualY;
    });

    constexpr float kMinimumLabelSpacing = 19.0f;
    for (std::size_t index = 1; index < labelPlacements.size(); ++index) {
        labelPlacements[index].labelY =
            std::max(labelPlacements[index].labelY, labelPlacements[index - 1].labelY + kMinimumLabelSpacing);
    }
    if (!labelPlacements.empty()) {
        const float maxLabelY = canvas.inner.Max.y - 10.0f;
        float overflow = std::max(0.0f, labelPlacements.back().labelY - maxLabelY);
        for (auto it = labelPlacements.rbegin(); it != labelPlacements.rend(); ++it) {
            it->labelY -= overflow;
            if ((it + 1) != labelPlacements.rend()) {
                overflow = std::max(0.0f, (it + 1)->labelY + kMinimumLabelSpacing - it->labelY);
            }
        }
    }

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const float lineLeft = canvas.inner.Min.x + 10.0f;
    const float lineRight = canvas.inner.Max.x - 94.0f;
    drawLegendWrapped(drawList,
                      ImVec2(canvas.inner.Min.x + 10.0f, canvas.outer.Min.y + 2.0f),
                      canvas.inner.GetWidth() - 20.0f,
                      {{IM_COL32(164, 194, 244, 220), "基线能级", false, false},
                       {IM_COL32(242, 128, 90, 240), "初态", false, false},
                       {IM_COL32(84, 196, 244, 240), "末态", false, false},
                       {IM_COL32(255, 108, 196, 240), "修正后子能级", true, true}});
    std::vector<ImVec2> anchors;
    std::vector<int> anchorIndices;
    anchors.reserve(placements.size());
    for (const auto& placement : placements) {
        ImU32 color = IM_COL32(164, 194, 244, 220);
        float thickness = 1.6f;
        if (placement.initialState) {
            color = IM_COL32(242, 128, 90, 240);
            thickness = 2.4f;
        } else if (placement.finalState) {
            color = IM_COL32(84, 196, 244, 240);
            thickness = 2.4f;
        } else if (placement.sourceIndex == selectedIndex) {
            color = IM_COL32(255, 208, 92, 235);
            thickness = 2.2f;
        }
        drawList->AddLine(ImVec2(lineLeft, placement.actualY),
                          ImVec2(lineRight, placement.actualY),
                          color,
                          thickness);
        anchors.emplace_back((lineLeft + lineRight) * 0.5f, placement.actualY);
        anchorIndices.push_back(placement.sourceIndex);
    }

    for (const auto& labelPlacement : labelPlacements) {
        std::string label = "n=" + std::to_string(labelPlacement.level->n);
        if (labelPlacement.initialState) {
            label += "（初）";
        } else if (labelPlacement.finalState) {
            label += "（末）";
        }
        const ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
        const ImVec2 labelMin(canvas.inner.Max.x - textSize.x - 12.0f, labelPlacement.labelY - textSize.y * 0.5f - 2.0f);
        const ImVec2 labelMax(canvas.inner.Max.x - 6.0f, labelPlacement.labelY + textSize.y * 0.5f + 2.0f);
        drawList->AddLine(ImVec2(lineRight + 4.0f, labelPlacement.actualY),
                          ImVec2(labelMin.x - 4.0f, labelPlacement.labelY),
                          IM_COL32(110, 120, 132, 180),
                          1.0f);
        drawList->AddRectFilled(labelMin, labelMax, IM_COL32(22, 28, 36, 220), 3.0f);
        drawList->AddText(ImVec2(labelMin.x + 4.0f, labelPlacement.labelY - textSize.y * 0.5f),
                          IM_COL32(220, 226, 233, 255),
                          label.c_str());
    }

    std::vector<ImVec2> correctedAnchors;
    std::vector<int> correctedIndices;
    std::vector<bool> correctedInitialFlags;
    if (correction.applied) {
        const auto drawCorrectedLevel = [&](int levelN, double correctedEnergyEv, ImU32 color, float centerT, bool isInitial) {
            auto it = std::find_if(placements.begin(), placements.end(), [levelN](const LabelPlacement& placement) {
                return placement.level->n == levelN;
            });
            if (it == placements.end()) {
                return;
            }
            const float correctedY =
                toPlotPoint(canvas.inner, 0.5, correctedEnergyEv, range.minX, range.maxX, range.minY, range.maxY).y;
            const float centerX = canvas.inner.Min.x + static_cast<float>(centerT) * canvas.inner.GetWidth();
            const float halfWidth = 26.0f;
            drawList->AddLine(ImVec2(centerX, it->actualY), ImVec2(centerX, correctedY), IM_COL32(196, 204, 214, 160), 1.0f);
            constexpr float dash = 5.0f;
            constexpr float gap = 3.0f;
            float x = centerX - halfWidth;
            while (x < centerX + halfWidth) {
                const float nextX = std::min(centerX + halfWidth, x + dash);
                drawList->AddLine(ImVec2(x, correctedY), ImVec2(nextX, correctedY), color, 2.0f);
                x = nextX + gap;
            }
            const ImVec2 marker(centerX, correctedY);
            drawList->AddCircleFilled(marker, 4.0f, color);
            correctedAnchors.push_back(marker);
            correctedIndices.push_back(it->sourceIndex);
            correctedInitialFlags.push_back(isInitial);
        };
        drawCorrectedLevel(transition.initial.n, correction.initial.correctedEnergyEv, IM_COL32(255, 108, 196, 240), 0.26f, true);
        drawCorrectedLevel(transition.final.n, correction.final.correctedEnergyEv, IM_COL32(255, 108, 196, 240), 0.48f, false);
    }

    const bool mouseInside = canvas.hovered && pointInRect(canvas.inner, canvas.mousePosition);
    const int hoveredIndex = mouseInside ? nearestPointIndex(anchors, canvas.mousePosition, 10.0f) : -1;
    const int hoveredCorrectedIndex =
        mouseInside ? nearestPointIndex(correctedAnchors, canvas.mousePosition, 10.0f) : -1;
    if (mouseInside && hoveredCorrectedIndex >= 0 && static_cast<std::size_t>(hoveredCorrectedIndex) < correctedAnchors.size()) {
        drawHighlightedPoint(drawList,
                             correctedAnchors[static_cast<std::size_t>(hoveredCorrectedIndex)],
                             IM_COL32(255, 108, 196, 255));
        ImGui::BeginTooltip();
        const int sourceIndex = correctedIndices[static_cast<std::size_t>(hoveredCorrectedIndex)];
        const auto& level = levels[static_cast<std::size_t>(sourceIndex)];
        const bool isInitial = correctedInitialFlags[static_cast<std::size_t>(hoveredCorrectedIndex)];
        const auto& stateCorrection = isInitial ? correction.initial : correction.final;
        ImGui::Text("%s修正后子能级", isInitial ? "初态" : "末态");
        ImGui::Text("n=%d, l=%d, j=%.1f, m_j=%.1f",
                    isInitial ? transition.initial.n : transition.final.n,
                    isInitial ? transition.initial.l : transition.final.l,
                    stateCorrection.j,
                    stateCorrection.mJ);
        ImGui::Text("基线能量: %s eV", formatPlotValue(level.energyEv).c_str());
        ImGui::Text("修正后能量: %s eV", formatPlotValue(stateCorrection.correctedEnergyEv).c_str());
        ImGui::Text("总修正: %s eV", formatPlotValue(stateCorrection.totalShiftEv, 6).c_str());
        ImGui::EndTooltip();
    } else if (mouseInside && hoveredIndex >= 0 && static_cast<std::size_t>(hoveredIndex) < anchorIndices.size()) {
        drawHighlightedPoint(drawList, anchors[static_cast<std::size_t>(hoveredIndex)], IM_COL32(164, 194, 244, 255));
        const auto& level = levels[static_cast<std::size_t>(anchorIndices[static_cast<std::size_t>(hoveredIndex)])];
        ImGui::BeginTooltip();
        ImGui::Text("能级 n=%d", level.n);
        ImGui::Text("基线能量: %s eV", formatPlotValue(level.energyEv).c_str());
        if (correction.applied && (level.n == transition.initial.n || level.n == transition.final.n)) {
            const bool isInitial = level.n == transition.initial.n;
            const auto& stateCorrection = isInitial ? correction.initial : correction.final;
            ImGui::Separator();
            ImGui::Text("%s修正后: %s eV", isInitial ? "初态" : "末态", formatPlotValue(stateCorrection.correctedEnergyEv).c_str());
            ImGui::Text("总修正量: %s eV", formatPlotValue(stateCorrection.totalShiftEv, 6).c_str());
        }
        ImGui::EndTooltip();
    }
    if (canvas.clicked) {
        if (hoveredCorrectedIndex >= 0 && static_cast<std::size_t>(hoveredCorrectedIndex) < correctedIndices.size()) {
            selectedIndex = correctedIndices[static_cast<std::size_t>(hoveredCorrectedIndex)];
        } else if (hoveredIndex >= 0 && static_cast<std::size_t>(hoveredIndex) < anchorIndices.size()) {
            selectedIndex = anchorIndices[static_cast<std::size_t>(hoveredIndex)];
        }
    }
    if (selectedIndex >= 0) {
        const auto it = std::find(anchorIndices.begin(), anchorIndices.end(), selectedIndex);
        if (it != anchorIndices.end()) {
            const std::size_t anchorIndex = static_cast<std::size_t>(std::distance(anchorIndices.begin(), it));
            if (anchorIndex < anchors.size()) {
                drawHighlightedPoint(drawList, anchors[anchorIndex], IM_COL32(255, 208, 92, 255));
            }
        }
    }
}

} // namespace

UiFrameResult AppUi::draw(quantum::app::SimulationState& state,
                          const quantum::physics::ElementDatabase& elementDatabase,
                          const quantum::core::PerformanceSnapshot& performance,
                          const quantum::core::Logger& logger,
                          GLuint sceneTexture,
                          const glm::ivec2& framebufferSize) const {
    UiFrameResult result;
    result.sceneSize = glm::vec2(static_cast<float>(framebufferSize.x), static_cast<float>(framebufferSize.y));

    const ImGuiID dockspaceId = ImGui::GetID("MainDockSpace");
    ImGui::DockSpaceOverViewport(dockspaceId, ImGui::GetMainViewport(), ImGuiDockNodeFlags_None);
    ensureDefaultDockLayout(dockspaceId, layoutInitialized_);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    if (ImGui::Begin(kPanelScene, nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(12, 18, 24, 235));
        ImGui::BeginChild("##scene_toolbar", ImVec2(0.0f, 68.0f), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::SetCursorPos(ImVec2(10.0f, 8.0f));
        if (ImGui::Button("适配全部内容")) {
            result.fitSceneRequested = true;
            result.fitSceneMode = quantum::app::SceneFitMode::Full;
        }
        ImGui::SameLine();
        if (ImGui::Button("仅核")) {
            result.fitSceneRequested = true;
            result.fitSceneMode = quantum::app::SceneFitMode::Nucleus;
        }
        ImGui::SameLine();
        if (ImGui::Button("仅轨道")) {
            result.fitSceneRequested = true;
            result.fitSceneMode = quantum::app::SceneFitMode::Orbit;
        }
        ImGui::SameLine();
        if (ImGui::Button("仅云")) {
            result.fitSceneRequested = true;
            result.fitSceneMode = quantum::app::SceneFitMode::Cloud;
        }
        ImGui::SameLine();
        ImGui::TextDisabled("F 键快速适配全部");
        ImGui::SetCursorPos(ImVec2(10.0f, 40.0f));
        const double fitDurationMin = 0.10;
        const double fitDurationMax = 2.50;
        ImGui::SetNextItemWidth(170.0f);
        if (ImGui::SliderScalar("过渡时长 (s)", ImGuiDataType_Double, &state.view.fitTransitionSeconds, &fitDurationMin, &fitDurationMax, "%.2f")) {
            state.view.fitTransitionSeconds = std::clamp(state.view.fitTransitionSeconds, fitDurationMin, fitDurationMax);
        }
        ImGui::SameLine();
        ImGui::Checkbox("锁定当前观察目标", &state.view.fitLockTarget);
        ImGui::EndChild();
        ImGui::PopStyleColor();

        const ImVec2 avail = ImGui::GetContentRegionAvail();
        const ImVec2 canvasSize(std::max(avail.x, 1.0f), std::max(avail.y, 1.0f));
        result.sceneSize = {canvasSize.x, canvasSize.y};
        result.sceneFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

        ImGui::InvisibleButton("##scene_canvas", canvasSize);
        const ImRect canvasRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
        result.sceneHovered = ImGui::IsItemHovered();
        result.sceneActive = ImGui::IsItemActive();

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(canvasRect.Min, canvasRect.Max, IM_COL32(8, 12, 16, 255), 6.0f);
        drawList->AddRect(canvasRect.Min, canvasRect.Max, IM_COL32(38, 48, 58, 255), 6.0f);

        if (sceneTexture != 0) {
            drawList->PushClipRect(canvasRect.Min, canvasRect.Max, true);
            drawList->AddRectFilled(canvasRect.Min, canvasRect.Max, IM_COL32(14, 20, 28, 255), 4.0f);
            drawList->AddImage(reinterpret_cast<void*>(static_cast<intptr_t>(sceneTexture)),
                               canvasRect.Min,
                               canvasRect.Max,
                               ImVec2(0.0f, 1.0f),
                               ImVec2(1.0f, 0.0f));
            drawList->PopClipRect();
        } else {
            const char* message = "场景帧缓冲尚未准备完成。";
            const ImVec2 textSize = ImGui::CalcTextSize(message);
            drawList->AddText(ImVec2(canvasRect.Min.x + (canvasSize.x - textSize.x) * 0.5f,
                                     canvasRect.Min.y + (canvasSize.y - textSize.y) * 0.5f),
                              IM_COL32(188, 198, 210, 255),
                              message);
        }
        drawSceneStatusOverlay(canvasRect, state, performance);
    }
    ImGui::End();
    ImGui::PopStyleVar();

    if (ImGui::Begin(kPanelInspector)) {
        drawScenarioButtons(state);
        ImGui::Separator();

        const auto& elements = elementDatabase.elements();
        int currentIndex = 0;
        for (std::size_t index = 0; index < elements.size(); ++index) {
            if (elements[index].atomicNumber == state.atomicNumber) {
                currentIndex = static_cast<int>(index);
                break;
            }
        }
        std::string currentElementLabel = "未加载";
        if (!elements.empty()) {
            currentElementLabel = elements[currentIndex].symbol + std::string(" · ") + localizedElementName(elements[currentIndex]);
        }
        if (ImGui::BeginCombo("元素", currentElementLabel.c_str())) {
            for (std::size_t index = 0; index < elements.size(); ++index) {
                const bool selected = static_cast<int>(index) == currentIndex;
                const std::string itemLabel = elements[index].symbol + std::string(" · ") + localizedElementName(elements[index]);
                if (ImGui::Selectable(itemLabel.c_str(), selected)) {
                    state.atomicNumber = elements[index].atomicNumber;
                    markAllDirty(state);
                }
            }
            ImGui::EndCombo();
        }

        if (ImGui::SliderInt("电荷态", &state.chargeState, 0, std::max(0, state.atomicNumber - 1))) {
            markAllDirty(state);
        }

        if (ImGui::BeginCombo("模型", modelLabel(state.modelKind))) {
            for (const auto kind : {quantum::app::ModelKind::Bohr,
                                    quantum::app::ModelKind::WaveParticle,
                                    quantum::app::ModelKind::SchrodingerCloud,
                                    quantum::app::ModelKind::Compare}) {
                const bool selected = kind == state.modelKind;
                if (ImGui::Selectable(modelLabel(kind), selected)) {
                    applyModelPreset(state, kind);
                }
            }
            ImGui::EndCombo();
        }

        if (ImGui::BeginCombo("近似模式", approximationLabel(state.approximationMode))) {
            for (const auto mode : {quantum::physics::ApproximationMode::StrictHydrogenic,
                                    quantum::physics::ApproximationMode::MultiElectronApproximation}) {
                const bool selected = mode == state.approximationMode;
                if (ImGui::Selectable(approximationLabel(mode), selected)) {
                    state.approximationMode = mode;
                    markAllDirty(state);
                }
            }
            ImGui::EndCombo();
        }

        ImGui::SeparatorText("玻尔模型");
        if (ImGui::SliderInt("主量子数 n", &state.bohr.principalN, 1, 8)) {
            state.dirty.physics = true;
        }
        if (ImGui::SliderInt("谱线最大 n", &state.bohr.maxSpectrumN, 2, 10)) {
            state.dirty.physics = true;
        }
        if (ImGui::Checkbox("启用约化质量", &state.bohr.useReducedMass)) {
            markAllDirty(state);
        }

        ImGui::SeparatorText("跃迁");
        if (ImGui::SliderInt("初态 n_i", &state.transition.initial.n, 1, 10)) {
            state.dirty.physics = true;
        }
        state.transition.initial.l = std::clamp(state.transition.initial.l, 0, std::max(0, state.transition.initial.n - 1));
        if (ImGui::SliderInt("初态 l_i", &state.transition.initial.l, 0, std::max(0, state.transition.initial.n - 1))) {
            state.dirty.physics = true;
        }
        state.transition.initial.m = std::clamp(state.transition.initial.m, -state.transition.initial.l, state.transition.initial.l);
        if (ImGui::SliderInt("初态 m_i", &state.transition.initial.m, -state.transition.initial.l, state.transition.initial.l)) {
            state.dirty.physics = true;
        }
        if (ImGui::SliderInt("末态 n_f", &state.transition.final.n, 1, std::max(1, state.transition.initial.n - 1))) {
            state.dirty.physics = true;
        }
        state.transition.final.l = std::clamp(state.transition.final.l, 0, std::max(0, state.transition.final.n - 1));
        if (ImGui::SliderInt("末态 l_f", &state.transition.final.l, 0, std::max(0, state.transition.final.n - 1))) {
            state.dirty.physics = true;
        }
        state.transition.final.m = std::clamp(state.transition.final.m, -state.transition.final.l, state.transition.final.l);
        if (ImGui::SliderInt("末态 m_f", &state.transition.final.m, -state.transition.final.l, state.transition.final.l)) {
            state.dirty.physics = true;
        }
        if (ImGui::Checkbox("电偶极选择规则", &state.transition.enforceRules)) {
            state.dirty.physics = true;
        }
        if (ImGui::Checkbox("限制 Delta m", &state.transition.enforceDeltaM)) {
            state.dirty.physics = true;
        }

        ImGui::SeparatorText("概率云");
        if (ImGui::SliderInt("点数", &state.cloud.pointCount, 10000, 200000)) {
            state.dirty.cloud = true;
        }
        state.cloud.mediumLodPointCount = std::clamp(state.cloud.mediumLodPointCount, 1000, state.cloud.pointCount);
        state.cloud.farLodPointCount = std::clamp(state.cloud.farLodPointCount, 1000, state.cloud.pointCount);
        state.cloud.interactionPointCount = std::clamp(state.cloud.interactionPointCount, 1000, state.cloud.pointCount);
        state.cloud.farLodDistance = std::max(state.cloud.farLodDistance, state.cloud.mediumLodDistance + 1.0f);
        ImGui::Checkbox("启用细节层级", &state.cloud.enableLod);
        ImGui::Checkbox("交互时降级", &state.cloud.reduceDuringInteraction);
        if (state.cloud.enableLod) {
            ImGui::SliderInt("中距离点数", &state.cloud.mediumLodPointCount, 1000, state.cloud.pointCount);
            ImGui::SliderInt("远距离点数", &state.cloud.farLodPointCount, 1000, state.cloud.pointCount);
            ImGui::SliderInt("拖拽点数", &state.cloud.interactionPointCount, 1000, state.cloud.pointCount);
            ImGui::SliderFloat("中距离阈值", &state.cloud.mediumLodDistance, 6.0f, 40.0f);
            ImGui::SliderFloat("远距离阈值", &state.cloud.farLodDistance, 8.0f, 80.0f);
        }
        if (ImGui::SliderInt("体渲染分辨率", &state.cloud.volumeResolution, 16, 96)) {
            state.dirty.cloud = true;
        }
        const double extentMin = 6.0;
        const double extentMax = 30.0;
        if (ImGui::SliderScalar("显示范围倍率", ImGuiDataType_Double, &state.cloud.extentScale, &extentMin, &extentMax)) {
            state.dirty.cloud = true;
        }
        if (ImGui::Checkbox("构建体渲染", &state.cloud.buildVolume)) {
            state.dirty.cloud = true;
        }
        if (ImGui::Checkbox("使用数值径向解", &state.cloud.useNumericalRadial)) {
            state.dirty.cloud = true;
        }
        if (ImGui::Checkbox("自适应采样", &state.cloud.adaptiveSampling)) {
            state.dirty.cloud = true;
        }
        if (ImGui::BeginCombo("采样质量", samplingQualityLabel(state.cloud.samplingQuality))) {
            for (const auto preset : {quantum::app::SamplingQualityPreset::Fast,
                                      quantum::app::SamplingQualityPreset::Balanced,
                                      quantum::app::SamplingQualityPreset::HighFidelity}) {
                if (ImGui::Selectable(samplingQualityLabel(preset), preset == state.cloud.samplingQuality)) {
                    state.cloud.samplingQuality = preset;
                    state.dirty.cloud = true;
                }
            }
            ImGui::EndCombo();
        }

        if (ImGui::BeginCombo("显示模式", cloudModeLabel(state.view.cloudRenderMode))) {
            for (const auto mode : {quantum::app::CloudRenderMode::PointCloud,
                                    quantum::app::CloudRenderMode::VolumeSlices,
                                    quantum::app::CloudRenderMode::Hybrid}) {
                if (ImGui::Selectable(cloudModeLabel(mode), mode == state.view.cloudRenderMode)) {
                    state.view.cloudRenderMode = mode;
                }
            }
            ImGui::EndCombo();
        }
        ImGui::Checkbox("显示轨道环", &state.view.showOrbitRings);
        ImGui::Checkbox("显示网格", &state.view.showGrid);
        ImGui::SliderFloat("点大小", &state.view.pointSize, 1.0f, 12.0f);
        ImGui::SliderFloat("曝光度", &state.view.exposure, 0.1f, 4.0f);

        ImGui::SeparatorText("量子态叠加");
        for (std::size_t index = 0; index < state.cloud.components.size(); ++index) {
            auto& component = state.cloud.components[index];
            ImGui::PushID(static_cast<int>(index));
            ImGui::Text("分量 %d", static_cast<int>(index + 1));
            if (ImGui::SliderInt("n", &component.qn.n, 1, 8)) {
                state.dirty.cloud = true;
            }
            component.qn.l = std::clamp(component.qn.l, 0, std::max(0, component.qn.n - 1));
            if (ImGui::SliderInt("l", &component.qn.l, 0, std::max(0, component.qn.n - 1))) {
                state.dirty.cloud = true;
            }
            component.qn.m = std::clamp(component.qn.m, -component.qn.l, component.qn.l);
            if (ImGui::SliderInt("m", &component.qn.m, -component.qn.l, component.qn.l)) {
                state.dirty.cloud = true;
            }
            const double amplitudeMin = 0.0;
            const double amplitudeMax = 1.5;
            if (ImGui::SliderScalar("幅值", ImGuiDataType_Double, &component.amplitude, &amplitudeMin, &amplitudeMax)) {
                state.dirty.cloud = true;
            }
            const double phaseMin = -3.1415926535;
            const double phaseMax = 3.1415926535;
            if (ImGui::SliderScalar("相位", ImGuiDataType_Double, &component.phaseRadians, &phaseMin, &phaseMax)) {
                state.dirty.cloud = true;
            }
            if (ImGui::Button("移除") && state.cloud.components.size() > 1) {
                state.cloud.components.erase(state.cloud.components.begin() + static_cast<std::ptrdiff_t>(index));
                state.dirty.cloud = true;
                ImGui::PopID();
                break;
            }
            ImGui::Separator();
            ImGui::PopID();
        }
        if (state.cloud.components.size() < 3 && ImGui::Button("添加分量")) {
            state.cloud.components.push_back({{2, 1, 0}, 0.5, 0.0, state.derived.activeZeff});
            state.dirty.cloud = true;
        }

        ImGui::SeparatorText("数值求解器");
        if (ImGui::SliderInt("求解 n", &state.solver.qn.n, 1, 8)) {
            state.dirty.solver = true;
        }
        state.solver.qn.l = std::clamp(state.solver.qn.l, 0, std::max(0, state.solver.qn.n - 1));
        if (ImGui::SliderInt("求解 l", &state.solver.qn.l, 0, std::max(0, state.solver.qn.n - 1))) {
            state.dirty.solver = true;
        }
        if (ImGui::SliderInt("网格点数", &state.solver.gridPoints, 512, 16384)) {
            state.dirty.solver = true;
        }
        if (ImGui::SliderInt("收敛轮数", &state.solver.convergencePasses, 2, 6)) {
            state.dirty.solver = true;
        }
        const double radiusMin = 40.0;
        const double radiusMax = 200.0;
        if (ImGui::SliderScalar("最大缩放半径", ImGuiDataType_Double, &state.solver.maxScaledRadius, &radiusMin, &radiusMax)) {
            state.dirty.solver = true;
        }
        ImGui::SeparatorText("第一层：中心场");
        if (ImGui::Checkbox("启用屏蔽中心场", &state.solver.useScreenedCentralField)) {
            state.dirty.solver = true;
        }
        if (state.solver.useScreenedCentralField) {
            if (ImGui::Checkbox("自动残余电荷", &state.solver.autoResidualCharge)) {
                state.dirty.solver = true;
            }
            if (!state.solver.autoResidualCharge) {
                const double residualMin = 0.1;
                const double residualMax = static_cast<double>(std::max(1, state.atomicNumber));
                if (ImGui::SliderScalar("残余电荷", ImGuiDataType_Double, &state.solver.residualCharge, &residualMin, &residualMax)) {
                    state.dirty.solver = true;
                }
            }
            const double screeningMin = 0.2;
            const double screeningMax = 8.0;
            if (ImGui::SliderScalar("屏蔽长度 (a0)", ImGuiDataType_Double, &state.solver.screeningLengthBohr, &screeningMin, &screeningMax)) {
                state.dirty.solver = true;
            }
        }

        ImGui::SeparatorText("第三层：谱学修正");
        if (ImGui::Checkbox("精细结构修正", &state.spectroscopy.applyFineStructure)) {
            state.dirty.physics = true;
        }
        if (ImGui::Checkbox("Zeeman 修正", &state.spectroscopy.applyZeeman)) {
            state.dirty.physics = true;
        }
        if (state.spectroscopy.applyZeeman) {
            const double fieldMin = 0.0;
            const double fieldMax = 12.0;
            if (ImGui::SliderScalar("磁场 B (T)", ImGuiDataType_Double, &state.spectroscopy.magneticFieldTesla, &fieldMin, &fieldMax)) {
                state.dirty.physics = true;
            }
            if (ImGui::Checkbox("初态取 j=l-1/2 支", &state.spectroscopy.initialBranch.lowerJBranch)) {
                state.dirty.physics = true;
            }
            if (ImGui::Checkbox("末态取 j=l-1/2 支", &state.spectroscopy.finalBranch.lowerJBranch)) {
                state.dirty.physics = true;
            }
        }
        if (ImGui::Checkbox("Stark 修正", &state.spectroscopy.applyStark)) {
            state.dirty.physics = true;
        }
        if (state.spectroscopy.applyStark) {
            const double electricMin = 0.0;
            const double electricMax = 5.0e7;
            if (ImGui::SliderScalar("电场 E (V/m)", ImGuiDataType_Double, &state.spectroscopy.electricFieldVPerM, &electricMin, &electricMax)) {
                state.dirty.physics = true;
            }
        }
    }
    ImGui::End();

    if (ImGui::Begin(kPanelPhysics)) {
        ImGui::Text("元素: %s (%s)", localizedElementName(state.derived.element).c_str(), state.derived.element.symbol.c_str());
        ImGui::Text("Z = %d, 电荷态 = %d, 电子数 = %d", state.atomicNumber, state.chargeState, state.derived.electronCount);
        ImGui::Text("电子组态: %s", state.derived.configuration.notation.c_str());
        ImGui::Text("当前 Z_eff = %.4f", state.derived.activeZeff);
        ImGui::Text("屏蔽常数 sigma = %.4f", state.derived.slater.shieldingSigma);
        ImGui::Separator();
        ImGui::Text("E_n = %.6f eV", state.derived.bohrMetrics.energyEv);
        ImGui::Text("r_n = %.6f nm", state.derived.bohrMetrics.orbitalRadiusNm);
        ImGui::Text("v_n = %.6e m/s", state.derived.bohrMetrics.orbitalSpeedMps);
        ImGui::Text("约化质量系数 = %.8f", state.derived.bohrMetrics.reducedMassFactor);
        ImGui::Separator();
        ImGui::Text("Delta E = %.6f eV", state.derived.transition.deltaEnergyEv);
        ImGui::Text("f = %.6e Hz", state.derived.transition.frequencyHz);
        ImGui::Text("lambda = %.6f nm", state.derived.transition.wavelengthNm);
        ImGui::Text("谱系 = %s", state.derived.transition.seriesName.c_str());
        ImGui::Text("是否允许 = %s", yesNoLabel(state.derived.transition.allowed));
        if (!state.derived.transition.reason.empty()) {
            ImGui::TextWrapped("规则说明: %s", state.derived.transition.reason.c_str());
        }
        ImGui::Separator();
        ImGui::Text("归一化估计 = %.6f", state.derived.cloud.stats.normalizationEstimate);
        ImGui::Text("归一化误差 = %.6f", state.derived.cloud.stats.normalizationError);
        ImGui::Text("采样效率 = %d / %d", state.derived.cloud.stats.acceptedCount, state.derived.cloud.stats.candidateCount);
        ImGui::Text("候选倍率 = %.2f", state.derived.cloud.stats.candidateMultiplier);
        ImGui::Text("径向 CDF / 角向扫描 / 蒙特卡洛 = %d / %d / %d",
                    state.derived.cloud.stats.radialCdfSamples,
                    state.derived.cloud.stats.angularScanResolution,
                    state.derived.cloud.stats.monteCarloSamples);
        ImGui::TextWrapped("云中使用数值径向解 = %s（%d 个分量）",
                           yesNoLabel(state.derived.cloud.stats.usedNumericalRadial),
                           state.derived.cloud.stats.numericalComponentCount);
        ImGui::Separator();
        ImGui::Text("求解能量 = %.6f eV", state.derived.solver.energyEv);
        ImGui::Text("求解误差 = %.6e eV", state.derived.solver.errorEv);
        ImGui::TextWrapped("求解状态: %s", state.derived.solver.message.c_str());
        if (!state.derived.approximationWarning.empty()) {
            ImGui::Separator();
            ImGui::TextWrapped("%s", state.derived.approximationWarning.c_str());
        }
        ImGui::SeparatorText("高阶修正与中心场");
        ImGui::Text("中心场模式: %s", state.solver.useScreenedCentralField ? "第一层屏蔽中心场" : "氢样库仑势");
        if (!state.derived.centralField.samples.empty()) {
            ImGui::Text("近核/远场有效电荷: %.4f / %.4f",
                        state.derived.centralField.samples.front().effectiveCharge,
                        state.derived.centralField.samples.back().effectiveCharge);
        }
        if (state.solver.useScreenedCentralField) {
            ImGui::Text("屏蔽长度: %.3f a0", state.solver.screeningLengthBohr);
            if (!state.derived.centralField.description.empty()) {
                ImGui::TextWrapped("%s", state.derived.centralField.description.c_str());
            }
        }
        if (state.derived.spectroscopy.applied) {
            ImGui::Text("修正后 Delta E = %.6f eV", state.derived.spectroscopy.correctedDeltaEnergyEv);
            ImGui::Text("修正后 lambda = %.6f nm", state.derived.spectroscopy.correctedWavelengthNm);
            ImGui::Text("初态 j/m_j = %.1f / %.1f", state.derived.spectroscopy.initial.j, state.derived.spectroscopy.initial.mJ);
            ImGui::Text("末态 j/m_j = %.1f / %.1f", state.derived.spectroscopy.final.j, state.derived.spectroscopy.final.mJ);
            ImGui::TextWrapped("第三层说明: %s", state.derived.spectroscopy.notes.c_str());
        } else {
            ImGui::TextWrapped("第三层当前未启用。可在控制面板中开启精细结构、Zeeman 或 Stark 教学修正。");
        }
        drawMethodTags(state);
        if (ImGui::Button("导出验证报告")) {
            state.reporting.exportValidationReportRequested = true;
        }
        ImGui::SameLine();
        ImGui::TextWrapped("%s", state.reporting.exportStatus.c_str());
        drawValidationTable(state.derived.validationRecords);
    }
    ImGui::End();

    if (ImGui::Begin(kPanelPlots)) {
        drawEnergyLevelsPlot(state.derived.energyLevels, state.transition, state.derived.spectroscopy, energySelectionIndex_);
        drawSpectrumPlot(state.derived.spectrumLines,
                         state.derived.referenceLines,
                         state.transition,
                         state.derived.transition,
                         state.derived.spectroscopy,
                         state.transition.enforceRules,
                         spectrumSelectionIndex_,
                         spectrumSelectionReference_);

        std::vector<double> radii;
        std::vector<double> values;
        radii.reserve(state.derived.radialDistribution.size());
        values.reserve(state.derived.radialDistribution.size());
        for (const auto& sample : state.derived.radialDistribution) {
            radii.push_back(sample.first * 1.0e9);
            values.push_back(sample.second);
        }
        drawPolylinePlot("径向分布",
                         radii,
                         values,
                         radialSelectionIndex_,
                         IM_COL32(127, 196, 109, 230),
                         220.0f,
                         "半径 r (nm)",
                         "径向概率 P(r)");

        std::vector<double> centralRadii;
        std::vector<double> centralPotential;
        centralRadii.reserve(state.derived.centralField.samples.size());
        centralPotential.reserve(state.derived.centralField.samples.size());
        for (const auto& sample : state.derived.centralField.samples) {
            centralRadii.push_back(sample.radiusBohr);
            centralPotential.push_back(sample.potentialHartree);
        }
        drawPolylinePlot("中心场势 V(r)",
                         centralRadii,
                         centralPotential,
                         centralFieldSelectionIndex_,
                         IM_COL32(102, 187, 255, 230),
                         220.0f,
                         "半径 r (a0)",
                         "势能 (Hartree)");

        std::vector<double> grids;
        std::vector<double> errors;
        grids.reserve(state.derived.solver.convergence.size());
        errors.reserve(state.derived.solver.convergence.size());
        for (const auto& sample : state.derived.solver.convergence) {
            grids.push_back(sample.gridStepMeters * 1.0e12);
            errors.push_back(sample.errorEv);
        }
        drawPolylinePlot("求解收敛曲线（步长 pm）", grids, errors, convergenceSelectionIndex_, IM_COL32(255, 153, 102, 230), 220.0f, "步长 (pm)", "误差 (eV)");
        ImGui::SeparatorText("图表详情");
        const PlotViewState& energyView = plotViewState("能级图");
        const PlotViewState& spectrumView = plotViewState("谱线图");
        const PlotViewState& radialView = plotViewState("径向分布");
        const PlotViewState& centralFieldView = plotViewState("中心场势 V(r)");
        const PlotViewState& convergenceView = plotViewState("求解收敛曲线（步长 pm）");
        syncReportingPlotWindow(state.reporting.energyPlotWindow, energyView);
        syncReportingPlotWindow(state.reporting.spectrumPlotWindow, spectrumView);
        syncReportingPlotWindow(state.reporting.radialPlotWindow, radialView);
        syncReportingPlotWindow(state.reporting.centralFieldPlotWindow, centralFieldView);
        syncReportingPlotWindow(state.reporting.convergencePlotWindow, convergenceView);

        char csvPathBuffer[260];
        std::snprintf(csvPathBuffer, sizeof(csvPathBuffer), "%s", state.reporting.csvOutputPath.c_str());
        if (ImGui::InputText("当前窗口 CSV", csvPathBuffer, sizeof(csvPathBuffer))) {
            state.reporting.csvOutputPath = csvPathBuffer;
        }
        ImGui::Checkbox("文件名追加时间戳", &state.reporting.csvAppendTimestamp);
        ImGui::SameLine();
        ImGui::Checkbox("按图表拆分导出", &state.reporting.csvSplitByPlot);
        ImGui::SameLine();
        if (ImGui::Button("导出当前窗口 CSV")) {
            state.reporting.exportCurrentPlotCsvRequested = true;
        }
        ImGui::TextWrapped("%s", state.reporting.csvExportStatus.c_str());

        if (energySelectionIndex_ >= 0 && static_cast<std::size_t>(energySelectionIndex_) < state.derived.energyLevels.size()) {
            const auto& level = state.derived.energyLevels[static_cast<std::size_t>(energySelectionIndex_)];
            ImGui::Text("选中能级: n=%d, 基线能量=%.6f eV", level.n, level.energyEv);
            if (state.derived.spectroscopy.applied &&
                (level.n == state.transition.initial.n || level.n == state.transition.final.n)) {
                const bool isInitial = level.n == state.transition.initial.n;
                const auto& correctedState = isInitial ? state.derived.spectroscopy.initial : state.derived.spectroscopy.final;
                ImGui::Text("%s修正后: %.6f eV (j=%.1f, m_j=%.1f, Δ=%.6e eV)",
                            isInitial ? "初态" : "末态",
                            correctedState.correctedEnergyEv,
                            correctedState.j,
                            correctedState.mJ,
                            correctedState.totalShiftEv);
            }
        }
        if (radialSelectionIndex_ >= 0 && static_cast<std::size_t>(radialSelectionIndex_) < radii.size()) {
            ImGui::Text("选中径向点: r=%.6f nm, P(r)=%.6f",
                        radii[static_cast<std::size_t>(radialSelectionIndex_)],
                        values[static_cast<std::size_t>(radialSelectionIndex_)]);
        }
        if (convergenceSelectionIndex_ >= 0 && static_cast<std::size_t>(convergenceSelectionIndex_) < grids.size()) {
            ImGui::Text("选中收敛点: 步长=%.6f pm, 误差=%.6e eV",
                        grids[static_cast<std::size_t>(convergenceSelectionIndex_)],
                        errors[static_cast<std::size_t>(convergenceSelectionIndex_)]);
        }
        if (centralFieldSelectionIndex_ >= 0 &&
            static_cast<std::size_t>(centralFieldSelectionIndex_) < state.derived.centralField.samples.size()) {
            const auto& sample = state.derived.centralField.samples[static_cast<std::size_t>(centralFieldSelectionIndex_)];
            ImGui::Text("选中中心场点: r=%.4f a0, Z_eff(r)=%.4f, 势能=%.6f Hartree",
                        sample.radiusBohr,
                        sample.effectiveCharge,
                        sample.potentialHartree);
        }
        if (state.derived.spectroscopy.applied) {
            ImGui::Text("当前跃迁修正前/后: ΔE %.6f -> %.6f eV, λ %.6f -> %.6f nm",
                        state.derived.transition.deltaEnergyEv,
                        state.derived.spectroscopy.correctedDeltaEnergyEv,
                        state.derived.transition.wavelengthNm,
                        state.derived.spectroscopy.correctedWavelengthNm);
        }
        const auto& selectedSpectrumSource = spectrumSelectionReference_ ? state.derived.referenceLines : state.derived.spectrumLines;
        if (spectrumSelectionIndex_ >= 0 && static_cast<std::size_t>(spectrumSelectionIndex_) < selectedSpectrumSource.size()) {
            const auto& line = selectedSpectrumSource[static_cast<std::size_t>(spectrumSelectionIndex_)];
            ImGui::Text("选中谱线: %s, λ=%.6f nm, E=%.6f eV", line.seriesName.c_str(), line.wavelengthNm, line.photonEnergyEv);
        }

        ImGui::SeparatorText("局部数据联动");
        if (ImGui::CollapsingHeader("能级图窗口", ImGuiTreeNodeFlags_DefaultOpen)) {
            drawPlotRangeSummary("能级图", energyView, nullptr, "能量 (eV)");
            int visibleCount = 0;
            for (const auto& level : state.derived.energyLevels) {
                if (valueWithin(level.energyEv, energyView.range.minY, energyView.range.maxY)) {
                    ++visibleCount;
                }
            }
            ImGui::Text("当前窗口能级数: %d / %zu", visibleCount, state.derived.energyLevels.size());
            if (visibleCount > 0 &&
                ImGui::BeginTable("##energy_window_table",
                                  4,
                                  ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp |
                                      ImGuiTableFlags_ScrollY,
                                  ImVec2(0.0f, 150.0f))) {
                ImGui::TableSetupColumn("能级");
                ImGui::TableSetupColumn("基线能量 (eV)");
                ImGui::TableSetupColumn("修正后 (eV)");
                ImGui::TableSetupColumn("状态");
                ImGui::TableHeadersRow();
                for (std::size_t index = 0; index < state.derived.energyLevels.size(); ++index) {
                    const auto& level = state.derived.energyLevels[index];
                    if (!valueWithin(level.energyEv, energyView.range.minY, energyView.range.maxY)) {
                        continue;
                    }
                    const bool isSelected = static_cast<int>(index) == energySelectionIndex_;
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    std::string selectableLabel = "n=" + std::to_string(level.n) + "##energy_window_" + std::to_string(index);
                    if (ImGui::Selectable(selectableLabel.c_str(), isSelected)) {
                        energySelectionIndex_ = static_cast<int>(index);
                    }
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text("%.6f", level.energyEv);
                    ImGui::TableSetColumnIndex(2);
                    if (state.derived.spectroscopy.applied &&
                        (level.n == state.transition.initial.n || level.n == state.transition.final.n)) {
                        const bool isInitial = level.n == state.transition.initial.n;
                        const auto& correctedState = isInitial ? state.derived.spectroscopy.initial : state.derived.spectroscopy.final;
                        ImGui::Text("%.6f", correctedState.correctedEnergyEv);
                    } else {
                        ImGui::TextUnformatted("—");
                    }
                    ImGui::TableSetColumnIndex(3);
                    if (level.n == state.transition.initial.n) {
                        ImGui::TextUnformatted("初态");
                    } else if (level.n == state.transition.final.n) {
                        ImGui::TextUnformatted("末态");
                    } else {
                        ImGui::TextUnformatted("可见");
                    }
                }
                ImGui::EndTable();
            }
        }

        if (ImGui::CollapsingHeader("谱线图窗口", ImGuiTreeNodeFlags_DefaultOpen)) {
            drawPlotRangeSummary("谱线图", spectrumView, "波长 (nm)", "能量 (eV)");
            int visibleCount = 0;
            for (std::size_t index = 0; index < state.derived.spectrumLines.size(); ++index) {
                const auto& line = state.derived.spectrumLines[index];
                if (state.transition.enforceRules && !line.allowed) {
                    continue;
                }
                if (pointWithinPlotRange(spectrumView.range, line.wavelengthNm, line.photonEnergyEv)) {
                    ++visibleCount;
                }
            }
            for (const auto& line : state.derived.referenceLines) {
                if (pointWithinPlotRange(spectrumView.range, line.wavelengthNm, line.photonEnergyEv)) {
                    ++visibleCount;
                }
            }
            ImGui::Text("当前窗口谱线数: %d", visibleCount);
            if (visibleCount > 0 &&
                ImGui::BeginTable("##spectrum_window_table",
                                  5,
                                  ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp |
                                      ImGuiTableFlags_ScrollY,
                                  ImVec2(0.0f, 180.0f))) {
                ImGui::TableSetupColumn("来源");
                ImGui::TableSetupColumn("系列");
                ImGui::TableSetupColumn("跃迁");
                ImGui::TableSetupColumn("波长 (nm)");
                ImGui::TableSetupColumn("能量 (eV)");
                ImGui::TableHeadersRow();

                for (std::size_t index = 0; index < state.derived.spectrumLines.size(); ++index) {
                    const auto& line = state.derived.spectrumLines[index];
                    if (state.transition.enforceRules && !line.allowed) {
                        continue;
                    }
                    if (!pointWithinPlotRange(spectrumView.range, line.wavelengthNm, line.photonEnergyEv)) {
                        continue;
                    }
                    const bool isSelected = !spectrumSelectionReference_ && static_cast<int>(index) == spectrumSelectionIndex_;
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    if (ImGui::Selectable(("计算##spectrum_calc_" + std::to_string(index)).c_str(), isSelected)) {
                        spectrumSelectionReference_ = false;
                        spectrumSelectionIndex_ = static_cast<int>(index);
                    }
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted(line.seriesName.c_str());
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("(%d,%d,%d)->(%d,%d,%d)",
                                line.initial.n,
                                line.initial.l,
                                line.initial.m,
                                line.final.n,
                                line.final.l,
                                line.final.m);
                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%.6f", line.wavelengthNm);
                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text("%.6f", line.photonEnergyEv);
                }
                for (std::size_t index = 0; index < state.derived.referenceLines.size(); ++index) {
                    const auto& line = state.derived.referenceLines[index];
                    if (!pointWithinPlotRange(spectrumView.range, line.wavelengthNm, line.photonEnergyEv)) {
                        continue;
                    }
                    const bool isSelected = spectrumSelectionReference_ && static_cast<int>(index) == spectrumSelectionIndex_;
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    if (ImGui::Selectable(("参考##spectrum_ref_" + std::to_string(index)).c_str(), isSelected)) {
                        spectrumSelectionReference_ = true;
                        spectrumSelectionIndex_ = static_cast<int>(index);
                    }
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted(line.seriesName.c_str());
                    ImGui::TableSetColumnIndex(2);
                    ImGui::Text("(%d,%d,%d)->(%d,%d,%d)",
                                line.initial.n,
                                line.initial.l,
                                line.initial.m,
                                line.final.n,
                                line.final.l,
                                line.final.m);
                    ImGui::TableSetColumnIndex(3);
                    ImGui::Text("%.6f", line.wavelengthNm);
                    ImGui::TableSetColumnIndex(4);
                    ImGui::Text("%.6f", line.photonEnergyEv);
                }
                ImGui::EndTable();
            }
        }

        if (ImGui::CollapsingHeader("曲线窗口")) {
            drawPlotRangeSummary("径向分布", radialView, "半径 r (nm)", "径向概率 P(r)");
            int radialVisibleCount = 0;
            for (std::size_t index = 0; index < radii.size(); ++index) {
                if (pointWithinPlotRange(radialView.range, radii[index], values[index])) {
                    ++radialVisibleCount;
                }
            }
            ImGui::Text("径向分布可见点: %d / %zu", radialVisibleCount, radii.size());

            drawPlotRangeSummary("中心场势", centralFieldView, "半径 r (a0)", "势能 (Hartree)");
            int centralVisibleCount = 0;
            for (const auto& sample : state.derived.centralField.samples) {
                if (pointWithinPlotRange(centralFieldView.range, sample.radiusBohr, sample.potentialHartree)) {
                    ++centralVisibleCount;
                }
            }
            ImGui::Text("中心场可见点: %d / %zu", centralVisibleCount, state.derived.centralField.samples.size());

            drawPlotRangeSummary("收敛曲线", convergenceView, "步长 (pm)", "误差 (eV)");
            int convergenceVisibleCount = 0;
            for (std::size_t index = 0; index < grids.size(); ++index) {
                if (pointWithinPlotRange(convergenceView.range, grids[index], errors[index])) {
                    ++convergenceVisibleCount;
                }
            }
            ImGui::Text("收敛曲线可见点: %d / %zu", convergenceVisibleCount, grids.size());
        }

        std::vector<std::size_t> visibleEnergyIndices;
        std::vector<std::size_t> visibleSpectrumIndices;
        std::vector<std::size_t> visibleConvergenceIndices;
        visibleEnergyIndices.reserve(state.derived.energyLevels.size());
        visibleSpectrumIndices.reserve(state.derived.spectrumLines.size());
        visibleConvergenceIndices.reserve(state.derived.solver.convergence.size());
        for (std::size_t index = 0; index < state.derived.energyLevels.size(); ++index) {
            if (valueWithin(state.derived.energyLevels[index].energyEv, energyView.range.minY, energyView.range.maxY)) {
                visibleEnergyIndices.push_back(index);
            }
        }
        for (std::size_t index = 0; index < state.derived.spectrumLines.size(); ++index) {
            const auto& line = state.derived.spectrumLines[index];
            if (state.transition.enforceRules && !line.allowed) {
                continue;
            }
            if (pointWithinPlotRange(spectrumView.range, line.wavelengthNm, line.photonEnergyEv)) {
                visibleSpectrumIndices.push_back(index);
            }
        }
        for (std::size_t index = 0; index < state.derived.solver.convergence.size(); ++index) {
            if (pointWithinPlotRange(convergenceView.range, grids[index], errors[index])) {
                visibleConvergenceIndices.push_back(index);
            }
        }

        if (ImGui::BeginTable("##plot_summary_table", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("能级");
            ImGui::TableSetupColumn("谱线");
            ImGui::TableSetupColumn("谱学修正");
            ImGui::TableSetupColumn("收敛");
            ImGui::TableHeadersRow();
            const std::size_t rowCount = std::max<std::size_t>(
                4,
                std::max({visibleEnergyIndices.size(), visibleSpectrumIndices.size(), visibleConvergenceIndices.size(), static_cast<std::size_t>(2)}));
            for (std::size_t row = 0; row < rowCount; ++row) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (row < visibleEnergyIndices.size()) {
                    const auto& level = state.derived.energyLevels[visibleEnergyIndices[row]];
                    ImGui::Text("n=%d | %.4f eV", level.n, level.energyEv);
                }
                ImGui::TableNextColumn();
                if (row < visibleSpectrumIndices.size()) {
                    const auto& line = state.derived.spectrumLines[visibleSpectrumIndices[row]];
                    ImGui::Text("%s | %.3f nm", line.seriesName.c_str(), line.wavelengthNm);
                }
                ImGui::TableNextColumn();
                if (state.derived.spectroscopy.applied) {
                    if (row == 0) {
                        ImGui::Text("基线 λ=%.4f nm", state.derived.transition.wavelengthNm);
                    } else if (row == 1) {
                        ImGui::Text("修正后 λ=%.4f nm", state.derived.spectroscopy.correctedWavelengthNm);
                    } else if (row == 2) {
                        ImGui::Text("Δλ=%.6f nm",
                                    state.derived.spectroscopy.correctedWavelengthNm - state.derived.transition.wavelengthNm);
                    }
                } else if (row == 0) {
                    ImGui::TextUnformatted("未启用");
                }
                ImGui::TableNextColumn();
                if (row < visibleConvergenceIndices.size()) {
                    const auto& sample = state.derived.solver.convergence[visibleConvergenceIndices[row]];
                    ImGui::Text("%.3f pm | %.3e eV", sample.gridStepMeters * 1.0e12, sample.errorEv);
                }
            }
            ImGui::EndTable();
        }
    }
    ImGui::End();

    if (ImGui::Begin(kPanelPerformance)) {
        ImGui::Text("帧率 FPS: %.2f", performance.fps);
        ImGui::Text("平均帧时间: %.3f ms", performance.averageFrameMs);
        ImGui::Text("CPU 用时: %.3f ms", performance.cpuFrameMs);
        if (!performance.frameHistoryMs.empty()) {
            ImGui::PlotLines("帧时间历史 (ms)",
                             performance.frameHistoryMs.data(),
                             static_cast<int>(performance.frameHistoryMs.size()),
                             0,
                             nullptr,
                             0.0f,
                             40.0f,
                             ImVec2(-1.0f, 72.0f));
        }
        ImGui::Text("云构建线程: %s", performance.cloudBuildInFlight ? "运行中" : "空闲");
        ImGui::Text("云构建队列: %s", performance.cloudBuildQueued ? "有待处理请求" : "空");
        if (performance.gpuTimersSupported) {
            ImGui::Text("GPU 帧时间: %.3f ms", performance.gpuFrameMs);
            ImGui::Text("GPU 点云通道: %.3f ms", performance.gpuPointMs);
            ImGui::Text("GPU 体渲染通道: %.3f ms", performance.gpuVolumeMs);
        } else {
            ImGui::TextUnformatted("GPU 计时器：当前驱动或上下文不支持");
        }
        ImGui::Text("点云生成: %.3f ms", performance.cloudBuildMs);
        ImGui::Text("体数据生成: %.3f ms", performance.volumeBuildMs);
        ImGui::Text("当前云阶段: %s", performance.previewStage ? "预览" : "全质量");
        ImGui::Text("预览质量比: %.2f", performance.qualityRatio);
        ImGui::Separator();
        ImGui::Text("细节层级: %s", lodLabel(performance.lodLevel));
        ImGui::Text("相机距离: %.2f", performance.cameraDistance);
        ImGui::Text("请求/接受/绘制点数: %d / %d / %d",
                    performance.requestedPointCount,
                    performance.pointCount,
                    performance.renderedPointCount);
        ImGui::Text("候选点数: %d", performance.candidateCount);
        ImGui::Text("体切片数: %d", performance.volumeSliceCount);
        ImGui::Text("切片轴向: %s", sliceAxisLabel(performance.volumeSliceAxis));
        ImGui::Text("自适应倍率: %.2f", performance.candidateMultiplier);
        ImGui::Text("径向 CDF / 角向扫描 / 蒙特卡洛: %d / %d / %d",
                    performance.radialCdfSamples,
                    performance.angularScanResolution,
                    performance.monteCarloSamples);
        ImGui::Separator();
        ImGui::Text("点云缓冲 (VBO): %s", formatBytes(performance.pointBufferBytes).c_str());
        ImGui::Text("三维体纹理: %s", formatBytes(performance.volumeTextureBytes).c_str());
        ImGui::Text("帧缓冲: %s", formatBytes(performance.framebufferBytes).c_str());
        ImGui::Text("GPU 跟踪总量: %s", formatBytes(performance.trackedGpuBytes).c_str());
    }
    ImGui::End();

    if (ImGui::Begin(kPanelHelp)) {
        ImGui::TextWrapped("鼠标位于场景窗口时：左键拖拽旋转，中键拖拽平移，滚轮缩放。");
        ImGui::TextWrapped("严格氢样模式适用于单电子体系，例如 H 与 He+。");
        ImGui::TextWrapped("多电子模式使用 Aufbau + Slater Z_eff，仅用于教学近似。");
        ImGui::TextWrapped("第一层中心场是现象学单电子有效势；第三层谱学修正是氢样微扰扩展，不应解释为完整复杂原子谱学。");
        drawAnimationDisclosure(state);
        ImGui::SeparatorText("验证报告");
        char reportPathBuffer[260];
        std::snprintf(reportPathBuffer, sizeof(reportPathBuffer), "%s", state.reporting.outputPath.c_str());
        if (ImGui::InputText("报告文件", reportPathBuffer, sizeof(reportPathBuffer))) {
            state.reporting.outputPath = reportPathBuffer;
        }
        if (ImGui::Button("在帮助面板导出报告")) {
            state.reporting.exportValidationReportRequested = true;
        }
        ImGui::TextWrapped("%s", state.reporting.exportStatus.c_str());
        ImGui::SeparatorText("自动演示");
        ImGui::Checkbox("启用自动演示", &state.demo.enabled);
        ImGui::Checkbox("循环播放", &state.demo.loop);
        const double demoDurationMin = 2.0;
        const double demoDurationMax = 18.0;
        ImGui::SliderScalar("单步时长 (s)", ImGuiDataType_Double, &state.demo.stepDurationSeconds, &demoDurationMin, &demoDurationMax);
        ImGui::Text("当前场景: %s", state.demo.currentStepName.c_str());
        ImGui::ProgressBar(static_cast<float>(state.demo.stepProgress), ImVec2(-1.0f, 0.0f));
        if (ImGui::Button("重新开始演示")) {
            state.demo.enabled = true;
            state.demo.restartRequested = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("下一步")) {
            state.demo.enabled = true;
            state.demo.advanceRequested = true;
        }
        ImGui::Checkbox("使用脚本回放", &state.demo.useScriptPlayback);
        ImGui::Text("当前脚本: %s", state.demo.activeScriptName.c_str());
        ImGui::Text("脚本状态: %s", state.demo.scriptStatus.c_str());
        ImGui::Text("已加载/已录制步骤: %d / %d", state.demo.loadedStepCount, state.demo.recordedStepCount);
        char scriptPathBuffer[260];
        std::snprintf(scriptPathBuffer, sizeof(scriptPathBuffer), "%s", state.demo.scriptPath.c_str());
        if (ImGui::InputText("脚本文件", scriptPathBuffer, sizeof(scriptPathBuffer))) {
            state.demo.scriptPath = scriptPathBuffer;
        }
        if (ImGui::Button("加载脚本")) {
            state.demo.loadScriptRequested = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("保存脚本")) {
            state.demo.saveScriptRequested = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("导出内置脚本")) {
            state.demo.exportBuiltInScriptRequested = true;
        }
        if (ImGui::Button("录制当前场景")) {
            state.demo.captureStepRequested = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("清空录制")) {
            state.demo.clearRecordedStepsRequested = true;
        }
        ImGui::Separator();
        if (ImGui::Button("重置布局")) {
            layoutInitialized_ = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("复制演示脚本")) {
            std::ostringstream stream;
            stream << "元素 Z=" << state.atomicNumber << "，电荷态=" << state.chargeState
                   << "，模型=" << modelLabel(state.modelKind)
                   << "，近似=" << approximationLabel(state.approximationMode)
                   << "，跃迁=(" << state.transition.initial.n << "," << state.transition.initial.l << "," << state.transition.initial.m
                   << ")->(" << state.transition.final.n << "," << state.transition.final.l << "," << state.transition.final.m << ")";
            ImGui::SetClipboardText(stream.str().c_str());
        }
    }
    ImGui::End();

    if (ImGui::Begin(kPanelLog)) {
        static bool autoScroll = true;
        const auto entries = logger.entries();

        if (ImGui::Button("清空日志")) {
            const_cast<quantum::core::Logger&>(logger).clear();
        }
        ImGui::SameLine();
        if (ImGui::Button("复制全部日志")) {
            std::string combined;
            combined.reserve(entries.size() * 96);
            for (const auto& entry : entries) {
                combined += formatLogEntry(entry);
                combined.push_back('\n');
            }
            ImGui::SetClipboardText(combined.c_str());
        }
        ImGui::SameLine();
        if (ImGui::Button("复制最后一条") && !entries.empty()) {
            const std::string lastLine = formatLogEntry(entries.back());
            ImGui::SetClipboardText(lastLine.c_str());
        }
        ImGui::SameLine();
        ImGui::Checkbox("自动滚动", &autoScroll);
        ImGui::Separator();

        ImGui::BeginChild("##runtime_log_entries", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar);
        for (const auto& entry : entries) {
            const ImVec4 color = (entry.level == quantum::core::LogLevel::Error)
                                     ? ImVec4(0.92f, 0.23f, 0.17f, 1.0f)
                                     : (entry.level == quantum::core::LogLevel::Warning)
                                           ? ImVec4(0.95f, 0.72f, 0.16f, 1.0f)
                                           : ImVec4(0.75f, 0.80f, 0.86f, 1.0f);
            std::string line = formatLogEntry(entry);
            ImGui::TextColored(color, "%s", line.c_str());
        }
        if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 24.0f) {
            ImGui::SetScrollHereY(1.0f);
        }
        ImGui::EndChild();
    }
    ImGui::End();

    return result;
}

} // namespace quantum::ui
