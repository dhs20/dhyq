# Performance Report

- Generated: 2026-04-29
- Scope: deterministic non-GPU performance gates plus observational GPU/UI metrics.

## Measured/Tracked Metrics

| Metric | Source | Gate |
| --- | --- | --- |
| Solver execution and convergence | `quantum_atom_tests` | Pass/fail regression |
| Small fixed cloud generation | `ProbabilityCloudGenerator` tests | Pass/fail regression |
| Adaptive cloud generation stats | `ProbabilityCloudGenerator` tests | Pass/fail regression |
| Validation report export | `ValidationReportWriter` test | Pass/fail regression |
| Package health check startup | `--health-check` | Pass/fail regression |
| Point/volume build time | `PerformanceTracker` and UI panel | Observational |
| FPS/frame time | `PerformanceTracker` and UI panel | Observational |
| GPU timer query values | OpenGL timestamp queries | Observational due driver variance |
| GPU memory estimate | renderer-owned resource accounting | Observational |

## Deterministic Thresholds

| Path | Threshold Intent |
| --- | --- |
| Hydrogenic/numerical tests | Complete within normal unit-test runtime on Windows x64 |
| Small cloud generation | Maintains requested accepted point counts and bounded deterministic stats |
| Report export | Produces Markdown and JSON files without UI dependency |
| Health check | Loads required catalogs and exits deterministically with code 0 |

## Optimization Actions

- Kept cloud generation cached and dirty-flag driven.
- Retained asynchronous cloud regeneration path.
- Preserved interaction LOD for responsive camera work.
- Added package health checks to catch missing assets before interactive startup.
- Added coverage/report scripts to make regressions visible in release evidence.

## Remaining Performance Risks

- GPU timings and FPS depend on driver, display, power mode, and window focus.
- Large volume texture rebuilds remain CPU-heavy.
- Cross-platform performance has not been release-gated for Linux/macOS.
