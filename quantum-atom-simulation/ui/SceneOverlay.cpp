#include "quantum/ui/AppUi.h"
#include "quantum/ui/UiUtils.h"
#include "quantum/ui/UiStringKey.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <cstdio>
#include <sstream>

namespace quantum::ui {
namespace {

std::string animatedEllipsis() {
    const int phase = static_cast<int>(ImGui::GetTime() * 3.0) % 4;
    return std::string(static_cast<std::size_t>(phase), '.');
}

} // namespace

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
    if (state.nuclearAnimation.enabled) {
        stream << " | 核过程 " << nuclearAnimationModeLabel(state.nuclearAnimation.mode);
        if (state.derived.nuclearCrossSection.valid) {
            stream << " | σ " << state.derived.nuclearCrossSection.queryCrossSectionBarn << " b";
        }
        if (state.derived.nuclearTransport.valid) {
            stream << " | 透射 " << state.derived.nuclearTransport.transmissionAtExit;
        }
        if (state.derived.nuclearDecay.valid && !state.derived.nuclearDecay.samples.empty()) {
            stream << " | 剩余 " << state.derived.nuclearDecay.samples.back().parentFraction;
        }
    }
    if (performance.cloudBuildInFlight) {
        stream << " | 云数据生成中" << animatedEllipsis();
    } else if (performance.cloudBuildQueued) {
        stream << " | 已排队等待更新";
    }

    drawList->AddText(ImVec2(min.x + 10.0f, min.y + 7.0f), IM_COL32(228, 234, 241, 255), stream.str().c_str());
}

void drawCloudBuildProgress(const ImRect& rect, const quantum::core::PerformanceSnapshot& performance) {
    if (!performance.cloudBuildInFlight) return;

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const float ringRadius = 18.0f;
    const float padding = 12.0f;
    const ImVec2 ringCenter(rect.Min.x + padding + ringRadius + 6.0f,
                            rect.Max.y - padding - ringRadius - 6.0f);
    const int numSegments = 32;
    const float animatedAngle = static_cast<float>(ImGui::GetTime() * 2.5f);

    drawList->AddCircle(ringCenter, ringRadius, IM_COL32(60, 70, 84, 200), numSegments, 2.5f);

    for (int i = 0; i < numSegments; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(numSegments);
        float angle = animatedAngle + t * 2.0f * 3.1415926535f;
        float alpha = 0.3f + 0.7f * t;
        ImU32 color = IM_COL32(
            static_cast<int>(72.0f + alpha * 120.0f),
            static_cast<int>(134.0f + alpha * 80.0f),
            static_cast<int>(212.0f - alpha * 60.0f),
            static_cast<int>(120.0f + alpha * 100.0f));
        ImVec2 p(ringCenter.x + ringRadius * 0.6f * std::cos(angle),
                 ringCenter.y + ringRadius * 0.6f * std::sin(angle));
        drawList->AddCircleFilled(p, 2.5f, color);
    }

    const char* buildLabel = performance.previewStage ? "生成预览..." : "生成完整云...";
    drawList->AddText(ImVec2(ringCenter.x + ringRadius + 8.0f, ringCenter.y - 8.0f),
                      IM_COL32(160, 170, 184, 255), buildLabel);
}

void drawSceneHudOverlay(const ImRect& rect,
                         const quantum::app::SimulationState& state) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const float hudWidth = 220.0f;
    const float hudHeight = 56.0f;
    const float padding = 12.0f;
    const ImVec2 min(rect.Max.x - hudWidth - padding, rect.Min.y + padding);
    const ImVec2 max(rect.Max.x - padding, rect.Min.y + padding + hudHeight);

    ImU32 bgColor;
    ImU32 statusColor;
    const char* statusText;
    if (state.derived.strictHydrogenicValid &&
        state.approximationMode == quantum::physics::ApproximationMode::StrictHydrogenic) {
        bgColor = IM_COL32(12, 42, 24, 200);
        statusColor = IM_COL32(72, 212, 108, 255);
        statusText = "严格氢样 可靠";
    } else if (!state.derived.approximationWarning.empty()) {
        bgColor = IM_COL32(52, 32, 12, 200);
        statusColor = IM_COL32(232, 148, 68, 255);
        statusText = "配置冲突";
    } else {
        bgColor = IM_COL32(42, 38, 14, 200);
        statusColor = IM_COL32(212, 188, 72, 255);
        statusText = "多电子近似 教学级";
    }

    drawList->AddRectFilled(min, max, bgColor, 8.0f);
    drawList->AddRect(min, max, statusColor, 8.0f, 0, 1.5f);

    const std::string elementLabel = state.derived.element.symbol + "("
        + localizedElementName(state.derived.element) + ")";
    drawList->AddText(ImVec2(min.x + 10.0f, min.y + 6.0f),
                      IM_COL32(240, 244, 248, 255),
                      elementLabel.c_str());
    drawList->AddText(ImVec2(min.x + 10.0f, min.y + 28.0f),
                      statusColor,
                      statusText);

    if (state.nuclearAnimation.enabled) {
        drawList->AddText(ImVec2(min.x + 140.0f, min.y + 28.0f),
                          IM_COL32(180, 190, 200, 255),
                          nuclearAnimationModeLabel(state.nuclearAnimation.mode));
    }

    if (state.demo.enabled) {
        const float barWidth = hudWidth - 24.0f;
        const ImVec2 barMin(min.x + 10.0f, max.y - 10.0f);
        const ImVec2 barMax(min.x + 10.0f + barWidth, max.y - 4.0f);
        drawList->AddRectFilled(barMin, barMax, IM_COL32(40, 44, 52, 200), 2.0f);
        const float progress = static_cast<float>(state.demo.stepProgress);
        const ImVec2 fillMax(barMin.x + barWidth * progress, barMax.y);
        drawList->AddRectFilled(barMin, fillMax, IM_COL32(92, 158, 232, 200), 2.0f);
    }
}

} // namespace quantum::ui
