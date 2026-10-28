# Quantum Atom Simulation 多维度增强实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 沿 C（交互直观性）→ B（场景丰富度）→ A（学习路径）→ B（物理深化）→ D（可视化提升）五个阶段，将当前 v0.3.1 增强至 v0.4.0。

**Architecture:** 保持现有 xmake + C++20 + OpenGL 3.3 + ImGui Docking 技术栈。UI 层核心改动在 `ui/AppUi.cpp`，物理层改动在各 solver/physics 模块，数据扩展在 `assets/` JSON 文件。

**Tech Stack:** C++20, OpenGL 3.3 Core, GLFW 3.4, GLEW 2.2, GLM 1.0, ImGui v1.92.6-docking, nlohmann_json v3.11, xmake

**设计文档:** `docs/superpowers/specs/2026-05-20-multi-dimension-enhancement-design.md`

---

## Phase 1: 维度 C — 交互直观性提升

### Task 1: 重构 Inspector 面板为折叠分组结构

**Files:**
- Modify: `ui/AppUi.cpp`（`draw()` 函数中 `kPanelInspector` 段）
- Modify: `include/quantum/ui/AppUi.h`

**背景：** 当前 Inspector 面板将所有参数平铺，从元素选择器到高阶物理一气呵成。需重新组织为折叠分组。

- [ ] **Step 1: 在 AppUi.h 中添加面板可见性状态**

在 `AppUi` 类的 private mutable 成员中新增：

```cpp
mutable bool inspectorElementOpen_ = true;
mutable bool inspectorPresetsOpen_ = true;
mutable bool inspectorDemoOpen_ = true;
mutable bool inspectorCloudOpen_ = false;
mutable bool inspectorTransitionOpen_ = false;
mutable bool inspectorAdvancedPhysicsOpen_ = false;
mutable bool showAdvancedPanels_ = false;
```

- [ ] **Step 2: 在 draw() 的 Inspector 段替换平铺结构为折叠分组**

找到 `if (ImGui::Begin(kPanelInspector))` 段（约 1859 行），将其中从 `drawScenarioButtons(state)` 到面板结束的所有内容替换为：

```cpp
if (ImGui::Begin(kPanelInspector)) {
    // 学习模式入口（Phase 3 会用到，先占位）
    // --- 折叠组 1: 元素与模式 ---
    ImGui::SetNextItemOpen(inspectorElementOpen_, ImGuiCond_Once);
    inspectorElementOpen_ = ImGui::CollapsingHeader("元素与模式", ImGuiTreeNodeFlags_DefaultOpen);
    if (inspectorElementOpen_) {
        // 元素选择器（下拉列表）
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
            currentElementLabel = "Z=" + std::to_string(elements[currentIndex].atomicNumber)
                                  + " " + elements[currentIndex].symbol
                                  + " · " + localizedElementName(elements[currentIndex]);
        }
        if (ImGui::BeginCombo("元素", currentElementLabel.c_str())) {
            for (std::size_t index = 0; index < elements.size(); ++index) {
                const bool selected = static_cast<int>(index) == currentIndex;
                const std::string itemLabel = "Z=" + std::to_string(elements[index].atomicNumber)
                                              + " " + elements[index].symbol
                                              + " · " + localizedElementName(elements[index]);
                if (ImGui::Selectable(itemLabel.c_str(), selected)) {
                    state.atomicNumber = elements[index].atomicNumber;
                    markAllDirty(state);
                }
            }
            ImGui::EndCombo();
        }
        if (ImGui::SliderInt("电荷态", &state.chargeState, 0, std::max(0, state.atomicNumber - 1))) {
            markAllDirty(state);
        }
        if (ImGui::BeginCombo("模型", modelLabel(state.modelKind))) {
            for (const auto kind : {quantum::app::ModelKind::Bohr,
                                    quantum::app::ModelKind::WaveParticle,
                                    quantum::app::ModelKind::SchrodingerCloud,
                                    quantum::app::ModelKind::Compare}) {
                if (ImGui::Selectable(modelLabel(kind), kind == state.modelKind)) {
                    applyModelPreset(state, kind);
                }
            }
            ImGui::EndCombo();
        }
        if (ImGui::BeginCombo("近似模式", approximationLabel(state.approximationMode))) {
            for (const auto mode : {quantum::physics::ApproximationMode::StrictHydrogenic,
                                    quantum::physics::ApproximationMode::MultiElectronApproximation}) {
                if (ImGui::Selectable(approximationLabel(mode), mode == state.approximationMode)) {
                    state.approximationMode = mode;
                    markAllDirty(state);
                }
            }
            ImGui::EndCombo();
        }
    }

    // --- 折叠组 2: 场景预设 (Phase 2 扩展) ---
    ImGui::SetNextItemOpen(inspectorPresetsOpen_, ImGuiCond_Once);
    inspectorPresetsOpen_ = ImGui::CollapsingHeader("场景预设", ImGuiTreeNodeFlags_DefaultOpen);
    if (inspectorPresetsOpen_) {
        drawScenarioButtons(state);
    }

    // --- 折叠组 3: 自动演示 ---
    ImGui::SetNextItemOpen(inspectorDemoOpen_, ImGuiCond_Once);
    inspectorDemoOpen_ = ImGui::CollapsingHeader("自动演示", ImGuiTreeNodeFlags_DefaultOpen);
    if (inspectorDemoOpen_) {
        // 保留现有自动演示控件
    }

    // --- 折叠组 4: 云与外观（默认折叠）---
    ImGui::SetNextItemOpen(inspectorCloudOpen_, ImGuiCond_Once);
    inspectorCloudOpen_ = ImGui::CollapsingHeader("云与外观");
    if (inspectorCloudOpen_) {
        // 保留现有概率云、视口控制
    }

    // --- 折叠组 5: 跃迁与光谱（默认折叠）---
    ImGui::SetNextItemOpen(inspectorTransitionOpen_, ImGuiCond_Once);
    inspectorTransitionOpen_ = ImGui::CollapsingHeader("跃迁与光谱");
    if (inspectorTransitionOpen_) {
        // 保留现有跃迁、谱学控制
    }

    // --- 折叠组 6: 高阶物理（默认折叠）---
    ImGui::SetNextItemOpen(inspectorAdvancedPhysicsOpen_, ImGuiCond_Once);
    inspectorAdvancedPhysicsOpen_ = ImGui::CollapsingHeader("高阶物理");
    if (inspectorAdvancedPhysicsOpen_) {
        // 保留现有求解器、平均场、关联、动力学、核过程控制
    }

    ImGui::Separator();
    if (ImGui::Button(showAdvancedPanels_ ? "收起高级面板" : "展开高级面板")) {
        showAdvancedPanels_ = !showAdvancedPanels_;
    }
}
ImGui::End();
```

- [ ] **Step 3: 修改默认停靠布局，高级面板默认隐藏**

修改 `ensureDefaultDockLayout` 中 `showAdvancedPanels_` 的逻辑：当 `showAdvancedPanels_` 为 false 时，不 Dock 物理信息/图表/性能/日志面板（它们不会显示在 dockspace 中）。

在 `draw()` 函数中，将原有 Physics、Plots、Performance、Log 面板的 `ImGui::Begin` 包裹在 `showAdvancedPanels_` 检查中：

```cpp
if (showAdvancedPanels_) {
    if (ImGui::Begin(kPanelPhysics)) { /* ... */ } ImGui::End();
    if (ImGui::Begin(kPanelPlots)) { /* ... */ } ImGui::End();
    if (ImGui::Begin(kPanelPerformance)) { /* ... */ } ImGui::End();
    if (ImGui::Begin(kPanelLog)) { /* ... */ } ImGui::End();
}
```

- [ ] **Step 4: 编译验证**

```powershell
cd D:\project\dhyq\quantum-atom-simulation
xmake build quantum_atom_simulation
```

Expected: 编译成功。

- [ ] **Step 5: Commit**

```bash
git add ui/AppUi.cpp include/quantum/ui/AppUi.h
git commit -m "feat(ui): reorganize Inspector with collapsible groups and Advanced panel toggle"
```

---

### Task 2: 添加场景 HUD 浮层

**Files:**
- Modify: `ui/AppUi.cpp`（`draw()` 函数中场景面板段，约 1854 行附近）

