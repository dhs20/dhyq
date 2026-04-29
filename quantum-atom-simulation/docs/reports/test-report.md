# Test Report

- Generated: 2026-04-29
- Test target: `quantum_atom_tests`
- Release target: Windows x64 Release

## Regression Coverage

| Module | Covered Scenarios |
| --- | --- |
| `physics` | constants, hydrogenic metrics, transition rules, Slater shielding, radial distribution, central field, finite-difference solver, cloud sampling |
| `data` | elements JSON, isotope catalog, reference JSON, CSV fallback, built-in element fallback, provenance/source metadata |
| `basis` | angular labels, subshell capacities, Aufbau order |
| `spectroscopy` | fine/Zeeman/Stark/hyperfine corrections, structured spectral records, term labels |
| `solvers` | Tier 2 mean-field convergence, exchange correction, Tier 4 correlation energy and weights |
| `dynamics` | no-field conservation, Rabi population transfer, damping behavior, nuclear teaching models |
| `validation` | Markdown/JSON export and Tier 2/3/4/5 method sections |
| `scripts/package` | required package assets and health check via package verifier |

## Required Commands

```powershell
xmake build quantum_atom_tests
xmake run quantum_atom_tests
xmake build quantum_atom_simulation
powershell -ExecutionPolicy Bypass -File .\scripts\health_check.ps1 -Configuration Release
powershell -ExecutionPolicy Bypass -File .\scripts\smoke_test.ps1 -Configuration Release
powershell -ExecutionPolicy Bypass -File .\scripts\package.ps1 -Configuration Release -Build -IncludeTests -Verify
powershell -ExecutionPolicy Bypass -File .\scripts\coverage_report.ps1 -Configuration Release
```

## Coverage Policy

OpenCppCoverage is used when available on `PATH`. If it is not available, `scripts/coverage_report.ps1` writes `docs/reports/coverage-matrix.md`; the line coverage percentage is marked unavailable and the release gate relies on the critical module test matrix above.

Target: >=80% line coverage for non-render core modules when OpenCppCoverage is available.
