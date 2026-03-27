#include "quantum/ui/AppUi.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <glm/common.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <sstream>
#include <string>
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

ImVec2 fitRectPreservingAspect(const ImVec2& available, float aspectRatio) {
    ImVec2 size(std::max(available.x, 1.0f), std::max(available.y, 1.0f));
    if (aspectRatio <= 1.0e-6f) {
        return size;
    }

    const float availableAspect = size.x / std::max(size.y, 1.0f);
    if (availableAspect > aspectRatio) {
        size.x = size.y * aspectRatio;
    } else {
        size.y = size.x / aspectRatio;
    }
    return size;
}

void markAllDirty(quantum::app::SimulationState& state) {
    state.dirty.physics = true;
    state.dirty.cloud = true;
    state.dirty.solver = true;
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
    ImVec2 mousePosition{};
};

std::string formatPlotValue(double value, int precision = 4) {
    std::ostringstream stream;
    stream.setf(std::ios::fixed, std::ios::floatfield);
    stream.precision(precision);
    stream << value;
    return stream.str();
}

bool pointInRect(const ImRect& rect, const ImVec2& point) {
    return point.x >= rect.Min.x && point.x <= rect.Max.x && point.y >= rect.Min.y && point.y <= rect.Max.y;
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

PlotCanvas beginPlotCanvas(const char* label, float height, const char* xAxisLabel, const char* yAxisLabel) {
    ImGui::TextUnformatted(label);
    const ImVec2 canvasSize(ImGui::GetContentRegionAvail().x, height);
    const std::string buttonId = std::string("##plot_") + label;
    ImGui::InvisibleButton(buttonId.c_str(), canvasSize);
    const ImRect outer(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    const ImRect inner(ImVec2(outer.Min.x + 58.0f, outer.Min.y + 16.0f), ImVec2(outer.Max.x - 20.0f, outer.Max.y - 34.0f));
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
    return {outer, inner, ImGui::IsItemHovered(), ImGui::IsItemClicked(ImGuiMouseButton_Left), ImGui::GetIO().MousePos};
}

void drawPlotBounds(const PlotCanvas& canvas, double minX, double maxX, double minY, double maxY, bool showX = true, bool showY = true) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    auto formatValue = [](double value) {
        std::ostringstream stream;
        stream.setf(std::ios::fixed, std::ios::floatfield);
        stream.precision(std::abs(value) >= 100.0 ? 0 : (std::abs(value) >= 10.0 ? 1 : 3));
        stream << value;
        return stream.str();
    };

    const std::string minXText = formatValue(minX);
    const std::string maxXText = formatValue(maxX);
    const std::string minYText = formatValue(minY);
    const std::string maxYText = formatValue(maxY);
    if (showX) {
        drawList->AddText(ImVec2(canvas.inner.Min.x, canvas.outer.Max.y - 24.0f), IM_COL32(160, 170, 180, 255), minXText.c_str());
        const ImVec2 maxXSize = ImGui::CalcTextSize(maxXText.c_str());
        drawList->AddText(ImVec2(canvas.inner.Max.x - maxXSize.x, canvas.outer.Max.y - 24.0f),
                          IM_COL32(160, 170, 180, 255),
                          maxXText.c_str());
    }
    if (showY) {
        drawList->AddText(ImVec2(canvas.outer.Min.x + 10.0f, canvas.inner.Max.y - 8.0f), IM_COL32(160, 170, 180, 255), minYText.c_str());
        drawList->AddText(ImVec2(canvas.outer.Min.x + 10.0f, canvas.inner.Min.y - 8.0f), IM_COL32(160, 170, 180, 255), maxYText.c_str());
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
           << " | 帧率 " << performance.fps;
    if (state.demo.enabled) {
        stream << " | 演示 " << state.demo.currentStepName << " (" << static_cast<int>(state.demo.stepProgress * 100.0) << "%)";
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
    const PlotCanvas canvas = beginPlotCanvas(label, height, xAxisLabel, yAxisLabel);
    if (xs.size() < 2 || ys.size() < 2) {
        return;
    }
    const auto [minXIt, maxXIt] = std::minmax_element(xs.begin(), xs.end());
    const auto [minYIt, maxYIt] = std::minmax_element(ys.begin(), ys.end());
    const double minX = *minXIt;
    const double maxX = *maxXIt;
    const double minY = *minYIt;
    const double maxY = *maxYIt;
    drawPlotBounds(canvas, minX, maxX, minY, maxY);
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    std::vector<ImVec2> points;
    points.reserve(xs.size());
    for (std::size_t index = 1; index < xs.size(); ++index) {
        const ImVec2 p0 = toPlotPoint(canvas.inner, xs[index - 1], ys[index - 1], minX, maxX, minY, maxY);
        const ImVec2 p1 = toPlotPoint(canvas.inner, xs[index], ys[index], minX, maxX, minY, maxY);
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

void drawSpectrumPlot(const std::vector<quantum::physics::SpectrumLine>& calculated,
                      const std::vector<quantum::physics::SpectrumLine>& references,
                      bool filterAllowed,
                      int& selectedIndex,
                      bool& selectedReference) {
    const PlotCanvas canvas = beginPlotCanvas("谱线图", 220.0f, "波长 (nm)", "能量 (eV)");
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
    if (maxX <= minX) {
        return;
    }

    drawPlotBounds(canvas, minX, maxX, 0.0, maxY);
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    std::vector<ImVec2> calculatedAnchors;
    std::vector<const quantum::physics::SpectrumLine*> calculatedLines;
    std::vector<ImVec2> referenceAnchors;
    std::vector<const quantum::physics::SpectrumLine*> referenceLines;
    drawList->AddLine(ImVec2(canvas.inner.Min.x + 10.0f, canvas.outer.Min.y + 10.0f),
                      ImVec2(canvas.inner.Min.x + 30.0f, canvas.outer.Min.y + 10.0f),
                      IM_COL32(72, 176, 255, 220),
                      2.0f);
    drawList->AddText(ImVec2(canvas.inner.Min.x + 36.0f, canvas.outer.Min.y + 2.0f),
                      IM_COL32(210, 218, 226, 255),
                      "计算谱线");
    drawList->AddLine(ImVec2(canvas.inner.Min.x + 114.0f, canvas.outer.Min.y + 10.0f),
                      ImVec2(canvas.inner.Min.x + 134.0f, canvas.outer.Min.y + 10.0f),
                      IM_COL32(237, 184, 56, 220),
                      2.0f);
    drawList->AddText(ImVec2(canvas.inner.Min.x + 140.0f, canvas.outer.Min.y + 2.0f),
                      IM_COL32(210, 218, 226, 255),
                      "参考谱线");
    for (const auto& line : calculated) {
        if (filterAllowed && !line.allowed) {
            continue;
        }
        const ImVec2 p0 = toPlotPoint(canvas.inner, line.wavelengthNm, 0.0, minX, maxX, 0.0, maxY);
        const ImVec2 p1 = toPlotPoint(canvas.inner, line.wavelengthNm, line.photonEnergyEv, minX, maxX, 0.0, maxY);
        drawList->AddLine(p0, p1, IM_COL32(72, 176, 255, 220), 1.5f);
        calculatedAnchors.push_back(p1);
        calculatedLines.push_back(&line);
    }
    for (const auto& line : references) {
        const ImVec2 p0 = toPlotPoint(canvas.inner, line.wavelengthNm, 0.0, minX, maxX, 0.0, maxY);
        const ImVec2 p1 = toPlotPoint(canvas.inner, line.wavelengthNm, line.photonEnergyEv, minX, maxX, 0.0, maxY);
        drawList->AddLine(p0, p1, IM_COL32(237, 184, 56, 220), 2.0f);
        referenceAnchors.push_back(p1);
        referenceLines.push_back(&line);
    }

    const bool mouseInside = canvas.hovered && pointInRect(canvas.inner, canvas.mousePosition);
    const int hoveredCalculated = mouseInside ? nearestPointIndex(calculatedAnchors, canvas.mousePosition, 9.0f) : -1;
    const int hoveredReference = mouseInside ? nearestPointIndex(referenceAnchors, canvas.mousePosition, 9.0f) : -1;
    bool hoveredIsReference = false;
    int hoveredIndex = hoveredCalculated;
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
    if (mouseInside && hoveredIndex >= 0) {
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
        ImGui::EndTooltip();
    }
    if (canvas.clicked && hoveredIndex >= 0) {
        selectedIndex = hoveredIndex;
        selectedReference = hoveredIsReference;
    }
    if (selectedIndex >= 0) {
        const auto& anchors = selectedReference ? referenceAnchors : calculatedAnchors;
        if (static_cast<std::size_t>(selectedIndex) < anchors.size()) {
            drawHighlightedPoint(drawList,
                                 anchors[static_cast<std::size_t>(selectedIndex)],
                                 IM_COL32(255, 208, 92, 255));
        }
    }
}

void drawEnergyLevelsPlot(const std::vector<quantum::physics::EnergyLevelSample>& levels, int& selectedIndex) {
    const PlotCanvas canvas = beginPlotCanvas("能级图", 220.0f, nullptr, "能量 (eV)");
    if (levels.empty()) {
        return;
    }
    double minY = levels.front().energyEv;
    double maxY = levels.front().energyEv;
    for (const auto& level : levels) {
        minY = std::min(minY, level.energyEv);
        maxY = std::max(maxY, level.energyEv);
    }
    drawPlotBounds(canvas, 0.0, 1.0, minY, maxY, false, true);

    struct LabelPlacement {
        const quantum::physics::EnergyLevelSample* level = nullptr;
        float actualY = 0.0f;
        float labelY = 0.0f;
    };

    std::vector<LabelPlacement> placements;
    placements.reserve(levels.size());
    for (const auto& level : levels) {
        const ImVec2 anchor = toPlotPoint(canvas.inner, 0.0, level.energyEv, 0.0, 1.0, minY, maxY);
        placements.push_back({&level, anchor.y, anchor.y});
    }
    std::sort(placements.begin(), placements.end(), [](const LabelPlacement& lhs, const LabelPlacement& rhs) {
        return lhs.actualY < rhs.actualY;
    });

    constexpr float kMinimumLabelSpacing = 18.0f;
    for (std::size_t index = 1; index < placements.size(); ++index) {
        placements[index].labelY = std::max(placements[index].labelY, placements[index - 1].labelY + kMinimumLabelSpacing);
    }
    if (!placements.empty()) {
        const float maxLabelY = canvas.inner.Max.y - 10.0f;
        float overflow = std::max(0.0f, placements.back().labelY - maxLabelY);
        for (auto it = placements.rbegin(); it != placements.rend(); ++it) {
            it->labelY -= overflow;
            if ((it + 1) != placements.rend()) {
                overflow = std::max(0.0f, (it + 1)->labelY + kMinimumLabelSpacing - it->labelY);
            }
        }
    }

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const float lineLeft = canvas.inner.Min.x + 10.0f;
    const float lineRight = canvas.inner.Max.x - 78.0f;
    std::vector<ImVec2> anchors;
    anchors.reserve(placements.size());
    for (const auto& placement : placements) {
        const std::string label = "n=" + std::to_string(placement.level->n);
        const ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
        const ImVec2 labelMin(canvas.inner.Max.x - textSize.x - 12.0f, placement.labelY - textSize.y * 0.5f - 2.0f);
        const ImVec2 labelMax(canvas.inner.Max.x - 6.0f, placement.labelY + textSize.y * 0.5f + 2.0f);

        drawList->AddLine(ImVec2(lineLeft, placement.actualY),
                          ImVec2(lineRight, placement.actualY),
                          IM_COL32(164, 194, 244, 220),
                          2.0f);
        anchors.emplace_back((lineLeft + lineRight) * 0.5f, placement.actualY);
        drawList->AddLine(ImVec2(lineRight + 4.0f, placement.actualY),
                          ImVec2(labelMin.x - 4.0f, placement.labelY),
                          IM_COL32(110, 120, 132, 180),
                          1.0f);
        drawList->AddRectFilled(labelMin, labelMax, IM_COL32(22, 28, 36, 220), 3.0f);
        drawList->AddText(ImVec2(labelMin.x + 4.0f, placement.labelY - textSize.y * 0.5f),
                          IM_COL32(220, 226, 233, 255),
                          label.c_str());
    }

    const bool mouseInside = canvas.hovered && pointInRect(canvas.inner, canvas.mousePosition);
    const int hoveredIndex = mouseInside ? nearestPointIndex(anchors, canvas.mousePosition, 10.0f) : -1;
    if (mouseInside && hoveredIndex >= 0 && static_cast<std::size_t>(hoveredIndex) < placements.size()) {
        drawHighlightedPoint(drawList, anchors[static_cast<std::size_t>(hoveredIndex)], IM_COL32(164, 194, 244, 255));
        ImGui::BeginTooltip();
        ImGui::Text("能级 n=%d", placements[static_cast<std::size_t>(hoveredIndex)].level->n);
        ImGui::Text("能量: %s eV", formatPlotValue(placements[static_cast<std::size_t>(hoveredIndex)].level->energyEv).c_str());
        ImGui::EndTooltip();
    }
    if (canvas.clicked && hoveredIndex >= 0) {
        selectedIndex = hoveredIndex;
    }
    if (selectedIndex >= 0 && static_cast<std::size_t>(selectedIndex) < anchors.size()) {
        drawHighlightedPoint(drawList, anchors[static_cast<std::size_t>(selectedIndex)], IM_COL32(255, 208, 92, 255));
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
            const float framebufferAspect =
                (framebufferSize.y > 0) ? (static_cast<float>(framebufferSize.x) / static_cast<float>(framebufferSize.y))
                                        : (canvasSize.x / canvasSize.y);
            const ImVec2 imageSize = fitRectPreservingAspect(canvasSize, framebufferAspect);
            const ImVec2 imagePos(canvasRect.Min.x + (canvasSize.x - imageSize.x) * 0.5f,
                                  canvasRect.Min.y + (canvasSize.y - imageSize.y) * 0.5f);
            drawList->AddRectFilled(imagePos,
                                    ImVec2(imagePos.x + imageSize.x, imagePos.y + imageSize.y),
                                    IM_COL32(14, 20, 28, 255),
                                    4.0f);
            drawList->AddImage(reinterpret_cast<void*>(static_cast<intptr_t>(sceneTexture)),
                               imagePos,
                               ImVec2(imagePos.x + imageSize.x, imagePos.y + imageSize.y),
                               ImVec2(0.0f, 1.0f),
                               ImVec2(1.0f, 0.0f));
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
        if (ImGui::BeginCombo("元素", elements.empty() ? "未加载" : elements[currentIndex].symbol.c_str())) {
            for (std::size_t index = 0; index < elements.size(); ++index) {
                const bool selected = static_cast<int>(index) == currentIndex;
                if (ImGui::Selectable(elements[index].symbol.c_str(), selected)) {
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
                    state.modelKind = kind;
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
        ImGui::Checkbox("启用 LOD", &state.cloud.enableLod);
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
        ImGui::Text("CDF/角向/MC = %d / %d / %d",
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
    }
    ImGui::End();

    if (ImGui::Begin(kPanelPlots)) {
        drawEnergyLevelsPlot(state.derived.energyLevels, energySelectionIndex_);
        drawSpectrumPlot(state.derived.spectrumLines,
                         state.derived.referenceLines,
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
        drawPolylinePlot("径向分布", radii, values, IM_COL32(127, 196, 109, 230), 220.0f, "r (nm)", "P(r)");

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
        if (energySelectionIndex_ >= 0 && static_cast<std::size_t>(energySelectionIndex_) < state.derived.energyLevels.size()) {
            const auto& level = state.derived.energyLevels[static_cast<std::size_t>(energySelectionIndex_)];
            ImGui::Text("选中能级: n=%d, E=%.6f eV", level.n, level.energyEv);
        }
        if (convergenceSelectionIndex_ >= 0 && static_cast<std::size_t>(convergenceSelectionIndex_) < grids.size()) {
            ImGui::Text("选中收敛点: 步长=%.6f pm, 误差=%.6e eV",
                        grids[static_cast<std::size_t>(convergenceSelectionIndex_)],
                        errors[static_cast<std::size_t>(convergenceSelectionIndex_)]);
        }
        const auto& selectedSpectrumSource = spectrumSelectionReference_ ? state.derived.referenceLines : state.derived.spectrumLines;
        if (spectrumSelectionIndex_ >= 0 && static_cast<std::size_t>(spectrumSelectionIndex_) < selectedSpectrumSource.size()) {
            const auto& line = selectedSpectrumSource[static_cast<std::size_t>(spectrumSelectionIndex_)];
            ImGui::Text("选中谱线: %s, λ=%.6f nm, E=%.6f eV", line.seriesName.c_str(), line.wavelengthNm, line.photonEnergyEv);
        }
        if (ImGui::BeginTable("##plot_summary_table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("能级");
            ImGui::TableSetupColumn("谱线");
            ImGui::TableSetupColumn("收敛");
            ImGui::TableHeadersRow();
            const std::size_t rowCount = std::max<std::size_t>(
                4,
                std::max(state.derived.energyLevels.size(),
                         std::max(state.derived.spectrumLines.size(), state.derived.solver.convergence.size())));
            for (std::size_t row = 0; row < rowCount; ++row) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (row < state.derived.energyLevels.size()) {
                    const auto& level = state.derived.energyLevels[row];
                    ImGui::Text("n=%d | %.4f eV", level.n, level.energyEv);
                }
                ImGui::TableNextColumn();
                if (row < state.derived.spectrumLines.size()) {
                    const auto& line = state.derived.spectrumLines[row];
                    ImGui::Text("%s | %.3f nm", line.seriesName.c_str(), line.wavelengthNm);
                }
                ImGui::TableNextColumn();
                if (row < state.derived.solver.convergence.size()) {
                    const auto& sample = state.derived.solver.convergence[row];
                    ImGui::Text("%.3f pm | %.3e eV", sample.gridStepMeters * 1.0e12, sample.errorEv);
                }
            }
            ImGui::EndTable();
        }
    }
    ImGui::End();

    if (ImGui::Begin(kPanelPerformance)) {
        ImGui::Text("FPS: %.2f", performance.fps);
        ImGui::Text("平均帧时间: %.3f ms", performance.averageFrameMs);
        ImGui::Text("CPU 帧时间: %.3f ms", performance.cpuFrameMs);
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
        ImGui::Text("LOD 级别: %s", lodLabel(performance.lodLevel));
        ImGui::Text("相机距离: %.2f", performance.cameraDistance);
        ImGui::Text("请求/接受/绘制点数: %d / %d / %d",
                    performance.requestedPointCount,
                    performance.pointCount,
                    performance.renderedPointCount);
        ImGui::Text("候选点数: %d", performance.candidateCount);
        ImGui::Text("体切片数: %d", performance.volumeSliceCount);
        ImGui::Text("切片轴向: %s", sliceAxisLabel(performance.volumeSliceAxis));
        ImGui::Text("自适应倍率: %.2f", performance.candidateMultiplier);
        ImGui::Text("CDF/角向/MC: %d / %d / %d",
                    performance.radialCdfSamples,
                    performance.angularScanResolution,
                    performance.monteCarloSamples);
        ImGui::Separator();
        ImGui::Text("点云 VBO: %s", formatBytes(performance.pointBufferBytes).c_str());
        ImGui::Text("三维体纹理: %s", formatBytes(performance.volumeTextureBytes).c_str());
        ImGui::Text("帧缓冲: %s", formatBytes(performance.framebufferBytes).c_str());
        ImGui::Text("GPU 跟踪总量: %s", formatBytes(performance.trackedGpuBytes).c_str());
    }
    ImGui::End();

    if (ImGui::Begin(kPanelHelp)) {
        ImGui::TextWrapped("鼠标位于场景窗口时：左键拖拽旋转，中键拖拽平移，滚轮缩放。");
        ImGui::TextWrapped("严格氢样模式适用于单电子体系，例如 H 与 He+。");
        ImGui::TextWrapped("多电子模式使用 Aufbau + Slater Z_eff，仅用于教学近似。");
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