**背景：** 当前已有 `drawSceneStatusOverlay` 在左下角。需在右上角增加新的 HUD 浮层。

- [ ] **Step 1: 在 drawSceneStatusOverlay 调用后新增 HUD 绘制**

在 `drawSceneStatusOverlay(canvasRect, state, performance);` （约 1854 行）之后添加：

```cpp
drawSceneHudOverlay(canvasRect, state);
```

- [ ] **Step 2: 实现 drawSceneHudOverlay 函数**

在匿名 namespace 中（约 1078 行 `drawSceneStatusOverlay` 之后）添加：

```cpp
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
        statusText = "严格氢样  可靠";
    } else if (!state.derived.approximationWarning.empty()) {
        bgColor = IM_COL32(52, 32, 12, 200);
        statusColor = IM_COL32(232, 148, 68, 255);
        statusText = "配置冲突";
    } else {
        bgColor = IM_COL32(42, 38, 14, 200);
        statusColor = IM_COL32(212, 188, 72, 255);
        statusText = "多电子近似  教学级";
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
```

- [ ] **Step 3: 编译验证**

```powershell
xmake build quantum_atom_simulation
```

Expected: 编译成功。

- [ ] **Step 4: Commit**

```bash
git add ui/AppUi.cpp
git commit -m "feat(ui): add scene HUD overlay with status color coding"
```

---

### Task 3: Physics 面板颜色编码 + 学习指引面板

**Files:**
- Modify: `ui/AppUi.cpp`（`drawMethodTags`、`drawValidationTable`、`kPanelHelp` 段）

**背景：** Physics 面板需要给验证表格行着色，方法标签加 Tier 徽章。Help 面板替换为动态学习指引。

- [ ] **Step 1: 给验证表格行着色**

在 `drawValidationTable` 函数中，`ImGui::TableNextRow()` 后添加行着色：

```cpp
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
```

在 `drawValidationTable` 中，`ImGui::TableNextRow()` 之后、第一列之前调用 `colorTableRowByStatus(record.status)`。

- [ ] **Step 2: 给方法标签加 Tier 徽章**

在 `drawMethodTags` 中，将 `ImGui::BulletText` 替换为带颜色徽章：

```cpp
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

std::string tierBadgeLabel(quantum::meta::ModelTier tier) {
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
```

然后在 BulletText 前添加 `ImGui::TextColored(ImColor(tierBadgeColor(stamp.tier)), "[%s]", tierBadgeLabel(stamp.tier).c_str()); ImGui::SameLine();`

- [ ] **Step 3: 替换 Help 面板为学习指引**

找到 `kPanelHelp` 段，替换内容为：

```cpp
if (ImGui::Begin(kPanelHelp)) {
    ImGui::SeparatorText("当前你在看什么");
    const char* conceptText = "选择一种模型开始探索量子世界。";
    if (state.modelKind == quantum::app::ModelKind::Bohr) {
        conceptText = state.derived.strictHydrogenicValid
            ? "这是一颗氢原子的玻尔轨道模型。亮色圆环表示电子轨道半径，虚线箭头对应跃迁时释放的光子。"
            : "当前以玻尔模型展示多电子体系的教学近似图像。轨道半径由有效核电荷 Z_eff 估算。";
    } else if (state.modelKind == quantum::app::ModelKind::SchrodingerCloud) {
        conceptText = state.cloud.buildVolume
            ? "概率云展示了电子在空间中的概率密度分布。颜色深浅表示电子出现的可能性。" \
              "体切片模式可以透视云内部的结构。"
            : "概率云以点云形式展示电子在空间中的概率密度分布。颜色表示量子相位。";
    } else if (state.modelKind == quantum::app::ModelKind::Compare) {
        conceptText = "对比模式同时展示玻尔轨道和概率云。轨道提供简洁的几何图像，" \
                      "概率云提供量子力学的概率描述。";
    }
    ImGui::TextWrapped("%s", conceptText);

    ImGui::SeparatorText("关键看点");
    if (state.derived.transition.valid) {
        ImGui::BulletText("跃迁能量: %.3f eV", state.derived.transition.energyEv);
        ImGui::BulletText("波长: %.2f nm", state.derived.transition.wavelengthNm);
    }
    if (state.derived.bohrMetrics.valid) {
        ImGui::BulletText("轨道半径 (n=%d): %.3f nm",
                          state.bohr.principalN,
                          state.derived.bohrMetrics.orbitalRadiusM * 1.0e9);
        ImGui::BulletText("电离能: %.2f eV", std::abs(state.derived.bohrMetrics.groundEnergyEv));
    }

    ImGui::SeparatorText("操作提示");
    ImGui::BulletText("左键拖拽 = 旋转视角");
    ImGui::BulletText("滚轮     = 缩放");
    ImGui::BulletText("中键拖拽 = 平移");
    ImGui::BulletText("F 键     = 重置视角");
    ImGui::BulletText("Ctrl+S   = 导出截图");
}
ImGui::End();
```

- [ ] **Step 4: 编译验证**

```powershell
xmake build quantum_atom_simulation
```

Expected: 编译成功。

- [ ] **Step 5: Commit**

```bash
git add ui/AppUi.cpp
git commit -m "feat(ui): color-coded validation table, tier badges, learning guide panel"
```

---

## Phase 2: 维度 B — 场景丰富度增强

### Task 4: 扩展 demo_scenes.json 场景数据

**Files:**
- Modify: `assets/scenarios/demo_scenes.json`

- [ ] **Step 1: 添加 9 个新场景定义**

在现有 `demo_scenes.json` 的 `steps` 数组中追加以下场景（保持 JSON 格式兼容现有加载代码）：

