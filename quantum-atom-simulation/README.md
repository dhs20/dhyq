# Quantum Atom Simulation

面向教学展示、数值验证和后续扩展的多保真度原子结构 / 光谱 / 可视化平台。

当前项目的第一目标是交付一个在 Windows x64 下可构建、可运行、可验证的 MVP，而不是把教学近似包装成研究级原子结构软件。

## 项目定位

- 工程定位：可 clean clone、可构建、可运行、可打包、可继续扩展的桌面科学可视化程序
- 科学定位：以氢样体系、径向中心势、教学型多电子近似和本地参考光谱对照为主
- 表达原则：明确区分教学近似、可视化演示、数值求解、真实时间依赖模拟、高精度理论

## 当前真实能力

- 现代桌面 GUI：OpenGL 3.3 Core + GLFW + GLEW + ImGui Docking
- 严格氢样模型：能级、轨道半径、跃迁、谱线、约化质量修正
- 多电子教学近似：Aufbau / Pauli / Hund + Slater `Z_eff`
- 概率云可视化：点云、体切片、相位着色、叠加态、双阶段云构建
- 数值求解：径向有限差分本征值法
- Tier 1 中心场：可切换氢样库仑势或现象学屏蔽中心势
- Tier 3 谱学修正：氢样精细结构、Zeeman、一阶受限 Stark 教学修正
- 自动验证：解析值、收敛、局部参考光谱、构建与启动 smoke test
- 交付链路：脚本化构建、运行、验证、打包到 `dist/`

## 当前明确不支持

- Hartree / Hartree-Fock / Dirac-Fock 自洽平均场
- DFT / CI / MCSCF / CASSCF 等多电子相关方法
- 严格多电子反对称多体波函数与电子关联
- 完整 fine / hyperfine / Lamb shift / QED 修正
- 完整 Zeeman / Stark 高阶谱学求解
- 真实 TDSE / TD-CI 外场驱动动力学
- 全量 NIST ASD 在线联动数据库

## 元数据覆盖与物理覆盖

- 静态元数据覆盖：`assets/data/elements.json` 现在覆盖完整周期表 `Z=1..118`
- 高精度物理覆盖：没有覆盖完整周期表
- 当前经过重点验证的物理链路：`H`、`He+`、`Ne`、氢样跃迁、局部参考谱线、径向求解收敛
- 结论：元数据完整不等于高精度理论完整

## 保真度分层

- Tier 0：教学与演示层
  - Bohr、`Z_eff + Slater`、概率云、脚本动画
  - 适合课堂展示和概念理解
- Tier 1：改进中心场层
  - 径向单电子中心势、屏蔽中心场、有限差分本征值法
  - 适合做氢样到改进单电子近似的过渡
- Tier 2：自洽平均场层
  - 仅预留接口，未实现 HF/Dirac-Fock
- Tier 3：谱学修正层
  - 当前提供氢样精细结构、Zeeman、受限 Stark 教学修正
- Tier 4：多电子关联层
  - 仅预留接口，未实现 CI / MCSCF
- Tier 5：真实动力学层
  - 仅预留接口，当前动画不是 TDSE

## 目录结构

- `app/`：应用入口、任务调度、验证报告导出
- `core/`：路径、日志、性能统计
- `data/`：可追溯数据导入与元数据映射
- `physics/`：原子物理、中心场、云采样、数值求解
- `spectroscopy/`：谱学修正模块
- `render/`：相机、OpenGL 资源、场景渲染
- `ui/`：Scene、Inspector、Physics、Plots、Performance、Help、Log 面板
- `assets/`：元素元数据、参考谱线、演示场景、脚本
- `validation/`：验证报告输出
- `tests/`：物理与回归测试
- `scripts/`：构建、运行、打包、smoke test
- `legacy/`：旧原型代码，已与当前主构建隔离
- `dist/`：脚本生成的发版目录

## 快速开始

```powershell
cd D:\project\dhyq\quantum-atom-simulation
powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Configuration Release
powershell -ExecutionPolicy Bypass -File .\scripts\run.ps1 -Configuration Release
```

一键验证：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\smoke_test.ps1 -Configuration Release
```

完整验证并打包：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build_and_run.ps1 -Configuration Release -RunTests
powershell -ExecutionPolicy Bypass -File .\scripts\package.ps1 -Configuration Release -Build -IncludeTests
```

## 输出位置

- 程序：`build/windows/x64/<config>/quantum_atom_simulation.exe`
- 测试：`build/windows/x64/<config>/quantum_atom_tests.exe`
- 发行包：`dist/windows/x64/<config>/quantum_atom_simulation/`
- 运行日志：`logs/runtime.log`
- 验证报告：`docs/reports/latest-validation-report.md` 和同名 `.json`

## 运行时资源

- `assets/data/elements.json`
- `assets/data/nist_reference_lines.csv`
- `assets/scenarios/demo_scenes.json`
- `assets/scenarios/demo_script.json`

程序会优先从可执行文件相对路径和当前工作目录定位 `assets/`。资源缺失时会输出明确日志，并在元素元数据失败时回退到内置轻元素子集。

## 当前已知限制

- Windows x64 是当前优先验证平台
- 多电子部分仍是教学近似，不适合研究级谱学结论
- Tier 1 屏蔽中心场是现象学模型，不应解释为自洽平均场
- Tier 3 修正是氢样微扰扩展，不覆盖完整复杂原子谱学
- 自动演示和场景动画不是时间依赖量子动力学

## 相关文档

- 构建与打包：[README_BUILD.md](/D:/project/dhyq/quantum-atom-simulation/README_BUILD.md)
- 物理边界与方法标签：[README_PHYSICS.md](/D:/project/dhyq/quantum-atom-simulation/README_PHYSICS.md)
- 架构设计：[design.md](/D:/project/dhyq/quantum-atom-simulation/design.md)
- 模型说明：[docs/physics-model.md](/D:/project/dhyq/quantum-atom-simulation/docs/physics-model.md)
- 验证说明：[docs/validation.md](/D:/project/dhyq/quantum-atom-simulation/docs/validation.md)
- 升级状态：[docs/upgrade-status.md](/D:/project/dhyq/quantum-atom-simulation/docs/upgrade-status.md)
- Legacy 说明：[docs/legacy-prototype.md](/D:/project/dhyq/quantum-atom-simulation/docs/legacy-prototype.md)
