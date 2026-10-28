#include "quantum/ui/AppUi.h"
#include "quantum/app/Application.h"
#include "quantum/ui/UiUtils.h"
#include "quantum/ui/UiStringKey.h"

#include <imgui.h>

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <string>

namespace quantum::ui {

void drawScenarioBrowser(quantum::app::SimulationState& state);

void drawInspectorContent(quantum::app::SimulationState& state,
                          const quantum::physics::ElementDatabase& elementDatabase,
                          bool& showAdvancedPanels) {
    static bool inspectorElementOpen_ = true;
    static bool inspectorPresetsOpen_ = true;
    static bool inspectorDemoOpen_ = true;
    static bool inspectorTransitionOpen_ = false;
    static bool inspectorCloudOpen_ = false;
    static bool inspectorAdvancedPhysicsOpen_ = false;

    ImGui::SetNextItemOpen(inspectorElementOpen_, ImGuiCond_Once);
    inspectorElementOpen_ = ImGui::CollapsingHeader("元素与模式", ImGuiTreeNodeFlags_DefaultOpen);
    if (inspectorElementOpen_) {
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

    if (isParamLockedGlobal(state, "chargeState")) ImGui::BeginDisabled();
    if (ImGui::SliderInt("电荷态", &state.chargeState, 0, std::max(0, state.atomicNumber - 1))) {
        markAllDirty(state);
    }
    if (isParamLockedGlobal(state, "chargeState")) ImGui::EndDisabled();

    if (isParamLockedGlobal(state, "modelKind")) ImGui::BeginDisabled();
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
    if (isParamLockedGlobal(state, "modelKind")) ImGui::EndDisabled();

    if (isParamLockedGlobal(state, "approximationMode")) ImGui::BeginDisabled();
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
    if (isParamLockedGlobal(state, "approximationMode")) ImGui::EndDisabled();
    }

    ImGui::SetNextItemOpen(inspectorPresetsOpen_, ImGuiCond_Once);
    inspectorPresetsOpen_ = ImGui::CollapsingHeader("场景预设", ImGuiTreeNodeFlags_DefaultOpen);
    if (inspectorPresetsOpen_) {
        drawScenarioBrowser(state);
    }

    ImGui::SetNextItemOpen(inspectorDemoOpen_, ImGuiCond_Once);
    inspectorDemoOpen_ = ImGui::CollapsingHeader("自动演示", ImGuiTreeNodeFlags_DefaultOpen);
    if (inspectorDemoOpen_) {
    }

    ImGui::SetNextItemOpen(inspectorTransitionOpen_, ImGuiCond_Once);
    inspectorTransitionOpen_ = ImGui::CollapsingHeader("跃迁与光谱");
    if (inspectorTransitionOpen_) {
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
    if (ImGui::Checkbox("超精细修正", &state.spectroscopy.applyHyperfine)) {
        state.dirty.physics = true;
    }
    if (state.spectroscopy.applyHyperfine) {
        const double spinMin = 0.5;
        const double spinMax = 4.5;
        if (ImGui::SliderScalar("核自旋 I", ImGuiDataType_Double, &state.spectroscopy.nuclearSpin, &spinMin, &spinMax, "%.1f")) {
            state.dirty.physics = true;
        }
        const double aMin = 0.0;
        const double aMax = 30.0;
        if (ImGui::SliderScalar("超精细 A (ueV)", ImGuiDataType_Double, &state.spectroscopy.hyperfineAConstantMicroEv, &aMin, &aMax, "%.3f")) {
            state.dirty.physics = true;
        }
    }
    }

    ImGui::SetNextItemOpen(inspectorCloudOpen_, ImGuiCond_Once);
    inspectorCloudOpen_ = ImGui::CollapsingHeader("云与外观");
    if (inspectorCloudOpen_) {
        ImGui::SeparatorText("概率云");
    if (ImGui::SliderInt("点数", &state.cloud.pointCount, 10000, 200000)) {
        state.dirty.cloud = true;
    }
    if (state.cloud.pointCount <= 20000 && !state.cloud.buildVolume) {
        ImGui::TextDisabled("适合实时交互和课堂演示");
    } else if (state.cloud.buildVolume) {
        ImGui::TextDisabled("包含体切片渲染，适合高质量出图");
    } else if (state.cloud.pointCount >= 100000) {
        ImGui::TextDisabled("高精度采样，适合静态分析和最终输出");
    } else {
        ImGui::TextDisabled("均衡性能与质量");
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
    ImGui::SeparatorText("渲染");
    if (ImGui::BeginCombo("色图", colorMapLabel(state.cloud.colorMap))) {
        for (const auto cm : {quantum::app::ColorMap::PhaseHue,
                              quantum::app::ColorMap::HeatMap,
                              quantum::app::ColorMap::BlueWhite,
                              quantum::app::ColorMap::Rainbow,
                              quantum::app::ColorMap::Grayscale}) {
            if (ImGui::Selectable(colorMapLabel(cm), cm == state.cloud.colorMap)) {
                state.cloud.colorMap = cm;
                state.dirty.cloud = true;
            }
        }
        ImGui::EndCombo();
    }
    if (ImGui::BeginCombo("着色依据", cloudColorModeLabel(state.cloud.cloudColorMode))) {
        for (const auto ccm : {quantum::app::CloudColorMode::ByPhase,
                                quantum::app::CloudColorMode::ByRadialDistance,
                                quantum::app::CloudColorMode::ByProbabilityDensity}) {
            if (ImGui::Selectable(cloudColorModeLabel(ccm), ccm == state.cloud.cloudColorMode)) {
                state.cloud.cloudColorMode = ccm;
                state.dirty.cloud = true;
            }
        }
        ImGui::EndCombo();
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
    }

    ImGui::SetNextItemOpen(inspectorAdvancedPhysicsOpen_, ImGuiCond_Once);
    inspectorAdvancedPhysicsOpen_ = ImGui::CollapsingHeader("高阶物理");
    if (inspectorAdvancedPhysicsOpen_) {
        ImGui::SeparatorText("核过程教学动画");
    ImGui::Checkbox("启用核过程演示", &state.nuclearAnimation.enabled);
    if (ImGui::BeginCombo("过程类型", nuclearAnimationModeLabel(state.nuclearAnimation.mode))) {
        for (const auto mode : {quantum::app::NuclearAnimationMode::ParticleZoo,
                                quantum::app::NuclearAnimationMode::FusionDT,
                                quantum::app::NuclearAnimationMode::FissionU235,
                                quantum::app::NuclearAnimationMode::AlphaDecay,
                                quantum::app::NuclearAnimationMode::BetaMinusDecay,
                                quantum::app::NuclearAnimationMode::BetaPlusDecay}) {
            if (ImGui::Selectable(nuclearAnimationModeLabel(mode), mode == state.nuclearAnimation.mode)) {
                state.nuclearAnimation.mode = mode;
            }
        }
        ImGui::EndCombo();
    }
    const double nuclearSpeedMin = 0.2;
    const double nuclearSpeedMax = 3.5;
    ImGui::SliderScalar("动画速度", ImGuiDataType_Double, &state.nuclearAnimation.speed, &nuclearSpeedMin, &nuclearSpeedMax, "%.2f");
    const double nuclearScaleMin = 0.4;
    const double nuclearScaleMax = 3.0;
    ImGui::SliderScalar("动画尺度", ImGuiDataType_Double, &state.nuclearAnimation.scale, &nuclearScaleMin, &nuclearScaleMax, "%.2f");
    ImGui::Checkbox("显示次级粒子", &state.nuclearAnimation.showSecondaryParticles);
    ImGui::Checkbox("显示粒子拖尾", &state.nuclearAnimation.showTrails);
    ImGui::SeparatorText("核结构近似");
    if (ImGui::SliderInt("结构 Z", &state.nuclearAnimation.structureAtomicNumber, 1, 118)) {
        state.dirty.physics = true;
    }
    if (ImGui::SliderInt("结构 A", &state.nuclearAnimation.structureMassNumber, 2, 320)) {
        state.dirty.physics = true;
    }
    if (ImGui::SliderInt("同位素链窗口", &state.nuclearAnimation.structureIsotopeWindow, 4, 40)) {
        state.dirty.physics = true;
    }

    if (state.nuclearAnimation.mode == quantum::app::NuclearAnimationMode::FusionDT) {
        const double fusionEnergyMin = 1.0;
        const double fusionEnergyMax = 300.0;
        if (ImGui::SliderScalar("D-T 碰撞能量 (keV)", ImGuiDataType_Double, &state.nuclearAnimation.fusionEnergyKev, &fusionEnergyMin, &fusionEnergyMax, "%.1f")) {
            state.dirty.physics = true;
        }
    } else if (state.nuclearAnimation.mode == quantum::app::NuclearAnimationMode::FissionU235) {
        const double neutronEnergyMin = 1.0e-4;
        const double neutronEnergyMax = 20.0;
        if (ImGui::SliderScalar("中子能量 (eV)", ImGuiDataType_Double, &state.nuclearAnimation.neutronEnergyEv, &neutronEnergyMin, &neutronEnergyMax, "%.4f")) {
            state.dirty.physics = true;
        }
        const double densityScaleMin = 0.1;
        const double densityScaleMax = 20.0;
        if (ImGui::SliderScalar("靶数密度 (×1e28 /m^3)", ImGuiDataType_Double, &state.nuclearAnimation.targetNumberDensityScale, &densityScaleMin, &densityScaleMax, "%.2f")) {
            state.dirty.physics = true;
        }
        const double slabMin = 0.01;
        const double slabMax = 50.0;
        if (ImGui::SliderScalar("靶厚度 (cm)", ImGuiDataType_Double, &state.nuclearAnimation.slabThicknessCm, &slabMin, &slabMax, "%.2f")) {
            state.dirty.physics = true;
        }
    } else if (state.nuclearAnimation.mode == quantum::app::NuclearAnimationMode::AlphaDecay ||
               state.nuclearAnimation.mode == quantum::app::NuclearAnimationMode::BetaMinusDecay ||
               state.nuclearAnimation.mode == quantum::app::NuclearAnimationMode::BetaPlusDecay) {
        const double halfLifeMin = 1.0e-6;
        const double halfLifeMax = 1.0e6;
        if (ImGui::SliderScalar("半衰期 (s)", ImGuiDataType_Double, &state.nuclearAnimation.decayHalfLifeSeconds, &halfLifeMin, &halfLifeMax, "%.6f")) {
            state.dirty.physics = true;
        }
        const double observationMin = 1.0e-6;
        const double observationMax = 1.0e7;
        if (ImGui::SliderScalar("观测窗口 (s)", ImGuiDataType_Double, &state.nuclearAnimation.decayObservationWindowSeconds, &observationMin, &observationMax, "%.6f")) {
            state.dirty.physics = true;
        }
    }
    ImGui::TextWrapped("说明：场景动画仍是教学可视化；下方物理与图表面板会提供简化核结构、截面、束流衰减和衰变率方程结果。");

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

    ImGui::SeparatorText("第二层：教学平均场");
    if (ImGui::Checkbox("启用教学 SCF", &state.meanField.enabled)) {
        state.dirty.physics = true;
    }
    if (state.meanField.enabled) {
        if (ImGui::BeginCombo("平均场模式", uiMeanFieldModeLabel(state.meanField.mode))) {
            for (const auto mode : {quantum::solvers::MeanFieldMode::HartreeScreened,
                                    quantum::solvers::MeanFieldMode::SlaterExchange,
                                    quantum::solvers::MeanFieldMode::ScalarRelativisticPreview}) {
                if (ImGui::Selectable(uiMeanFieldModeLabel(mode), mode == state.meanField.mode)) {
                    state.meanField.mode = mode;
                    state.dirty.physics = true;
                }
            }
            ImGui::EndCombo();
        }
        if (ImGui::SliderInt("SCF 最大迭代", &state.meanField.maxIterations, 6, 120)) {
            state.dirty.physics = true;
        }
        const double toleranceMin = 1.0e-8;
        const double toleranceMax = 1.0e-3;
        if (ImGui::SliderScalar("SCF 容差", ImGuiDataType_Double, &state.meanField.convergenceTolerance, &toleranceMin, &toleranceMax, "%.1e")) {
            state.dirty.physics = true;
        }
        if (ImGui::Checkbox("启用混合", &state.meanField.useMixing)) {
            state.dirty.physics = true;
        }
        if (state.meanField.useMixing) {
            const double mixMin = 0.1;
            const double mixMax = 0.9;
            if (ImGui::SliderScalar("SCF 混合系数", ImGuiDataType_Double,
                                    &state.meanField.mixingAlpha, &mixMin, &mixMax, "%.2f")) {
                state.dirty.physics = true;
            }
        }
    }

    ImGui::SeparatorText("第四层：有限基关联");
    if (ImGui::Checkbox("启用双组态 CI", &state.correlation.enabled)) {
        state.dirty.physics = true;
    }
    if (state.correlation.enabled) {
        const double couplingMin = 0.01;
        const double couplingMax = 5.0;
        if (ImGui::SliderScalar("组态耦合 (eV)", ImGuiDataType_Double, &state.correlation.couplingStrengthEv, &couplingMin, &couplingMax, "%.3f")) {
            state.dirty.physics = true;
        }
    }

    ImGui::SeparatorText("第五层：有限基时间动力学");
    if (ImGui::Checkbox("启用二能级 TDSE", &state.dynamics.enabled)) {
        state.dirty.physics = true;
    }
    if (state.dynamics.enabled) {
        if (ImGui::Checkbox("同步到云相位", &state.dynamics.syncCloudPhase)) {
            state.dirty.physics = true;
        }
        const double totalMin = 1.0e-16;
        const double totalMax = 2.0e-12;
        if (ImGui::SliderScalar("总时间 (s)", ImGuiDataType_Double, &state.dynamics.totalTimeSeconds, &totalMin, &totalMax, "%.2e")) {
            state.dirty.physics = true;
        }
        const double stepMin = 1.0e-18;
        const double stepMax = 5.0e-14;
        if (ImGui::SliderScalar("时间步 (s)", ImGuiDataType_Double, &state.dynamics.timeStepSeconds, &stepMin, &stepMax, "%.2e")) {
            state.dirty.physics = true;
        }
        const double couplingMin = 0.0;
        const double couplingMax = 2.0;
        if (ImGui::SliderScalar("驱动耦合 (eV)", ImGuiDataType_Double, &state.dynamics.couplingEv, &couplingMin, &couplingMax, "%.3f")) {
            state.dirty.physics = true;
        }
        const double detuningMin = -2.0;
        const double detuningMax = 2.0;
        if (ImGui::SliderScalar("失谐 (eV)", ImGuiDataType_Double, &state.dynamics.detuningEv, &detuningMin, &detuningMax, "%.3f")) {
            state.dirty.physics = true;
        }
        if (ImGui::Checkbox("加入阻尼包络", &state.dynamics.includeDissipation)) {
            state.dirty.physics = true;
        }
        if (state.dynamics.includeDissipation) {
            const double dampingMin = 0.0;
            const double dampingMax = 2.0e15;
            if (ImGui::SliderScalar("阻尼率 (1/s)", ImGuiDataType_Double, &state.dynamics.dampingRatePerSecond, &dampingMin, &dampingMax, "%.2e")) {
                state.dirty.physics = true;
            }
        }
    }
    }

    ImGui::Separator();
    if (ImGui::Button(showAdvancedPanels ? "收起高级面板" : "展开高级面板")) {
        showAdvancedPanels = !showAdvancedPanels;
    }
}

} // namespace quantum::ui