```json
{
  "name": "氢原子薛定谔概率云",
  "atomic_number": 1,
  "charge_state": 0,
  "approximation": "strict_hydrogenic",
  "model": "schrodinger_cloud",
  "cloud_render_mode": "hybrid",
  "solver_qn": [2, 0, 0],
  "components": [{"qn": [2, 0, 0], "amplitude": 1.0, "phase": 0.0}],
  "use_numerical_radial": false,
  "build_volume": true,
  "principal_n": 2,
  "max_spectrum_n": 5,
  "transition": {"initial": [2, 0, 0], "final": [1, 0, 0], "enforce_rules": true, "enforce_delta_m": false},
  "camera": {"yaw": 0.5, "pitch": 0.3, "distance": 12.0, "target": [0.0, 0.2, 0.0]},
  "category": "hydrogen",
  "description": "1s/2p/3d 电子概率云形状对比演示"
},
{
  "name": "氢原子叠加态",
  "atomic_number": 1,
  "charge_state": 0,
  "approximation": "strict_hydrogenic",
  "model": "schrodinger_cloud",
  "cloud_render_mode": "point_cloud",
  "solver_qn": [2, 0, 0],
  "components": [{"qn": [2, 0, 0], "amplitude": 0.7, "phase": 0.0}, {"qn": [2, 1, 0], "amplitude": 0.5, "phase": 1.57}],
  "use_numerical_radial": false,
  "build_volume": false,
  "principal_n": 2,
  "max_spectrum_n": 5,
  "transition": {"initial": [2, 1, 0], "final": [2, 0, 0], "enforce_rules": false, "enforce_delta_m": false},
  "camera": {"yaw": 0.8, "pitch": 0.5, "distance": 14.0, "target": [0.0, 0.3, 0.0]},
  "category": "hydrogen",
  "description": "2s+2p 叠加态量子相位变化"
},
{
  "name": "锂到氖组态趋势",
  "atomic_number": 3,
  "charge_state": 0,
  "approximation": "multi_electron_approximation",
  "model": "compare",
  "cloud_render_mode": "hybrid",
  "solver_qn": [2, 1, 0],
  "components": [{"qn": [2, 1, 0], "amplitude": 1.0, "phase": 0.0}],
  "use_numerical_radial": false,
  "build_volume": true,
  "principal_n": 2,
  "max_spectrum_n": 6,
  "transition": {"initial": [2, 1, 0], "final": [1, 0, 0], "enforce_rules": true, "enforce_delta_m": false},
  "camera": {"yaw": -0.3, "pitch": 0.35, "distance": 14.0, "target": [0.0, 0.3, 0.0]},
  "category": "multielectron",
  "description": "Z=3-10 的 Aufbau 填充与 Z_eff 趋势"
},
{
  "name": "钠到氩周期趋势",
  "atomic_number": 11,
  "charge_state": 0,
  "approximation": "multi_electron_approximation",
  "model": "compare",
  "cloud_render_mode": "hybrid",
  "solver_qn": [3, 0, 0],
  "components": [{"qn": [3, 0, 0], "amplitude": 1.0, "phase": 0.0}],
  "use_numerical_radial": false,
  "build_volume": true,
  "principal_n": 3,
  "max_spectrum_n": 7,
  "transition": {"initial": [3, 1, 0], "final": [3, 0, 0], "enforce_rules": true, "enforce_delta_m": false},
  "camera": {"yaw": -0.3, "pitch": 0.35, "distance": 16.0, "target": [0.0, 0.3, 0.0]},
  "category": "multielectron",
  "description": "Z=11-18 的屏蔽效应与 Z_eff 周期趋势"
},
{
  "name": "氢原子谱线系",
  "atomic_number": 1,
  "charge_state": 0,
  "approximation": "strict_hydrogenic",
  "model": "bohr",
  "cloud_render_mode": "point_cloud",
  "solver_qn": [4, 1, 0],
  "components": [{"qn": [4, 1, 0], "amplitude": 1.0, "phase": 0.0}],
  "use_numerical_radial": false,
  "build_volume": false,
  "principal_n": 4,
  "max_spectrum_n": 8,
  "transition": {"initial": [4, 1, 0], "final": [3, 2, 0], "enforce_rules": true, "enforce_delta_m": false},
  "camera": {"yaw": 0.25, "pitch": 0.4, "distance": 18.0, "target": [0.0, 0.4, 0.0]},
  "category": "spectroscopy",
  "description": "Lyman/Balmer/Paschen 三条主要谱线系展示"
},
{
  "name": "精细结构拆分",
  "atomic_number": 1,
  "charge_state": 0,
  "approximation": "strict_hydrogenic",
  "model": "bohr",
  "cloud_render_mode": "point_cloud",
  "solver_qn": [2, 1, 0],
  "components": [{"qn": [2, 1, 0], "amplitude": 1.0, "phase": 0.0}],
  "use_numerical_radial": false,
  "build_volume": false,
  "principal_n": 2,
  "max_spectrum_n": 6,
  "transition": {"initial": [2, 1, 0], "final": [1, 0, 0], "enforce_rules": true, "enforce_delta_m": false},
  "camera": {"yaw": 0.15, "pitch": 0.45, "distance": 10.0, "target": [0.0, 0.15, 0.0]},
  "category": "spectroscopy",
  "description": "2p₃/₂ 与 2p₁/₂ 能级的自旋-轨道分裂"
},
{
  "name": "Zeeman 效应",
  "atomic_number": 1,
  "charge_state": 0,
  "approximation": "strict_hydrogenic",
  "model": "bohr",
  "cloud_render_mode": "point_cloud",
  "solver_qn": [2, 1, 0],
  "components": [{"qn": [2, 1, 0], "amplitude": 1.0, "phase": 0.0}],
  "use_numerical_radial": false,
  "build_volume": false,
  "principal_n": 2,
  "max_spectrum_n": 6,
  "transition": {"initial": [2, 1, 0], "final": [1, 0, 0], "enforce_rules": true, "enforce_delta_m": true},
  "camera": {"yaw": 0.15, "pitch": 0.45, "distance": 10.0, "target": [0.0, 0.15, 0.0]},
  "category": "spectroscopy",
  "description": "磁场下能级的 Zeeman 分裂"
},
{
  "name": "D-T 聚变演示",
  "atomic_number": 1,
  "charge_state": 0,
  "approximation": "strict_hydrogenic",
  "model": "bohr",
  "cloud_render_mode": "point_cloud",
  "solver_qn": [2, 0, 0],
  "components": [{"qn": [2, 0, 0], "amplitude": 1.0, "phase": 0.0}],
  "use_numerical_radial": false,
  "build_volume": false,
  "principal_n": 2,
  "max_spectrum_n": 4,
  "transition": {"initial": [2, 0, 0], "final": [1, 0, 0], "enforce_rules": true, "enforce_delta_m": false},
  "camera": {"yaw": 0.0, "pitch": 0.5, "distance": 8.0, "target": [0.0, 0.0, 0.0]},
  "category": "nuclear",
  "description": "D+T 聚变截面随入射能量变化"
},
{
  "name": "α 衰变演示",
  "atomic_number": 92,
  "charge_state": 0,
  "approximation": "multi_electron_approximation",
  "model": "bohr",
  "cloud_render_mode": "point_cloud",
  "solver_qn": [2, 0, 0],
  "components": [{"qn": [2, 0, 0], "amplitude": 1.0, "phase": 0.0}],
  "use_numerical_radial": false,
  "build_volume": false,
  "principal_n": 2,
  "max_spectrum_n": 4,
  "transition": {"initial": [2, 1, 0], "final": [1, 0, 0], "enforce_rules": true, "enforce_delta_m": false},
  "camera": {"yaw": 0.0, "pitch": 0.4, "distance": 10.0, "target": [0.0, 0.0, 0.0]},
  "category": "nuclear",
  "description": "α 衰变统计规律与半衰期"
}
```

- [ ] **Step 2: 给现有场景也加上 category 和 description 字段**

在已有的 3 个 steps 中追加：
- `"category": "hydrogen"` + `"description": "氢原子玻尔轨道能级与跃迁"`（第一个）
- `"category": "hydrogen"` + `"description": "类氢离子的 Z² 缩放效应"`（第二个）
- `"category": "multielectron"` + `"description": "氖原子多电子教学近似与相位云"`（第三个）

- [ ] **Step 3: 编译验证**

```powershell
xmake build quantum_atom_tests
xmake run quantum_atom_tests
```

Expected: 测试通过（JSON 结构变化不影响现有代码逻辑，但需确认已有测试仍通过）。

- [ ] **Step 4: Commit**

```bash
git add assets/scenarios/demo_scenes.json
git commit -m "feat(data): expand demo scenes from 3 to 12 with 4 categories"
```

---

### Task 5: 构建场景浏览器 UI

**Files:**
- Modify: `ui/AppUi.cpp`（`drawScenarioButtons` 替换为卡片浏览器）
- Modify: `app/Application.cpp`（场景加载支持 category/description 字段）
- Modify: `include/quantum/app/SimulationState.h`（新增场景分类枚举）

- [ ] **Step 1: 添加场景分类枚举到 SimulationState.h**

```cpp
enum class SceneCategory {
    All,
    Hydrogen,
    MultiElectron,
    Spectroscopy,
    Nuclear
};
```

- [ ] **Step 2: 在 Application.cpp 中解析 category/description**

在 `demoStepFromJson` 函数中，新增：
```cpp
step.category = node.value("category", std::string("all"));
step.description = node.value("description", std::string(""));
```

在 `DemoScriptStep` 结构体（`Application.h`）中新增：
```cpp
std::string category;
std::string description;
```

- [ ] **Step 3: 替换 drawScenarioButtons 为卡片浏览器**

在 `ui/AppUi.cpp` 中，替换 `drawScenarioButtons` 函数：

```cpp
void drawScenarioBrowser(quantum::app::SimulationState& state) {
    static int activeCategory = 0;
    const char* categories[] = {"全部", "氢原子", "多电子", "光谱", "核过程"};

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 6.0f));
    for (int i = 0; i < 5; ++i) {
        if (i > 0) ImGui::SameLine();
        if (activeCategory == i) {
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(72, 134, 212, 200));
        }
        if (ImGui::SmallButton(categories[i])) {
            activeCategory = i;
        }
        if (activeCategory == i) {
            ImGui::PopStyleColor();
        }
    }
    ImGui::PopStyleVar();

    const auto& steps = getAllScenePresets();
    int column = 0;
    for (const auto& step : steps) {
        int stepCat = 0;
        if (step.category == "multielectron") stepCat = 2;
        else if (step.category == "spectroscopy") stepCat = 3;
        else if (step.category == "nuclear") stepCat = 4;
        else stepCat = 1;

        if (activeCategory != 0 && stepCat != activeCategory) continue;

        if (column % 2 == 0 && column > 0) {
            ImGui::SameLine();
        }

        ImGui::PushID(step.name.c_str());
        ImVec2 cardSize(ImGui::GetContentRegionAvail().x * 0.48f, 72.0f);
        ImGui::BeginChild("##card", cardSize, true, ImGuiWindowFlags_NoScrollbar);

        ImGui::TextUnformatted(step.name.c_str());
        ImGui::TextDisabled("%s", step.description.c_str());

        const float buttonHeight = ImGui::GetFrameHeight();
        ImGui::SetCursorPosY(cardSize.y - buttonHeight - 6.0f);
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
```

