# Deployment Verification

- Generated: 2026-04-29
- Deployment target: Windows x64 Release desktop bundle
- Artifact path: `D:\project\dhyq\quantum-atom-simulation\dist\windows\x64\release\quantum_atom_simulation\`

## Package Contents Gate

`scripts/verify_package.ps1` checks:

- `quantum_atom_simulation.exe`
- `quantum_atom_tests.exe` when `-IncludeTests` is used
- `run_app.bat`
- `README.md`, `README_BUILD.md`, `README_PHYSICS.md`
- `assets/data/elements.json`
- `assets/data/reference_catalog.json`
- `assets/data/isotopes.json`
- `assets/data/nist_reference_lines.csv`
- `assets/scenarios/demo_scenes.json`
- `assets/scenarios/demo_script.json`
- `docs/validation.md`
- `docs/performance.md`
- `logs/`

## Health Check

The application supports:

```powershell
quantum_atom_simulation.exe --health-check
```

This mode validates required resource files, loads element metadata, loads isotope metadata, loads reference transitions through JSON or CSV fallback, runs a hydrogen sanity check, and exits without opening the interactive window.

## Deployment Command

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\package.ps1 -Configuration Release -Build -IncludeTests -Verify
```

## Rollback Strategy

The package is self-contained under `dist/windows/x64/release/quantum_atom_simulation/`. Rollback is performed by replacing that directory with the previous verified bundle. Runtime logs remain under the bundle-local `logs/` directory.

## Release Decision

Deployable for Windows x64 teaching/scientific visualization once the release gate commands pass. Linux/macOS are documented as outside this release gate.
