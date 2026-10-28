#include "quantum/ui/AppUi.h"

#include <imgui.h>

#include <string>
#include <string_view>
#include <vector>

#include "quantum/app/Application.h"
#include "quantum/ui/UiUtils.h"
#include "quantum/ui/UiStringKey.h"

namespace quantum::ui {
namespace {

void applyScenePresetToState(quantum::app::SimulationState& state,
                             const quantum::app::DemoScriptStep& step) {
    state.demo.enabled = false;
    state.demo.currentStepName = step.name;
    state.demo.stepProgress = 0.0;
    state.atomicNumber = step.atomicNumber;
    state.chargeState = step.chargeState;
    state.approximationMode = step.approximationMode;
    applyModelPreset(state, step.modelKind);
    state.cloud.components = step.components;
    state.solver.qn = step.solverQn;
    state.cloud.buildVolume = step.buildVolume;
    state.cloud.useNumericalRadial = step.useNumericalRadial;
    state.bohr.principalN = step.principalN;
    state.bohr.maxSpectrumN = step.maxSpectrumN;
    state.transition = step.transition;
    if (step.category == "nuclear") {
        if (step.name.find("聚变") != std::string::npos) {
            state.nuclearAnimation.enabled = true;
            state.nuclearAnimation.mode = quantum::app::NuclearAnimationMode::FusionDT;
        } else if (step.name.find("裂变") != std::string::npos) {
            state.nuclearAnimation.enabled = true;
            state.nuclearAnimation.mode = quantum::app::NuclearAnimationMode::FissionU235;
        } else if (step.name.find("衰变") != std::string::npos) {
            state.nuclearAnimation.enabled = true;
            state.nuclearAnimation.mode = quantum::app::NuclearAnimationMode::AlphaDecay;
        } else {
            state.nuclearAnimation.enabled = false;
        }
    } else {
        state.nuclearAnimation.enabled = false;
    }
    markAllDirty(state);
}

const std::vector<quantum::app::DemoScriptStep>& getSceneBrowserPresets() {
    using namespace quantum::app;
    using namespace quantum::physics;

    static const std::vector<DemoScriptStep> kPresets = {
        {"氢原子玻尔谱线", 1, 0, ApproximationMode::StrictHydrogenic,
         ModelKind::Bohr, CloudRenderMode::PointCloud,
         {2, 1, 0}, {{{2, 1, 0}, 1.0, 0.0, 1.0}},
         false, false, 2, 6,
         "hydrogen", "氢原子玻尔轨道能级与跃迁",
         {{2, 1, 0}, {1, 0, 0}, true, false}},
        {"氦离子相位云", 2, 1, ApproximationMode::StrictHydrogenic,
         ModelKind::SchrodingerCloud, CloudRenderMode::Hybrid,
         {3, 2, 0}, {{{3, 2, 0}, 1.0, 0.0, 2.0}},
         true, true, 3, 7,
         "hydrogen", "类氢离子的 Z^2 缩放效应与相位云",
         {{3, 2, 0}, {2, 1, 0}, true, false}},
        {"氢原子薛定谔概率云", 1, 0, ApproximationMode::StrictHydrogenic,
         ModelKind::SchrodingerCloud, CloudRenderMode::Hybrid,
         {2, 0, 0}, {{{2, 0, 0}, 1.0, 0.0, 1.0}},
         false, true, 2, 5,
         "hydrogen", "1s/2p/3d 电子概率云形状对比演示",
         {{2, 0, 0}, {1, 0, 0}, true, false}},
        {"氢原子叠加态", 1, 0, ApproximationMode::StrictHydrogenic,
         ModelKind::SchrodingerCloud, CloudRenderMode::PointCloud,
         {2, 0, 0}, {{{2, 0, 0}, 0.7, 0.0, 1.0}, {{2, 1, 0}, 0.5, 1.57, 1.0}},
         false, false, 2, 5,
         "hydrogen", "2s+2p 叠加态量子相位变化",
         {{2, 1, 0}, {2, 0, 0}, false, false}},
        {"氖原子多电子近似", 10, 0, ApproximationMode::MultiElectronApproximation,
         ModelKind::Compare, CloudRenderMode::Hybrid,
         {2, 1, 0}, {{{2, 1, 0}, 0.8, 0.0, 1.0}, {{2, 1, 1}, 0.6, 1.57, 1.0}},
         false, true, 2, 6,
         "multielectron", "氖原子多电子教学近似与相位云",
         {{3, 1, 0}, {2, 0, 0}, true, false}},
        {"锂到氖组态趋势", 3, 0, ApproximationMode::MultiElectronApproximation,
         ModelKind::Compare, CloudRenderMode::Hybrid,
         {2, 1, 0}, {{{2, 1, 0}, 1.0, 0.0, 1.0}},
         false, true, 2, 6,
         "multielectron", "Z=3-10 的 Aufbau 填充与 Z_eff 趋势",
         {{2, 1, 0}, {1, 0, 0}, true, false}},
        {"钠到氩周期趋势", 11, 0, ApproximationMode::MultiElectronApproximation,
         ModelKind::Compare, CloudRenderMode::Hybrid,
         {3, 0, 0}, {{{3, 0, 0}, 1.0, 0.0, 1.0}},
         false, true, 3, 7,
         "multielectron", "Z=11-18 的屏蔽效应与 Z_eff 周期趋势",
         {{3, 1, 0}, {3, 0, 0}, true, false}},
        {"氢原子谱线系", 1, 0, ApproximationMode::StrictHydrogenic,
         ModelKind::Bohr, CloudRenderMode::PointCloud,
         {4, 1, 0}, {{{4, 1, 0}, 1.0, 0.0, 1.0}},
         false, false, 4, 8,
         "spectroscopy", "Lyman/Balmer/Paschen 三条主要谱线系展示",
         {{4, 1, 0}, {3, 2, 0}, true, false}},
        {"精细结构拆分", 1, 0, ApproximationMode::StrictHydrogenic,
         ModelKind::Bohr, CloudRenderMode::PointCloud,
         {2, 1, 0}, {{{2, 1, 0}, 1.0, 0.0, 1.0}},
         false, false, 2, 6,
         "spectroscopy", "2p(3/2) 与 2p(1/2) 能级的自旋-轨道分裂",
         {{2, 1, 0}, {1, 0, 0}, true, false}},
        {"Zeeman 效应", 1, 0, ApproximationMode::StrictHydrogenic,
         ModelKind::Bohr, CloudRenderMode::PointCloud,
         {2, 1, 0}, {{{2, 1, 0}, 1.0, 0.0, 1.0}},
         false, false, 2, 6,
         "spectroscopy", "磁场下能级的 Zeeman 分裂",
         {{2, 1, 0}, {1, 0, 0}, true, true}},
        {"D-T 聚变演示", 1, 0, ApproximationMode::StrictHydrogenic,
         ModelKind::Bohr, CloudRenderMode::PointCloud,
         {2, 0, 0}, {{{2, 0, 0}, 1.0, 0.0, 1.0}},
         false, false, 2, 4,
         "nuclear", "D+T 聚变截面随入射能量变化",
         {{2, 0, 0}, {1, 0, 0}, true, false}},
        {"α 衰变演示", 92, 0, ApproximationMode::MultiElectronApproximation,
         ModelKind::Bohr, CloudRenderMode::PointCloud,
         {2, 0, 0}, {{{2, 0, 0}, 1.0, 0.0, 1.0}},
         false, false, 2, 4,
         "nuclear", "α 衰变统计规律与半衰期",
         {{2, 1, 0}, {1, 0, 0}, true, false}},
    };
    return kPresets;
}

int sceneCategoryIndex(const std::string& category) {
    if (category == "multielectron") return 2;
    if (category == "spectroscopy") return 3;
    if (category == "nuclear") return 4;
    return 1;
}

} // namespace

void drawScenarioBrowser(quantum::app::SimulationState& state) {
    static int activeCategory = 0;
    const char* categoryLabels[] = {
        uiStr(UiStringKey::SceneBrowser_All),
        uiStr(UiStringKey::SceneBrowser_Hydrogen),
        uiStr(UiStringKey::SceneBrowser_MultiElectron),
        uiStr(UiStringKey::SceneBrowser_Spectroscopy),
        uiStr(UiStringKey::SceneBrowser_Nuclear),
    };

    for (int i = 0; i < 5; ++i) {
        if (i > 0) ImGui::SameLine();
        bool isActive = (activeCategory == i);
        if (isActive) {
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(72, 134, 212, 200));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(92, 154, 232, 220));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(52, 114, 192, 255));
        }
        if (ImGui::SmallButton(categoryLabels[i])) {
            activeCategory = i;
        }
        if (isActive) {
            ImGui::PopStyleColor(3);
        }
    }

    ImGui::Spacing();

    const auto& steps = getSceneBrowserPresets();
    int column = 0;
    for (const auto& step : steps) {
        const int stepCat = sceneCategoryIndex(step.category);
        if (activeCategory != 0 && stepCat != activeCategory) continue;

        if (column % 2 == 1) {
            ImGui::SameLine();
        }

        ImGui::PushID(step.name.c_str());
        const float availWidth = ImGui::GetContentRegionAvail().x;
        const float cardWidth = availWidth * 0.48f;
        const float cardHeight = 78.0f;
        ImGui::BeginChild("##card", ImVec2(cardWidth, cardHeight), true, ImGuiWindowFlags_NoScrollbar);

        ImGui::TextUnformatted(step.name.c_str());
        if (!step.description.empty()) {
            ImGui::TextDisabled("%s", step.description.c_str());
        }

        const float buttonHeight = ImGui::GetFrameHeight();
        ImGui::SetCursorPosY(cardHeight - buttonHeight - 8.0f);
        if (ImGui::SmallButton("切换")) {
            applyScenePresetToState(state, step);
        }
        ImGui::EndChild();
        ImGui::PopID();

        if (column % 2 == 1) {
            ImGui::NewLine();
        }
        ++column;
    }
}

} // namespace quantum::ui