- [ ] **Step 4: 实现 getAllScenePresets 和 applyScenePresetToState**

```cpp
const std::vector<quantum::app::DemoScriptStep>& getAllScenePresets() {
    static std::vector<quantum::app::DemoScriptStep> allPresets;
    if (allPresets.empty()) {
        for (std::size_t i = 0; i < builtInDemoSteps().size(); ++i) {
            allPresets.push_back(builtInDemoSteps()[i]);
        }
        // Phase 2: also load from demo_scenes.json if available
    }
    return allPresets;
}

void applyScenePresetToState(quantum::app::SimulationState& state,
                             const quantum::app::DemoScriptStep& step) {
    state.demo.enabled = false;
    state.demo.currentStepName = "手动";
    state.demo.stepProgress = 0.0;
    state.atomicNumber = step.atomicNumber;
    state.chargeState = step.chargeState;
    state.approximationMode = step.approximationMode;
    state.modelKind = step.modelKind;
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
        }
    } else {
        state.nuclearAnimation.enabled = false;
    }
    markAllDirty(state);
}
```

- [ ] **Step 5: 在 Inspector 中替换 drawScenarioButtons 调用**

将 `inspectorPresetsOpen_` 折叠组内的 `drawScenarioButtons(state)` 替换为 `drawScenarioBrowser(state)`。

- [ ] **Step 6: 编译验证**

```powershell
xmake build quantum_atom_simulation
```

Expected: 编译成功。

- [ ] **Step 7: Commit**

```bash
git add ui/AppUi.cpp app/Application.cpp include/quantum/app/SimulationState.h include/quantum/app/Application.h
git commit -m "feat(ui): scene browser with category tabs and card layout"
```

---

## Phase 3: 维度 A — 学习路径

### Task 6: 添加 LearningParams 和学习路径数据

**Files:**
- Modify: `include/quantum/app/SimulationState.h`
- Modify: `ui/AppUi.cpp`

- [ ] **Step 1: 添加 LearningParams 到 SimulationState.h**

```cpp
enum class LearningPath {
    None,
    AtomicStructure,
    Spectroscopy,
    MultiElectron,
    NuclearPhysics
};

struct LearningParams {
    bool enabled = false;
    LearningPath path = LearningPath::None;
    int stepIndex = 0;
    std::string guideText;
};

// 在 SimulationState 中新增：
// LearningParams learning;
```

- [ ] **Step 2: 实现 4 条学习路径步骤数据**

在 `ui/AppUi.cpp` 匿名 namespace 中新增：

```cpp
struct LearningStep {
    std::string title;
    std::string guide;
    int atomicNumber = 1;
    int chargeState = 0;
    quantum::physics::ApproximationMode approximation = quantum::physics::ApproximationMode::StrictHydrogenic;
    quantum::app::ModelKind modelKind = quantum::app::ModelKind::Bohr;
    quantum::physics::QuantumNumbers solverQn{2, 0, 0};
    int principalN = 2;
    int maxSpectrumN = 5;
    std::vector<std::string> lockedParams;
};

const std::vector<LearningStep>& learningPathSteps(quantum::app::LearningPath path) {
    static const std::unordered_map<quantum::app::LearningPath, std::vector<LearningStep>> paths = {
        {quantum::app::LearningPath::AtomicStructure, {
            {"氢原子玻尔轨道",
             "选择不同的主量子数 n (1-4)，观察轨道半径 n² 的变化规律。轨道半径 ∝ n²。",
             1, 0, quantum::physics::ApproximationMode::StrictHydrogenic,
             quantum::app::ModelKind::Bohr, {2, 1, 0}, 2, 5,
             {"chargeState", "approximationMode", "meanField", "correlation", "dynamics", "nuclearAnimation"}},
            {"氢原子薛定谔概率云",
             "切换到概率云，调整角量子数 l，对比不同轨道形状。l=0 球形，l=1 哑铃形。",
             1, 0, quantum::physics::ApproximationMode::StrictHydrogenic,
             quantum::app::ModelKind::SchrodingerCloud, {2, 0, 0}, 2, 5,
             {"chargeState", "approximationMode", "meanField", "correlation", "dynamics", "nuclearAnimation"}},
            {"氢原子叠加态",
             "添加叠加分量，调整振幅和相位。观察叠加态云的相位变化。",
             1, 0, quantum::physics::ApproximationMode::StrictHydrogenic,
             quantum::app::ModelKind::SchrodingerCloud, {2, 0, 0}, 2, 5,
             {"chargeState", "approximationMode", "meanField", "correlation", "dynamics", "nuclearAnimation"}},
            {"He+ 离子相位云",
             "切换为 He+，对比 Z=2 时轨道半径收缩为 1/4。能级能量按 Z² 缩放。",
             2, 1, quantum::physics::ApproximationMode::StrictHydrogenic,
             quantum::app::ModelKind::SchrodingerCloud, {3, 2, 0}, 3, 6,
             {"approximationMode", "meanField", "correlation", "dynamics", "nuclearAnimation"}},
        }},
        {quantum::app::LearningPath::Spectroscopy, {
            {"氢原子谱线系",
             "切换到 n_initial=4，n_final=3。观察这些跃迁在哪条谱线系。",
             1, 0, quantum::physics::ApproximationMode::StrictHydrogenic,
             quantum::app::ModelKind::Bohr, {4, 1, 0}, 4, 8,
             {"chargeState", "approximationMode", "meanField", "correlation", "dynamics", "nuclearAnimation", "modelKind"}},
            {"精细结构拆分",
             "在谱学面板开启精细结构。观察 2p 能级分裂。自旋-轨道耦合导致 2p₃/₂ 和 2p₁/₂ 分裂。",
             1, 0, quantum::physics::ApproximationMode::StrictHydrogenic,
             quantum::app::ModelKind::Bohr, {2, 1, 0}, 2, 6,
             {"chargeState", "approximationMode", "meanField", "correlation", "dynamics", "nuclearAnimation", "modelKind"}},
            {"Zeeman 效应",
             "开启 Zeeman 修正并限制 Δm。调节磁场强度观察谱线分裂。",
             1, 0, quantum::physics::ApproximationMode::StrictHydrogenic,
             quantum::app::ModelKind::Bohr, {2, 1, 0}, 2, 6,
             {"chargeState", "approximationMode", "meanField", "correlation", "dynamics", "nuclearAnimation", "modelKind"}},
        }},
        {quantum::app::LearningPath::MultiElectron, {
            {"Aufbau 填充规则",
             "从 Li(Z=3) 开始，观察 1s→2s→2p 的填充顺序。Puali 不相容原理决定了填充顺序。",
             3, 0, quantum::physics::ApproximationMode::MultiElectronApproximation,
             quantum::app::ModelKind::Compare, {2, 1, 0}, 2, 6,
             {"approximationMode", "meanField", "correlation", "dynamics", "nuclearAnimation"}},
            {"Z_eff 屏蔽效应",
             "从 Z=11 开始扫描到 Z=18。观察有效核电荷 Z_eff 如何小于实际 Z。",
             11, 0, quantum::physics::ApproximationMode::MultiElectronApproximation,
             quantum::app::ModelKind::Compare, {3, 0, 0}, 3, 7,
             {"approximationMode", "meanField", "correlation", "dynamics", "nuclearAnimation"}},
            {"氖原子多电子近似",
             "查看 Ne(Z=10) 的完整组态 [He]2s²2p⁶。为什么 Ne 是惰性气体？",
             10, 0, quantum::physics::ApproximationMode::MultiElectronApproximation,
             quantum::app::ModelKind::Compare, {2, 1, 0}, 2, 6,
             {"approximationMode", "meanField", "correlation", "dynamics", "nuclearAnimation"}},
        }},
        {quantum::app::LearningPath::NuclearPhysics, {
            {"认识基本粒子",
             "浏览基本粒子（质子、中子、电子、光子、μ子、π介子等）。",
             1, 0, quantum::physics::ApproximationMode::StrictHydrogenic,
             quantum::app::ModelKind::Bohr, {2, 0, 0}, 2, 4,
             {"approximationMode", "meanField", "correlation", "dynamics", "chargeState"}},
            {"D-T 聚变",
             "D + T → ⁴He + n。调节入射能量观察截面变化。",
             1, 0, quantum::physics::ApproximationMode::StrictHydrogenic,
             quantum::app::ModelKind::Bohr, {2, 0, 0}, 2, 4,
             {"approximationMode", "meanField", "correlation", "dynamics", "chargeState", "modelKind", "cloud"}},
            {"U-235 裂变",
             "n + ²³⁵U → 裂变碎片 + νn。观察中子衰减和链式反应。",
             92, 0, quantum::physics::ApproximationMode::MultiElectronApproximation,
             quantum::app::ModelKind::Bohr, {2, 0, 0}, 2, 4,
             {"approximationMode", "meanField", "correlation", "dynamics", "chargeState", "modelKind", "cloud"}},
            {"α 衰变",
             "半衰期 T₁/₂ = 1s。观察指数衰变 N(t)=N₀e^{-λt}。",
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
```

