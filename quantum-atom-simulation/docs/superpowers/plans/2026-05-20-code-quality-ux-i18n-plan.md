# 工程健壮性 + UX 连续性 + 国际化 实施计划

> **For agentic workers:** Use superpowers:subagent-driven-development or superpowers:executing-plans. Steps use checkbox (`- [ ]`) syntax.

**Goal:** 将 AppUi.cpp 从 3600+ 行拆分至 ~800 行，添加会话持久化/撤销重做，提取 UI 字符串资源

**Architecture:** 按职责拆分 5 个 UI 子模块 + 2 个 app 级新模块 + 1 个 i18n 模块。每个子模块暴露一个 `drawXxx()` 入口，由 AppUi.cpp 主调度器按面板调用

**Tech Stack:** C++20, ImGui, nlohmann_json, GLFW, xmake

---

### Task 1: UiUtils.h — 提取公共工具函数

**Files:**
- Create: `ui/UiUtils.h`
- Modify: `ui/AppUi.cpp:22-100` (移除已提取的函数)

- [ ] **Step 1: 创建 UiUtils.h**

```cpp
// ui/UiUtils.h
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

} // namespace quantum::ui
```

- [ ] **Step 2: 从 AppUi.cpp 中移除已提取的内容**

删除 AppUi.cpp 第 22-100 行（namespace { 内的 kPanel* 常量 + 5 个 label 函数），替换为 `#include "quantum/ui/UiUtils.h"`。

在 AppUi.cpp 中，删除 `n0namespace {` 之后的 25-100 行（constexpr 面板标签 + modelLabel + approximationLabel + cloudModeLabel + colorMapLabel + cloudColorModeLabel）。

纯删操作：search `25:constexpr const char\* kPanelScene = "场景";` 到 `100:}` 之间的所有内容，替换为 `// see UiUtils.h`

- [ ] **Step 3: 从后续文件中找到 markAllDirty 定义，从 AppUi.cpp 中删除**

在 AppUi.cpp 中找到 `void markAllDirty(quantum::app::SimulationState& state) {` 的完整定义（约第 653 行，约 6 行），检查它已被加入 UiUtils.h 后删除。

- [ ] **Step 4: 编译验证**

Run: `xmake build quantum_atom_simulation`
Expected: PASS（无链接错误，因为 inline 函数已在头文件中）

- [ ] **Step 5: 提交**

```bash
git add ui/UiUtils.h ui/AppUi.cpp
git commit -m "refactor: extract shared UI utilities to UiUtils.h"
```

---

### Task 2: UiStringKey.h + UiStrings.cpp — 国际化字符串资源

**Files:**
- Create: `include/quantum/ui/UiStringKey.h`
- Create: `ui/UiStrings.cpp`

- [ ] **Step 1: 创建 UiStringKey.h**

```cpp
// include/quantum/ui/UiStringKey.h
#pragma once

#include <cstdint>

namespace quantum::ui {

enum class UiStringKey : uint16_t {
    Panel_Inspector,
    Panel_Charts,
    Panel_Help,
    Panel_Physics,
    Panel_Performance,
    Panel_Log,

    Group_ElementAndMode,
    Group_CloudAndAppearance,
    Group_TransitionAndSpectroscopy,
    Group_AdvancedPhysics,
    Group_ScenePresets,

    SceneBrowser_All,
    SceneBrowser_Hydrogen,
    SceneBrowser_MultiElectron,
    SceneBrowser_Spectroscopy,
    SceneBrowser_Nuclear,

    Learning_Off,
    Learning_AtomicStructure,
    Learning_Spectroscopy,
    Learning_MultiElectron,
    Learning_NuclearPhysics,
    Learning_StepCount,
    Learning_PreviousStep,
    Learning_NextStep,
    Learning_Mode,

    Hint_CtrlS_Screenshot,
    Hint_CtrlZ_Undo,
    Hint_ResetView,
    Hint_F_Fit,
    Hint_ExportReport,
    Hint_ExportCSV,

    Status_Preview,
    Status_BuildingCloud,
    Status_RecoveryTitle,
    Status_RecoveryPrompt,
    Status_Recover,
    Status_Discard,

    Action_ExportReport,
    Action_ExportCSV,

    COUNT
};

const char* uiStr(UiStringKey key);

} // namespace quantum::ui
```

- [ ] **Step 2: 创建 UiStrings.cpp**

```cpp
// ui/UiStrings.cpp
#include "quantum/ui/UiStringKey.h"

namespace quantum::ui {

const char* const kUiStringsZh[] = {
    [static_cast<int>(UiStringKey::Panel_Inspector)]  = "控制面板",
    [static_cast<int>(UiStringKey::Panel_Charts)]      = "图表",
    [static_cast<int>(UiStringKey::Panel_Help)]        = "帮助",
    [static_cast<int>(UiStringKey::Panel_Physics)]     = "物理信息",
    [static_cast<int>(UiStringKey::Panel_Performance)] = "性能监视",
    [static_cast<int>(UiStringKey::Panel_Log)]         = "日志",

    [static_cast<int>(UiStringKey::Group_ElementAndMode)]           = "元素与模式",
    [static_cast<int>(UiStringKey::Group_CloudAndAppearance)]       = "云与外观",
    [static_cast<int>(UiStringKey::Group_TransitionAndSpectroscopy)] = "跃迁与光谱",
    [static_cast<int>(UiStringKey::Group_AdvancedPhysics)]          = "高阶物理",
    [static_cast<int>(UiStringKey::Group_ScenePresets)]             = "场景预设",

    [static_cast<int>(UiStringKey::SceneBrowser_All)]          = "全部",
    [static_cast<int>(UiStringKey::SceneBrowser_Hydrogen)]     = "氢原子",
    [static_cast<int>(UiStringKey::SceneBrowser_MultiElectron)] = "多电子",
    [static_cast<int>(UiStringKey::SceneBrowser_Spectroscopy)]  = "光谱",
    [static_cast<int>(UiStringKey::SceneBrowser_Nuclear)]       = "核过程",

    [static_cast<int>(UiStringKey::Learning_Off)]              = "关闭",
    [static_cast<int>(UiStringKey::Learning_AtomicStructure)]  = "原子结构入门",
    [static_cast<int>(UiStringKey::Learning_Spectroscopy)]     = "光谱探秘",
    [static_cast<int>(UiStringKey::Learning_MultiElectron)]    = "多电子原子",
    [static_cast<int>(UiStringKey::Learning_NuclearPhysics)]   = "核物理入门",
    [static_cast<int>(UiStringKey::Learning_StepCount)]        = "步骤 %d/%d",
    [static_cast<int>(UiStringKey::Learning_PreviousStep)]     = "上一步",
    [static_cast<int>(UiStringKey::Learning_NextStep)]         = "下一步",
    [static_cast<int>(UiStringKey::Learning_Mode)]             = "学习模式",

    [static_cast<int>(UiStringKey::Hint_CtrlS_Screenshot)] = "截图: 使用 Ctrl+S 快捷键保存当前视图为 PPM 文件",
    [static_cast<int>(UiStringKey::Hint_CtrlZ_Undo)]       = "撤销: Ctrl+Z  重做: Ctrl+Y",
    [static_cast<int>(UiStringKey::Hint_ResetView)]        = "双击右键重置视图 | 右键菜单",
    [static_cast<int>(UiStringKey::Hint_F_Fit)]            = "F 键快速适配全部",
    [static_cast<int>(UiStringKey::Hint_ExportReport)]     = "导出当前验证报告",
    [static_cast<int>(UiStringKey::Hint_ExportCSV)]        = "导出当前窗口 CSV",

    [static_cast<int>(UiStringKey::Status_Preview)]        = "生成预览...",
    [static_cast<int>(UiStringKey::Status_BuildingCloud)]  = "生成完整云...",
    [static_cast<int>(UiStringKey::Status_RecoveryTitle)]  = "检测到未保存的会话",
    [static_cast<int>(UiStringKey::Status_RecoveryPrompt)] = "上次运行在 %s 中断。是否恢复上次的仿真参数？",
    [static_cast<int>(UiStringKey::Status_Recover)]        = "恢复",
    [static_cast<int>(UiStringKey::Status_Discard)]        = "放弃，从头开始",

    [static_cast<int>(UiStringKey::Action_ExportReport)] = "导出验证报告",
    [static_cast<int>(UiStringKey::Action_ExportCSV)]    = "导出当前窗口 CSV",
};

const char* uiStr(UiStringKey key) {
    return kUiStringsZh[static_cast<int>(key)];
}

} // namespace quantum::ui
```

- [ ] **Step 3: 编译验证**

Run: `xmake build quantum_atom_simulation`
Expected: PASS

- [ ] **Step 4: 提交**

```bash
git add include/quantum/ui/UiStringKey.h ui/UiStrings.cpp
git commit -m "feat: add compiled-time UI string resource system"
```

---

### Task 3: UndoStack.h/.cpp — 撤销/重做栈

**Files:**
- Create: `include/quantum/app/UndoStack.h`
- Create: `app/UndoStack.cpp`

- [ ] **Step 1: 创建 UndoStack.h**

```cpp
// include/quantum/app/UndoStack.h
#pragma once

#include "quantum/app/SimulationState.h"

#include <optional>
#include <vector>

namespace quantum::app {

struct ParamSnapshot {
    int atomicNumber = 1;
    int chargeState = 0;
    int principalN = 2;
    int maxSpectrumN = 6;
    double solverQnN = 2.0;
    double solverQnL = 1.0;
    double solverQnM = 0.0;
    ModelKind modelKind = ModelKind::Bohr;
    quantum::physics::ApproximationMode approximationMode =
        quantum::physics::ApproximationMode::StrictHydrogenic;
    CloudRenderMode cloudRenderMode = CloudRenderMode::Hybrid;
    bool showOrbitRings = true;
    bool showGrid = true;
    float pointSize = 3.0f;
    float exposure = 1.0f;
    double fitTransitionSeconds = 0.65;

    static ParamSnapshot fromState(const SimulationState& state);
    void applyTo(SimulationState& state) const;
};

class UndoStack {
public:
    void push(const ParamSnapshot& before);
    std::optional<ParamSnapshot> undo(const ParamSnapshot& current);
    std::optional<ParamSnapshot> redo();
    void clear();

private:
    std::vector<ParamSnapshot> snapshots_;
    int position_ = -1;
    static constexpr int kMaxDepth = 50;
};

} // namespace quantum::app
```

- [ ] **Step 2: 创建 UndoStack.cpp**

```cpp
// app/UndoStack.cpp
#include "quantum/app/UndoStack.h"

namespace quantum::app {

ParamSnapshot ParamSnapshot::fromState(const SimulationState& state) {
    ParamSnapshot snap;
    snap.atomicNumber = state.atomicNumber;
    snap.chargeState = state.chargeState;
    snap.principalN = state.bohr.principalN;
    snap.maxSpectrumN = state.bohr.maxSpectrumN;
    snap.solverQnN = static_cast<double>(state.solver.qn.n);
    snap.solverQnL = static_cast<double>(state.solver.qn.l);
    snap.solverQnM = static_cast<double>(state.solver.qn.m);
    snap.modelKind = state.modelKind;
    snap.approximationMode = state.approximationMode;
    snap.cloudRenderMode = state.view.cloudRenderMode;
    snap.showOrbitRings = state.view.showOrbitRings;
    snap.showGrid = state.view.showGrid;
    snap.pointSize = state.view.pointSize;
    snap.exposure = state.view.exposure;
    snap.fitTransitionSeconds = state.view.fitTransitionSeconds;
    return snap;
}

void ParamSnapshot::applyTo(SimulationState& state) const {
    state.atomicNumber = atomicNumber;
    state.chargeState = chargeState;
    state.bohr.principalN = principalN;
    state.bohr.maxSpectrumN = maxSpectrumN;
    state.solver.qn.n = static_cast<int>(solverQnN);
    state.solver.qn.l = static_cast<int>(solverQnL);
    state.solver.qn.m = static_cast<int>(solverQnM);
    state.modelKind = modelKind;
    state.approximationMode = approximationMode;
    state.view.cloudRenderMode = cloudRenderMode;
    state.view.showOrbitRings = showOrbitRings;
    state.view.showGrid = showGrid;
    state.view.pointSize = pointSize;
    state.view.exposure = exposure;
    state.view.fitTransitionSeconds = fitTransitionSeconds;
    state.dirty.physics = true;
    state.dirty.cloud = true;
    state.dirty.solver = true;
    state.dirty.reference = true;
}

void UndoStack::push(const ParamSnapshot& before) {
    if (snapshots_.size() > static_cast<std::size_t>(kMaxDepth)) {
        snapshots_.erase(snapshots_.begin());
        --position_;
    }
    ++position_;
    if (static_cast<int>(snapshots_.size()) > position_) {
        snapshots_.erase(snapshots_.begin() + position_ + 1, snapshots_.end());
    }
    snapshots_.insert(snapshots_.begin() + position_, before);
    if (static_cast<int>(snapshots_.size()) > kMaxDepth) {
        snapshots_.erase(snapshots_.begin());
        --position_;
    }
}

std::optional<ParamSnapshot> UndoStack::undo(const ParamSnapshot& current) {
    if (position_ < 0) return std::nullopt;
    ParamSnapshot result = snapshots_[static_cast<std::size_t>(position_)];
    snapshots_[static_cast<std::size_t>(position_)] = current;
    --position_;
    return result;
}

std::optional<ParamSnapshot> UndoStack::redo() {
    if (position_ + 1 >= static_cast<int>(snapshots_.size())) return std::nullopt;
    ++position_;
    return snapshots_[static_cast<std::size_t>(position_)];
}

void UndoStack::clear() {
    snapshots_.clear();
    position_ = -1;
}

} // namespace quantum::app
```

- [ ] **Step 3: 编译验证**

Run: `xmake build quantum_atom_simulation`
Expected: PASS

- [ ] **Step 4: 提交**

```bash
git add include/quantum/app/UndoStack.h app/UndoStack.cpp
git commit -m "feat: add UndoStack for Ctrl+Z/Y parameter undo/redo"
```

---

### Task 4: SessionPersistence.h/.cpp — 会话序列化

**Files:**
- Create: `include/quantum/app/SessionPersistence.h`
- Create: `app/SessionPersistence.cpp`

- [ ] **Step 1: 创建 SessionPersistence.h**

```cpp
// include/quantum/app/SessionPersistence.h
#pragma once

#include "quantum/app/SimulationState.h"

#include <filesystem>
#include <optional>
#include <string>

namespace quantum::app {

struct SessionRecord {
    std::string timestamp;
    int atomicNumber = 1;
    int chargeState = 0;
    int principalN = 2;
    int maxSpectrumN = 6;
    double solverQnN = 2.0;
    double solverQnL = 1.0;
    double solverQnM = 0.0;
    int modelKindInt = 0;
    int approximationModeInt = 0;
    int cloudRenderModeInt = 0;
    bool showOrbitRings = true;
    bool showGrid = true;
    float pointSize = 3.0f;
    float exposure = 1.0f;
    double fitTransitionSeconds = 0.65;
    bool learningEnabled = false;
    int learningPathInt = 0;
    int learningStepIndex = 0;
    bool meanFieldEnabled = true;
    bool meanFieldUseMixing = true;
    double meanFieldMixingAlpha = 0.3;

    static SessionRecord fromState(const SimulationState& state);
    void applyTo(SimulationState& state) const;
};

bool saveSession(const std::filesystem::path& path, const SessionRecord& session);
std::optional<SessionRecord> loadSession(const std::filesystem::path& path);
void writeRecoveryFile(const std::filesystem::path& path, const SessionRecord& session);
void deleteRecoveryFile(const std::filesystem::path& path);
bool hasRecoveryFile(const std::filesystem::path& path);

} // namespace quantum::app
```

- [ ] **Step 2: 创建 SessionPersistence.cpp**

```cpp
// app/SessionPersistence.cpp
#include "quantum/app/SessionPersistence.h"

#include <nlohmann/json.hpp>

#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace quantum::app {

static std::string isoTimeNow() {
    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_s(&tm, &timeT);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H%M%S");
    return oss.str();
}

SessionRecord SessionRecord::fromState(const SimulationState& state) {
    SessionRecord rec;
    rec.timestamp = isoTimeNow();
    rec.atomicNumber = state.atomicNumber;
    rec.chargeState = state.chargeState;
    rec.principalN = state.bohr.principalN;
    rec.maxSpectrumN = state.bohr.maxSpectrumN;
    rec.solverQnN = static_cast<double>(state.solver.qn.n);
    rec.solverQnL = static_cast<double>(state.solver.qn.l);
    rec.solverQnM = static_cast<double>(state.solver.qn.m);
    rec.modelKindInt = static_cast<int>(state.modelKind);
    rec.approximationModeInt = static_cast<int>(state.approximationMode);
    rec.cloudRenderModeInt = static_cast<int>(state.view.cloudRenderMode);
    rec.showOrbitRings = state.view.showOrbitRings;
    rec.showGrid = state.view.showGrid;
    rec.pointSize = state.view.pointSize;
    rec.exposure = state.view.exposure;
    rec.fitTransitionSeconds = state.view.fitTransitionSeconds;
    rec.learningEnabled = state.learning.enabled;
    rec.learningPathInt = static_cast<int>(state.learning.path);
    rec.learningStepIndex = state.learning.stepIndex;
    rec.meanFieldEnabled = state.meanField.enabled;
    rec.meanFieldUseMixing = state.meanField.useMixing;
    rec.meanFieldMixingAlpha = state.meanField.mixingAlpha;
    return rec;
}

void SessionRecord::applyTo(SimulationState& state) const {
    state.atomicNumber = atomicNumber;
    state.chargeState = chargeState;
    state.bohr.principalN = principalN;
    state.bohr.maxSpectrumN = maxSpectrumN;
    state.solver.qn.n = static_cast<int>(solverQnN);
    state.solver.qn.l = static_cast<int>(solverQnL);
    state.solver.qn.m = static_cast<int>(solverQnM);
    state.modelKind = static_cast<ModelKind>(modelKindInt);
    state.approximationMode = static_cast<quantum::physics::ApproximationMode>(approximationModeInt);
    state.view.cloudRenderMode = static_cast<CloudRenderMode>(cloudRenderModeInt);
    state.view.showOrbitRings = showOrbitRings;
    state.view.showGrid = showGrid;
    state.view.pointSize = pointSize;
    state.view.exposure = exposure;
    state.view.fitTransitionSeconds = fitTransitionSeconds;
    state.learning.enabled = learningEnabled;
    state.learning.path = static_cast<LearningPath>(learningPathInt);
    state.learning.stepIndex = learningStepIndex;
    state.meanField.enabled = meanFieldEnabled;
    state.meanField.useMixing = meanFieldUseMixing;
    state.meanField.mixingAlpha = meanFieldMixingAlpha;
    state.dirty.physics = true;
    state.dirty.cloud = true;
    state.dirty.solver = true;
    state.dirty.reference = true;
}

bool saveSession(const std::filesystem::path& path, const SessionRecord& session) {
    nlohmann::json j;
    j["timestamp"] = session.timestamp;
    j["atomicNumber"] = session.atomicNumber;
    j["chargeState"] = session.chargeState;
    j["principalN"] = session.principalN;
    j["maxSpectrumN"] = session.maxSpectrumN;
    j["solverQnN"] = session.solverQnN;
    j["solverQnL"] = session.solverQnL;
    j["solverQnM"] = session.solverQnM;
    j["modelKind"] = session.modelKindInt;
    j["approximationMode"] = session.approximationModeInt;
    j["cloudRenderMode"] = session.cloudRenderModeInt;
    j["showOrbitRings"] = session.showOrbitRings;
    j["showGrid"] = session.showGrid;
    j["pointSize"] = session.pointSize;
    j["exposure"] = session.exposure;
    j["fitTransitionSeconds"] = session.fitTransitionSeconds;
    j["learningEnabled"] = session.learningEnabled;
    j["learningPath"] = session.learningPathInt;
    j["learningStepIndex"] = session.learningStepIndex;
    j["meanFieldEnabled"] = session.meanFieldEnabled;
    j["meanFieldUseMixing"] = session.meanFieldUseMixing;
    j["meanFieldMixingAlpha"] = session.meanFieldMixingAlpha;

    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path);
    if (!file.is_open()) return false;
    file << j.dump(2);
    return true;
}

std::optional<SessionRecord> loadSession(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) return std::nullopt;
    nlohmann::json j;
    try {
        file >> j;
    } catch (...) {
        return std::nullopt;
    }
    SessionRecord rec;
    rec.timestamp = j.value("timestamp", "");
    rec.atomicNumber = j.value("atomicNumber", 1);
    rec.chargeState = j.value("chargeState", 0);
    rec.principalN = j.value("principalN", 2);
    rec.maxSpectrumN = j.value("maxSpectrumN", 6);
    rec.solverQnN = j.value("solverQnN", 2.0);
    rec.solverQnL = j.value("solverQnL", 1.0);
    rec.solverQnM = j.value("solverQnM", 0.0);
    rec.modelKindInt = j.value("modelKind", 0);
    rec.approximationModeInt = j.value("approximationMode", 0);
    rec.cloudRenderModeInt = j.value("cloudRenderMode", 0);
    rec.showOrbitRings = j.value("showOrbitRings", true);
    rec.showGrid = j.value("showGrid", true);
    rec.pointSize = j.value("pointSize", 3.0f);
    rec.exposure = j.value("exposure", 1.0f);
    rec.fitTransitionSeconds = j.value("fitTransitionSeconds", 0.65);
    rec.learningEnabled = j.value("learningEnabled", false);
    rec.learningPathInt = j.value("learningPath", 0);
    rec.learningStepIndex = j.value("learningStepIndex", 0);
    rec.meanFieldEnabled = j.value("meanFieldEnabled", true);
    rec.meanFieldUseMixing = j.value("meanFieldUseMixing", true);
    rec.meanFieldMixingAlpha = j.value("meanFieldMixingAlpha", 0.3);
    return rec;
}

void writeRecoveryFile(const std::filesystem::path& path, const SessionRecord& session) {
    saveSession(path, session);
}

void deleteRecoveryFile(const std::filesystem::path& path) {
    std::error_code ec;
    std::filesystem::remove(path, ec);
}

bool hasRecoveryFile(const std::filesystem::path& path) {
    return std::filesystem::exists(path);
}

} // namespace quantum::app
```

- [ ] **Step 3: 编译验证**

Run: `xmake build quantum_atom_simulation`
Expected: PASS

- [ ] **Step 4: 提交**

```bash
git add include/quantum/app/SessionPersistence.h app/SessionPersistence.cpp
git commit -m "feat: add SessionPersistence for save/load/recovery"
```

---

### Task 5: SceneBrowser.cpp — 提取场景浏览器

**Files:**
- Create: `ui/SceneBrowser.cpp`
- Modify: `ui/AppUi.cpp:805-876` (移除 drawScenarioBrowser + applyScenePreset + getSceneBrowserPresets + sceneCategoryIndex)

- [ ] **Step 1: 创建 SceneBrowser.cpp**

```cpp
// ui/SceneBrowser.cpp
#include "quantum/ui/AppUi.h"
#include "quantum/app/Application.h"
#include "quantum/ui/UiUtils.h"
#include "quantum/ui/UiStringKey.h"

#include <imgui.h>

namespace quantum::ui {
namespace {

bool applyScenePresetToState(quantum::app::SimulationState& state,
                             const quantum::app::DemoScriptStep& step) {
    state.atomicNumber = step.atomicNumber;
    state.chargeState = step.chargeState;
    state.approximationMode = step.approximationMode;
    state.modelKind = step.modelKind;
    state.solver.qn = step.solverQn;
    state.bohr.principalN = step.principalN;
    state.bohr.maxSpectrumN = step.maxSpectrumN;
    state.view.cloudRenderMode = step.cloudRenderMode;
    state.cloud.buildVolume = step.buildVolume;
    state.cloud.useNumericalRadial = step.useNumericalRadial;
    if (!step.components.empty()) {
        state.cloud.components = step.components;
    }
    markAllDirty(state);
    return true;
}

const std::vector<quantum::app::DemoScriptStep>& getSceneBrowserPresets() {
    using namespace quantum::app;
    using namespace quantum::physics;
    static const std::vector<DemoScriptStep> presets = []() {
        std::vector<DemoScriptStep> steps;

        DemoScriptStep s;
        // Hydrogen Lyman/Balmer
        s.name = "Hydrogen Lyman/Balmer"; s.category = "hydrogen"; s.description = "氢原子玻尔轨道能级与跃迁";
        s.atomicNumber = 1; s.chargeState = 0; s.approximationMode = ApproximationMode::StrictHydrogenic;
        s.modelKind = ModelKind::Bohr; s.solverQn = {2, 1, 0}; s.principalN = 2; s.maxSpectrumN = 6;
        s.buildVolume = true; s.useNumericalRadial = false; s.components = {{ {2, 1, 0}, 1.0f, 0.0f, 1.0f }};
        s.transition = { {2, 1, 0}, {1, 0, 0} };
        steps.push_back(s);
        // Helium Ion 3d
        s = {}; s.name = "Helium Ion 3d"; s.category = "hydrogen"; s.description = "类氢离子的 Z² 缩放效应与相位云";
        s.atomicNumber = 2; s.chargeState = 1; s.approximationMode = ApproximationMode::StrictHydrogenic;
        s.modelKind = ModelKind::SchrodingerCloud; s.solverQn = {3, 2, 0}; s.principalN = 3; s.maxSpectrumN = 6;
        s.buildVolume = true; s.useNumericalRadial = false; s.components = {{ {3, 2, 0}, 1.0f, 0.0f, 1.0f }};
        s.transition = { {3, 2, 0}, {2, 1, 0} };
        steps.push_back(s);
        // Ne Approximate 2p
        s = {}; s.name = "Ne Approximate 2p"; s.category = "multielectron"; s.description = "Aufbau 填充与 Z_eff";
        s.atomicNumber = 10; s.chargeState = 0; s.approximationMode = ApproximationMode::MultiElectronApproximation;
        s.modelKind = ModelKind::Compare; s.solverQn = {2, 1, 0}; s.principalN = 2; s.maxSpectrumN = 6;
        s.buildVolume = true; s.useNumericalRadial = false;
        s.components = {{ {2, 1, 0}, 0.8f, 0.0f, 1.0f }, { {2, 1, 1}, 0.6f, 1.57f, 1.0f }};
        s.transition = { {2, 1, 0}, {1, 0, 0} };
        steps.push_back(s);
        // Helium SCF CI
        s = {}; s.name = "Helium SCF And CI"; s.category = "multielectron"; s.description = "教学平均场与关联";
        s.atomicNumber = 2; s.chargeState = 0; s.approximationMode = ApproximationMode::MultiElectronApproximation;
        s.modelKind = ModelKind::Compare; s.solverQn = {1, 0, 0}; s.principalN = 1; s.maxSpectrumN = 5;
        s.buildVolume = true; s.useNumericalRadial = false;
        s.components = {{ {1, 0, 0}, 0.9f, 0.0f, 1.0f }, { {2, 0, 0}, 0.35f, 0.8f, 1.0f }};
        s.transition = { {2, 1, 0}, {1, 0, 0} };
        steps.push_back(s);
        // H Hyperfine Rabi
        s = {}; s.name = "Hydrogen Hyperfine Rabi"; s.category = "nuclear"; s.description = "超精细 + Rabi 二能级动力学";
        s.atomicNumber = 1; s.chargeState = 0; s.approximationMode = ApproximationMode::StrictHydrogenic;
        s.modelKind = ModelKind::SchrodingerCloud; s.solverQn = {1, 0, 0}; s.principalN = 1; s.maxSpectrumN = 6;
        s.buildVolume = true; s.useNumericalRadial = false;
        s.components = {{ {1, 0, 0}, 0.8f, 0.0f, 1.0f }, { {2, 1, 0}, 0.6f, 1.57f, 1.0f }};
        s.transition = { {2, 1, 0}, {1, 0, 0} };
        steps.push_back(s);
        // H Schrodinger Cloud
        s = {}; s.name = "Hydrogen Schrodinger Cloud"; s.category = "hydrogen"; s.description = "1s/2p/3d 概率云形状对比";
        s.atomicNumber = 1; s.chargeState = 0; s.approximationMode = ApproximationMode::StrictHydrogenic;
        s.modelKind = ModelKind::SchrodingerCloud; s.solverQn = {2, 0, 0}; s.principalN = 2; s.maxSpectrumN = 6;
        s.buildVolume = true; s.useNumericalRadial = false; s.components = {{ {2, 0, 0}, 1.0f, 0.0f, 1.0f }};
        s.transition = { {2, 0, 0}, {1, 0, 0} };
        steps.push_back(s);
        // H Superposition
        s = {}; s.name = "Hydrogen Superposition"; s.category = "hydrogen"; s.description = "2s+2p 叠加态相位变化";
        s.atomicNumber = 1; s.chargeState = 0; s.approximationMode = ApproximationMode::StrictHydrogenic;
        s.modelKind = ModelKind::SchrodingerCloud; s.solverQn = {2, 0, 0}; s.principalN = 2; s.maxSpectrumN = 6;
        s.buildVolume = true; s.useNumericalRadial = false;
        s.components = {{ {2, 0, 0}, 0.7f, 0.0f, 1.0f }, { {2, 1, 0}, 0.5f, 1.57f, 1.0f }};
        s.transition = { {2, 1, 0}, {2, 0, 0} };
        steps.push_back(s);
        // Li-Ne Trend
        s = {}; s.name = "Li-Ne Trend"; s.category = "multielectron"; s.description = "Li 到 Ne 组态趋势";
        s.atomicNumber = 3; s.chargeState = 0; s.approximationMode = ApproximationMode::MultiElectronApproximation;
        s.modelKind = ModelKind::Compare; s.solverQn = {2, 1, 0}; s.principalN = 2; s.maxSpectrumN = 6;
        s.buildVolume = true; s.useNumericalRadial = false; s.components = {{ {2, 1, 0}, 1.0f, 0.0f, 1.0f }};
        s.transition = { {2, 1, 0}, {1, 0, 0} };
        steps.push_back(s);
        // Na-Ar Trend
        s = {}; s.name = "Na-Ar Period Trend"; s.category = "multielectron"; s.description = "钠到氩周期趋势";
        s.atomicNumber = 11; s.chargeState = 0; s.approximationMode = ApproximationMode::MultiElectronApproximation;
        s.modelKind = ModelKind::Compare; s.solverQn = {3, 0, 0}; s.principalN = 3; s.maxSpectrumN = 7;
        s.buildVolume = true; s.useNumericalRadial = false; s.components = {{ {3, 0, 0}, 1.0f, 0.0f, 1.0f }};
        s.transition = { {3, 1, 0}, {3, 0, 0} };
        steps.push_back(s);
        // H Spectral Series
        s = {}; s.name = "Hydrogen Spectral Series"; s.category = "spectroscopy"; s.description = "Lyman/Balmer/Paschen 三系谱线";
        s.atomicNumber = 1; s.chargeState = 0; s.approximationMode = ApproximationMode::StrictHydrogenic;
        s.modelKind = ModelKind::Bohr; s.solverQn = {4, 1, 0}; s.principalN = 4; s.maxSpectrumN = 8;
        s.buildVolume = true; s.useNumericalRadial = false; s.components = {{ {4, 1, 0}, 1.0f, 0.0f, 1.0f }};
        s.transition = { {4, 1, 0}, {3, 2, 0} };
        steps.push_back(s);
        // Fine Structure
        s = {}; s.name = "Fine Structure Splitting"; s.category = "spectroscopy"; s.description = "2p 能级自旋-轨道分裂";
        s.atomicNumber = 1; s.chargeState = 0; s.approximationMode = ApproximationMode::StrictHydrogenic;
        s.modelKind = ModelKind::Bohr; s.solverQn = {2, 1, 0}; s.principalN = 2; s.maxSpectrumN = 6;
        s.buildVolume = true; s.useNumericalRadial = false; s.components = {{ {2, 1, 0}, 1.0f, 0.0f, 1.0f }};
        s.transition = { {2, 1, 0}, {1, 0, 0} };
        steps.push_back(s);
        // Zeeman
        s = {}; s.name = "Zeeman Effect"; s.category = "spectroscopy"; s.description = "磁场下能级 Zeeman 分裂";
        s.atomicNumber = 1; s.chargeState = 0; s.approximationMode = ApproximationMode::StrictHydrogenic;
        s.modelKind = ModelKind::Bohr; s.solverQn = {2, 1, 0}; s.principalN = 2; s.maxSpectrumN = 6;
        s.buildVolume = true; s.useNumericalRadial = false; s.components = {{ {2, 1, 0}, 1.0f, 0.0f, 1.0f }};
        s.transition = { {2, 1, 0}, {1, 0, 0} };
        steps.push_back(s);
        // D-T Fusion
        s = {}; s.name = "D-T Fusion Demo"; s.category = "nuclear"; s.description = "D+T 聚变截面";
        s.atomicNumber = 1; s.chargeState = 0; s.approximationMode = ApproximationMode::StrictHydrogenic;
        s.modelKind = ModelKind::Bohr; s.solverQn = {2, 0, 0}; s.principalN = 2; s.maxSpectrumN = 4;
        s.buildVolume = false; s.useNumericalRadial = false; s.components = {{ {2, 0, 0}, 1.0f, 0.0f, 1.0f }};
        s.transition = { {2, 0, 0}, {1, 0, 0} };
        steps.push_back(s);
        // Alpha Decay
        s = {}; s.name = "Alpha Decay Demo"; s.category = "nuclear"; s.description = "α 衰变，指数衰变规律";
        s.atomicNumber = 92; s.chargeState = 0; s.approximationMode = ApproximationMode::MultiElectronApproximation;
        s.modelKind = ModelKind::Bohr; s.solverQn = {2, 0, 0}; s.principalN = 2; s.maxSpectrumN = 4;
        s.buildVolume = false; s.useNumericalRadial = false; s.components = {{ {2, 0, 0}, 1.0f, 0.0f, 1.0f }};
        s.transition = { {2, 0, 0}, {1, 0, 0} };
        steps.push_back(s);
        return steps;
    }();
    return presets;
}

int sceneCategoryIndex(const std::string& cat) {
    if (cat == "hydrogen") return 1;
    if (cat == "multielectron") return 2;
    if (cat == "spectroscopy") return 3;
    if (cat == "nuclear") return 4;
    return 0;
}

} // namespace

void drawScenarioBrowser(quantum::app::SimulationState& state) {
    static int activeCategory = 0;
    const char* categoryLabels[] = {
        uiStr(UiStringKey::SceneBrowser_All),
        uiStr(UiStringKey::SceneBrowser_Hydrogen),
        uiStr(UiStringKey::SceneBrowser_MultiElectron),
        uiStr(UiStringKey::SceneBrowser_Spectroscopy),
        uiStr(UiStringKey::SceneBrowser_Nuclear)
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
    const auto& presets = getSceneBrowserPresets();

    int columnIndex = 0;
    for (const auto& step : presets) {
        int catIdx = sceneCategoryIndex(step.category);
        if (activeCategory != 0 && catIdx != activeCategory) continue;

        if (columnIndex > 0) ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(28, 34, 44, 200));
        ImGui::BeginChild((step.name + "##card").c_str(), ImVec2(0.0f, 64.0f), true);
        ImGui::TextUnformatted(step.name.c_str());
        ImGui::TextDisabled("%s", step.description.c_str());
        ImGui::EndChild();
        ImGui::PopStyleColor();

        if (ImGui::SmallButton(("切换##" + step.name).c_str())) {
            applyScenePresetToState(state, step);
        }
        ImGui::EndGroup();

        ++columnIndex;
        if (columnIndex >= 2) columnIndex = 0;
    }
    if (columnIndex != 0) ImGui::NewLine();
}

} // namespace quantum::ui
```

- [ ] **Step 2: 从 AppUi.cpp 中删除已提取的代码**

删除 AppUi.cpp 中第 652-831 行的以下函数定义：
- `applyScenePresetToState`（约 652-685 行）
- `getSceneBrowserPresets`（约 687-766 行）
- `sceneCategoryIndex`（约 768-773 行）
- `drawScenarioBrowser`（约 775-831 行）

在 AppUi.cpp 顶部 `#include` 区域新增：`#include "quantum/ui/UiStringKey.h"`
在 namespace 结束前保留对 `drawScenarioBrowser` 的调用（在 draw() 方法中）。

- [ ] **Step 3: 编译验证**

Run: `xmake build quantum_atom_simulation`
Expected: PASS

- [ ] **Step 4: 提交**

```bash
git add ui/SceneBrowser.cpp ui/AppUi.cpp
git commit -m "refactor: extract SceneBrowser to standalone module"
```

---

### Task 6: LearningPanel.cpp — 提取学习模式 UI

**Files:**
- Create: `ui/LearningPanel.cpp`
- Modify: `ui/AppUi.cpp:863-950` (移除 LearningStep + getLearningPathSteps + isParamLocked)
- Modify: `ui/AppUi.cpp:2283-2364` (移除 draw() 中的学习模式 UI 代码)

- [ ] **Step 1: 创建 LearningPanel.cpp**

```cpp
// ui/LearningPanel.cpp
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
            {"氢原子玻尔轨道", "选择不同的主量子数 n (1-4)，观察轨道半径 n² 的变化规律。轨道半径 ∝ n²。",
             1, 0, quantum::physics::ApproximationMode::StrictHydrogenic,
             quantum::app::ModelKind::Bohr, {2, 1, 0}, 2, 5,
             {"chargeState", "approximationMode", "meanField", "correlation", "dynamics", "nuclearAnimation"}},
            {"氢原子薛定谔概率云", "切换到概率云，调整角量子数 l，对比不同轨道形状。l=0 球形，l=1 哑铃形。",
             1, 0, quantum::physics::ApproximationMode::StrictHydrogenic,
             quantum::app::ModelKind::SchrodingerCloud, {2, 0, 0}, 2, 5,
             {"chargeState", "approximationMode", "meanField", "correlation", "dynamics", "nuclearAnimation"}},
            {"氢原子叠加态", "添加叠加分量，调整振幅和相位。观察叠加态云的相位变化。",
             1, 0, quantum::physics::ApproximationMode::StrictHydrogenic,
             quantum::app::ModelKind::SchrodingerCloud, {2, 0, 0}, 2, 5,
             {"chargeState", "approximationMode", "meanField", "correlation", "dynamics", "nuclearAnimation"}},
            {"He+ 离子相位云", "切换为 He+，对比 Z=2 时轨道半径缩小为 1/4，能级按 Z² 缩放。",
             2, 1, quantum::physics::ApproximationMode::StrictHydrogenic,
             quantum::app::ModelKind::SchrodingerCloud, {3, 2, 0}, 3, 6,
             {"approximationMode", "meanField", "correlation", "dynamics", "nuclearAnimation"}},
        }},
        {quantum::app::LearningPath::Spectroscopy, {
            {"氢原子谱线系", "设置初态 n=4、末态 n=3，观察此跃迁属于哪条谱线系。",
             1, 0, quantum::physics::ApproximationMode::StrictHydrogenic,
             quantum::app::ModelKind::Bohr, {4, 1, 0}, 4, 8,
             {"chargeState", "approximationMode", "meanField", "correlation", "dynamics", "nuclearAnimation", "modelKind"}},
            {"精细结构拆分", "在谱学面板开启精细结构。观察 2p 能级因自旋-轨道耦合分裂为 2p₃/₂ 和 2p₁/₂。",
             1, 0, quantum::physics::ApproximationMode::StrictHydrogenic,
             quantum::app::ModelKind::Bohr, {2, 1, 0}, 2, 6,
             {"chargeState", "approximationMode", "meanField", "correlation", "dynamics", "nuclearAnimation", "modelKind"}},
            {"Zeeman 效应", "开启 Zeeman 修正和限制 Δm。调节磁场强度观察谱线分裂。",
             1, 0, quantum::physics::ApproximationMode::StrictHydrogenic,
             quantum::app::ModelKind::Bohr, {2, 1, 0}, 2, 6,
             {"chargeState", "approximationMode", "meanField", "correlation", "dynamics", "nuclearAnimation", "modelKind"}},
        }},
        {quantum::app::LearningPath::MultiElectron, {
            {"Aufbau 填充规则", "从 Li(Z=3) 开始，观察 1s→2s→2p 的填充顺序。Pauli 不相容原理决定填充。",
             3, 0, quantum::physics::ApproximationMode::MultiElectronApproximation,
             quantum::app::ModelKind::Compare, {2, 1, 0}, 2, 6,
             {"approximationMode", "meanField", "correlation", "dynamics", "nuclearAnimation"}},
            {"Z_eff 屏蔽效应", "从 Z=11 到 Z=18 扫描。观察有效核电荷 Z_eff 小于实际 Z。",
             11, 0, quantum::physics::ApproximationMode::MultiElectronApproximation,
             quantum::app::ModelKind::Compare, {3, 0, 0}, 3, 7,
             {"approximationMode", "meanField", "correlation", "dynamics", "nuclearAnimation"}},
            {"氖原子多电子近似", "查看 Ne(Z=10) 的完整组态 [He]2s²2p⁶。为什么 Ne 是惰性气体？",
             10, 0, quantum::physics::ApproximationMode::MultiElectronApproximation,
             quantum::app::ModelKind::Compare, {2, 1, 0}, 2, 6,
             {"approximationMode", "meanField", "correlation", "dynamics", "nuclearAnimation"}},
        }},
        {quantum::app::LearningPath::NuclearPhysics, {
            {"认识基本粒子", "浏览基本粒子（质子、中子、电子、光子、μ子、π介子等）。",
             1, 0, quantum::physics::ApproximationMode::StrictHydrogenic,
             quantum::app::ModelKind::Bohr, {2, 0, 0}, 2, 4,
             {"approximationMode", "meanField", "correlation", "dynamics", "chargeState"}},
            {"D-T 聚变", "D + T → ⁴He + n。调节入射能量观察截面变化。",
             1, 0, quantum::physics::ApproximationMode::StrictHydrogenic,
             quantum::app::ModelKind::Bohr, {2, 0, 0}, 2, 4,
             {"approximationMode", "meanField", "correlation", "dynamics", "chargeState", "modelKind", "cloud"}},
            {"U-235 裂变", "n + ²³⁵U → 裂变碎片 + νn。观察中子衰减和链式反应。",
             92, 0, quantum::physics::ApproximationMode::MultiElectronApproximation,
             quantum::app::ModelKind::Bohr, {2, 0, 0}, 2, 4,
             {"approximationMode", "meanField", "correlation", "dynamics", "chargeState", "modelKind", "cloud"}},
            {"α 衰变", "T₁/₂ = 1s。观察指数衰变 N(t) = N₀e^{-λt}。",
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
    const auto& locked = steps[static_cast<std::size_t>(state.learning.stepIndex)].lockedParams;
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
```

- [ ] **Step 2: 从 AppUi.cpp 中删除已提取的代码**

删除第 863-950 行：`LearningStep` 结构体、`getLearningPathSteps`、`isParamLocked` 函数定义。
删除第 2283-2364 行（draw() 中的学习模式 UI 代码段，从 `const char* pathNames[]` 到 `ImGui::Separator();`），替换为一行：

```cpp
drawLearningPanel(state);
```

在 AppUi.cpp 的 Inspector panel 段中，需要在 `drawInspectorContent(state, performance)` 调用之前添加 `drawLearningPanel(state)` 调用。

- [ ] **Step 3: 编译验证**

Run: `xmake build quantum_atom_simulation`
Expected: PASS

- [ ] **Step 4: 提交**

```bash
git add ui/LearningPanel.cpp ui/AppUi.cpp
git commit -m "refactor: extract LearningPanel to standalone module"
```

---

### Task 7: SceneOverlay.cpp — 提取场景视口浮层

**Files:**
- Create: `ui/SceneOverlay.cpp`
- Modify: `ui/AppUi.cpp:1412-1553` (移除 drawSceneStatusOverlay, drawCloudBuildProgress, drawSceneHudOverlay)

- [ ] **Step 1: 创建 SceneOverlay.cpp**

直接复制 AppUi.cpp 中第 1412-1553 行的三个函数（drawSceneStatusOverlay, drawCloudBuildProgress, drawSceneHudOverlay），放入新文件中。替换其中的 `uiStr()` 调用。

由于代码量较大，此处略去完整代码，规则是：将 AppUi.cpp 1412-1553 行三个函数的完整实现原样复制到 `ui/SceneOverlay.cpp` 中（包装在 `namespace quantum::ui { namespace { ... } void drawSceneStatusOverlay(...) { ... } void drawCloudBuildProgress(...) { ... } void drawSceneHudOverlay(...) { ... } } // namespace` 中）。

- [ ] **Step 2: 从 AppUi.cpp 中删除已提取的三个函数**

删除第 1412-1553 行。

- [ ] **Step 3: 编译验证**

Run: `xmake build quantum_atom_simulation`
Expected: PASS

---

### Task 8: InspectorPanel.cpp — 提取 Inspector 控件

**Files:**
- Create: `ui/InspectorPanel.cpp`
- Modify: `ui/AppUi.cpp:2366-end` (移除 draw() 中的 Inspector 控件代码)

> **重要**: 这是最大的一次拆分。Inspector 控件从 AppUi.cpp 第 2366 行（`CollapsingHeader("元素与模式")`）开始一直到第 ~3048 行（报告导出按钮之前）。大约 680 行。

由于该模块非常大，此处不列出完整代码。实现策略如下：

- [ ] **Step 1: 创建 InspectorPanel.cpp**，暴露 `void drawInspectorContent(SimulationState& state, const ElementDatabase& elements, const PerformanceSnapshot& perf)` 入口函数

- [ ] **Step 2: 将 AppUi.cpp 第 2366 行至约 3048 行的 Inspector 控件代码移动到 InspectorPanel.cpp 中**

这包括：
- `CollapsingHeader("元素与模式")` 内的所有控件（元素选择、电荷态、近似模式、模型切换、核动画）
- `CollapsingHeader("云与外观")` 内的所有控件（点云数量、预览、采样质量、质量提示、渲染色图、着色依据）
- `CollapsingHeader("跃迁与光谱")` 内的所有控件
- `CollapsingHeader("高阶物理")` 内的所有控件
- `CollapsingHeader("场景预设")` 中的 `drawScenarioBrowser` 调用
- 参数锁定检查（`isParamLocked`）
- 导出按钮

- [ ] **Step 3: 在 AppUi.cpp draw() 的 Inspector 段中替换为**

```cpp
drawLearningPanel(state);
drawInspectorContent(state, elementDatabase, performance);
```

- [ ] **Step 4: 编译验证**

Run: `xmake build quantum_atom_simulation`
Expected: PASS

---

### Task 9: ChartPanel.cpp — 提取图表系统

**Files:**
- Create: `ui/ChartPanel.cpp`
- Modify: `ui/AppUi.cpp:1084-2193` (移除图表系统代码)

> 图表系统从第 1084 行到第 2193 行（~1110 行），包含 PlotCanvas、PlotViewState、所有绘图函数和 11 个图表的渲染代码。

- [ ] **Step 1: 创建 ChartPanel.cpp**，暴露 `void drawChartPanels(SimulationState& state, SharedViewStates& views)` 入口函数。将 AppUi.cpp 1084-2193 行的所有函数原样移入。

- [ ] **Step 2: 从 AppUi.cpp 中删除第 1084-2193 行**

- [ ] **Step 3: 编译验证**

Run: `xmake build quantum_atom_simulation`
Expected: PASS

---

### Task 10: AppUi.cpp — 缩减为主调度器

**Files:**
- Modify: `ui/AppUi.cpp`（大幅缩减）

- [ ] **Step 1: 验证 AppUi.cpp 当前行数**

Run: `wc -l ui/AppUi.cpp` (or equivalent powerShell)
Expected: 约 800 行（所有子模块提取完成后）

- [ ] **Step 2: 确认 draw() 只保留骨架**

```cpp
UiFrameResult AppUi::draw(...) const {
    // 布局初始化
    // 场景面板：场景视口 + drawSceneStatusOverlay + drawCloudBuildProgress + drawSceneHudOverlay
    // Inspector 面板：drawLearningPanel + drawInspectorContent
    // 图表面板：drawChartPanels
    // 物理面板：...
    // 帮助面板：... 
}
```

- [ ] **Step 3: 编译验证**

Run: `xmake build quantum_atom_simulation`
Expected: PASS（零编译错误）

- [ ] **Step 4: 提交**

```bash
git add ui/AppUi.cpp ui/InspectorPanel.cpp ui/ChartPanel.cpp ui/SceneOverlay.cpp
git commit -m "refactor: shrink AppUi.cpp to ~800-line dispatcher with 5 sub-modules"
```

---

### Task 11: Application.cpp — 集成撤销/重做 + 会话持久化

**Files:**
- Modify: `app/Application.cpp`

- [ ] **Step 1: 在 Application.h 中添加成员**

在 `Application.h` 的 private 成员区，`SimulationState state_` 后添加：
```cpp
quantum::app::UndoStack undoStack_;
bool showRecoveryDialog_ = false;
bool sessionRecoveryDone_ = false;
```

并在顶部添加 `#include "quantum/app/UndoStack.h"` 和 `#include "quantum/app/SessionPersistence.h"`。

- [ ] **Step 2: 在 Application::initialize() 末尾添加恢复检测**

在 `logger_.info("应用初始化完成。");` 之前添加：
```cpp
const auto recoveryPath = projectRoot_ / "docs" / "reports" / "sessions" / "recovery.json";
if (quantum::app::hasRecoveryFile(recoveryPath)) {
    showRecoveryDialog_ = true;
}
```

- [ ] **Step 3: 在 run() 主循环中添加恢复对话框**

在 `ImGui::NewFrame()` 之后、appUi_ draw 调用之前添加：
```cpp
if (showRecoveryDialog_) {
    ImGui::OpenPopup("##recovery_dialog");
    showRecoveryDialog_ = false;
}
if (ImGui::BeginPopupModal("##recovery_dialog", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
    ImGui::Text("检测到未保存的会话");
    const auto recoveryPath = projectRoot_ / "docs" / "reports" / "sessions" / "recovery.json";
    auto rec = quantum::app::loadSession(recoveryPath);
    if (rec) {
        ImGui::Text("上次运行在 %s 中断。是否恢复上次的仿真参数？", rec->timestamp.c_str());
    }
    if (ImGui::Button("恢复", ImVec2(120, 0))) {
        if (rec) {
            rec->applyTo(state_);
            recomputeDerived();
        }
        ImGui::CloseCurrentPopup();
        sessionRecoveryDone_ = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("放弃，从头开始", ImVec2(180, 0))) {
        quantum::app::deleteRecoveryFile(recoveryPath);
        ImGui::CloseCurrentPopup();
        sessionRecoveryDone_ = true;
    }
    ImGui::EndPopup();
}
```

- [ ] **Step 4: 添加 Ctrl+Z/Y 快捷键**

在主循环 `ImGui::IsKeyPressed(ImGuiKey_S)` 截图处理的附近添加：
```cpp
if (ImGui::IsKeyPressed(ImGuiKey_Z) && ImGui::GetIO().KeyCtrl &&
    !ImGui::GetIO().WantTextInput) {
    auto current = quantum::app::ParamSnapshot::fromState(state_);
    auto undone = undoStack_.undo(current);
    if (undone) {
        undone->applyTo(state_);
        recomputeDerived();
    }
}
if (ImGui::IsKeyPressed(ImGuiKey_Y) && ImGui::GetIO().KeyCtrl &&
    !ImGui::GetIO().WantTextInput) {
    auto redone = undoStack_.redo();
    if (redone) {
        redone->applyTo(state_);
        recomputeDerived();
    }
}
```

- [ ] **Step 5: 添加会话自动保存**

在 run() 主循环末尾（glfwSwapBuffers 附近）添加 5 分钟定时保存：
```cpp
static double autoSaveTimer = 0.0;
autoSaveTimer += deltaSeconds;
if (autoSaveTimer >= 300.0) {
    autoSaveTimer = 0.0;
    const auto sessionPath = projectRoot_ / "docs" / "reports" / "sessions" / "recovery.json";
    auto rec = quantum::app::SessionRecord::fromState(state_);
    quantum::app::writeRecoveryFile(sessionPath, rec);
}
```

- [ ] **Step 6: 在 shutdown() 中处理正常退出**

在该函数末尾添加：
```cpp
const auto recoveryPath = projectRoot_ / "docs" / "reports" / "sessions" / "recovery.json";
quantum::app::deleteRecoveryFile(recoveryPath);
```

- [ ] **Step 7: 编译验证**

Run: `xmake build quantum_atom_simulation`
Expected: PASS

---

### Task 12: TestMain.cpp — 新增测试

**Files:**
- Modify: `tests/TestMain.cpp`

- [ ] **Step 1: 添加会话往返测试**

在 TestMain.cpp 中 main 函数之前添加：
```cpp
#include "quantum/app/SessionPersistence.h"
#include "quantum/app/UndoStack.h"

void testSessionRoundtrip() {
    quantum::app::SimulationState state;
    state.atomicNumber = 6;
    state.chargeState = 2;
    state.bohr.principalN = 3;
    state.meanField.mixingAlpha = 0.42;

    auto rec = quantum::app::SessionRecord::fromState(state);
    const auto tmpPath = std::filesystem::temp_directory_path() / "qa_test_session.json";
    bool ok = quantum::app::saveSession(tmpPath, rec);
    assert(ok);

    auto loaded = quantum::app::loadSession(tmpPath);
    assert(loaded.has_value());
    assert(loaded->atomicNumber == 6);
    assert(loaded->chargeState == 2);
    assert(loaded->principalN == 3);
    assert(std::abs(loaded->meanFieldMixingAlpha - 0.42) < 0.01);

    std::filesystem::remove(tmpPath);
    std::cout << "PASS: testSessionRoundtrip\n";
}

void testUndoRedo() {
    quantum::app::UndoStack stack;
    quantum::app::SimulationState state;
    state.atomicNumber = 1;

    auto snap1 = quantum::app::ParamSnapshot::fromState(state);
    stack.push(snap1);
    state.atomicNumber = 8;
    auto snap2 = quantum::app::ParamSnapshot::fromState(state);

    auto undone = stack.undo(snap2);
    assert(undone.has_value());
    assert(undone->atomicNumber == 1);

    auto redone = stack.redo();
    assert(redone.has_value());
    assert(redone->atomicNumber == 8);

    // Test empty undo
    auto emptyUndo = stack.undo(quantum::app::ParamSnapshot::fromState(state));
    assert(emptyUndo.has_value());
    auto emptyRedo2 = stack.redo();
    assert(emptyRedo2.has_value());

    std::cout << "PASS: testUndoRedo\n";
}

void testUiStringCoverage() {
    for (int i = 0; i < static_cast<int>(quantum::ui::UiStringKey::COUNT); ++i) {
        auto key = static_cast<quantum::ui::UiStringKey>(i);
        const char* str = quantum::ui::uiStr(key);
        assert(str != nullptr);
        assert(str[0] != '\0');
    }
    std::cout << "PASS: testUiStringCoverage\n";
}
```

- [ ] **Step 2: 在 main() 中调用新测试**

```cpp
testSessionRoundtrip();
testUndoRedo();
testUiStringCoverage();
```

- [ ] **Step 3: 编译并运行测试**

Run:
```powershell
xmake build quantum_atom_tests
xmake run quantum_atom_tests
```
Expected: 所有测试 PASS，返回 0

- [ ] **Step 4: 提交**

```bash
git add tests/TestMain.cpp
git commit -m "test: add session roundtrip, undo/redo, and i18n coverage tests"
```

---

### Task 13: 最终集成与提交

- [ ] **Step 1: 完整编译**

Run: `xmake build quantum_atom_simulation quantum_atom_tests`
Expected: PASS（零警告）

- [ ] **Step 2: 运行测试**

Run: `xmake run quantum_atom_tests`
Expected: 所有测试 PASS

- [ ] **Step 3: 最终文件清单验证**

| 操作 | 文件 |
|------|------|
| 新建 | `ui/UiUtils.h` |
| 新建 | `ui/UiStrings.cpp` |
| 新建 | `ui/InspectorPanel.cpp` |
| 新建 | `ui/ChartPanel.cpp` |
| 新建 | `ui/SceneBrowser.cpp` |
| 新建 | `ui/LearningPanel.cpp` |
| 新建 | `ui/SceneOverlay.cpp` |
| 新建 | `include/quantum/ui/UiStringKey.h` |
| 新建 | `include/quantum/app/SessionPersistence.h` |
| 新建 | `include/quantum/app/UndoStack.h` |
| 新建 | `app/SessionPersistence.cpp` |
| 新建 | `app/UndoStack.cpp` |
| 修改 | `ui/AppUi.cpp` (~3600→~800 行) |
| 修改 | `include/quantum/app/Application.h` |
| 修改 | `app/Application.cpp` |
| 修改 | `tests/TestMain.cpp` |

- [ ] **Step 4: 运行应用验证 UI 行为未变**

```bash
xmake run quantum_atom_simulation
# 手动验证：Inspector 折叠组、场景浏览器、学习路径、Ctrl+Z/Y、截图、HUD
```