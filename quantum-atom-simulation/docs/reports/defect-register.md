# Defect Register

- Generated: 2026-04-29
- Severity scale: P0 fatal, P1 serious, P2 normal, P3 minor.

| ID | Severity | Status | Reproduction / Trigger | Root Cause | Fix | Validation |
| --- | --- | --- | --- | --- | --- | --- |
| DEF-001 | P1 | Fixed | Build included placeholder-like `basis` source | Module existed only as a marker | Replaced with `BasisMetadata` implementation | Unit regression for labels, capacities, Aufbau order |
| DEF-002 | P1 | Fixed | Release package could be created without checking required assets | Package script had no verification step | Added `verify_package.ps1` and `package.ps1 -Verify` | Package verifier checks exe, docs, assets, scenarios, logs, tests |
| DEF-003 | P2 | Fixed | UI claimed correlation/relativity/field/time status from static wording | Capability text was not driven by actual method metadata | `drawMethodTags` now scans active `MethodStamp`s | Build/tests and manual UI inspection path |
| DEF-004 | P2 | Fixed | Build file implicitly referenced local package cache | Environment-specific fallback hidden in `xmake.lua` | Added explicit `local_pkg_root` option and build script support | `scripts/build.ps1` logs selected local package root |
| DEF-005 | P2 | Fixed | No formal coverage artifact when line coverage tooling unavailable | Release process lacked fallback evidence model | Added OpenCppCoverage path and coverage matrix fallback | `coverage_report.ps1` writes coverage artifact |
| DEF-006 | P2 | Fixed | Missing `reference_catalog.json` could fall back to CSV without explicit warning | App logged only total reference-load failure | Added warning for JSON-to-CSV fallback | Data fallback regression plus runtime log |
| DEF-007 | P3 | Fixed | Animation disclosure always said no TDSE/field/dissipation | Text ignored Tier 5 settings | Disclosure now follows `state.dynamics` | Build/UI path |

## Residual Risks

| Risk | Severity | Mitigation |
| --- | --- | --- |
| GPU timings vary by driver | P3 | Treat GPU metrics as observational, not pass/fail |
| Linux/macOS not release-gated | P2 | Documented as outside current Windows x64 release target |
| Teaching physics may be mistaken for research-grade | P2 | Method labels, reports, README, and caveats explicitly mark prototypes |
