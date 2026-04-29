# Release Checklist

- Generated: 2026-04-29
- Version: 0.3.1
- Target: Windows x64 Release

| Item | Status | Evidence |
| --- | --- | --- |
| Version bumped from 0.2.0 | Done | `xmake.lua` |
| xmake remains the only main build chain | Done | `README_BUILD.md` |
| Local package cache is explicit/configurable | Done | `local_pkg_root`, `scripts/build.ps1` |
| Placeholder-like basis module removed | Done | `BasisMetadata` implementation |
| UI stale capability labels corrected | Done | `AppUi.cpp` method tags and animation disclosure |
| Health-check mode exists | Done | `app/main.cpp`, `scripts/health_check.ps1` |
| Package verification exists | Done | `scripts/verify_package.ps1` |
| Release validation runner exists | Done | `scripts/validate_release.ps1` |
| Coverage path exists | Done | `scripts/coverage_report.ps1` |
| Offline catalogs included | Done | `assets/data/*.json`, CSV fallback |
| Documentation describes teaching boundary | Done | `README.md`, `README_PHYSICS.md`, `design.md` |
| Reports evidence set exists | Done | `docs/reports/*.md` |
| Windows release package path documented | Done | `deployment-verification.md` |

## Final Gate

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\validate_release.ps1 -Configuration Release
```

Expected release artifact:

`D:\project\dhyq\quantum-atom-simulation\dist\windows\x64\release\quantum_atom_simulation\`
