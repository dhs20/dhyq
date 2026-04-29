# Physics Scope And Labels

## 项目真实定位

这个项目当前是一个“多保真度原子结构 / 光谱 / 可视化平台”的 MVP。

当前最可靠的主线仍然是：
- 氢样原子与氢样离子的解析模型
- 径向中心势数值求解
- 教学型多电子近似 `Z_eff + Slater`
- 局部参考谱线对照
- 教学型核过程动画与低保真核物理模型

它当前不应被表述为：
- 研究级高精度原子结构程序
- 完整核数据库或核反应工程代码
- 已实现 HF / DFT / CI / MCSCF / TDSE / TDHF 的成熟平台

## 当前原子物理能力

- 严格氢样：能级、轨道半径、跃迁、谱线、约化质量修正
- 解析波函数：`psi = R_nl(r) Y_lm(theta, phi)`，支持相位和叠加态可视化
- 数值求解：径向有限差分本征值法
- Tier 1：现象学屏蔽中心场
- Tier 2：教学级屏蔽 SCF、Slater 交换修正、标量相对论预览
- Tier 3：氢样精细结构、Zeeman、受限 Stark、超精细教学修正
- Tier 4：有限基双组态 CI mixing 教学原型
- Tier 5：二能级 TDSE / Rabi 动力学教学求解
- 多电子近似：Aufbau / Pauli / Hund + Slater `Z_eff`

## 当前核物理能力

当前新增的“核”部分是低保真、可追溯、明确标注边界的近似层，主要包括：

- 核结构近似：
  - Weizsacker 半经验质量公式
  - 结合能、每核子结合能、核半径、同位素链趋势
- 反应截面近似：
  - D-T 聚变的 Gamow 型现象学截面曲线
  - U-235 热中子裂变的低能 `1/v` 截面估计
- 输运近似：
  - 固定截面的 1D 平板 Beer-Lambert 束流衰减
  - 平均自由程、透射率、反应概率
- 时间依赖近似：
  - 单道指数衰变率方程
  - 半衰期、平均寿命、母核/子核比例随时间变化

## 动画标签规则

当前动画必须严格区分：

- `illustrative animation`
  - 纯示意，不由求解器直接驱动
- `pedagogical animation`
  - 教学动画，可由低保真模型参数驱动，但不是完整微观动力学
- `basis-superposition animation`
  - 由定态叠加得到的可视化，不等于真实外场驱动时间推进
- `true time-dependent quantum simulation`
  - 只有真实求解 TDSE / TD-CI / TDHF 之类方程时才允许使用

当前仓库中的核过程场景属于：
- 教学动画
- 部分参数可由简化截面 / 衰减 / 衰变模型驱动
- 不等于完整核反应输运
- 不等于真实量子多体核动力学

## 方法标签规范

结果对象应尽量携带：
- `method`
- `approximation`
- `electron correlation`
- `antisymmetry`
- `relativity`
- `external field`
- `time-dependent`
- `data source`
- `validation status`

推荐表述：
- `Strict hydrogenic analytic model`
- `Teaching approximation: Z_eff + Slater`
- `Finite-difference radial solver`
- `Semi-empirical liquid-drop binding estimate`
- `Gamow-like D-T fusion cross-section estimate`
- `Single-speed slab attenuation model`
- `Single-channel radioactive decay law`

禁止表述：
- `Research-grade atomic structure`
- `Research-grade nuclear reaction solver`
- `Real-time quantum dynamics`
- `HF / DFT / CI / MCSCF implemented`

## 分层状态

### Tier 0：教学与演示层
- 当前已实现
- 包括 Bohr、概率云、脚本演示、核过程教学动画、低保真核模型

### Tier 1：改进中心场层
- 当前部分实现
- 包括现象学屏蔽中心场和更可靠的单电子近似接口

### Tier 2：自洽平均场层
- 当前提供教学级屏蔽 SCF 原型
- 包括 Slater 交换修正和标量相对论预览
- 不是研究级 HF / Dirac-Fock

### Tier 3：谱学修正层
- 当前部分实现
- 包括氢样精细结构、Zeeman、受限 Stark、超精细教学修正和 term symbol 展示

### Tier 4：多电子关联层
- 当前提供有限基双组态 CI mixing 教学原型
- 不是完整 CI / MCSCF / CASSCF

### Tier 5：真实动力学层
- 当前提供二能级有限基 TDSE / Rabi 教学求解
- 只有该求解器输出可标注为有限基时间依赖；场景动画仍不应表述为真实量子多体时间推进

## 当前已知边界

- 多电子部分仍然只是教学近似，不是 HF / CI / MCSCF
- 核结构不是壳层模型、集体模型或从头算方法
- D-T 截面不是 Bosch-Hale 基准实现
- U-235 裂变截面目前只做热区 `1/v` 估计，不含共振结构
- 输运不是多群、角分布、散射耦合或临界计算
- 衰变当前是单道率方程，不含分支比和探测器响应

## 建议如何向用户表述

可以说：
- “教学型核过程动画，叠加低保真核结构 / 截面 / 衰减模型”
- “可用于概念展示、趋势观察和架构演示”
- “当前重点仍是原子物理可视化和可扩展工程框架”

不要说：
- “真实核反应求解”
- “研究级原子核结构平台”
- “完整时间依赖核量子动力学”
