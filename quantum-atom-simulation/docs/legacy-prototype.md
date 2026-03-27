# Legacy Prototype Status

当前仓库已经把历史教学原型从主源码树迁到独立的 `legacy/` 目录。

## 当前主线

- [app](/D:/project/dhyq/quantum-atom-simulation/app)
- [core](/D:/project/dhyq/quantum-atom-simulation/core)
- [data](/D:/project/dhyq/quantum-atom-simulation/data)
- [physics](/D:/project/dhyq/quantum-atom-simulation/physics)
- [render](/D:/project/dhyq/quantum-atom-simulation/render)
- [ui](/D:/project/dhyq/quantum-atom-simulation/ui)
- [validation](/D:/project/dhyq/quantum-atom-simulation/validation)
- [include/quantum](/D:/project/dhyq/quantum-atom-simulation/include/quantum)

这是当前 `xmake` 主构建线和 Windows MVP 交付路径。

## Legacy 原型

- [legacy/src](/D:/project/dhyq/quantum-atom-simulation/legacy/src)
- [legacy/include](/D:/project/dhyq/quantum-atom-simulation/legacy/include)

这些文件来自早期 freeglut/OpenGL 教学原型，当前不参与主构建，也不应被用于描述现有 MVP 的能力范围。

## 处理原则

- 保留源码，便于后续对照、迁移和回溯
- 与当前主线物理/渲染/UI 体系彻底隔离
- 所有 README、设计文档和交付说明都以主线目录为准
