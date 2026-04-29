# Basis

该目录承载轻量级基组/轨道元数据能力，并为后续更完整的基组表示层预留位置。

当前已实现：

- `angularMomentumLabel(l)`：角动量标签 `s/p/d/f/g`
- `subshellCapacity(l)`：子壳层容量 `2(2l+1)`
- `aufbauOrder(maxPrincipalNumber)`：教学级 Aufbau 轨道顺序元数据

后续规划：

- 径向网格与离散基函数
- 角动量代数与球谐缓存
- 单电子轨道数据结构

当前仓库尚未把这一层扩展成完整研究级基组模块；高保真波函数、积分和多体组态空间仍由后续版本处理。
