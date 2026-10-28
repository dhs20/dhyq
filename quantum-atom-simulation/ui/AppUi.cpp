#include "quantum/ui/AppUi.h"

#include <imgui.h>
#include <imgui_internal.h>

#include <glm/common.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "quantum/ui/UiUtils.h"

namespace quantum::ui {
namespace {



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
    if (phrase == "Hydrogenic spectral record engine") return "氢样谱学记录引擎";
    if (phrase == "Tier 2 teaching mean-field prototype") return "第二层教学平均场原型";
    if (phrase == "Tier 4 finite-basis teaching CI prototype") return "第四层有限基 CI 教学原型";
    if (phrase == "Tier 5 finite-basis two-level TDSE teaching solver") return "第五层有限基二能级 TDSE 教学求解器";
    if (phrase == "screened Hartree-style scalar SCF teaching iteration") return "屏蔽 Hartree 风格标量 SCF 教学迭代";
    if (phrase == "screened Hartree-style SCF with Slater-exchange teaching correction") return "带 Slater 交换教学修正的屏蔽 SCF";
    if (phrase == "screened Hartree-style SCF with scalar-relativistic preview correction") return "带标量相对论预览修正的屏蔽 SCF";
    if (phrase == "two-configuration mixing over mean-field orbital estimates") return "基于平均场轨道估计的双组态混合";
    if (phrase == "closed-form Rabi dynamics in a two-state basis") return "二态基中的闭式 Rabi 动力学";
    if (phrase == "Pedagogical nuclear-process animation") return "教学型核过程动画";
    if (phrase == "Semi-empirical liquid-drop binding estimate") return "半经验液滴模型结合能估计";
    if (phrase == "Weizsacker semi-empirical mass formula without shell corrections") return "Weizsacker 半经验质量公式，不含壳层修正";
    if (phrase == "Qualitative nuclear-structure trend model; not a shell-model or ab initio calculation.") {
        return "用于定性核结构趋势，不是壳层模型或从头算核结构计算。";
    }
    if (phrase == "Gamow-like D-T fusion cross-section estimate") return "Gamow 型 D-T 聚变截面估计";
    if (phrase == "Barrier-penetration-inspired phenomenological fit") return "受穿透势垒启发的现象学拟合";
    if (phrase == "Qualitative D-T fusion cross-section curve; not a Bosch-Hale benchmark implementation.") {
        return "用于定性展示 D-T 聚变截面曲线，不是 Bosch-Hale 基准实现。";
    }
    if (phrase == "Thermal U-235 fission cross-section estimate") return "U-235 热中子裂变截面估计";
    if (phrase == "Low-energy 1/v law anchored at 0.0253 eV and 585 b") {
        return "以 0.0253 eV、585 b 为锚点的低能 1/v 定律";
    }
    if (phrase == "Matches the thermal anchor point exactly; does not resolve epithermal resonance structure.") {
        return "与热中子锚点一致，但不解析超热区共振结构。";
    }
    if (phrase == "Single-speed slab attenuation model") return "单速束流平板衰减模型";
    if (phrase == "Beer-Lambert attenuation with a fixed microscopic cross section") {
        return "使用固定微观截面的 Beer-Lambert 衰减";
    }
    if (phrase == "Solves time-dependent beam attenuation in a 1D slab, not full angular neutron transport.") {
        return "求解一维平板中的时间依赖束流衰减，不是完整角分布中子输运。";
    }
    if (phrase == "Single-channel radioactive decay law") return "单道放射性衰变定律";
    if (phrase == "Exponential rate equation without branching") return "不含分支比的指数率方程";
    if (phrase == "Exact for a single constant decay rate; not a microscopic quantum many-body decay calculation.") {
        return "对单一常数衰变率是精确的，但不是微观量子多体衰变计算。";
    }
    if (phrase == "metadata import") return "元数据导入";
    if (phrase == "local reference mapping") return "本地参考映射";
    if (phrase == "single-electron analytic model") return "单电子解析模型";
    if (phrase == "effective-charge hydrogenic estimate") return "有效核电荷氢样估计";
    if (phrase == "teaching configuration builder") return "教学型组态构建";
    if (phrase == "Z_eff teaching approximation") return "Z_eff 教学近似";
    if (phrase == "fine-structure + optional Zeeman + limited Stark estimate") {
        return "精细结构 + 可选 Zeeman + 受限 Stark 估计";
    }
    if (phrase == "fine-structure + optional Zeeman/Stark/hyperfine teaching estimates") {
        return "精细结构 + 可选 Zeeman/Stark/超精细教学估计";
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
    if (phrase == "Preset particle choreography without nuclear reaction dynamics") {
        return "预设粒子编排，不包含核反应动力学求解";
    }
    if (phrase == "Internal scene animation preset") {
        return "内置场景动画预设";
    }
    if (phrase == "Illustrative only; no quantitative nuclear validation") {
        return "仅作教学示意，未做定量核物理验证。";
    }
    if (phrase == "Scene preset selection") {
        return "场景预设选择";
    }
    if (phrase == "strict hydrogenic teaching preset") {
        return "严格氢样教学预设";
    }
    if (phrase == "multi-electron teaching preset") {
        return "多电子教学预设";
    }
    if (phrase == "Built-in scripted scene preset") {
        return "内置脚本化场景预设";
    }
    if (phrase == "Preset configures a pedagogical visualization, not a new physics solver.") {
        return "该预设只配置教学可视化，不代表新增物理解算器。";
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
    if (phrase == "Fine, Zeeman, Stark, and hyperfine terms are perturbative teaching-level corrections.") {
        return "精细结构、Zeeman、Stark 与超精细项目前仅作为微扰教学修正。";
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
    if (state.dynamics.enabled) {
        return "有限基二能级时间依赖量子模拟";
    }
    if (state.nuclearAnimation.enabled) {
        return "教学型核过程演示动画";
    }
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

std::vector<const char*> nuclearParticleLegend(const quantum::app::SimulationState& state) {
    using quantum::app::NuclearAnimationMode;
    std::vector<const char*> labels;
    switch (state.nuclearAnimation.mode) {
    case NuclearAnimationMode::ParticleZoo:
        labels = {"质子", "中子", "电子", "正电子", "μ 子", "反 μ 子", "光子", "电子中微子", "电子反中微子",
                  "μ 中微子", "μ 反中微子", "π+", "π-", "α 粒子", "氘核", "氚核", "氦-3", "氦-4"};
        break;
    case NuclearAnimationMode::FusionDT:
        labels = {"氘核", "氚核", "氦-4", "中子"};
        if (state.nuclearAnimation.showSecondaryParticles) {
            labels.push_back("光子");
        }
        break;
    case NuclearAnimationMode::FissionU235:
        labels = {"入射中子", "裂变碎片 A", "裂变碎片 B", "次级中子"};
        if (state.nuclearAnimation.showSecondaryParticles) {
            labels.push_back("光子");
        }
        break;
    case NuclearAnimationMode::AlphaDecay:
        labels = {"母核示意", "α 粒子"};
        if (state.nuclearAnimation.showSecondaryParticles) {
            labels.push_back("光子");
        }
        break;
    case NuclearAnimationMode::BetaMinusDecay:
        labels = {"核内质子/中子示意", "电子", "反中微子"};
        break;
    case NuclearAnimationMode::BetaPlusDecay:
        labels = {"核内质子/中子示意", "正电子", "中微子"};
        break;
    case NuclearAnimationMode::None:
    default:
        break;
    }
    return labels;
}

ImU32 tierBadgeColor(quantum::meta::ModelTier tier) {
    switch (tier) {
    case quantum::meta::ModelTier::Tier0Teaching: return IM_COL32(128, 192, 224, 255);
    case quantum::meta::ModelTier::Tier1CentralField: return IM_COL32(96, 192, 128, 255);
    case quantum::meta::ModelTier::Tier2MeanField: return IM_COL32(192, 160, 96, 255);
    case quantum::meta::ModelTier::Tier3SpectroscopyCorrections: return IM_COL32(192, 128, 160, 255);
    case quantum::meta::ModelTier::Tier4Correlation: return IM_COL32(160, 128, 192, 255);
    case quantum::meta::ModelTier::Tier5Dynamics: return IM_COL32(224, 128, 96, 255);
    default: return IM_COL32(160, 160, 160, 255);
    }
}

const char* tierBadgeLabel(quantum::meta::ModelTier tier) {
    switch (tier) {
    case quantum::meta::ModelTier::Tier0Teaching: return "T0";
    case quantum::meta::ModelTier::Tier1CentralField: return "T1";
    case quantum::meta::ModelTier::Tier2MeanField: return "T2";
    case quantum::meta::ModelTier::Tier3SpectroscopyCorrections: return "T3";
    case quantum::meta::ModelTier::Tier4Correlation: return "T4";
    case quantum::meta::ModelTier::Tier5Dynamics: return "T5";
    default: return "??";
    }
}

void drawMethodTags(const quantum::app::SimulationState& state) {
    ImGui::SeparatorText("方法标签");
    const bool strictHydrogenic = state.approximationMode == quantum::physics::ApproximationMode::StrictHydrogenic;
    const bool numericalRadial = state.cloud.useNumericalRadial && state.derived.solver.converged;
    const auto hasCapability = [&state](auto predicate) {
        return std::any_of(state.derived.methodStamps.begin(),
                           state.derived.methodStamps.end(),
                           predicate);
    };
    const bool hasCorrelation =
        hasCapability([](const quantum::meta::MethodStamp& stamp) { return stamp.includesElectronCorrelation; });
    const bool hasRelativity =
        hasCapability([](const quantum::meta::MethodStamp& stamp) { return stamp.includesRelativity; });
    const bool hasExternalField =
        hasCapability([](const quantum::meta::MethodStamp& stamp) { return stamp.includesExternalField; });
    const bool hasTimeDependent =
        hasCapability([](const quantum::meta::MethodStamp& stamp) { return stamp.isTimeDependent; });

    std::string method = strictHydrogenic ? "严格氢样解析模型" : "教学近似：Z_eff + Slater";
    if (numericalRadial) {
        method += " + 有限差分径向求解器";
    }

    ImGui::TextWrapped("方法: %s", method.c_str());
    ImGui::TextWrapped("近似层级: %s",
                       strictHydrogenic ? "单电子氢样模型"
                                        : "多电子教学近似");
    ImGui::TextWrapped("电子关联: %s", hasCorrelation ? "有限基教学 CI mixing 已启用" : "当前激活方法未包含电子关联");
    ImGui::TextWrapped("反对称性: %s",
                       strictHydrogenic ? "单电子体系，不涉及多电子反对称波函数"
                                        : "通过教学组态/CI 标签表达，非严格多体反对称波函数");
    ImGui::TextWrapped("相对论修正: %s", hasRelativity ? "教学级精细结构或标量相对论预览已启用" : "当前激活方法未包含相对论项");
    ImGui::TextWrapped("外场耦合: %s", hasExternalField ? "Zeeman/Stark 或二能级驱动项已启用" : "当前激活方法未包含外场项");
    ImGui::TextWrapped("时间依赖: %s", hasTimeDependent ? "有限基二能级时间推进已启用" : "当前场景不是时间依赖量子求解");
    ImGui::TextWrapped("数据来源: elements.json + reference_catalog.json + isotopes.json（CSV 兼容回退）");
    ImGui::TextWrapped("验证状态: %s",
                       strictHydrogenic ? "已做 H/He+ 解析值、跃迁和数值收敛验证"
                                        : "教学近似，仅做一致性与数值链路验证");
    if (state.nuclearAnimation.enabled) {
        ImGui::TextWrapped("核过程动画: %s，教学示意，不是核反应求解器", nuclearAnimationModeLabel(state.nuclearAnimation.mode));
    }
    if (!state.derived.methodStamps.empty()) {
        ImGui::SeparatorText("当前激活方法");
        for (const auto& stamp : state.derived.methodStamps) {
            const std::string localizedName = localizeMethodPhrase(stamp.methodName);
            ImGui::Bullet();
            ImGui::SameLine();
            ImGui::TextColored(ImColor(tierBadgeColor(stamp.tier)), "[%s]", tierBadgeLabel(stamp.tier));
            ImGui::SameLine();
            ImGui::Text("%s", localizedName.c_str());
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
    ImGui::TextWrapped("是否直接求解 TDSE/TD-CI: %s",
                       state.dynamics.enabled ? "是，二能级有限基 TDSE 教学模型" : "否");
    ImGui::TextWrapped("是否存在外场驱动: %s",
                       state.dynamics.enabled ? "是，经典驱动耦合项" : "否");
    ImGui::TextWrapped("是否包含耗散: %s",
                       (state.dynamics.enabled && state.dynamics.includeDissipation) ? "是，简化阻尼包络" : "否");
    if (state.nuclearAnimation.enabled) {
        ImGui::TextWrapped("核过程说明: 当前“%s”场景动画仍以教学示意为主；相关面板会给出简化的截面、衰减或衰变模型，但这里的画面本身不是完整核反应输运或微观核动力学求解。",
                           nuclearAnimationModeLabel(state.nuclearAnimation.mode));
    }
    ImGui::TextWrapped("说明: 只有启用二能级 TDSE 时，结果可标注为有限基时间依赖教学求解；其余相机过渡、核过程示意和基态/叠加态可视化不应解释为真实多体时间依赖量子动力学。");
}

void colorTableRowByStatus(quantum::meta::ValidationStatus status) {
    switch (status) {
    case quantum::meta::ValidationStatus::ReferenceMatched:
    case quantum::meta::ValidationStatus::ExperimentalComparison:
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(18, 64, 32, 160));
        break;
    case quantum::meta::ValidationStatus::Benchmarked:
    case quantum::meta::ValidationStatus::SanityChecked:
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(56, 48, 14, 160));
        break;
    case quantum::meta::ValidationStatus::NotValidated:
    default:
        ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(38, 38, 42, 160));
        break;
    }
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
            colorTableRowByStatus(record.status);
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
        drawCloudBuildProgress(canvasRect, performance);
        drawSceneHudOverlay(canvasRect, state);
    }
    ImGui::End();
    ImGui::PopStyleVar();

    if (ImGui::Begin(kPanelInspector)) {
        drawLearningPanel(state);

        drawInspectorContent(state, elementDatabase, showAdvancedPanels_);
    }
    ImGui::End();

    if (showAdvancedPanels_) {
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
        ImGui::SeparatorText("平均场、关联与动力学");
        if (state.derived.meanField.iterations > 0) {
            ImGui::Text("SCF 状态: %s, 迭代 %d, Z_eff=%.4f",
                        state.derived.meanField.converged ? "收敛" : "未收敛",
                        state.derived.meanField.iterations,
                        state.derived.meanField.finalEffectiveCharge);
            ImGui::Text("平均场总能 = %.6f eV", state.derived.meanField.totalEnergyEv);
            if (std::abs(state.derived.meanField.exchangeCorrectionEv) > 0.0) {
                ImGui::Text("Slater 交换修正 = %.6f eV", state.derived.meanField.exchangeCorrectionEv);
            }
            if (std::abs(state.derived.meanField.scalarRelativisticCorrectionEv) > 0.0) {
                ImGui::Text("标量相对论预览修正 = %.6f eV", state.derived.meanField.scalarRelativisticCorrectionEv);
            }
        } else {
            ImGui::TextWrapped("第二层教学平均场未启用。");
        }
        if (state.derived.correlation.solved) {
            ImGui::Text("CI 相关能 = %.6f eV", state.derived.correlation.correlationEnergyEv);
            ImGui::Text("相关后能量 = %.6f eV", state.derived.correlation.correlatedEnergyEv);
        } else {
            ImGui::TextWrapped("第四层有限基关联未启用或等待平均场收敛。");
        }
        if (state.derived.timeDynamics.solved && !state.derived.timeDynamics.upperPopulation.empty()) {
            ImGui::Text("TDSE 样本数 = %zu", state.derived.timeDynamics.sampleTimes.size());
            ImGui::Text("末态上能级布居 = %.6f", state.derived.timeDynamics.upperPopulation.back());
            ImGui::Text("末态偶极观测量 = %.6f", state.derived.timeDynamics.dipoleExpectation.back());
        } else {
            ImGui::TextWrapped("第五层有限基时间动力学未启用。");
        }
        if (!state.derived.approximationWarning.empty()) {
            ImGui::Separator();
            ImGui::TextWrapped("%s", state.derived.approximationWarning.c_str());
        }
        ImGui::SeparatorText("核过程动画");
        ImGui::Text("状态: %s", state.nuclearAnimation.enabled ? "已启用" : "未启用");
        if (state.nuclearAnimation.enabled) {
            ImGui::Text("当前过程: %s", nuclearAnimationModeLabel(state.nuclearAnimation.mode));
            ImGui::Text("速度 / 尺度: %.2f / %.2f", state.nuclearAnimation.speed, state.nuclearAnimation.scale);
            const auto particleLegend = nuclearParticleLegend(state);
            if (!particleLegend.empty()) {
                std::string particleText;
                for (std::size_t index = 0; index < particleLegend.size(); ++index) {
                    if (index > 0) {
                        particleText += "、";
                    }
                    particleText += particleLegend[index];
                }
                ImGui::TextWrapped("示意粒子: %s", particleText.c_str());
            }
            if (state.derived.nuclearCrossSection.valid) {
                const char* energyUnit =
                    (state.nuclearAnimation.mode == quantum::app::NuclearAnimationMode::FusionDT) ? "keV" : "eV";
                const double queryEnergy =
                    (state.nuclearAnimation.mode == quantum::app::NuclearAnimationMode::FusionDT)
                        ? (state.derived.nuclearCrossSection.queryEnergyEv / 1.0e3)
                        : state.derived.nuclearCrossSection.queryEnergyEv;
                const double peakEnergy =
                    (state.nuclearAnimation.mode == quantum::app::NuclearAnimationMode::FusionDT)
                        ? (state.derived.nuclearCrossSection.peakEnergyEv / 1.0e3)
                        : state.derived.nuclearCrossSection.peakEnergyEv;
                ImGui::Text("当前截面 sigma(E) = %.6f b", state.derived.nuclearCrossSection.queryCrossSectionBarn);
                ImGui::Text("查询能量 = %.6f %s", queryEnergy, energyUnit);
                ImGui::Text("曲线峰值 = %.6f b @ %.6f %s",
                            state.derived.nuclearCrossSection.peakCrossSectionBarn,
                            peakEnergy,
                            energyUnit);
            }
            if (state.derived.nuclearTransport.valid) {
                ImGui::Text("宏观截面 Sigma = %.6e 1/m", state.derived.nuclearTransport.macroscopicCrossSectionPerM);
                ImGui::Text("平均自由程 = %.6e m", state.derived.nuclearTransport.meanFreePathM);
                ImGui::Text("出射透射率 = %.6f", state.derived.nuclearTransport.transmissionAtExit);
                ImGui::Text("出射反应概率 = %.6f", state.derived.nuclearTransport.reactedFractionAtExit);
            }
            if (state.derived.nuclearDecay.valid) {
                ImGui::Text("半衰期 = %.6f s", state.derived.nuclearDecay.halfLifeSeconds);
                ImGui::Text("衰变常数 = %.6e 1/s", state.derived.nuclearDecay.decayConstantPerSecond);
                ImGui::Text("平均寿命 = %.6f s", state.derived.nuclearDecay.meanLifetimeSeconds);
                if (!state.derived.nuclearDecay.samples.empty()) {
                    ImGui::Text("观测窗口末端剩余母核 = %.6f", state.derived.nuclearDecay.samples.back().parentFraction);
                }
            }
            ImGui::TextWrapped("物理边界: 当前核面板提供的是半经验核结构、低保真截面、束流衰减和单道衰变率方程；不是完整核结构、核数据库、输运求解或真实量子多体动力学。");
        }
        ImGui::SeparatorText("核结构近似");
        ImGui::Text("结构核素: Z=%d, A=%d, N=%d",
                    state.derived.nuclearStructure.protonNumber,
                    state.derived.nuclearStructure.massNumber,
                    state.derived.nuclearStructure.neutronNumber);
        ImGui::Text("结合能 = %.6f MeV", state.derived.nuclearStructure.bindingEnergyMeV);
        ImGui::Text("每核子结合能 = %.6f MeV", state.derived.nuclearStructure.bindingPerNucleonMeV);
        ImGui::Text("半经验核半径 = %.6f fm", state.derived.nuclearStructure.radiusFm);
        ImGui::Text("库仑 / 不对称 / 配对项 = %.6f / %.6f / %.6f MeV",
                    state.derived.nuclearStructure.coulombTermMeV,
                    state.derived.nuclearStructure.asymmetryTermMeV,
                    state.derived.nuclearStructure.pairingTermMeV);
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
            ImGui::Text("初态 j/m_j/F = %.1f / %.1f / %.1f", state.derived.spectroscopy.initial.j, state.derived.spectroscopy.initial.mJ, state.derived.spectroscopy.initial.f);
            ImGui::Text("末态 j/m_j/F = %.1f / %.1f / %.1f", state.derived.spectroscopy.final.j, state.derived.spectroscopy.final.mJ, state.derived.spectroscopy.final.f);
            ImGui::TextWrapped("项符号: %s -> %s",
                               state.derived.spectroscopy.initialTermSymbol.c_str(),
                               state.derived.spectroscopy.finalTermSymbol.c_str());
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
        ImGui::Spacing();
        ImGui::TextDisabled("截图: 使用 Ctrl+S 快捷键保存当前视图为 PPM 文件");
        ImGui::Spacing();
        drawValidationTable(state.derived.validationRecords);
    }
    ImGui::End();
    }

    drawChartPanels(state);

    if (showAdvancedPanels_) {
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
    }

    if (ImGui::Begin(kPanelHelp)) {
        ImGui::SeparatorText("当前你在看什么");
        const char* conceptText = "选择一种模型开始探索量子世界。";
        if (state.modelKind == quantum::app::ModelKind::Bohr) {
            conceptText = state.derived.strictHydrogenicValid
                ? "这是一颗氢原子的玻尔轨道模型。亮色圆环表示电子轨道半径，虚线对应跃迁时释放的光子能量。"
                : "当前以玻尔模型展示多电子体系的教学近似图像。轨道半径由有效核电荷 Z_eff 估算。";
        } else if (state.modelKind == quantum::app::ModelKind::SchrodingerCloud) {
            conceptText = state.cloud.buildVolume
                ? "概率云展示了电子在空间中的概率密度分布。颜色深浅表示电子出现的可能性。体切片模式可以透视云内部的结构。"
                : "概率云以点云形式展示电子在空间中的概率密度分布。颜色表示量子相位。";
        } else if (state.modelKind == quantum::app::ModelKind::WaveParticle) {
            conceptText = "波粒演示展示了电子作为物质波的波长和干涉特性。";
        } else if (state.modelKind == quantum::app::ModelKind::Compare) {
            conceptText = "对比模式同时展示玻尔轨道和概率云。轨道提供简洁的几何图像，概率云提供量子力学的概率描述。";
        }
        ImGui::TextWrapped("%s", conceptText);

        ImGui::SeparatorText("关键看点");
        if (state.derived.transition.deltaEnergyEv != 0.0) {
            ImGui::BulletText("跃迁能量: %.3f eV", state.derived.transition.deltaEnergyEv);
            ImGui::BulletText("波长: %.2f nm", state.derived.transition.wavelengthNm);
            if (!state.derived.transition.seriesName.empty()) {
                ImGui::BulletText("谱系: %s", state.derived.transition.seriesName.c_str());
            }
        }
        if (state.derived.bohrMetrics.energyEv != 0.0) {
            ImGui::BulletText("轨道能量: %.3f eV", state.derived.bohrMetrics.energyEv);
            ImGui::BulletText("轨道半径 (n=%d): %.3f nm", state.bohr.principalN, state.derived.bohrMetrics.orbitalRadiusNm);
            ImGui::BulletText("有效核电荷 Z_eff: %.4f", state.derived.bohrMetrics.zeff);
        }

        ImGui::SeparatorText("操作提示");
        ImGui::BulletText("左键拖拽 = 旋转视角");
        ImGui::BulletText("滚轮 = 缩放");
        ImGui::BulletText("中键拖拽 = 平移");
        ImGui::BulletText("F 键 = 重置视角");
        ImGui::BulletText("Ctrl+S = 导出截图");

        ImGui::Separator();
        if (ImGui::Button("重置布局")) {
            layoutInitialized_ = false;
        }
    }
    ImGui::End();

    if (showAdvancedPanels_) {
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
    }

    return result;
}

} // namespace quantum::ui