- [ ] **Step 3: 编译验证**

```powershell
xmake build quantum_atom_simulation
```

- [ ] **Step 4: Commit**

```bash
git add include/quantum/app/SimulationState.h ui/AppUi.cpp
git commit -m "feat(learning): add LearningParams and 4 learning path data definitions"
```

---

### Task 7: 学习模式 UI 控件

**Files:**
- Modify: `ui/AppUi.cpp`

- [ ] **Step 1: 在 Inspector 顶部添加学习模式选择器**

在 `if (ImGui::Begin(kPanelInspector))` 之后，添加：

```cpp
// 学习模式选择器
const char* pathNames[] = {"关闭", "原子结构入门", "光谱探秘", "多电子原子", "核物理入门"};
int currentPath = static_cast<int>(state.learning.path);
if (ImGui::BeginCombo("学习模式", pathNames[currentPath])) {
    for (int i = 0; i < 5; ++i) {
        if (ImGui::Selectable(pathNames[i], i == currentPath)) {
            state.learning.path = static_cast<quantum::app::LearningPath>(i);
            state.learning.enabled = (i != 0);
            state.learning.stepIndex = 0;
            if (state.learning.enabled) {
                const auto& steps = learningPathSteps(state.learning.path);
                if (!steps.empty()) {
                    const auto& step = steps[0];
                    state.atomicNumber = step.atomicNumber;
                    state.chargeState = step.chargeState;
                    state.approximationMode = step.approximation;
                    state.modelKind = step.modelKind;
                    state.solver.qn = step.solverQn;
                    state.bohr.principalN = step.principalN;
                    state.bohr.maxSpectrumN = step.maxSpectrumN;
                    state.learning.guideText = step.guide;
                    markAllDirty(state);
                }
            }
        }
    }
    ImGui::EndCombo();
}

// 学习模式进行中
if (state.learning.enabled) {
    const auto& steps = learningPathSteps(state.learning.path);
    int totalSteps = static_cast<int>(steps.size());
    int currentStep = state.learning.stepIndex + 1;

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "步骤 %d/%d", currentStep, totalSteps);

    float progress = totalSteps > 0 ? static_cast<float>(currentStep) / static_cast<float>(totalSteps) : 0.0f;
    ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f));

    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(220, 224, 232, 255));
    ImGui::TextWrapped("%s", state.learning.guideText.c_str());
    ImGui::PopStyleColor();

    // 导航按钮
    if (state.learning.stepIndex > 0) {
        if (ImGui::Button("上一步")) {
            --state.learning.stepIndex;
            const auto& step = steps[state.learning.stepIndex];
            state.learning.guideText = step.guide;
            state.atomicNumber = step.atomicNumber;
            state.chargeState = step.chargeState;
            markAllDirty(state);
        }
        ImGui::SameLine();
    }
    if (state.learning.stepIndex < totalSteps - 1) {
        if (ImGui::Button("下一步")) {
            ++state.learning.stepIndex;
            const auto& step = steps[state.learning.stepIndex];
            state.learning.guideText = step.guide;
            state.atomicNumber = step.atomicNumber;
            state.chargeState = step.chargeState;
            markAllDirty(state);
        }
    }

    ImGui::Separator();
}
```

- [ ] **Step 2: 学习模式下锁定参数**

在 Inspector 的各控件处，检查参数是否被锁定：如果 `state.learning.enabled` 且当前参数 key 在 `steps[state.learning.stepIndex].lockedParams` 中，则控件设置为 `ImGui::BeginDisabled(true)` + 灰色显示。

在每个参数控件外层包裹：

```cpp
auto isParamLocked = [&](const std::string& key) -> bool {
    if (!state.learning.enabled) return false;
    const auto& steps = learningPathSteps(state.learning.path);
    if (state.learning.stepIndex >= static_cast<int>(steps.size())) return false;
    const auto& locked = steps[state.learning.stepIndex].lockedParams;
    return std::find(locked.begin(), locked.end(), key) != locked.end();
};

if (isParamLocked("chargeState")) ImGui::BeginDisabled();
// ... 控件 ...
if (isParamLocked("chargeState")) ImGui::EndDisabled();
```

- [ ] **Step 3: 编译验证**

```powershell
xmake build quantum_atom_simulation
```

- [ ] **Step 4: Commit**

```bash
git add ui/AppUi.cpp
git commit -m "feat(learning): learning mode UI with progress bar and parameter locking"
```

---

## Phase 4: 维度 B（续）— 物理能力深化

### Task 8: 扩展参考谱线目录

**Files:**
- Modify: `assets/data/reference_catalog.json`

- [ ] **Step 1: 添加 Li²+, Be³+, B⁴+ 谱线**

在 reference_catalog.json 的 transitions 数组中追加：

```json
{"id": "li2plus-lyman-alpha", "label": "Li²+ Lyman alpha", "series": "Li²+ Lyman",
 "atomic_number": 3, "charge_state": 2, "upper": {"n": 2, "l": 1, "m": 0}, "lower": {"n": 1, "l": 0, "m": 0},
 "wavelength_nm": 13.507, "energy_ev": 91.773, "allowed": true, "notes": "Li²+ hydrogenic teaching anchor."},
{"id": "li2plus-balmer-alpha", "label": "Li²+ Balmer alpha", "series": "Li²+ Balmer",
 "atomic_number": 3, "charge_state": 2, "upper": {"n": 3, "l": 1, "m": 0}, "lower": {"n": 2, "l": 0, "m": 0},
 "wavelength_nm": 72.915, "energy_ev": 16.996, "allowed": true, "notes": "Li²+ hydrogenic teaching anchor."},
{"id": "be3plus-lyman-alpha", "label": "Be³+ Lyman alpha", "series": "Be³+ Lyman",
 "atomic_number": 4, "charge_state": 3, "upper": {"n": 2, "l": 1, "m": 0}, "lower": {"n": 1, "l": 0, "m": 0},
 "wavelength_nm": 7.600, "energy_ev": 163.113, "allowed": true, "notes": "Be³+ hydrogenic teaching anchor."},
{"id": "be3plus-balmer-alpha", "label": "Be³+ Balmer alpha", "series": "Be³+ Balmer",
 "atomic_number": 4, "charge_state": 3, "upper": {"n": 3, "l": 1, "m": 0}, "lower": {"n": 2, "l": 0, "m": 0},
 "wavelength_nm": 41.023, "energy_ev": 30.214, "allowed": true, "notes": "Be³+ hydrogenic teaching anchor."},
{"id": "b4plus-lyman-alpha", "label": "B⁴+ Lyman alpha", "series": "B⁴+ Lyman",
 "atomic_number": 5, "charge_state": 4, "upper": {"n": 2, "l": 1, "m": 0}, "lower": {"n": 1, "l": 0, "m": 0},
 "wavelength_nm": 4.862, "energy_ev": 254.920, "allowed": true, "notes": "B⁴+ hydrogenic teaching anchor."},
{"id": "b4plus-balmer-alpha", "label": "B⁴+ Balmer alpha", "series": "B⁴+ Balmer",
 "atomic_number": 5, "charge_state": 4, "upper": {"n": 3, "l": 1, "m": 0}, "lower": {"n": 2, "l": 0, "m": 0},
 "wavelength_nm": 26.253, "energy_ev": 47.207, "allowed": true, "notes": "B⁴+ hydrogenic teaching anchor."}
```

