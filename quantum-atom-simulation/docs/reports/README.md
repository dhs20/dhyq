# Validation Reports

该目录用于存放从应用内导出的验证报告，以及 0.3.1 生产就绪硬化过程生成的交付证据。

当前导出格式：

- Markdown
- JSON

默认输出文件：

- `latest-validation-report.md`
- `latest-validation-report.json`

生产就绪证据：

- `production-readiness-audit.md`
- `placeholder-cleanup-ledger.md`
- `defect-register.md`
- `data-flow-verification.md`
- `test-report.md`
- `performance-report.md`
- `deployment-verification.md`
- `release-checklist.md`
- `coverage-matrix.md`，当 OpenCppCoverage 不可用时生成
- `release-validation-summary.md`，由 `scripts/validate_release.ps1` 生成

导出方式：

- 在应用的 `Physics` 面板点击“导出验证报告”
- 或在 `Help` 面板设置报告文件路径后导出

报告会包含：

- 当前元素与电荷态
- 当前模型与近似模式
- 当前激活方法标签
- 当前验证记录和误差统计
- Tier 2 / Tier 3 / Tier 4 / Tier 5 方法标签
- caveats、数据来源和 validation records

注意：

- 报告反映的是导出时刻的当前 UI/物理状态
- 多电子部分若使用 `Z_eff + Slater`，报告会明确标记为教学近似
