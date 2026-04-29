# Spectroscopy

该目录承载能级、选择定则、跃迁和教学级谱学修正层。

当前已实现：

- 基础跃迁能量
- 波长与频率
- 电偶极选择规则过滤
- 离线参考谱线 JSON catalog 对照，CSV 兼容回退
- 本地参考 CSV 兼容对照
- 氢样 fine structure 教学修正
- Zeeman 教学修正
- 受限 Stark 教学修正
- hyperfine 标量 A 常数教学估计
- term symbol / J / mJ / F 展示

当前边界：

- 复杂原子的完整 fine / hyperfine / Lamb / QED 求解
- 高阶 Zeeman / Stark 矩阵对角化
- 研究级 term system 与全量外部数据库联动
