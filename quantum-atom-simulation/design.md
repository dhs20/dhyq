# Quantum Atom Simulation Design

## 1. 项目定位

这是一个“教学展示 + 数值验证 + 可扩展架构预留”的多保真度原子结构/光谱/可视化平台。

当前第一目标不是研究级高精度原子结构，而是：

- clean clone 后可构建
- Windows x64 可运行
- 最小演示可完成
- 科学表述不误导
- 后续高阶物理模块有清晰落点

## 2. 当前真实能力

### 已实现

- `xmake` 主构建系统
- `GLFW + GLEW + OpenGL 3.3 Core`
- Docking GUI 与多面板工具界面
- 严格氢样解析模型
- `Z_eff + Slater` 多电子教学近似
- 径向有限差分求解器
- 谱线、能级、径向分布、收敛曲线
- 异步概率云构建、预览云优先
- 自动演示脚本录制与回放

### 未实现但必须诚实标注

- HF / Dirac-Fock / DFT
- CI / MCSCF / CASSCF
- fine structure / hyperfine / Zeeman / Stark
- 真实 TDSE / TD-CI 动力学
- 完整周期表高精度求解

## 3. 模块划分

### 当前实现模块

- `app/`
  - 程序入口、路径定位、任务调度、演示脚本、状态同步
- `core/`
  - 日志、路径、性能统计
- `physics/`
  - 常数、Bohr/氢样模型、`Z_eff`、Slater、电子组态、云采样、数值求解
- `render/`
  - 相机、OpenGL 对象、点云/体切片/轨道渲染
- `ui/`
  - Scene / Inspector / Physics / Plots / Performance / Help / Log
- `tests/`
  - 物理与数值回归测试

### 结构预留模块

- `basis/`
- `solvers/`
- `spectroscopy/`
- `dynamics/`
- `data/`
- `validation/`
- `examples/`

## 4. 结果元数据与接口骨架

新增接口和元数据类型位于 `include/quantum/...`：

- `meta/MethodMetadata.h`
  - `MethodStamp`
  - `ValidationRecord`
  - `SourceRecord`
  - `ProvenanceRecord`
- `data/Records.h`
  - `ElementRecord`
  - `IsotopeRecord`
  - `ConfigurationRecord`
  - `StateRecord`
  - `TransitionRecord`
- `data/DataProvider.h`
- `model/AtomModel.h`
- `solvers/OrbitalSolver.h`
- `solvers/MeanFieldSolver.h`
- `physics/ConfigurationBuilder.h`
- `physics/HamiltonianAssembler.h`
- `spectroscopy/SpectralEngine.h`
- `dynamics/DynamicsEngine.h`
- `validation/ValidationRunner.h`

设计原则：

- 每个结果对象都可挂方法与验证元数据
- 可视化对象不等于物理解本身
- 教学近似层与未来高保真层强制分离

## 5. 数据流

1. `Application` 定位项目根目录并加载资源。
2. `SimulationState` 保存 GUI 参数和脏标记。
3. `physics/` 根据状态生成：
   - Bohr/氢样指标
   - 跃迁与谱线
   - `Z_eff` 与电子组态
   - 径向分布
   - 数值求解结果
   - 概率云数据
4. `render/SceneRenderer` 将缓存结果转为 GPU 资源。
5. `ui/AppUi` 读取同一批缓存结果进行参数面板、图表和日志展示。

## 6. 渲染管线

当前真实渲染栈：

- 窗口：`GLFW`
- OpenGL loader：`GLEW`
- API：`OpenGL 3.3 Core`
- GUI：`ImGui Docking`
- 图表：当前为自绘交互图，不依赖 `ImPlot`

主要渲染对象：

- 核、轨道环
- 能级标记
- 概率点云
- 体切片
- 场景帧缓冲

性能策略：

- 仅在数据脏时重建 VBO/纹理
- 点云支持 LOD 与交互降级
- 概率云支持预览阶段与全质量阶段
- GPU 计时器和显存跟踪进入 Performance 面板

## 7. 物理能力分层

### Tier 0：教学与演示层

- Bohr
- 氢样解析轨道
- `Z_eff + Slater`
- 演示脚本与说明性动画

标签要求：

- `pedagogical`
- `illustrative`
- `not a research-grade result`

### Tier 1：改进中心场层

- 当前已部分具备：径向中心势数值求解
- 后续可接有效势、改进边界条件、更多验证

### Tier 2：自洽平均场层

- 仅预留接口
- 目标：Hartree / Hartree-Fock / Dirac-Fock

### Tier 3：谱学修正层

- 仅预留接口与标签规范
- 目标：fine、hyperfine、Zeeman、Stark

### Tier 4：多电子关联层

- 仅预留接口与路线
- 目标：CI、MCSCF 风格基组方法

### Tier 5：真实动力学层

- 仅预留接口与表述规则
- 只有这一层的动画才允许称为真实时间依赖量子动力学

## 8. UI 信息架构

- `Scene`
  - 3D 场景
  - 相机交互
  - 场景状态覆盖层
- `Inspector`
  - 元素、荷态、模型、量子数、采样、求解器、演示脚本
- `Physics`
  - 当前数值、单位、选择规则、近似警告、方法标签
- `Plots`
  - 能级图、谱线图、径向分布、收敛曲线
- `Performance`
  - FPS、CPU/GPU 时间、LOD、点数、显存、后台任务状态
- `Help`
  - 操作、概念说明、动画分类、脚本控制
- `Log`
  - 运行与验证日志

## 9. 资源定位策略

- 首先尝试基于可执行文件路径回溯项目根目录
- 再回退到当前工作目录
- 根目录识别标记：
  - `xmake.lua`
  - `assets/data/elements.json`

资源缺失处理：

- 元素数据库读取失败时回退内置轻元素子集
- 参考 CSV / 演示脚本失败时输出明确日志

## 10. 近期升级路线

### Must

- 保持 `xmake` 构建和 Windows x64 可执行交付
- 继续补方法标签和验证元数据落地
- 扩展元素数据和参考数据库映射

### Should

- 把谱线/状态/配置结果逐步迁到带 `MethodStamp` 的记录对象
- 引入更系统的验证报告输出
- 逐步隔离 legacy `src/` 原型代码

### Could

- Tier 1 有效势增强
- Tier 2 自洽平均场原型
- Tier 3 谱学修正插件化
