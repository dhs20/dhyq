# Physics Scope And Labels

## 项目真实定位

这是一个多保真度原子结构/光谱/可视化平台的 MVP，当前最可靠的范围是：

- 氢样体系的解析与数值验证
- 中心势定态的概率云与相位可视化
- 多电子 `Z_eff + Slater` 教学近似
- 光谱线、能级图和数值收敛图的展示与比对

当前不应被描述为：

- 研究级高精度原子结构程序
- 多电子严格反对称相关波函数求解器
- 已实现 HF / DFT / CI / MCSCF / TDSE 的成熟平台

## 方法标签规范

每类结果都应尽量携带这些标签：

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
- `Illustrative / pedagogical animation`
- `Finite-difference radial solver`
- `Local reference CSV comparison`

禁止表述：

- `Research-grade atomic structure`
- `Real-time quantum dynamics`，若实际只是演示动画
- `Hartree-Fock implemented`，若仓库内并无可运行实现

## 当前能力边界

### Tier 0：教学与演示层

- 物理模型：Bohr、氢样解析波函数、教学近似、多场景脚本演示
- 适用场景：课堂展示、概念理解、界面演示
- 明确限制：不代表研究级理论结果

### Tier 1：改进中心场层

- 当前已部分具备：径向中心势求解、有限差分本征值、可接有效势
- 适用场景：氢样验证、单电子有效势演示
- 当前限制：还不是自洽场

### Tier 2：自洽平均场层

- 当前状态：仅做接口与目录预留
- 目标：Hartree / Hartree-Fock / Dirac-Fock
- 当前不支持：SCF 迭代、交换势、自洽轨道更新

### Tier 3：谱学修正层

- 当前状态：仅做路线和标签预留
- 目标：fine structure、spin-orbit、hyperfine、Zeeman、Stark

### Tier 4：多电子关联层

- 当前状态：未实现
- 目标：CI、MCSCF/CASSCF 风格主动空间

### Tier 5：真实动力学层

- 当前状态：未实现
- 只有到这一层，动画才允许称为真实时间依赖量子动力学

## 数据边界

- 元素元数据当前覆盖 `H` 到 `Ar`
- 参考谱线当前为本地示例 CSV，不是完整 NIST ASD 联动
- 高精度理论覆盖当前并不存在，应与“数据库覆盖”严格区分

## 当前验证状态

已自动化验证的典型项目：

- Hydrogen / hydrogen-like 能级
- Lyman-alpha、Balmer-alpha
- 归一化与正交性
- `Ne` 的电子组态与 `Z_eff`
- 概率云采样统计
- 数值求解收敛曲线

详见：

- [docs/physics-model.md](/D:/project/dhyq/quantum-atom-simulation/docs/physics-model.md)
- [docs/validation.md](/D:/project/dhyq/quantum-atom-simulation/docs/validation.md)
