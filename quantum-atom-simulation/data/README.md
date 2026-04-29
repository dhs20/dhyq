# Data

数据层应区分四类内容：

- 静态元数据
- 理论计算产物
- 外部参考数据库映射
- 用户自定义扩展数据

当前实际静态资源仍位于 [assets/data](/D:/project/dhyq/quantum-atom-simulation/assets/data)。

当前已提供：

- `elements.json`：完整周期表静态元数据
- `reference_catalog.json`：离线教学参考谱线目录
- `isotopes.json`：同位素教学锚点目录
- `nist_reference_lines.csv`：旧 CSV 参考谱线兼容回退

本目录继续用于扩展规范、模式定义和离线缓存策略。
