#include "quantum/ui/AppUi.h"
#include "quantum/ui/UiUtils.h"
#include "quantum/ui/UiStringKey.h"

#include <imgui.h>

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

namespace quantum::ui {
namespace {

struct LearningStep {
    std::string title;
    std::string guide;
    int atomicNumber = 1;
    int chargeState = 0;
    quantum::physics::ApproximationMode approximation =
        quantum::physics::ApproximationMode::StrictHydrogenic;
    quantum::app::ModelKind modelKind = quantum::app::ModelKind::Bohr;
    quantum::physics::QuantumNumbers solverQn{2, 0, 0};
    int principalN = 2;
    int maxSpectrumN = 5;
    std::vector<std::string> lockedParams;
};

const std::vector<LearningStep>& getLearningPathSteps(quantum::app::LearningPath path) {
    static const std::unordered_map<quantum::app::LearningPath, std::vector<LearningStep>> paths = {
        {quantum::app::LearningPath::AtomicStructure, {
            {"\u6C22\u539F\u5B50\u73BB\u5C14\u8F68\u9053", "\u9009\u62E9\u4E0D\u540C\u7684\u4E3B\u91CF\u5B50\u6570 n (1-4)\uFF0C\u89C2\u5BDF\u8F68\u9053\u534A\u5F84 n\u00B2 \u7684\u53D8\u5316\u89C4\u5F8B\u3002\u8F68\u9053\u534A\u5F84 \u221D n\u00B2\u3002",
             1, 0, quantum::physics::ApproximationMode::StrictHydrogenic,
             quantum::app::ModelKind::Bohr, {2, 1, 0}, 2, 5,
             {"chargeState", "approximationMode", "meanField", "correlation", "dynamics", "nuclearAnimation"}},
            {"\u6C22\u539F\u5B50\u859B\u5B9A\u8C2E\u6982\u7387\u4E91", "\u5207\u6362\u5230\u6982\u7387\u4E91\uFF0C\u8C03\u6574\u89D2\u91CF\u5B50\u6570 l\uFF0C\u5BF9\u6BD4\u4E0D\u540C\u8F68\u9053\u5F62\u72B6\u3002l=0 \u7403\u5F62\uFF0Cl=1 \u54D1\u94C3\u5F62\u3002",
             1, 0, quantum::physics::ApproximationMode::StrictHydrogenic,
             quantum::app::ModelKind::SchrodingerCloud, {2, 0, 0}, 2, 5,
             {"chargeState", "approximationMode", "meanField", "correlation", "dynamics", "nuclearAnimation"}},
            {"\u6C22\u539F\u5B50\u53E0\u52A0\u6001", "\u6DFB\u52A0\u53E0\u52A0\u5206\u91CF\uFF0C\u8C03\u6574\u632F\u5E45\u548C\u76F8\u4F4D\u3002\u89C2\u5BDF\u53E0\u52A0\u6001\u4E91\u7684\u76F8\u4F4D\u53D8\u5316\u3002",
             1, 0, quantum::physics::ApproximationMode::StrictHydrogenic,
             quantum::app::ModelKind::SchrodingerCloud, {2, 0, 0}, 2, 5,
             {"chargeState", "approximationMode", "meanField", "correlation", "dynamics", "nuclearAnimation"}},
            {"He+ \u79BB\u5B50\u76F8\u4F4D\u4E91", "\u5207\u6362\u4E3A He+\uFF0C\u5BF9\u6BD4 Z=2 \u65F6\u8F68\u9053\u534A\u5F84\u7F29\u5C0F\u4E3A 1/4\uFF0C\u80FD\u7EA7\u6309 Z\u00B2 \u7F29\u653E\u3002",
             2, 1, quantum::physics::ApproximationMode::StrictHydrogenic,
             quantum::app::ModelKind::SchrodingerCloud, {3, 2, 0}, 3, 6,
             {"approximationMode", "meanField", "correlation", "dynamics", "nuclearAnimation"}},
        }},
        {quantum::app::LearningPath::Spectroscopy, {
            {"\u6C22\u539F\u5B50\u8C31\u7EBF\u7CFB", "\u8BBE\u7F6E\u521D\u6001 n=4\u3001\u672B\u6001 n=3\uFF0C\u89C2\u5BDF\u6B64\u8DC3\u8FC1\u5C5E\u4E8E\u54EA\u6761\u8C31\u7EBF\u7CFB\u3002",
             1, 0, quantum::physics::ApproximationMode::StrictHydrogenic,
             quantum::app::ModelKind::Bohr, {4, 1, 0}, 4, 8,
             {"chargeState", "approximationMode", "meanField", "correlation", "dynamics", "nuclearAnimation", "modelKind"}},
            {"\u7CBE\u7EC6\u7ED3\u6784\u62C6\u5206", "\u5728\u8C31\u5B66\u9762\u677F\u5F00\u542F\u7CBE\u7EC6\u7ED3\u6784\u3002\u89C2\u5BDF 2p \u80FD\u7EA7\u56E0\u81EA\u65CB-\u8F68\u9053\u8026\u5408\u5206\u88C2\u4E3A 2p\u2083/\u2082 \u548C 2p\u2081/\u2082\u3002",
             1, 0, quantum::physics::ApproximationMode::StrictHydrogenic,
             quantum::app::ModelKind::Bohr, {2, 1, 0}, 2, 6,
             {"chargeState", "approximationMode", "meanField", "correlation", "dynamics", "nuclearAnimation", "modelKind"}},
            {"Zeeman \u6548\u5E94", "\u5F00\u542F Zeeman \u4FEE\u6B63\u548C\u9650\u5236 \u0394m\u3002\u8C03\u8282\u78C1\u573A\u5F3A\u5EA6\u89C2\u5BDF\u8C31\u7EBF\u5206\u88C2\u3002",
             1, 0, quantum::physics::ApproximationMode::StrictHydrogenic,
             quantum::app::ModelKind::Bohr, {2, 1, 0}, 2, 6,
             {"chargeState", "approximationMode", "meanField", "correlation", "dynamics", "nuclearAnimation", "modelKind"}},
        }},
        {quantum::app::LearningPath::MultiElectron, {
            {"Aufbau \u586B\u5145\u89C4\u5219", "\u4ECE Li(Z=3) \u5F00\u59CB\uFF0C\u89C2\u5BDF 1s\u21922s\u21922p \u7684\u586B\u5145\u987A\u5E8F\u3002Pauli \u4E0D\u76F8\u5BB9\u539F\u7406\u51B3\u5B9A\u586B\u5145\u3002",
             3, 0, quantum::physics::ApproximationMode::MultiElectronApproximation,
             quantum::app::ModelKind::Compare, {2, 1, 0}, 2, 6,
             {"approximationMode", "meanField", "correlation", "dynamics", "nuclearAnimation"}},
            {"Z_eff \u5C4F\u853D\u6548\u5E94", "\u4ECE Z=11 \u5230 Z=18 \u626B\u63CF\u3002\u89C2\u5BDF\u6709\u6548\u6838\u7535\u8377 Z_eff \u5C0F\u4E8E\u5B9E\u9645 Z\u3002",
             11, 0, quantum::physics::ApproximationMode::MultiElectronApproximation,
             quantum::app::ModelKind::Compare, {3, 0, 0}, 3, 7,
             {"approximationMode", "meanField", "correlation", "dynamics", "nuclearAnimation"}},
            {"\u6C16\u539F\u5B50\u591A\u7535\u5B50\u8FD1\u4F3C", "\u67E5\u770B Ne(Z=10) \u7684\u5B8C\u6574\u7EC4\u6001 [He]2s\u00B22p\u2076\u3002\u4E3A\u4EC0\u4E48 Ne \u662F\u60F0\u6027\u6C14\u4F53\uFF1F",
             10, 0, quantum::physics::ApproximationMode::MultiElectronApproximation,
             quantum::app::ModelKind::Compare, {2, 1, 0}, 2, 6,
             {"approximationMode", "meanField", "correlation", "dynamics", "nuclearAnimation"}},
        }},
        {quantum::app::LearningPath::NuclearPhysics, {
            {"\u8BA4\u8BC6\u57FA\u672C\u7C92\u5B50", "\u6D4F\u89C8\u57FA\u672C\u7C92\u5B50\uFF08\u8D28\u5B50\u3001\u4E2D\u5B50\u3001\u7535\u5B50\u3001\u5149\u5B50\u3001\u03BC\u5B50\u3001\u03C0\u4ECB\u5B50\u7B49\uFF09\u3002",
             1, 0, quantum::physics::ApproximationMode::StrictHydrogenic,
             quantum::app::ModelKind::Bohr, {2, 0, 0}, 2, 4,
             {"approximationMode", "meanField", "correlation", "dynamics", "chargeState"}},
            {"D-T \u805A\u53D8", "D + T \u2192 \u2074He + n\u3002\u8C03\u8282\u5165\u5C04\u80FD\u91CF\u89C2\u5BDF\u622A\u9762\u53D8\u5316\u3002",
             1, 0, quantum::physics::ApproximationMode::StrictHydrogenic,
             quantum::app::ModelKind::Bohr, {2, 0, 0}, 2, 4,
             {"approximationMode", "meanField", "correlation", "dynamics", "chargeState", "modelKind", "cloud"}},
            {"U-235 \u88C2\u53D8", "n + \u00B2\u00B3\u2075U \u2192 \u88C2\u53D8\u788E\u7247 + \u03BDn\u3002\u89C2\u5BDF\u4E2D\u5B50\u8865\u51CF\u548C\u94FE\u5F0F\u53CD\u5E94\u3002",
             92, 0, quantum::physics::ApproximationMode::MultiElectronApproximation,
             quantum::app::ModelKind::Bohr, {2, 0, 0}, 2, 4,
             {"approximationMode", "meanField", "correlation", "dynamics", "chargeState", "modelKind", "cloud"}},
            {"\u03B1 \u8870\u53D8", "T\u2081/\u2082 = 1s\u3002\u89C2\u5BDF\u6307\u6570\u8870\u53D8 N(t) = N\u2080e^{-\\lambda t}\u3002",
             92, 0, quantum::physics::ApproximationMode::MultiElectronApproximation,
             quantum::app::ModelKind::Bohr, {2, 0, 0}, 2, 4,
             {"approximationMode", "meanField", "correlation", "dynamics", "chargeState", "modelKind", "cloud"}},
        }}
    };
    auto it = paths.find(path);
    if (it != paths.end()) return it->second;
    static const std::vector<LearningStep> empty;
    return empty;
}

bool isParamLocked(const quantum::app::SimulationState& state, const std::string& key) {
    if (!state.learning.enabled) return false;
    const auto& steps = getLearningPathSteps(state.learning.path);
    if (state.learning.stepIndex >= static_cast<int>(steps.size())) return false;
    const auto& locked = steps[state.learning.stepIndex].lockedParams;
    return std::find(locked.begin(), locked.end(), key) != locked.end();
}

void applyLearningStep(quantum::app::SimulationState& state, const LearningStep& step) {
    state.learning.guideText = step.guide;
    state.atomicNumber = step.atomicNumber;
    state.chargeState = step.chargeState;
    state.approximationMode = step.approximation;
    state.modelKind = step.modelKind;
    state.solver.qn = step.solverQn;
    state.bohr.principalN = step.principalN;
    state.bohr.maxSpectrumN = step.maxSpectrumN;
    markAllDirty(state);
}

} // namespace

void drawLearningPanel(quantum::app::SimulationState& state) {
    const char* pathNames[] = {
        uiStr(UiStringKey::Learning_Off),
        uiStr(UiStringKey::Learning_AtomicStructure),
        uiStr(UiStringKey::Learning_Spectroscopy),
        uiStr(UiStringKey::Learning_MultiElectron),
        uiStr(UiStringKey::Learning_NuclearPhysics)
    };
    int currentPath = static_cast<int>(state.learning.path);
    if (ImGui::BeginCombo(uiStr(UiStringKey::Learning_Mode), pathNames[currentPath])) {
        for (int i = 0; i < 5; ++i) {
            if (ImGui::Selectable(pathNames[i], i == currentPath)) {
                state.learning.path = static_cast<quantum::app::LearningPath>(i);
                state.learning.enabled = (i != 0);
                state.learning.stepIndex = 0;
                if (state.learning.enabled) {
                    const auto& steps = getLearningPathSteps(state.learning.path);
                    if (!steps.empty()) {
                        applyLearningStep(state, steps[0]);
                    }
                }
            }
        }
        ImGui::EndCombo();
    }

    if (state.learning.enabled) {
        const auto& steps = getLearningPathSteps(state.learning.path);
        int totalSteps = static_cast<int>(steps.size());
        int currentStepNum = state.learning.stepIndex + 1;

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f),
                           uiStr(UiStringKey::Learning_StepCount), currentStepNum, totalSteps);

