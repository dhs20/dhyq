# Validation Reports

该目录用于存放从应用内导出的验证报告。

当前导出格式：

- Markdown
- JSON

默认输出文件：

- `latest-validation-report.md`
- `latest-validation-report.json`

导出方式：

- 在应用的 `Physics` 面板点击“导出验证报告”
- 或在 `Help` 面板设置报告文件路径后导出

报告会包含：

- 当前元素与电荷态
- 当前模型与近似模式
- 当前激活方法标签
- 当前验证记录和误差统计

注意：

- 报告反映的是导出时刻的当前 UI/物理状态
- 多电子部分若使用 `Z_eff + Slater`，报告会明确标记为教学近似