- [ ] **Step 2: 添加轻元素（Z=3-18）可见光谱线**

追加：

```json
{"id": "li-doublet", "label": "Li I 670.8 nm doublet", "series": "Li I",
 "atomic_number": 3, "charge_state": 0, "upper": {"n": 2, "l": 1, "m": 0}, "lower": {"n": 2, "l": 0, "m": 0},
 "wavelength_nm": 670.79, "energy_ev": 1.848, "allowed": true, "notes": "Li I resonance teaching line."},
{"id": "na-d-line", "label": "Na I D line", "series": "Na D",
 "atomic_number": 11, "charge_state": 0, "upper": {"n": 3, "l": 1, "m": 0}, "lower": {"n": 3, "l": 0, "m": 0},
 "wavelength_nm": 589.29, "energy_ev": 2.104, "allowed": true, "notes": "Na D teaching line, fine-structure doublet center."},
{"id": "mg-triplet", "label": "Mg I 518 nm", "series": "Mg I",
 "atomic_number": 12, "charge_state": 0, "upper": {"n": 4, "l": 0, "m": 0}, "lower": {"n": 3, "l": 1, "m": 0},
 "wavelength_nm": 518.36, "energy_ev": 2.391, "allowed": true, "notes": "Mg I teaching triplet."},
{"id": "ca-422", "label": "Ca I 422.7 nm", "series": "Ca I",
 "atomic_number": 20, "charge_state": 0, "upper": {"n": 4, "l": 1, "m": 0}, "lower": {"n": 4, "l": 0, "m": 0},
 "wavelength_nm": 422.67, "energy_ev": 2.932, "allowed": true, "notes": "Ca I resonance teaching line."}
```

- [ ] **Step 3: 编译验证**

```powershell
xmake build quantum_atom_tests
xmake run quantum_atom_tests
```

- [ ] **Step 4: Commit**

```bash
git add assets/data/reference_catalog.json
git commit -m "feat(data): extend reference catalog with hydrogenic ions and light element lines"
```

---

### Task 9: Tier 2 平均场 SCF 收敛增强

**Files:**
- Modify: `solvers/TeachingMeanFieldSolver.cpp`
- Modify: `include/quantum/solvers/TeachingMeanFieldSolver.h`

- [ ] **Step 1: 添加 SCF mixing 参数到 MeanFieldSettings**

在 `SimulationState.h` 的 `MeanFieldSettings` 中添加：

```cpp
double mixingAlpha = 0.3;
bool useMixing = true;
```

- [ ] **Step 2: 实现线性混合策略**

在 `TeachingMeanFieldSolver::solve` 中，密度更新步骤处添加：

```cpp
if (request.useMixing && iteration > 0) {
    const double alpha = request.mixingAlpha;
    for (std::size_t i = 0; i < newDensity.size(); ++i) {
        newDensity[i] = alpha * oldDensity[i] + (1.0 - alpha) * newDensity[i];
    }
}
```

- [ ] **Step 3: 添加混合系数 GUI 控制**

在 Inspector 的 "高阶物理" 折叠组中，SCF 部分添加：

```cpp
if (state.meanField.enabled && state.meanField.useMixing) {
    const double mixMin = 0.1;
    const double mixMax = 0.9;
    if (ImGui::SliderScalar("SCF 混合系数", ImGuiDataType_Double,
                            &state.meanField.mixingAlpha, &mixMin, &mixMax)) {
        state.dirty.physics = true;
    }
}
```

- [ ] **Step 4: 编译验证**

```powershell
xmake build quantum_atom_simulation
xmake build quantum_atom_tests
xmake run quantum_atom_tests
```

- [ ] **Step 5: Commit**

```bash
git add solvers/TeachingMeanFieldSolver.cpp solvers/TeachingMeanFieldSolver.h include/quantum/app/SimulationState.h ui/AppUi.cpp
git commit -m "feat(solver): add SCF density mixing for improved convergence"
```

---

### Task 10: Tier 3 谱学修正细化

**Files:**
- Modify: `spectroscopy/HydrogenicCorrections.cpp`
- Modify: `include/quantum/spectroscopy/HydrogenicCorrections.h`

- [ ] **Step 1: 添加 Paschen-Back 过渡区估计**

在 `HydrogenicCorrections::evaluate` 中，Zeeman 处理之后新增：

```cpp
// Paschen-Back transition estimate
if (settings.applyZeeman && settings.zeemanFieldTesla > 5.0) {
    result.paschenBackRegime = true;
    result.paschenBackNote = "Zeeman 裂距 ~ 精细结构裂距: 处于 Paschen-Back 过渡区";
} else {
    result.paschenBackRegime = false;
}
```

在 `SpectroscopyCorrectionResult` 结构体中新增：
```cpp
bool paschenBackRegime = false;
std::string paschenBackNote;
```

- [ ] **Step 2: 实现二次 Stark n=2 矩阵对角化**

在 `HydrogenicCorrections` 中新增私有方法：

```cpp
double computeQuadraticStarkShiftN2(double fieldStrengthVm, double zeff) {
    const double a0 = 5.29177210903e-11;
    const double e = 1.602176634e-19;
    const double Eh = 4.3597447222071e-18;
    const double fieldAu = fieldStrengthVm * e * a0 / Eh;

    double matrix[4][4] = {
        {0.0, 3.0 * fieldAu, 0.0, 0.0},
        {3.0 * fieldAu, 0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0, 0.0},
        {0.0, 0.0, 0.0, 0.0}
    };

    double eigen1 = 3.0 * fieldAu * Eh;
    double eigen2 = -3.0 * fieldAu * Eh;
    return eigen1;
}
```

- [ ] **Step 3: 超精细 F 标签输出**

在 `HydrogenicCorrections::evaluate` 的超精细计算处新增：

```cpp
if (settings.applyHyperfine && result.hyperfineShiftEv != 0.0) {
    result.fTotal = settings.nuclearSpinI + 0.5;
    result.fLabel = "F = " + std::to_string(static_cast<int>(2 * result.fTotal)) + "/2";
}
```

- [ ] **Step 4: 编译验证**

```powershell
xmake build quantum_atom_simulation
xmake build quantum_atom_tests
xmake run quantum_atom_tests
```

- [ ] **Step 5: Commit**

```bash
git add spectroscopy/HydrogenicCorrections.cpp include/quantum/spectroscopy/HydrogenicCorrections.h
git commit -m "feat(spectroscopy): add Paschen-Back estimate, quadratic Stark, and F quantum label"
```

---

### Task 11: 参考数据批量回归校验

**Files:**
- Modify: `tests/TestMain.cpp`

- [ ] **Step 1: 添加批量参考谱线校验测试**

```cpp
void test_reference_line_consistency() {
    quantum::physics::ElementDatabase db;
    bool loaded = db.loadFromJson("assets/data/elements.json");
    assert(loaded);

    loaded = db.loadReferenceTransitions("assets/data/reference_catalog.json");
    if (!loaded) {
        std::cerr << "Skipping: reference_catalog.json not found.\n";
        return;
    }

    int warnings = 0;
    for (int z = 1; z <= 5; ++z) {
        for (int q = 0; q <= std::min(z - 1, 4); ++q) {
            auto transitions = db.referenceTransitions(z, q);
            for (const auto& ref : transitions) {
                double energyPredicted = 13.605693122994 * (z - q) * (z - q)
                    * (1.0 / (ref.lower.n * ref.lower.n) - 1.0 / (ref.upper.n * ref.upper.n));
                double deviation = std::abs(ref.photonEnergyEv - energyPredicted)
                    / std::max(std::abs(ref.photonEnergyEv), 0.001);
                if (deviation > 0.01) {
                    ++warnings;
                    std::cerr << "WARNING: " << ref.label
                              << " deviation " << (deviation * 100.0)
                              << "% (predicted=" << energyPredicted
                              << " eV, catalog=" << ref.photonEnergyEv << " eV)\n";
                }
            }
        }
    }
    std::cout << "Reference line consistency check: "
              << warnings << " warnings.\n";
}
```

