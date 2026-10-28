#include "quantum/ui/AppUi.h"
#include "quantum/ui/UiUtils.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace quantum::ui {
namespace {

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
    ImVec2 panStart{};
    bool showContextMenu = false;
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
    viewState.showContextMenu = false;
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

    if (mouseInside && !viewState.boxSelecting && !viewState.panning) {
        drawList->AddLine(ImVec2(canvas.mousePosition.x, canvas.inner.Min.y),
                          ImVec2(canvas.mousePosition.x, canvas.inner.Max.y),
                          IM_COL32(180, 190, 200, 80), 1.0f);
        drawList->AddLine(ImVec2(canvas.inner.Min.x, canvas.mousePosition.y),
                          ImVec2(canvas.inner.Max.x, canvas.mousePosition.y),
                          IM_COL32(180, 190, 200, 80), 1.0f);

        const double plotX = pixelToPlotX(canvas.inner, range, canvas.mousePosition.x);
        const double plotY = pixelToPlotY(canvas.inner, range, canvas.mousePosition.y);

        char tooltip[128];
        snprintf(tooltip, sizeof(tooltip), "%.2f, %.3f", plotX, plotY);
        const ImVec2 textSize = ImGui::CalcTextSize(tooltip);
        ImVec2 tooltipPos(canvas.mousePosition.x + 12.0f, canvas.mousePosition.y - textSize.y - 6.0f);

        tooltipPos.x = std::min(tooltipPos.x, canvas.inner.Max.x - textSize.x - 12.0f);
        tooltipPos.y = std::max(tooltipPos.y, canvas.inner.Min.y + 4.0f);

        drawList->AddRectFilled(tooltipPos,
                                ImVec2(tooltipPos.x + textSize.x + 8.0f, tooltipPos.y + textSize.y + 4.0f),
                                IM_COL32(24, 28, 36, 230), 3.0f);
        drawList->AddText(ImVec2(tooltipPos.x + 4.0f, tooltipPos.y + 2.0f),
                          IM_COL32(220, 228, 236, 255), tooltip);
    }

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
        viewState.panStart = canvas.mousePosition;
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
            const float panDist = std::sqrt(
                (canvas.mousePosition.x - viewState.panStart.x) * (canvas.mousePosition.x - viewState.panStart.x) +
                (canvas.mousePosition.y - viewState.panStart.y) * (canvas.mousePosition.y - viewState.panStart.y));
            if (panDist < 5.0f && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
                viewState.showContextMenu = true;
            }
            viewState.panning = false;
        }
    }

    if (viewState.showContextMenu) {
        ImGui::OpenPopup("##plot_context_menu");
        viewState.showContextMenu = false;
    }
    if (ImGui::BeginPopup("##plot_context_menu")) {
        if (ImGui::MenuItem("重置视图")) {
            clearPlotView(viewState);
        }
        ImGui::EndPopup();
    }

    if (mouseInside && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Right)) {
        clearPlotView(viewState);
    }

    if (viewState.hasOverride) {
        viewState.range = sanitizePlotRange(range);
        range = viewState.range;
    }
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