        float progress = totalSteps > 0 ? static_cast<float>(currentStepNum) / static_cast<float>(totalSteps) : 0.0f;
        ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f));

        if (state.learning.stepIndex < totalSteps) {
            ImGui::TextUnformatted(steps[static_cast<std::size_t>(state.learning.stepIndex)].title.c_str());
        }

        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(220, 224, 232, 255));
        ImGui::TextWrapped("%s", state.learning.guideText.c_str());
        ImGui::PopStyleColor();

        if (state.learning.stepIndex > 0) {
            if (ImGui::Button(uiStr(UiStringKey::Learning_PreviousStep))) {
                --state.learning.stepIndex;
                applyLearningStep(state, steps[static_cast<std::size_t>(state.learning.stepIndex)]);
            }
            ImGui::SameLine();
        }
        if (state.learning.stepIndex < totalSteps - 1) {
            if (ImGui::Button(uiStr(UiStringKey::Learning_NextStep))) {
                ++state.learning.stepIndex;
                applyLearningStep(state, steps[static_cast<std::size_t>(state.learning.stepIndex)]);
            }
        }
        ImGui::Separator();
    }
}

bool isParamLockedGlobal(const quantum::app::SimulationState& state, const std::string& key) {
    return isParamLocked(state, key);
}

} // namespace quantum::ui