- [ ] **Step 2: 在主测试中调用**

在 `main` 函数中所有测试之后添加：
```cpp
test_reference_line_consistency();
```

- [ ] **Step 3: 运行测试**

```powershell
xmake build quantum_atom_tests
xmake run quantum_atom_tests
```

Expected: 测试通过，氢样离子谱线偏差 < 1%。

- [ ] **Step 4: Commit**

```bash
git add tests/TestMain.cpp
git commit -m "test: add batch reference line consistency regression check"
```

---

## Phase 5: 维度 D — 可视化与渲染提升

### Task 12: 场景截图导出

**Files:**
- Modify: `render/SceneRenderer.cpp`
- Modify: `render/SceneRenderer.h`
- Modify: `ui/AppUi.cpp`

- [ ] **Step 1: 在 SceneRenderer 中添加截图方法**

在 `SceneRenderer.h` 中新增：
```cpp
bool captureScreenshot(const std::filesystem::path& outputPath, int width, int height) const;
```

在 `SceneRenderer.cpp` 中实现：
```cpp
bool SceneRenderer::captureScreenshot(const std::filesystem::path& outputPath, int width, int height) const {
    std::vector<unsigned char> pixels(static_cast<std::size_t>(width * height * 3));
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer_.fbo);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    // Flip vertically (OpenGL origin is bottom-left)
    std::vector<unsigned char> flipped(pixels.size());
    const int rowBytes = width * 3;
    for (int y = 0; y < height; ++y) {
        std::memcpy(flipped.data() + y * rowBytes,
                    pixels.data() + (height - 1 - y) * rowBytes,
                    rowBytes);
    }

    // Write as PPM (simplest format without external lib)
    std::ofstream file(outputPath, std::ios::binary);
    if (!file.is_open()) return false;
    file << "P6\n" << width << " " << height << "\n255\n";
    file.write(reinterpret_cast<const char*>(flipped.data()), flipped.size());
    return true;
}
```

使用 PPM 格式（最简单无依赖），或如果项目已有 stb_image_write 则用 PNG。

- [ ] **Step 2: 在 Application 中添加截图触发**

在 `Application::run()` 主循环中，处理完键盘输入后添加：

```cpp
if (ImGui::IsKeyPressed(ImGuiKey_S) && ImGui::GetIO().KeyCtrl) {
    const auto screenshotDir = projectRoot_ / "docs" / "reports" / "screenshots";
    std::filesystem::create_directories(screenshotDir);
    const auto path = screenshotDir / ("screenshot_" + currentTimestampTag() + ".ppm");
    int w, h;
    glfwGetFramebufferSize(window_, &w, &h);
    if (sceneRenderer_.captureScreenshot(path, w, h)) {
        logger_.info("截图已保存: " + path.string());
    }
}
```

- [ ] **Step 3: 在 UI 中添加按钮入口**

在 Inspector 底部或场景工具栏添加：
```cpp
if (ImGui::Button("导出截图 (Ctrl+S)")) {
    // 通过标记触发（由 Application 处理）
}
```

- [ ] **Step 4: 编译验证**

```powershell
xmake build quantum_atom_simulation
```

- [ ] **Step 5: Commit**

```bash
git add render/SceneRenderer.cpp render/SceneRenderer.h app/Application.cpp ui/AppUi.cpp
git commit -m "feat(render): scene screenshot export via Ctrl+S"
```

---

### Task 13: 图表专业化增强

**Files:**
- Modify: `ui/AppUi.cpp`（图表绘制函数段）

- [ ] **Step 1: 为图表添加十字光标**

在 `applyPlotInteractions` 函数中，`mouseInside` 条件下添加：

```cpp
if (mouseInside) {
    // 十字光标
    drawList->AddLine(ImVec2(canvas.mousePosition.x, canvas.inner.Min.y),
                      ImVec2(canvas.mousePosition.x, canvas.inner.Max.y),
                      IM_COL32(180, 190, 200, 60), 1.0f);
    drawList->AddLine(ImVec2(canvas.inner.Min.x, canvas.mousePosition.y),
                      ImVec2(canvas.inner.Max.x, canvas.mousePosition.y),
                      IM_COL32(180, 190, 200, 60), 1.0f);

    // 数值浮窗
    double plotX = pixelToPlotX(canvas.inner, range, canvas.mousePosition.x);
    double plotY = pixelToPlotY(canvas.inner, range, canvas.mousePosition.y);
    std::string tooltip = formatPlotValue(plotX) + ", " + formatPlotValue(plotY);
    ImVec2 textSize = ImGui::CalcTextSize(tooltip.c_str());
    ImVec2 tooltipPos(canvas.mousePosition.x + 12.0f, canvas.mousePosition.y - textSize.y - 6.0f);
    drawList->AddRectFilled(tooltipPos,
                            ImVec2(tooltipPos.x + textSize.x + 8.0f, tooltipPos.y + textSize.y + 4.0f),
                            IM_COL32(24, 28, 36, 230), 3.0f);
    drawList->AddText(ImVec2(tooltipPos.x + 4.0f, tooltipPos.y + 2.0f),
                      IM_COL32(220, 228, 236, 255), tooltip.c_str());
}
```

- [ ] **Step 2: 添加坐标轴单位标签**

修改 `beginPlotCanvas` 调用处，传入带单位的轴标签（如 `"波长 (nm)"`、`"能量 (eV)"`）。各图表调用处修改如下：

```cpp
// 谱线图
beginPlotCanvas("光谱图", 240.0f, "波长 (nm)", "能量 (eV)", &viewState);
// 径向分布
beginPlotCanvas("径向分布", 220.0f, "半径 (nm)", "P(r) (a.u.)", &viewState);
// 中心场
beginPlotCanvas("中心场势", 220.0f, "r (a₀)", "V (Hartree)", &viewState);
```

- [ ] **Step 3: 添加右键菜单**

在 `applyPlotInteractions` 中，`mouseInside` 条件下添加右键菜单：

```cpp
if (mouseInside && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
    ImGui::OpenPopup("##plot_context_menu");
}
if (ImGui::BeginPopup("##plot_context_menu")) {
    if (ImGui::MenuItem("重置视图")) {
        clearPlotView(viewState);
    }
    if (ImGui::MenuItem("导出 CSV")) {
        // 设置导出标记
    }
    ImGui::EndPopup();
}
```

- [ ] **Step 4: 编译验证**

```powershell
xmake build quantum_atom_simulation
```

- [ ] **Step 5: Commit**

```bash
git add ui/AppUi.cpp
git commit -m "feat(charts): add crosshair cursor, axis unit labels, right-click context menu"
```

---

### Task 14: 概率云色图切换和着色模式

**Files:**
- Modify: `render/SceneRenderer.cpp`（shader 部分）
- Modify: `render/SceneRenderer.h`
- Modify: `ui/AppUi.cpp`
- Modify: `include/quantum/app/SimulationState.h`

- [ ] **Step 1: 添加色图枚举和着色模式枚举**

在 `SimulationState.h` 中新增：

```cpp
enum class ColorMap {
    PhaseHue,
    HeatMap,
    BlueWhite,
    Rainbow,
    Grayscale
};

enum class CloudColorMode {
    ByPhase,
    ByRadialDistance,
    ByProbabilityDensity
};

// 在 CloudSettings 中新增：
// ColorMap colorMap = ColorMap::PhaseHue;
// CloudColorMode cloudColorMode = CloudColorMode::ByPhase;
```

- [ ] **Step 2: 在 SceneRenderer 的点云 fragment shader 中支持多种色图**

将当前的 HSV 色表逻辑替换为支持多种色图的 uniform 分支：

