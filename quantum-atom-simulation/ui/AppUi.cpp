#include "quantum/ui/AppUi.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <glm/common.hpp>

#include <algorithm>
#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

namespace quantum::ui {
namespace {

const char* modelLabel(quantum::app::ModelKind kind) {
    switch (kind) {
    case quantum::app::ModelKind::Bohr:
        return "Bohr";
    case quantum::app::ModelKind::WaveParticle:
        return "Wave-Particle";
    case quantum::app::ModelKind::SchrodingerCloud:
        return "Schrodinger Cloud";
    case quantum::app::ModelKind::Compare:
        return "Compare";
    default:
        return "Unknown";
    }
}

const char* approximationLabel(quantum::physics::ApproximationMode mode) {
    switch (mode) {
    case quantum::physics::ApproximationMode::StrictHydrogenic:
        return "Strict Hydrogenic";
    case quantum::physics::ApproximationMode::MultiElectronApproximation:
        return "Multi-Electron Approx";
    default:
        return "Unknown";
    }
}

const char* cloudModeLabel(quantum::app::CloudRenderMode mode) {
    switch (mode) {
    case quantum::app::CloudRenderMode::PointCloud:
        return "Point Cloud";
    case quantum::app::CloudRenderMode::VolumeSlices:
        return "Volume Slices";
    case quantum::app::CloudRenderMode::Hybrid:
        return "Hybrid";
    default:
        return "Unknown";
    }
}

const char* samplingQualityLabel(quantum::app::SamplingQualityPreset preset) {
    switch (preset) {
    case quantum::app::SamplingQualityPreset::Fast:
        return "Fast";
    case quantum::app::SamplingQualityPreset::Balanced:
        return "Balanced";
    case quantum::app::SamplingQualityPreset::HighFidelity:
        return "High Fidelity";
    default:
        return "Unknown";
    }
}

const char* lodLabel(int lodLevel) {
    switch (lodLevel) {
    case 0:
        return "Full";
    case 1:
        return "Medium";
    case 2:
        return "Far";
    case 3:
        return "Interaction";
    default:
        return "Unknown";
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

void markAllDirty(quantum::app::SimulationState& state) {
    state.dirty.physics = true;
    state.dirty.cloud = true;
    state.dirty.solver = true;
}

void drawScenarioButtons(quantum::app::SimulationState& state) {
    if (ImGui::Button("H")) {
        state.atomicNumber = 1;
        state.chargeState = 0;
        state.approximationMode = quantum::physics::ApproximationMode::StrictHydrogenic;
        state.cloud.components = {{{2, 1, 0}, 1.0, 0.0, 1.0}};
        state.solver.qn = {2, 1, 0};
        markAllDirty(state);
    }
    ImGui::SameLine();
    if (ImGui::Button("He+")) {
        state.atomicNumber = 2;
        state.chargeState = 1;
        state.approximationMode = quantum::physics::ApproximationMode::StrictHydrogenic;
        state.cloud.components = {{{3, 2, 0}, 1.0, 0.0, 2.0}};
        state.solver.qn = {3, 2, 0};
        markAllDirty(state);
    }
    ImGui::SameLine();
    if (ImGui::Button("Ne Approx")) {
        state.atomicNumber = 10;
        state.chargeState = 0;
        state.approximationMode = quantum::physics::ApproximationMode::MultiElectronApproximation;
        state.cloud.components = {{{2, 1, 0}, 0.8, 0.0, 1.0}, {{2, 1, 1}, 0.6, 1.57, 1.0}};
        state.solver.qn = {2, 1, 0};
        markAllDirty(state);
    }
}

template <typename Getter>
std::vector<double> collectSeries(const std::vector<std::pair<double, double>>& data, Getter getter) {
    std::vector<double> values;
    values.reserve(data.size());
    for (const auto& item : data) {
        values.push_back(getter(item));
    }
    return values;
}

ImRect beginPlotCanvas(const char* label, float height) {
    ImGui::TextUnformatted(label);
    const ImVec2 canvasSize(ImGui::GetContentRegionAvail().x, height);
    ImGui::InvisibleButton(label, canvasSize);
    const ImRect rect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(rect.Min, rect.Max, IM_COL32(20, 24, 31, 255), 4.0f);
    drawList->AddRect(rect.Min, rect.Max, IM_COL32(70, 78, 88, 255), 4.0f);
    return rect;
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
                      const ImU32 color,
                      float height = 220.0f) {
    const ImRect rect = beginPlotCanvas(label, height);
    if (xs.size() < 2 || ys.size() < 2) {
        return;
    }
    const auto [minXIt, maxXIt] = std::minmax_element(xs.begin(), xs.end());
    const auto [minYIt, maxYIt] = std::minmax_element(ys.begin(), ys.end());
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    for (std::size_t index = 1; index < xs.size(); ++index) {
        const ImVec2 p0 = toPlotPoint(rect, xs[index - 1], ys[index - 1], *minXIt, *maxXIt, *minYIt, *maxYIt);
        const ImVec2 p1 = toPlotPoint(rect, xs[index], ys[index], *minXIt, *maxXIt, *minYIt, *maxYIt);
        drawList->AddLine(p0, p1, color, 2.0f);
    }
}

void drawSpectrumPlot(const std::vector<quantum::physics::SpectrumLine>& calculated,
                      const std::vector<quantum::physics::SpectrumLine>& references,
                      bool filterAllowed) {
    const ImRect rect = beginPlotCanvas("Spectrum", 220.0f);
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

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    for (const auto& line : calculated) {
        if (filterAllowed && !line.allowed) {
            continue;
        }
        const ImVec2 p0 = toPlotPoint(rect, line.wavelengthNm, 0.0, minX, maxX, 0.0, maxY);
        const ImVec2 p1 = toPlotPoint(rect, line.wavelengthNm, line.photonEnergyEv, minX, maxX, 0.0, maxY);
        drawList->AddLine(p0, p1, IM_COL32(72, 176, 255, 220), 1.5f);
    }
    for (const auto& line : references) {
        const ImVec2 p0 = toPlotPoint(rect, line.wavelengthNm, 0.0, minX, maxX, 0.0, maxY);
        const ImVec2 p1 = toPlotPoint(rect, line.wavelengthNm, line.photonEnergyEv, minX, maxX, 0.0, maxY);
        drawList->AddLine(p0, p1, IM_COL32(237, 184, 56, 220), 2.0f);
    }
}

void drawEnergyLevelsPlot(const std::vector<quantum::physics::EnergyLevelSample>& levels) {
    const ImRect rect = beginPlotCanvas("Energy Levels", 220.0f);
    if (levels.empty()) {
        return;
    }
    double minY = levels.front().energyEv;
    double maxY = levels.front().energyEv;
    for (const auto& level : levels) {
        minY = std::min(minY, level.energyEv);
        maxY = std::max(maxY, level.energyEv);
    }
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    for (const auto& level : levels) {
        const ImVec2 left = toPlotPoint(rect, 0.0, level.energyEv, 0.0, 1.0, minY, maxY);
        const ImVec2 right = toPlotPoint(rect, 1.0, level.energyEv, 0.0, 1.0, minY, maxY);
        drawList->AddLine({left.x + 12.0f, left.y}, {right.x - 12.0f, right.y}, IM_COL32(164, 194, 244, 220), 2.0f);
        drawList->AddText({left.x + 16.0f, left.y - 14.0f}, IM_COL32(220, 226, 233, 255), ("n=" + std::to_string(level.n)).c_str());
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

    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

    if (ImGui::Begin("Scene")) {
        const ImVec2 avail = ImGui::GetContentRegionAvail();
        result.sceneSize = {std::max(avail.x, 1.0f), std::max(avail.y, 1.0f)};
        result.sceneHovered = ImGui::IsWindowHovered();
        result.sceneFocused = ImGui::IsWindowFocused();
        if (sceneTexture != 0) {
            ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(sceneTexture)),
                         avail,
                         ImVec2(0.0f, 1.0f),
                         ImVec2(1.0f, 0.0f));
        } else {
            ImGui::TextUnformatted("Scene framebuffer not ready.");
        }
    }
    ImGui::End();

    if (ImGui::Begin("Inspector")) {
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
        if (ImGui::BeginCombo("Element", elements.empty() ? "N/A" : elements[currentIndex].symbol.c_str())) {
            for (std::size_t index = 0; index < elements.size(); ++index) {
                const bool selected = static_cast<int>(index) == currentIndex;
                if (ImGui::Selectable(elements[index].symbol.c_str(), selected)) {
                    state.atomicNumber = elements[index].atomicNumber;
                    markAllDirty(state);
                }
            }
            ImGui::EndCombo();
        }

        if (ImGui::SliderInt("Charge State", &state.chargeState, 0, std::max(0, state.atomicNumber - 1))) {
            markAllDirty(state);
        }

        if (ImGui::BeginCombo("Model", modelLabel(state.modelKind))) {
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

        if (ImGui::BeginCombo("Approximation", approximationLabel(state.approximationMode))) {
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

        ImGui::SeparatorText("Bohr");
        if (ImGui::SliderInt("n", &state.bohr.principalN, 1, 8)) {
            state.dirty.physics = true;
        }
        if (ImGui::SliderInt("Spectrum n_max", &state.bohr.maxSpectrumN, 2, 10)) {
            state.dirty.physics = true;
        }
        if (ImGui::Checkbox("Use Reduced Mass", &state.bohr.useReducedMass)) {
            markAllDirty(state);
        }

        ImGui::SeparatorText("Transition");
        if (ImGui::SliderInt("n_i", &state.transition.initial.n, 1, 10)) {
            state.dirty.physics = true;
        }
        state.transition.initial.l = std::clamp(state.transition.initial.l, 0, std::max(0, state.transition.initial.n - 1));
        if (ImGui::SliderInt("l_i", &state.transition.initial.l, 0, std::max(0, state.transition.initial.n - 1))) {
            state.dirty.physics = true;
        }
        state.transition.initial.m = std::clamp(state.transition.initial.m, -state.transition.initial.l, state.transition.initial.l);
        if (ImGui::SliderInt("m_i", &state.transition.initial.m, -state.transition.initial.l, state.transition.initial.l)) {
            state.dirty.physics = true;
        }
        if (ImGui::SliderInt("n_f", &state.transition.final.n, 1, std::max(1, state.transition.initial.n - 1))) {
            state.dirty.physics = true;
        }
        state.transition.final.l = std::clamp(state.transition.final.l, 0, std::max(0, state.transition.final.n - 1));
        if (ImGui::SliderInt("l_f", &state.transition.final.l, 0, std::max(0, state.transition.final.n - 1))) {
            state.dirty.physics = true;
        }
        state.transition.final.m = std::clamp(state.transition.final.m, -state.transition.final.l, state.transition.final.l);
        if (ImGui::SliderInt("m_f", &state.transition.final.m, -state.transition.final.l, state.transition.final.l)) {
            state.dirty.physics = true;
        }
        if (ImGui::Checkbox("Dipole Filter", &state.transition.enforceRules)) {
            state.dirty.physics = true;
        }
        if (ImGui::Checkbox("Delta m", &state.transition.enforceDeltaM)) {
            state.dirty.physics = true;
        }

        ImGui::SeparatorText("Cloud");
        if (ImGui::SliderInt("Point Count", &state.cloud.pointCount, 10000, 200000)) {
            state.dirty.cloud = true;
        }
        state.cloud.mediumLodPointCount = std::clamp(state.cloud.mediumLodPointCount, 1000, state.cloud.pointCount);
        state.cloud.farLodPointCount = std::clamp(state.cloud.farLodPointCount, 1000, state.cloud.pointCount);
        state.cloud.interactionPointCount = std::clamp(state.cloud.interactionPointCount, 1000, state.cloud.pointCount);
        state.cloud.farLodDistance = std::max(state.cloud.farLodDistance, state.cloud.mediumLodDistance + 1.0f);
        ImGui::Checkbox("Enable LOD", &state.cloud.enableLod);
        ImGui::Checkbox("Interaction LOD", &state.cloud.reduceDuringInteraction);
        if (state.cloud.enableLod) {
            ImGui::SliderInt("LOD Medium Points", &state.cloud.mediumLodPointCount, 1000, state.cloud.pointCount);
            ImGui::SliderInt("LOD Far Points", &state.cloud.farLodPointCount, 1000, state.cloud.pointCount);
            ImGui::SliderInt("LOD Drag Points", &state.cloud.interactionPointCount, 1000, state.cloud.pointCount);
            ImGui::SliderFloat("LOD Medium Dist", &state.cloud.mediumLodDistance, 6.0f, 40.0f);
            ImGui::SliderFloat("LOD Far Dist", &state.cloud.farLodDistance, 8.0f, 80.0f);
        }
        if (ImGui::SliderInt("Volume Res", &state.cloud.volumeResolution, 16, 96)) {
            state.dirty.cloud = true;
        }
        const double extentMin = 6.0;
        const double extentMax = 30.0;
        if (ImGui::SliderScalar("Extent Scale", ImGuiDataType_Double, &state.cloud.extentScale, &extentMin, &extentMax)) {
            state.dirty.cloud = true;
        }
        if (ImGui::Checkbox("Build Volume", &state.cloud.buildVolume)) {
            state.dirty.cloud = true;
        }
        if (ImGui::Checkbox("Use Numerical Radial", &state.cloud.useNumericalRadial)) {
            state.dirty.cloud = true;
        }
        if (ImGui::Checkbox("Adaptive Sampling", &state.cloud.adaptiveSampling)) {
            state.dirty.cloud = true;
        }
        if (ImGui::BeginCombo("Sampling Quality", samplingQualityLabel(state.cloud.samplingQuality))) {
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

        if (ImGui::BeginCombo("Cloud Mode", cloudModeLabel(state.view.cloudRenderMode))) {
            for (const auto mode : {quantum::app::CloudRenderMode::PointCloud,
                                    quantum::app::CloudRenderMode::VolumeSlices,
                                    quantum::app::CloudRenderMode::Hybrid}) {
                if (ImGui::Selectable(cloudModeLabel(mode), mode == state.view.cloudRenderMode)) {
                    state.view.cloudRenderMode = mode;
                }
            }
            ImGui::EndCombo();
        }
        ImGui::Checkbox("Show Orbits", &state.view.showOrbitRings);
        ImGui::Checkbox("Show Grid", &state.view.showGrid);
        ImGui::SliderFloat("Point Size", &state.view.pointSize, 1.0f, 12.0f);
        ImGui::SliderFloat("Exposure", &state.view.exposure, 0.1f, 4.0f);

        ImGui::SeparatorText("Superposition");
        for (std::size_t index = 0; index < state.cloud.components.size(); ++index) {
            auto& component = state.cloud.components[index];
            ImGui::PushID(static_cast<int>(index));
            ImGui::Text("Component %d", static_cast<int>(index + 1));
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
            if (ImGui::SliderScalar("Amplitude", ImGuiDataType_Double, &component.amplitude, &amplitudeMin, &amplitudeMax)) {
                state.dirty.cloud = true;
            }
            const double phaseMin = -3.1415926535;
            const double phaseMax = 3.1415926535;
            if (ImGui::SliderScalar("Phase", ImGuiDataType_Double, &component.phaseRadians, &phaseMin, &phaseMax)) {
                state.dirty.cloud = true;
            }
            if (ImGui::Button("Remove") && state.cloud.components.size() > 1) {
                state.cloud.components.erase(state.cloud.components.begin() + static_cast<std::ptrdiff_t>(index));
                state.dirty.cloud = true;
                ImGui::PopID();
                break;
            }
            ImGui::Separator();
            ImGui::PopID();
        }
        if (state.cloud.components.size() < 3 && ImGui::Button("Add Component")) {
            state.cloud.components.push_back({{2, 1, 0}, 0.5, 0.0, state.derived.activeZeff});
            state.dirty.cloud = true;
        }

        ImGui::SeparatorText("Solver");
        if (ImGui::SliderInt("Solver n", &state.solver.qn.n, 1, 8)) {
            state.dirty.solver = true;
        }
        state.solver.qn.l = std::clamp(state.solver.qn.l, 0, std::max(0, state.solver.qn.n - 1));
        if (ImGui::SliderInt("Solver l", &state.solver.qn.l, 0, std::max(0, state.solver.qn.n - 1))) {
            state.dirty.solver = true;
        }
        if (ImGui::SliderInt("Grid Points", &state.solver.gridPoints, 512, 16384)) {
            state.dirty.solver = true;
        }
        if (ImGui::SliderInt("Convergence Passes", &state.solver.convergencePasses, 2, 6)) {
            state.dirty.solver = true;
        }
        const double radiusMin = 40.0;
        const double radiusMax = 200.0;
        if (ImGui::SliderScalar("Max Scaled Radius", ImGuiDataType_Double, &state.solver.maxScaledRadius, &radiusMin, &radiusMax)) {
            state.dirty.solver = true;
        }
    }
    ImGui::End();

    if (ImGui::Begin("Physics")) {
        ImGui::Text("Element: %s (%s)", state.derived.element.name.c_str(), state.derived.element.symbol.c_str());
        ImGui::Text("Z = %d, q = %d, electrons = %d", state.atomicNumber, state.chargeState, state.derived.electronCount);
        ImGui::Text("Configuration: %s", state.derived.configuration.notation.c_str());
        ImGui::Text("Active Z_eff = %.4f", state.derived.activeZeff);
        ImGui::Text("Shielding sigma = %.4f", state.derived.slater.shieldingSigma);
        ImGui::Separator();
        ImGui::Text("E_n = %.6f eV", state.derived.bohrMetrics.energyEv);
        ImGui::Text("r_n = %.6f nm", state.derived.bohrMetrics.orbitalRadiusNm);
        ImGui::Text("v_n = %.6e m/s", state.derived.bohrMetrics.orbitalSpeedMps);
        ImGui::Text("Reduced mass factor = %.8f", state.derived.bohrMetrics.reducedMassFactor);
        ImGui::Separator();
        ImGui::Text("Delta E = %.6f eV", state.derived.transition.deltaEnergyEv);
        ImGui::Text("f = %.6e Hz", state.derived.transition.frequencyHz);
        ImGui::Text("lambda = %.6f nm", state.derived.transition.wavelengthNm);
        ImGui::Text("Series = %s", state.derived.transition.seriesName.c_str());
        ImGui::Text("Allowed = %s", state.derived.transition.allowed ? "yes" : "no");
        if (!state.derived.transition.reason.empty()) {
            ImGui::TextWrapped("Rule note: %s", state.derived.transition.reason.c_str());
        }
        ImGui::Separator();
        ImGui::Text("Cloud normalization estimate = %.6f", state.derived.cloud.stats.normalizationEstimate);
        ImGui::Text("Cloud normalization error = %.6f", state.derived.cloud.stats.normalizationError);
        ImGui::Text("Sampling efficiency = %d / %d", state.derived.cloud.stats.acceptedCount, state.derived.cloud.stats.candidateCount);
        ImGui::Text("Candidate multiplier = %.2f", state.derived.cloud.stats.candidateMultiplier);
        ImGui::Text("CDF/Angle/MC = %d / %d / %d",
                    state.derived.cloud.stats.radialCdfSamples,
                    state.derived.cloud.stats.angularScanResolution,
                    state.derived.cloud.stats.monteCarloSamples);
        ImGui::Separator();
        ImGui::Text("Solver E = %.6f eV", state.derived.solver.energyEv);
        ImGui::Text("Solver |error| = %.6e eV", state.derived.solver.errorEv);
        ImGui::TextWrapped("Solver status: %s", state.derived.solver.message.c_str());
        if (!state.derived.approximationWarning.empty()) {
            ImGui::Separator();
            ImGui::TextWrapped("%s", state.derived.approximationWarning.c_str());
        }
    }
    ImGui::End();

    if (ImGui::Begin("Plots")) {
        drawEnergyLevelsPlot(state.derived.energyLevels);
        drawSpectrumPlot(state.derived.spectrumLines, state.derived.referenceLines, state.transition.enforceRules);

        std::vector<double> radii;
        std::vector<double> values;
        radii.reserve(state.derived.radialDistribution.size());
        values.reserve(state.derived.radialDistribution.size());
        for (const auto& sample : state.derived.radialDistribution) {
            radii.push_back(sample.first * 1.0e9);
            values.push_back(sample.second);
        }
        drawPolylinePlot("Radial Distribution", radii, values, IM_COL32(127, 196, 109, 230));

        std::vector<double> grids;
        std::vector<double> errors;
        grids.reserve(state.derived.solver.convergence.size());
        errors.reserve(state.derived.solver.convergence.size());
        for (const auto& sample : state.derived.solver.convergence) {
            grids.push_back(sample.gridStepMeters * 1.0e12);
            errors.push_back(sample.errorEv);
        }
        drawPolylinePlot("Solver Convergence (pm step)", grids, errors, IM_COL32(255, 153, 102, 230));
    }
    ImGui::End();

    if (ImGui::Begin("Performance")) {
        ImGui::Text("FPS: %.2f", performance.fps);
        ImGui::Text("Average frame: %.3f ms", performance.averageFrameMs);
        ImGui::Text("CPU frame: %.3f ms", performance.cpuFrameMs);
        if (performance.gpuTimersSupported) {
            ImGui::Text("GPU frame: %.3f ms", performance.gpuFrameMs);
            ImGui::Text("GPU point pass: %.3f ms", performance.gpuPointMs);
            ImGui::Text("GPU volume pass: %.3f ms", performance.gpuVolumeMs);
        } else {
            ImGui::TextUnformatted("GPU timers: unavailable on this driver/context");
        }
        ImGui::Text("Cloud build: %.3f ms", performance.cloudBuildMs);
        ImGui::Text("Volume build: %.3f ms", performance.volumeBuildMs);
        ImGui::Separator();
        ImGui::Text("LOD: %s", lodLabel(performance.lodLevel));
        ImGui::Text("Camera distance: %.2f", performance.cameraDistance);
        ImGui::Text("Requested/Accepted/Rendered: %d / %d / %d",
                    performance.requestedPointCount,
                    performance.pointCount,
                    performance.renderedPointCount);
        ImGui::Text("Candidates: %d", performance.candidateCount);
        ImGui::Text("Volume slices: %d", performance.volumeSliceCount);
        ImGui::Text("Adaptive multiplier: %.2f", performance.candidateMultiplier);
        ImGui::Text("CDF/Angle/MC: %d / %d / %d",
                    performance.radialCdfSamples,
                    performance.angularScanResolution,
                    performance.monteCarloSamples);
        ImGui::Separator();
        ImGui::Text("Point VBO: %s", formatBytes(performance.pointBufferBytes).c_str());
        ImGui::Text("Volume 3D tex: %s", formatBytes(performance.volumeTextureBytes).c_str());
        ImGui::Text("Framebuffer: %s", formatBytes(performance.framebufferBytes).c_str());
        ImGui::Text("Tracked GPU total: %s", formatBytes(performance.trackedGpuBytes).c_str());
    }
    ImGui::End();

    if (ImGui::Begin("Help")) {
        ImGui::TextWrapped("Mouse over Scene: left drag orbit, middle drag pan, wheel dolly.");
        ImGui::TextWrapped("Strict Hydrogenic mode is valid for one-electron systems such as H and He+.");
        ImGui::TextWrapped("Multi-electron mode uses Aufbau + Slater Z_eff and is labeled as an approximation.");
        if (ImGui::Button("Copy Demo Script")) {
            std::ostringstream stream;
            stream << "Element Z=" << state.atomicNumber << ", charge=" << state.chargeState
                   << ", model=" << modelLabel(state.modelKind)
                   << ", approx=" << approximationLabel(state.approximationMode)
                   << ", transition=(" << state.transition.initial.n << "," << state.transition.initial.l << "," << state.transition.initial.m
                   << ")->(" << state.transition.final.n << "," << state.transition.final.l << "," << state.transition.final.m << ")";
            ImGui::SetClipboardText(stream.str().c_str());
        }
    }
    ImGui::End();

    if (ImGui::Begin("Log")) {
        for (const auto& entry : logger.entries()) {
            const ImVec4 color = (entry.level == quantum::core::LogLevel::Error)
                                     ? ImVec4(0.92f, 0.23f, 0.17f, 1.0f)
                                     : (entry.level == quantum::core::LogLevel::Warning)
                                           ? ImVec4(0.95f, 0.72f, 0.16f, 1.0f)
                                           : ImVec4(0.75f, 0.80f, 0.86f, 1.0f);
            ImGui::TextColored(color, "%s", entry.message.c_str());
        }
    }
    ImGui::End();

    return result;
}

} // namespace quantum::ui