void AppUi::drawChartPanels(quantum::app::SimulationState& state) const {
    if (!showAdvancedPanels_) {
        return;
    }
    if (!ImGui::Begin(kPanelPlots)) {
        return;
    }
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

    std::vector<double> scfIterations;
    std::vector<double> scfResiduals;
    scfIterations.reserve(state.derived.meanField.residualHistory.size());
    scfResiduals.reserve(state.derived.meanField.residualHistory.size());
    for (std::size_t index = 0; index < state.derived.meanField.residualHistory.size(); ++index) {
        scfIterations.push_back(static_cast<double>(index + 1));
        scfResiduals.push_back(state.derived.meanField.residualHistory[index]);
    }
    if (scfIterations.size() >= 2) {
        drawPolylinePlot("SCF 残差",
                         scfIterations,
                         scfResiduals,
                         convergenceSelectionIndex_,
                         IM_COL32(126, 216, 255, 230),
                         220.0f,
                         "迭代",
                         "残差");
    }

    std::vector<double> ciIndex;
    std::vector<double> ciWeights;
    ciIndex.reserve(state.derived.correlation.components.size());
    ciWeights.reserve(state.derived.correlation.components.size());
    for (std::size_t index = 0; index < state.derived.correlation.components.size(); ++index) {
        ciIndex.push_back(static_cast<double>(index + 1));
        ciWeights.push_back(state.derived.correlation.components[index].weight);
    }
    if (ciIndex.size() >= 2) {
        drawPolylinePlot("CI 权重",
                         ciIndex,
                         ciWeights,
                         convergenceSelectionIndex_,
                         IM_COL32(190, 235, 120, 230),
                         180.0f,
                         "组态",
                         "权重");
    }

    std::vector<double> tdTimes;
    std::vector<double> tdUpper;
    tdTimes.reserve(state.derived.timeDynamics.sampleTimes.size());
    tdUpper.reserve(state.derived.timeDynamics.upperPopulation.size());
    for (std::size_t index = 0; index < state.derived.timeDynamics.sampleTimes.size(); ++index) {
        tdTimes.push_back(state.derived.timeDynamics.sampleTimes[index] * 1.0e15);
        tdUpper.push_back(state.derived.timeDynamics.upperPopulation[index]);
    }
    if (tdTimes.size() >= 2) {
        drawPolylinePlot("TD 布居",
                         tdTimes,
                         tdUpper,
                         convergenceSelectionIndex_,
                         IM_COL32(255, 190, 92, 230),
                         220.0f,
                         "时间 (fs)",
                         "上能级布居");
    }

    std::vector<double> isotopeMassNumbers;
    std::vector<double> bindingPerNucleon;
    isotopeMassNumbers.reserve(state.derived.nuclearStructure.isotopeChain.size());
    bindingPerNucleon.reserve(state.derived.nuclearStructure.isotopeChain.size());
    for (const auto& sample : state.derived.nuclearStructure.isotopeChain) {
        isotopeMassNumbers.push_back(static_cast<double>(sample.massNumber));
        bindingPerNucleon.push_back(sample.bindingPerNucleonMeV);
    }
    if (isotopeMassNumbers.size() >= 2) {
        drawPolylinePlot("核结构：同位素链结合能/核子",
                         isotopeMassNumbers,
                         bindingPerNucleon,
                         nuclearStructureSelectionIndex_,
                         IM_COL32(214, 120, 255, 230),
                         220.0f,
                         "质量数 A",
                         "B/A (MeV)");
    }

    std::vector<double> nuclearEnergies;
    std::vector<double> nuclearCrossSections;
    nuclearEnergies.reserve(state.derived.nuclearCrossSection.samples.size());
    nuclearCrossSections.reserve(state.derived.nuclearCrossSection.samples.size());
    for (const auto& sample : state.derived.nuclearCrossSection.samples) {
        nuclearEnergies.push_back((state.nuclearAnimation.mode == quantum::app::NuclearAnimationMode::FusionDT) ? (sample.energyEv / 1.0e3)
                                                                                                                 : sample.energyEv);
        nuclearCrossSections.push_back(sample.crossSectionBarn);
    }
    if (nuclearEnergies.size() >= 2) {
        drawPolylinePlot("核反应截面",
                         nuclearEnergies,
                         nuclearCrossSections,
                         nuclearCrossSectionSelectionIndex_,
                         IM_COL32(255, 118, 88, 230),
                         220.0f,
                         (state.nuclearAnimation.mode == quantum::app::NuclearAnimationMode::FusionDT) ? "能量 (keV)"
                                                                                                        : "能量 (eV)",
                         "sigma (barn)");
    }

    std::vector<double> transportTimes;
    std::vector<double> transportSurvival;
    transportTimes.reserve(state.derived.nuclearTransport.samples.size());
    transportSurvival.reserve(state.derived.nuclearTransport.samples.size());
    for (const auto& sample : state.derived.nuclearTransport.samples) {
        transportTimes.push_back(sample.timeSeconds * 1.0e6);
        transportSurvival.push_back(sample.survivalFraction);
    }
    if (transportTimes.size() >= 2) {
        drawPolylinePlot("核输运：束流存活率",
                         transportTimes,
                         transportSurvival,
                         nuclearTransportSelectionIndex_,
                         IM_COL32(102, 228, 188, 230),
                         220.0f,
                         "时间 (us)",
                         "存活率");
    }

    std::vector<double> decayTimes;
    std::vector<double> decayParent;
    decayTimes.reserve(state.derived.nuclearDecay.samples.size());
    decayParent.reserve(state.derived.nuclearDecay.samples.size());
    for (const auto& sample : state.derived.nuclearDecay.samples) {
        decayTimes.push_back(sample.timeSeconds);
        decayParent.push_back(sample.parentFraction);
    }
    if (decayTimes.size() >= 2) {
        drawPolylinePlot("核衰变：母核剩余比例",
                         decayTimes,
                         decayParent,
                         nuclearDecaySelectionIndex_,
                         IM_COL32(255, 214, 94, 230),
                         220.0f,
                         "时间 (s)",
                         "母核比例");
    }
    ImGui::SeparatorText("图表详情");
    const PlotViewState& energyView = plotViewState("能级图");
    const PlotViewState& spectrumView = plotViewState("谱线图");
    const PlotViewState& radialView = plotViewState("径向分布");
    const PlotViewState& centralFieldView = plotViewState("中心场势 V(r)");
    const PlotViewState& convergenceView = plotViewState("求解收敛曲线（步长 pm）");
    const PlotViewState& meanFieldView = plotViewState("SCF 残差");
    const PlotViewState& correlationView = plotViewState("CI 权重");
    const PlotViewState& dynamicsView = plotViewState("TD 布居");
    syncReportingPlotWindow(state.reporting.energyPlotWindow, energyView);
    syncReportingPlotWindow(state.reporting.spectrumPlotWindow, spectrumView);
    syncReportingPlotWindow(state.reporting.radialPlotWindow, radialView);
    syncReportingPlotWindow(state.reporting.centralFieldPlotWindow, centralFieldView);
    syncReportingPlotWindow(state.reporting.convergencePlotWindow, convergenceView);
    syncReportingPlotWindow(state.reporting.meanFieldPlotWindow, meanFieldView);
    syncReportingPlotWindow(state.reporting.correlationPlotWindow, correlationView);
    syncReportingPlotWindow(state.reporting.dynamicsPlotWindow, dynamicsView);

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
    if (!state.derived.meanField.residualHistory.empty()) {
        ImGui::Text("SCF 末次残差: %.6e", state.derived.meanField.residualHistory.back());
    }
    if (state.derived.correlation.solved) {
        ImGui::Text("CI 权重归一: %.6f",
                    state.derived.correlation.components.empty()
                        ? 0.0
                        : state.derived.correlation.components.front().weight +
                              state.derived.correlation.components.back().weight);
    }
    if (state.derived.timeDynamics.solved && !state.derived.timeDynamics.upperPopulation.empty()) {
        ImGui::Text("TD 末端: t=%.6f fs, 上能级=%.6f",
                    state.derived.timeDynamics.sampleTimes.back() * 1.0e15,
                    state.derived.timeDynamics.upperPopulation.back());
    }
    if (centralFieldSelectionIndex_ >= 0 &&
        static_cast<std::size_t>(centralFieldSelectionIndex_) < state.derived.centralField.samples.size()) {
        const auto& sample = state.derived.centralField.samples[static_cast<std::size_t>(centralFieldSelectionIndex_)];
        ImGui::Text("选中中心场点: r=%.4f a0, Z_eff(r)=%.4f, 势能=%.6f Hartree",
                    sample.radiusBohr,
                    sample.effectiveCharge,
                    sample.potentialHartree);
    }
    if (nuclearStructureSelectionIndex_ >= 0 &&
        static_cast<std::size_t>(nuclearStructureSelectionIndex_) < state.derived.nuclearStructure.isotopeChain.size()) {
        const auto& sample =
            state.derived.nuclearStructure.isotopeChain[static_cast<std::size_t>(nuclearStructureSelectionIndex_)];
        ImGui::Text("选中核结构点: A=%d, B=%.6f MeV, B/A=%.6f MeV",
                    sample.massNumber,
                    sample.bindingEnergyMeV,
                    sample.bindingPerNucleonMeV);
    }
    if (nuclearCrossSectionSelectionIndex_ >= 0 &&
        static_cast<std::size_t>(nuclearCrossSectionSelectionIndex_) < state.derived.nuclearCrossSection.samples.size()) {
        const auto& sample =
            state.derived.nuclearCrossSection.samples[static_cast<std::size_t>(nuclearCrossSectionSelectionIndex_)];
        ImGui::Text("选中截面点: E=%.6f %s, sigma=%.6f b",
                    (state.nuclearAnimation.mode == quantum::app::NuclearAnimationMode::FusionDT) ? (sample.energyEv / 1.0e3)
                                                                                                    : sample.energyEv,
                    (state.nuclearAnimation.mode == quantum::app::NuclearAnimationMode::FusionDT) ? "keV" : "eV",
                    sample.crossSectionBarn);
    }
    if (nuclearTransportSelectionIndex_ >= 0 &&
        static_cast<std::size_t>(nuclearTransportSelectionIndex_) < state.derived.nuclearTransport.samples.size()) {
        const auto& sample =
            state.derived.nuclearTransport.samples[static_cast<std::size_t>(nuclearTransportSelectionIndex_)];
        ImGui::Text("选中输运点: t=%.6f us, x=%.6e m, 存活率=%.6f, 反应率=%.6f",
                    sample.timeSeconds * 1.0e6,
                    sample.positionM,
                    sample.survivalFraction,
                    sample.reactedFraction);
    }
    if (nuclearDecaySelectionIndex_ >= 0 &&
        static_cast<std::size_t>(nuclearDecaySelectionIndex_) < state.derived.nuclearDecay.samples.size()) {
        const auto& sample = state.derived.nuclearDecay.samples[static_cast<std::size_t>(nuclearDecaySelectionIndex_)];
        ImGui::Text("选中衰变点: t=%.6f s, 母核=%.6f, 子核=%.6f, 活度比=%.6f",
                    sample.timeSeconds,
                    sample.parentFraction,
                    sample.daughterFraction,
                    sample.activityFraction);
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
    ImGui::End();
}

} // namespace quantum::ui