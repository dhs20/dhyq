# Upgrade Status

更新日期：2026-04-29

## 当前判断

项目已经从旧版 `freeglut + 立即模式 + 快捷键/HUD` 教学原型，升级为一个在 Windows x64 下可构建、可运行、可验证、可打包、可健康检查的 0.3.1 桌面教学软件。

当前最重要的结论是：

- 工程交付主线已经打通，并补充了包完整性验证、健康检查和 release 验证脚本
- 科学表达已经基本做到“不过度包装”
- 物理能力仍以氢样体系、中心场近似、教学型多电子近似和有限基教学原型为主
- 后续研究级理论层保留清晰边界，没有伪装成已实现

## 已完成

### 工程交付

- 统一主构建系统为 [xmake.lua](/D:/project/dhyq/quantum-atom-simulation/xmake.lua)
- 补齐脚本化构建、运行、打包、smoke test：
  - [scripts/build.ps1](/D:/project/dhyq/quantum-atom-simulation/scripts/build.ps1)
  - [scripts/run.ps1](/D:/project/dhyq/quantum-atom-simulation/scripts/run.ps1)
  - [scripts/build_and_run.ps1](/D:/project/dhyq/quantum-atom-simulation/scripts/build_and_run.ps1)
  - [scripts/package.ps1](/D:/project/dhyq/quantum-atom-simulation/scripts/package.ps1)
  - [scripts/verify_package.ps1](/D:/project/dhyq/quantum-atom-simulation/scripts/verify_package.ps1)
  - [scripts/health_check.ps1](/D:/project/dhyq/quantum-atom-simulation/scripts/health_check.ps1)
  - [scripts/coverage_report.ps1](/D:/project/dhyq/quantum-atom-simulation/scripts/coverage_report.ps1)
  - [scripts/validate_release.ps1](/D:/project/dhyq/quantum-atom-simulation/scripts/validate_release.ps1)
  - [scripts/smoke_test.ps1](/D:/project/dhyq/quantum-atom-simulation/scripts/smoke_test.ps1)
- 已支持 `dist/` 打包发布目录
- 已支持 `docs/reports/` 生产就绪证据集
- 已加入 Windows CI 工作流：
  - [.github/workflows/windows-xmake.yml](/D:/project/dhyq/quantum-atom-simulation/.github/workflows/windows-xmake.yml)

### 架构与目录

- 当前主线代码已集中到：
  - [app](/D:/project/dhyq/quantum-atom-simulation/app)
  - [core](/D:/project/dhyq/quantum-atom-simulation/core)
  - [physics](/D:/project/dhyq/quantum-atom-simulation/physics)
  - [render](/D:/project/dhyq/quantum-atom-simulation/render)
  - [ui](/D:/project/dhyq/quantum-atom-simulation/ui)
  - [include/quantum](/D:/project/dhyq/quantum-atom-simulation/include/quantum)
- 旧原型代码已迁移隔离到：
  - [legacy/src](/D:/project/dhyq/quantum-atom-simulation/legacy/src)
  - [legacy/include](/D:/project/dhyq/quantum-atom-simulation/legacy/include)

### 物理与数据

- 严格氢样解析链已打通：
  - 能级
  - 轨道半径
  - 跃迁
  - 谱线
  - 约化质量修正
- 多电子教学近似已打通：
  - Aufbau / Pauli / Hund
  - Slater `Z_eff`
- 数值求解已接入径向有限差分本征值法
- Tier 1 已接入现象学屏蔽中心场
- Tier 2 已接入教学级屏蔽 SCF、Slater 交换修正、标量相对论预览
- Tier 3 已接入氢样精细结构、Zeeman、受限 Stark、超精细教学修正
- Tier 4 已接入有限基双组态 CI mixing 教学原型
- Tier 5 已接入二能级 TDSE / Rabi 教学求解
- 元数据层已补到完整周期表 `Z=1..118`
- 已新增离线参考谱线 JSON catalog 与同位素教学锚点
- 118 个元素中文名已下沉到数据层：
  - [assets/data/elements.json](/D:/project/dhyq/quantum-atom-simulation/assets/data/elements.json)

### 可视化与交互

- 现代 GUI 已成型：
  - Scene
  - Inspector
  - Physics
  - Plots
  - Performance
  - Help
  - Log
- 图表已支持：
  - 鼠标框选缩放
  - 平移
  - 重置视图
  - 局部窗口数据联动
  - 当前窗口 CSV 导出
- 场景已支持：
  - 适配全部
  - 仅核
  - 仅轨道
  - 仅云
  - 过渡时长
  - 锁定当前观察目标

### 验证与可追溯

- `MethodStamp / Source / Provenance / ValidationRecord` 已写入主要物理输出对象
- 已支持验证报告自动导出
- 已有本地测试覆盖：
  - 氢样解析值
  - 跃迁波长
  - 归一化/正交性
  - 中心场 sanity
  - 谱学修正 sanity
  - 周期表覆盖与中文名

## 部分完成

### 数据与数据库

- 当前参考谱线以本地 JSON catalog 为主，并保留 CSV 回退；不是完整外部数据库缓存层
- 虽然静态元数据已经覆盖 `Z=1..118`，但高精度物理能力并没有覆盖完整周期表

### 跨平台

- Windows x64 是当前优先验证平台
- Linux/macOS 构建路径已有设计，但尚未形成同等强度的实际验证闭环

### 图形与性能

- 已有 LOD、预览云/全质量云、GPU timer、显存跟踪估算
- 但还没有真正的 GPU 侧云生成与更高阶的屏幕误差 LOD

## 尚未实现的研究级能力

### 研究级高阶理论

- 研究级 Hartree-Fock / Dirac-Fock
- DFT
- 完整 CI
- MCSCF / CASSCF
- 严格多电子反对称多体波函数与研究级电子关联
- 真实多体 TDSE / TD-CI / TDHF 外场驱动时间推进

## 需要持续保持诚实表达的边界

- 多电子部分仍然是教学近似，不是研究级多电子结构程序
- 场景动画和自动演示不是 TDSE 动力学
- Tier 1 屏蔽中心场不是自洽平均场
- Tier 3 谱学修正不是完整复杂原子谱学求解
- 元数据全覆盖不等于高精度理论全覆盖

## 下一阶段建议

1. 继续收紧文档口径，让 README、Physics、UI 方法标签完全一致。
2. 做外部参考数据库缓存层，把本地 CSV 提升为可追溯数据映射系统。
3. 在 Tier 1.5 / Tier 2 方向上优先做更可靠的单电子有效势或小体系 SCF 原型。
4. 在不牺牲科学诚实的前提下继续优化图表、场景和报告输出。
