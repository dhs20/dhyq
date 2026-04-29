# Coverage Matrix

OpenCppCoverage was not found on PATH, so line coverage is marked unavailable for this run.

| Module | Automated Coverage Evidence | Status |
| --- | --- | --- |
| `physics` | constants, hydrogenic formulas, transitions, Slater, central field, radial solver, cloud generation | Covered by `quantum_atom_tests` |
| `spectroscopy` | fine/Zeeman/Stark/hyperfine corrections and structured spectral records | Covered by `quantum_atom_tests` |
| `solvers` | Tier 2 mean-field and Tier 4 correlation checks | Covered by `quantum_atom_tests` |
| `dynamics` | nuclear models and two-level TDSE dynamics | Covered by `quantum_atom_tests` |
| `data` | JSON catalog load, isotope catalog, reference fallback shape | Covered by `quantum_atom_tests` and package verification |
| `validation` | Markdown/JSON report export | Covered by `quantum_atom_tests` |
| `render/ui` | Build, smoke, and health checks; no line coverage gate due GPU/window dependency | Operationally verified |

Coverage gate: critical non-render modules must have explicit regression coverage. Line percentage is unavailable without OpenCppCoverage.
