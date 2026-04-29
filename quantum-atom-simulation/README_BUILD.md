# Build And Run

## 环境要求

- Windows 10/11 x64
- MSVC C++ toolchain
- PowerShell 5.1 或更新版本
- `xmake`

当前主构建系统只有一个：

- `xmake`

本仓库当前不提供项目级 `CMakeLists.txt`、`.sln` 或 `vcxproj` 主线工程文件。Visual Studio 的 `.vs/`、数据库和中间缓存不属于源码成果。

## 一键脚本

脚本目录在 [scripts](/D:/project/dhyq/quantum-atom-simulation/scripts)。

- [build.ps1](/D:/project/dhyq/quantum-atom-simulation/scripts/build.ps1)：构建 `Debug/Release`
- [run.ps1](/D:/project/dhyq/quantum-atom-simulation/scripts/run.ps1)：运行程序，可选先跑测试
- [build_and_run.ps1](/D:/project/dhyq/quantum-atom-simulation/scripts/build_and_run.ps1)：构建、测试、运行
- [package.ps1](/D:/project/dhyq/quantum-atom-simulation/scripts/package.ps1)：生成 `dist/` 发版目录
- [verify_package.ps1](/D:/project/dhyq/quantum-atom-simulation/scripts/verify_package.ps1)：检查发行包文件、资产、日志目录并运行包内健康检查
- [health_check.ps1](/D:/project/dhyq/quantum-atom-simulation/scripts/health_check.ps1)：无窗口资源/数据健康检查
- [smoke_test.ps1](/D:/project/dhyq/quantum-atom-simulation/scripts/smoke_test.ps1)：启动程序并做最小冒烟验证
- [coverage_report.ps1](/D:/project/dhyq/quantum-atom-simulation/scripts/coverage_report.ps1)：OpenCppCoverage 可用时生成行覆盖率，否则生成模块覆盖矩阵
- [validate_release.ps1](/D:/project/dhyq/quantum-atom-simulation/scripts/validate_release.ps1)：串联构建、测试、健康检查、smoke、打包验证和覆盖报告

对应的 `.bat` 包装脚本也已经提供，方便直接双击验证。

## Debug 构建

```powershell
cd D:\project\dhyq\quantum-atom-simulation
powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Configuration Debug
```

## Release 构建

```powershell
cd D:\project\dhyq\quantum-atom-simulation
powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Configuration Release
```

## 运行

```powershell
cd D:\project\dhyq\quantum-atom-simulation
powershell -ExecutionPolicy Bypass -File .\scripts\run.ps1 -Configuration Release
```

带测试运行：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\run.ps1 -Configuration Release -RunTests
```

## 构建并运行

```powershell
cd D:\project\dhyq\quantum-atom-simulation
powershell -ExecutionPolicy Bypass -File .\scripts\build_and_run.ps1 -Configuration Release -RunTests
```

## 打包

```powershell
cd D:\project\dhyq\quantum-atom-simulation
powershell -ExecutionPolicy Bypass -File .\scripts\package.ps1 -Configuration Release -Build -IncludeTests
```

带包完整性校验：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\package.ps1 -Configuration Release -Build -IncludeTests -Verify
```

发版目录输出到：

- `dist/windows/x64/release/quantum_atom_simulation/`

其中包含：

- `quantum_atom_simulation.exe`
- `quantum_atom_tests.exe`，如果启用了 `-IncludeTests`
- `assets/`
- `docs/`
- `README*.md`
- `run_app.bat`
- `logs/`

## 最小验证命令

构建后跑 smoke test：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\smoke_test.ps1 -Configuration Release
```

资源/数据健康检查：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\health_check.ps1 -Configuration Release
```

完整 Release 验证：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\validate_release.ps1 -Configuration Release
```

完整项目验证入口：

- [verify_project.bat](/D:/project/dhyq/quantum-atom-simulation/verify_project.bat)

它会执行：

- `xmake` 构建
- `quantum_atom_tests.exe`
- `smoke_test.ps1`

## 离线依赖缓存

`xmake.lua` 不再默认绑定仓库旁的 `.xmake-pkg-install` 目录。需要使用离线包缓存时，可以：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1 -Configuration Release -LocalPackageRoot D:\project\dhyq\.xmake-pkg-install
```

脚本也会在发现 `XMAKE_PKG_INSTALLDIR` 或仓库旁共享缓存目录时自动传入 `local_pkg_root`，以保持离线可复现构建。

## 可执行文件位置

- `build/windows/x64/debug/quantum_atom_simulation.exe`
- `build/windows/x64/release/quantum_atom_simulation.exe`
- `build/windows/x64/debug/quantum_atom_tests.exe`
- `build/windows/x64/release/quantum_atom_tests.exe`

## 运行时资源

程序默认使用以下资源：

- `assets/data/elements.json`
- `assets/data/reference_catalog.json`
- `assets/data/isotopes.json`
- `assets/data/nist_reference_lines.csv`
- `assets/scenarios/demo_scenes.json`
- `assets/scenarios/demo_script.json`

资源定位策略：

- 优先相对可执行文件寻找 `assets/`
- 如果当前工作目录更靠近项目根，也会从工作目录回溯定位
- 找不到资源时会输出日志，不会静默失败
- 元素元数据加载失败时会回退到内置轻元素子集

## 验证报告输出

默认导出位置：

- `docs/reports/latest-validation-report.md`
- `docs/reports/latest-validation-report.json`

程序内 Physics / Help 面板也可以触发导出。

## 常见问题

`xmake` 不在 `PATH`：

- 先确认 `xmake --version` 能在 PowerShell 中运行

程序启动但资源缺失：

- 检查 `assets/` 是否和可执行文件保持相对目录结构
- 查看 [logs/runtime.log](/D:/project/dhyq/quantum-atom-simulation/logs/runtime.log)

打包后运行失败：

- 确认使用了 [package.ps1](/D:/project/dhyq/quantum-atom-simulation/scripts/package.ps1)
- 确认 `dist/.../quantum_atom_simulation/assets/` 存在

中文显示异常：

- 程序会优先加载系统中文字体
- 如无可用字体，会回退默认字体并记录日志

## Legacy 说明

- 当前主线源码不再使用旧 `src/` 和旧根头文件
- 历史原型代码已迁移到 [legacy](/D:/project/dhyq/quantum-atom-simulation/legacy)
- `legacy/` 仅供参考，不参与当前 0.3.1 主构建
