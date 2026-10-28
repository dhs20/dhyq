#pragma once

#include "quantum/app/SimulationState.h"

namespace quantum::ui {

constexpr const char* kPanelScene = "场景";
constexpr const char* kPanelInspector = "控制面板";
constexpr const char* kPanelPhysics = "物理信息";
constexpr const char* kPanelPlots = "图表";
constexpr const char* kPanelPerformance = "性能监视";
constexpr const char* kPanelHelp = "帮助";
constexpr const char* kPanelLog = "日志";

inline const char* modelLabel(quantum::app::ModelKind kind) {
    switch (kind) {
    case quantum::app::ModelKind::Bohr:        return "玻尔模型";
    case quantum::app::ModelKind::WaveParticle: return "波粒演示";
    case quantum::app::ModelKind::SchrodingerCloud: return "薛定谔概率云";
    case quantum::app::ModelKind::Compare:      return "对比模式";
    default: return "未知";
    }
}

inline const char* approximationLabel(quantum::physics::ApproximationMode mode) {
    switch (mode) {
    case quantum::physics::ApproximationMode::StrictHydrogenic: return "严格氢样";
    case quantum::physics::ApproximationMode::MultiElectronApproximation: return "多电子近似";
    default: return "未知";
    }
}

inline const char* cloudModeLabel(quantum::app::CloudRenderMode mode) {
    switch (mode) {
    case quantum::app::CloudRenderMode::PointCloud:   return "点云";
    case quantum::app::CloudRenderMode::VolumeSlices: return "体切片";
    case quantum::app::CloudRenderMode::Hybrid:       return "混合";
    default: return "未知";
    }
}

inline const char* colorMapLabel(quantum::app::ColorMap colorMap) {
    switch (colorMap) {
    case quantum::app::ColorMap::PhaseHue:   return "相位色相";
    case quantum::app::ColorMap::HeatMap:    return "热力图";
    case quantum::app::ColorMap::BlueWhite:  return "蓝白";
    case quantum::app::ColorMap::Rainbow:    return "彩虹";
    case quantum::app::ColorMap::Grayscale:  return "灰度";
    default: return "未知";
    }
}

inline const char* cloudColorModeLabel(quantum::app::CloudColorMode colorMode) {
    switch (colorMode) {
    case quantum::app::CloudColorMode::ByPhase:             return "按相位着色";
    case quantum::app::CloudColorMode::ByRadialDistance:    return "按径向距离";
    case quantum::app::CloudColorMode::ByProbabilityDensity: return "按概率密度";
    default: return "未知";
    }
}

inline void markAllDirty(quantum::app::SimulationState& state) {
    state.dirty.physics = true;
    state.dirty.cloud = true;
    state.dirty.solver = true;
    state.dirty.reference = true;
}

inline void applyModelPreset(quantum::app::SimulationState& state, quantum::app::ModelKind kind) {
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

inline const char* samplingQualityLabel(quantum::app::SamplingQualityPreset preset) {
    switch (preset) {
    case quantum::app::SamplingQualityPreset::Fast:        return "快速";
    case quantum::app::SamplingQualityPreset::Balanced:    return "均衡";
    case quantum::app::SamplingQualityPreset::HighFidelity: return "高精度";
    default: return "未知";
    }
}

inline const char* uiMeanFieldModeLabel(quantum::solvers::MeanFieldMode mode) {
    switch (mode) {
    case quantum::solvers::MeanFieldMode::SlaterExchange:           return "Slater 交换平均场";
    case quantum::solvers::MeanFieldMode::ScalarRelativisticPreview: return "标量相对论预览";
    case quantum::solvers::MeanFieldMode::HartreeScreened:
    default: return "屏蔽 Hartree 教学 SCF";
    }
}

inline const char* nuclearAnimationModeLabel(quantum::app::NuclearAnimationMode mode) {
    switch (mode) {
    case quantum::app::NuclearAnimationMode::ParticleZoo:     return "基本粒子展示";
    case quantum::app::NuclearAnimationMode::FusionDT:        return "聚变示意（D+T）";
    case quantum::app::NuclearAnimationMode::FissionU235:     return "裂变示意（U-235）";
    case quantum::app::NuclearAnimationMode::AlphaDecay:      return "α 衰变示意";
    case quantum::app::NuclearAnimationMode::BetaMinusDecay:  return "β- 衰变示意";
    case quantum::app::NuclearAnimationMode::BetaPlusDecay:   return "β+ 衰变示意";
    case quantum::app::NuclearAnimationMode::None:
    default: return "关闭";
    }
}

inline std::string localizedElementName(const quantum::physics::ElementRecord& element) {
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

} // namespace quantum::ui