# Upgrade Status

更新日期：2026-03-27

## 已完成

### Must

- 历史原型已迁移到 [legacy/src](/D:/project/dhyq/quantum-atom-simulation/legacy/src) 和 [legacy/include](/D:/project/dhyq/quantum-atom-simulation/legacy/include)
- 主构建仅使用 [include/quantum](/D:/project/dhyq/quantum-atom-simulation/include/quantum) 与当前主线源码目录
- `MethodStamp` / `ValidationRecord` 已下沉到：
  - [AtomicPhysics.h](/D:/project/dhyq/quantum-atom-simulation/include/quantum/physics/AtomicPhysics.h) 中的谱线、能级、元素物理记录
  - [SimulationState.h](/D:/project/dhyq/quantum-atom-simulation/include/quantum/app/SimulationState.h) 中的派生状态聚合
  - [Application.cpp](/D:/project/dhyq/quantum-atom-simulation/app/Application.cpp) 中的参考谱线映射、验证报告汇总、当前方法标签
- 已新增脚本化 smoke test：
  - [scripts/smoke_test.ps1](/D:/project/dhyq/quantum-atom-simulation/scripts/smoke_test.ps1)
  - [scripts/smoke_test.bat](/D:/project/dhyq/quantum-atom-simulation/scripts/smoke_test.bat)
- 已新增 Windows CI 工作流：
  - [.github/workflows/windows-xmake.yml](/D:/project/dhyq/quantum-atom-simulation/.github/workflows/windows-xmake.yml)

### Should

- 元素元数据已通过 [JsonDataProvider.h](/D:/project/dhyq/quantum-atom-simulation/include/quantum/data/JsonDataProvider.h) / [JsonDataProvider.cpp](/D:/project/dhyq/quantum-atom-simulation/data/JsonDataProvider.cpp) 接入可追溯数据层
- [ElementDatabase.cpp](/D:/project/dhyq/quantum-atom-simulation/physics/ElementDatabase.cpp) 已从“直接解析”升级到“经由 DataProvider 映射”
- 本地参考 CSV 已升级为可追溯映射对象，再由应用层转成参考谱线显示
- 验证报告已可自动导出 Markdown 与 JSON，不再只是模板文档

## 部分完成

### Should

- [assets/data/elements.json](/D:/project/dhyq/quantum-atom-simulation/assets/data/elements.json) 已加入 schema/source/charge-state 元数据，但静态覆盖仍是 `H-Ar`
- 参考谱线映射目前仍是本地 CSV 导入，不是外部数据库缓存同步

## 尚未开始

### Could

- Tier 1：更可靠的有效中心势
- Tier 2：小体系 HF / 平均场原型
- Tier 3：fine structure / Zeeman / Stark 插件化
- Tier 5：最小 TDSE 基组时间推进

## 当前建议顺序

1. 继续清理和重写 README/README_BUILD 中剩余的历史乱码段落
2. 把 `demo_scenes.json` 和 `demo_script.json` 共用到统一的场景预设加载器
3. 将元素静态元数据扩展到完整周期表，再明确“元数据覆盖”和“高精度物理覆盖”是两回事
4. 在 Tier 1 做有效中心势和更可靠的单电子近似，再进入 Tier 3 谱学修正
