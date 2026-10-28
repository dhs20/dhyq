# 工程健壮性 + UX 连续性 + 国际化 — 设计文档

## 概述

在第一轮 ABCD 多维增强后，AppUi.cpp 膨胀至 3600+ 行、缺少会话持久化、UI 字符串全部硬编码。本轮三线推进，聚焦代码可维护性、用户体验连续性、国际化准备。

---

## 1. 工程健壮性：AppUi.cpp 模块化拆分

### 1.1 目标

将 AppUi.cpp 从 3600+ 行拆分至 ~800 行的主调度模块，按职责提取 5 个子模块。

### 1.2 拆分方案

```
ui/
  AppUi.cpp               ~800 行，主框架 + 面板布局调度 + draw()
  InspectorPanel.cpp       ~1200 行，折叠组 + 参数控件 + 参数锁定
  ChartPanel.cpp           ~800 行，十字光标 + 右键菜单 + 11 图表
  SceneBrowser.cpp         ~250 行，分类卡片浏览器
  LearningPanel.cpp        ~350 行，学习模式 Combo + 进度条 + 步骤导航
  SceneOverlay.cpp         ~250 行，HUD 浮层 + 进度环 + 状态文字
```

### 1.3 接口约定

每个子模块暴露一个入口函数：

```cpp
// InspectorPanel.cpp
void drawInspectorContent(quantum::app::SimulationState& state, const quantum::core::PerformanceTracker& perf);

// ChartPanel.cpp
void drawChartPanels(quantum::app::SimulationState& state, quantum::render::SharedViewStates& views);

// SceneBrowser.cpp
void drawScenarioBrowser(quantum::app::SimulationState& state);

// LearningPanel.cpp
void drawLearningPanel(quantum::app::SimulationState& state);

// SceneOverlay.cpp
void drawSceneStatusOverlay(const ImRect& rect, const quantum::app::SimulationState& state, const quantum::core::PerformanceTracker& perf);
void drawCloudBuildProgress(const ImRect& rect, const quantum::core::PerformanceTracker& perf);
void drawSceneHudOverlay(const ImRect& rect, const quantum::app::SimulationState& state);
```

### 1.4 共享依赖

- 每个子模块 `#include "quantum/ui/AppUi.h"` 以访问 `AppUi` 类成员
- 匿名 namespace 中的工具函数（如 `markAllDirty`、`formatTemperature` 等）提取到 `ui/UiUtils.h` 公共头文件
- `Application.h` 中的类型（`DemoScriptStep`、`ModelKind` 等）通过现有头文件访问

### 1.5 构建集成

- 新建 `ui/InspectorPanel.cpp`、`ui/ChartPanel.cpp`、`ui/SceneBrowser.cpp`、`ui/LearningPanel.cpp`、`ui/SceneOverlay.cpp`、`ui/UiUtils.h`
- xmake.lua 自动包含 `ui/` 目录下的所有 `.cpp` 文件（当前已配置）

---

## 2. UX 连续性：会话持久化 + 撤销/重做

### 2.1 会话持久化

#### 数据结构

```cpp
// include/quantum/app/SessionPersistence.h

struct SessionRecord {
    std::string timestamp;       // ISO 8601
    int atomicNumber;
    int chargeState;
    ApproximationMode approximationMode;
    ModelKind modelKind;
    // ... 所有用户可调参数 ...
    LearningParams learning;
    MeanFieldSettings meanField;
    CloudSettings cloud;
    // ... 等
};

// JSON 序列化/反序列化
bool saveSession(const std::filesystem::path& path, const SessionRecord& session);
std::optional<SessionRecord> loadSession(const std::filesystem::path& path);
```

#### 保存触发时机

| 触发条件 | 说明 |
|----------|------|
| 场景切换 | 浏览器中选择新场景时 |
| 学习步骤切换 | 上一步/下一步时 |
| 退出应用 | 窗口关闭时 |
| 定时自动保存 | 每 5 分钟（可选） |

#### 恢复流程

1. 启动时检测 `docs/reports/sessions/` 下是否有未保存的会话
2. 检测到最近一次退出时未正常关闭 → 弹出 "恢复上次会话?" 对话框
3. 用户确认 → 加载参数并标记 `markAllDirty`

#### 文件路径

```
docs/reports/sessions/
  session_2026-05-20T143022_auto.json
  session_2026-05-20T143022_manual.json
  recovery.json                    ← 启动时写入，正常退出时删除
```

### 2.2 撤销/重做

#### 命令结构

```cpp
// include/quantum/app/UndoStack.h

struct ParamSnapshot {
    int atomicNumber;
    int chargeState;
    int principalN;
    double solverQnN, solverQnL, solverQnM;
    ModelKind modelKind;
    ApproximationMode approximationMode;
    // ... 核心参数 ...
};

class UndoStack {
public:
    void push(const ParamSnapshot& before);   // 记录变更前状态
    std::optional<ParamSnapshot> undo(const ParamSnapshot& current);  // Ctrl+Z
    std::optional<ParamSnapshot> redo();                                // Ctrl+Y
    void clear();

private:
    std::vector<ParamSnapshot> snapshots_;
    int position_ = -1;
    static constexpr int kMaxDepth = 50;
};
```

#### 触发方式

| 输入 | 动作 |
|------|------|
| Ctrl+Z | undo |
| Ctrl+Y | redo |
| 参数变更时 | push 变更前快照 |

