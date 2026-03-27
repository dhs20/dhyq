# Legacy Prototype Notice

`src/` 下的这些 `.cpp` 文件属于早期 `freeglut + 立即模式 OpenGL` 原型实现。

当前主构建系统 [xmake.lua](/D:/project/dhyq/quantum-atom-simulation/xmake.lua) 不会编译它们。现行可交付主线使用：

- [app](/D:/project/dhyq/quantum-atom-simulation/app)
- [core](/D:/project/dhyq/quantum-atom-simulation/core)
- [physics](/D:/project/dhyq/quantum-atom-simulation/physics)
- [render](/D:/project/dhyq/quantum-atom-simulation/render)
- [ui](/D:/project/dhyq/quantum-atom-simulation/ui)

保留这些文件的原因是：

- 为历史原型留档
- 便于对照旧模型接口

它们不应被当作当前 MVP 的实现依据。