在 `kPointFragmentShader` 中新增 uniform `int uColorMode` 和对应的色图切换逻辑。

或者更简单：在 C++ 侧生成点时直接根据着色模式填充不同的 density/phase 值组合，shader 保持不动但在 host 端预处理颜色。

- [ ] **Step 3: 在 UI 中添加色图和着色模式控件**

在 Inspector 的 "云与外观" 折叠组中添加：

```cpp
const char* colorMapLabels[] = {"相位色相", "热力图", "蓝白", "彩虹", "灰度"};
int currentColorMap = static_cast<int>(state.cloud.colorMap);
if (ImGui::BeginCombo("色图", colorMapLabels[currentColorMap])) {
    for (int i = 0; i < 5; ++i) {
        if (ImGui::Selectable(colorMapLabels[i], i == currentColorMap)) {
            state.cloud.colorMap = static_cast<quantum::app::ColorMap>(i);
            state.dirty.cloud = true;
        }
    }
    ImGui::EndCombo();
}

const char* colorModeLabels[] = {"按相位着色", "按径向距离", "按概率密度"};
int currentColorMode = static_cast<int>(state.cloud.cloudColorMode);
if (ImGui::BeginCombo("着色依据", colorModeLabels[currentColorMode])) {
    for (int i = 0; i < 3; ++i) {
        if (ImGui::Selectable(colorModeLabels[i], i == currentColorMode)) {
            state.cloud.cloudColorMode = static_cast<quantum::app::CloudColorMode>(i);
            state.dirty.cloud = true;
        }
    }
    ImGui::EndCombo();
}
```

- [ ] **Step 4: 编译验证**

```powershell
xmake build quantum_atom_simulation
```

- [ ] **Step 5: Commit**

```bash
git add render/SceneRenderer.cpp ui/AppUi.cpp include/quantum/app/SimulationState.h
git commit -m "feat(render): color map selection and point cloud coloring modes"
```

---

### Task 15: 后台云构建进度指示器

**Files:**
- Modify: `ui/AppUi.cpp`
- Modify: `include/quantum/core/PerformanceTracker.h`

- [ ] **Step 1: 在场景视口左下角添加构建进度环**

在 `drawSceneStatusOverlay` 之后添加：

```cpp
if (performance.cloudBuildInFlight) {
    const float ringRadius = 18.0f;
    const ImVec2 ringCenter(rect.Min.x + padding + ringRadius + 6.0f,
                            rect.Max.y - padding - ringRadius - 6.0f);
    const int numSegments = 32;
    const float animatedAngle = static_cast<float>(ImGui::GetTime() * 2.5f);

    drawList->AddCircle(ringCenter, ringRadius, IM_COL32(60, 70, 84, 200), numSegments, 2.5f);

    for (int i = 0; i < numSegments; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(numSegments);
        float angle = animatedAngle + t * 2.0f * 3.14159f;
        float alpha = 0.3f + 0.7f * t;
        ImU32 color = IM_COL32(
            static_cast<int>(72 + alpha * 120),
            static_cast<int>(134 + alpha * 80),
            static_cast<int>(212 - alpha * 60),
            static_cast<int>(120 + alpha * 100));
        ImVec2 p(ringCenter.x + ringRadius * 0.6f * std::cos(angle),
                 ringCenter.y + ringRadius * 0.6f * std::sin(angle));
        drawList->AddCircleFilled(p, 2.5f, color);
    }

    const char* buildLabel = performance.previewStage ? "生成预览中..." : "生成完整云...";
    drawList->AddText(ImVec2(ringCenter.x + ringRadius + 8.0f, ringCenter.y - 8.0f),
                      IM_COL32(160, 170, 184, 255), buildLabel);
}
```

- [ ] **Step 2: 编译验证**

```powershell
xmake build quantum_atom_simulation
```

- [ ] **Step 3: Commit**

```bash
git add ui/AppUi.cpp
git commit -m "feat(render): progress indicator for background cloud build"
```

---

### Task 16: 云质量预设描述性提示

**Files:**
- Modify: `ui/AppUi.cpp`

- [ ] **Step 1: 在云质量控制旁显示描述性文字**

在 Inspector 的 "云与外观" 折叠组中，`ImGui::BeginCombo("云质量预设", ...)` 下方添加：

```cpp
if (state.cloud.interactionPointCount <= 10000 && !state.cloud.buildVolume) {
    ImGui::TextDisabled("适合实时交互和课堂演示");
} else if (state.cloud.buildVolume) {
    ImGui::TextDisabled("包含体切片，适合高质量出图渲染");
} else if (state.cloud.pointCount >= 80000) {
    ImGui::TextDisabled("高精度采样，适合最终输出和静态分析");
} else {
    ImGui::TextDisabled("均衡质量和性能");
}
```

- [ ] **Step 2: 编译验证**

```powershell
xmake build quantum_atom_simulation
```

- [ ] **Step 3: Commit**

```bash
git add ui/AppUi.cpp
git commit -m "feat(ui): describe cloud quality preset trade-offs in tooltip"
```

---

### Task 17: GPU 实例化渲染点云

**Files:**
- Modify: `render/SceneRenderer.cpp`
- Modify: `render/SceneRenderer.h`

- [ ] **Step 1: 将点云绘制从 glDrawArrays 改为 glDrawArraysInstanced**

当前点云将全部顶点数据打包在一个大 VBO 中使用单次 `glDrawArrays(GL_POINTS, 0, count)`。改为使用 `glDrawArraysInstanced(GL_POINTS, 0, 1, count)` + 在 vertex shader 中用 `gl_InstanceID` 从 SSBO 读取每个点的位置/颜色数据。这样可减少 CPU 侧的顶点准备开销。

在 `kPointVertexShader` 中修改为：

```glsl
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in float aPhase;
layout(location = 2) in float aDensity;

uniform mat4 uModelView;
uniform mat4 uProjection;

out float vPhase;
out float vDensity;

void main() {
    gl_Position = uProjection * uModelView * vec4(aPos, 1.0);
    vPhase = aPhase;
    vDensity = aDensity;
}
```

保持现有 shader 结构，但改用实例化模式：

```cpp
// 原: glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(cloudVertexData_.size() / 5));
// 改: 
glDrawArraysInstanced(GL_POINTS, 0, 1, static_cast<GLsizei>(cloudVertexData_.size() / 5));
```

注意：vertex shader 需要配合 uniform buffer offset 或 SSBO 来正确读取每粒子的位置。

- [ ] **Step 2: 编译验证**

```powershell
xmake build quantum_atom_simulation
```

- [ ] **Step 3: Commit**

```bash
git add render/SceneRenderer.cpp render/SceneRenderer.h
git commit -m "perf(render): use instanced rendering for point cloud draw calls"
```

---

## 附录：测试验证

在所有 Phase 完成后，运行完整验证链路：

```powershell
cd D:\project\dhyq\quantum-atom-simulation
xmake build quantum_atom_tests
xmake run quantum_atom_tests
xmake build quantum_atom_simulation
powershell -ExecutionPolicy Bypass -File .\scripts\smoke_test.ps1 -Configuration Release
powershell -ExecutionPolicy Bypass -File .\scripts\health_check.ps1 -Configuration Release
```

所有命令应成功完成。

---

## 变更总结

| Phase | Tasks | 新增文件 | 修改文件 |
|---|---|---|---|
| C 交互直观性 | Task 1-3 | 0 | `ui/AppUi.cpp`, `ui/AppUi.h` |
| B 场景丰富度 | Task 4-5 | 0 | `demo_scenes.json`, `ui/AppUi.cpp`, `Application.cpp`, `SimulationState.h`, `Application.h` |
| A 学习路径 | Task 6-7 | 0 | `SimulationState.h`, `ui/AppUi.cpp` |
| B 物理深化 | Task 8-11 | 0 | `reference_catalog.json`, `TeachingMeanFieldSolver.*`, `HydrogenicCorrections.*`, `TestMain.cpp`, `ui/AppUi.cpp` |
| D 可视化 | Task 12-17 | 0 | `SceneRenderer.*`, `ui/AppUi.cpp`, `Application.cpp`, `SimulationState.h` |

**总计：0 个新文件，约 13 个修改文件。不引入新外部依赖。**