#### 集成点

在 Inspector 参数变更回调中调用 `undoStack_.push(beforeSnapshot)`。Ctrl+Z/Y 在 Application::run() 主循环中处理。

### 2.3 恢复提醒对话框

使用 ImGui 模态对话框：

```
┌─────────────────────────────────┐
│  检测到未保存的会话              │
│                                 │
│  上次运行在 14:30 中断。         │
│  是否恢复上次的仿真参数？        │
│                                 │
│  [恢复]   [放弃，从头开始]       │
└─────────────────────────────────┘
```

---

## 3. 国际化准备：编译期字符串资源化

### 3.1 枚举定义

```cpp
// include/quantum/ui/UiStringKey.h

enum class UiStringKey : uint16_t {
    // 面板标题
    Panel_Inspector,
    Panel_Charts,
    Panel_Help,

    // Inspector 折叠组
    Group_ElementAndMode,
    Group_CloudAndAppearance,
    Group_TransitionAndSpectroscopy,
    Group_AdvancedPhysics,
    Group_ScenePresets,

    // 场景浏览器
    SceneBrowser_All,
    SceneBrowser_Hydrogen,
    SceneBrowser_MultiElectron,
    SceneBrowser_Spectroscopy,
    SceneBrowser_Nuclear,

    // 学习路径
    Learning_Off,
    Learning_AtomicStructure,
    Learning_Spectroscopy,
    Learning_MultiElectron,
    Learning_NuclearPhysics,
    Learning_StepCount,
    Learning_PreviousStep,
    Learning_NextStep,

    // 操作提示
    Hint_CtrlS_Screenshot,
    Hint_CtrlZ_Undo,
    Hint_ResetView,

    // 状态
    Status_Preview,
    Status_BuildingCloud,

    // 总数
    COUNT
};
```

### 3.2 字典表

```cpp
// ui/UiStrings.cpp

const char* const kUiStringsZh[] = {
    [Panel_Inspector]        = "检查器",
    [Panel_Charts]           = "图表",
    [Group_ElementAndMode]   = "元素与模式",
    [Group_CloudAndAppearance] = "云与外观",
    [SceneBrowser_All]       = "全部",
    [SceneBrowser_Hydrogen]  = "氢原子",
    [SceneBrowser_MultiElectron] = "多电子",
    // ... 约 200 条
};

// 预留英文
const char* const kUiStringsEn[] = { /* 后续填充 */ };

const char* uiStr(UiStringKey key) {
    return kUiStringsZh[static_cast<int>(key)];
}
```

### 3.3 替换策略

- 本轮仅替换折叠组标题、标签栏、按钮文字等高频重复字符串
- 学习路径引导文字、场景描述等动态内容保留中文硬编码（这些本身是数据，非 UI 框架）
- 不引入文件 I/O 依赖

### 3.4 文件清单

- `include/quantum/ui/UiStringKey.h` — 枚举定义
- `ui/UiStrings.cpp` — 字典表 + `uiStr()` 函数
- 各 UI 文件中 `"硬编码中文"` → `uiStr(UiStringKey::Xxx)`

---

## 4. 测试扩展

### 4.1 新增测试用例（TestMain.cpp）

| 测试 | 覆盖内容 |
|------|----------|
| `testSessionRoundtrip` | JSON 序列化 → 反序列化循环，验证所有字段保真 |
| `testUndoRedo` | push → undo → redo → undo×5 边界回绕 |
| `testUiStringCoverage` | 确认 COUNT 条字符串全部非空 |
| `testReferenceConsistency` | 已有（Task 11 添加） |

### 4.2 验证标准

- 单测全通过
- 零编译警告
- `xmake run quantum_atom_tests` 返回 0

---

## 5. 文件变更总览

| 操作 | 文件 | 说明 |
|------|------|------|
| 新建 | `ui/UiUtils.h` | 提取公共工具函数 |
| 新建 | `ui/InspectorPanel.cpp` | Inspector 折叠组 UI |
| 新建 | `ui/ChartPanel.cpp` | 图表面板 |
| 新建 | `ui/SceneBrowser.cpp` | 场景卡片浏览器 |
| 新建 | `ui/LearningPanel.cpp` | 学习模式 UI |
| 新建 | `ui/SceneOverlay.cpp` | 场景视口浮层 |
| 新建 | `ui/UiStrings.cpp` | 字符串字典 |
| 新建 | `include/quantum/ui/UiStringKey.h` | 字符串枚举 |
| 新建 | `include/quantum/app/SessionPersistence.h` | 会话持久化 |
| 新建 | `app/SessionPersistence.cpp` | 会话序列化 |
| 新建 | `include/quantum/app/UndoStack.h` | 撤销栈 |
| 新建 | `app/UndoStack.cpp` | 撤销栈实现 |
| 修改 | `ui/AppUi.cpp` | 缩减至 ~800 行调度器 |
| 修改 | `app/Application.cpp` | 集成撤销/重做 + 会话恢复 |
| 修改 | `tests/TestMain.cpp` | 新增测试用例 |

---

## 6. 风险与约束

- **编译验证**：每次新建 .cpp 后立即编译确认无链接错误
- **保留行为**：所有现有 UI 行为不变，仅移动代码位置
- **不修改 shader**：渲染管线零改动
- **不修改 JSON 数据文件**：场景和参考数据不变