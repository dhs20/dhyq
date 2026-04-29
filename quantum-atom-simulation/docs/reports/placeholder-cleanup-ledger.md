# Placeholder Cleanup Ledger

- Generated: 2026-04-29
- Policy: remove stale non-production wording and replace placeholder-like code with real, testable behavior.

| Path | Issue Type | Action | Verification |
| --- | --- | --- | --- |
| `basis/BasisModule.cpp` | Placeholder-like module file | Replaced by `basis/BasisMetadata.cpp` and `include/quantum/basis/BasisMetadata.h` | Added basis metadata regression checks |
| `ui/AppUi.cpp` | Stale fixed capability labels | Method tags now derive from active `MethodStamp`s; animation disclosure follows Tier 5 state | Build and UI-state regression coverage through tests/build |
| `app/Application.cpp` | Silent JSON-to-CSV reference fallback | Added explicit warning when `reference_catalog.json` falls back to CSV | Data fallback test and runtime logs |
| `xmake.lua` | Local machine package path coupling | Added optional `local_pkg_root` option instead of implicit hardcoded fallback | `scripts/build.ps1` passes local package root when configured |
| `README.md` | Old MVP wording | Updated to 0.3.1 deployable teaching software wording | Documentation scan |
| `README_PHYSICS.md` | Ambiguous high-tier method wording | Clarified teaching prototypes versus research-grade HF/DFT/CI/TDSE | Documentation scan |
| `README_BUILD.md` | Missing release-hardening scripts | Documented health, verify, coverage, validation, local package root | Documentation scan |
| `design.md` | Stale "reserved only" Tier 2/3/4/5 wording | Updated implemented teaching prototype status and boundaries | Documentation scan |
| `docs/current-status.md` | Stale "no correlation/hyperfine/TDSE" wording | Reframed as finite-basis teaching prototypes with research-grade caveats | Documentation scan |
| `docs/upgrade-status.md` | Old release date and MVP positioning | Updated to 2026-04-29 and 0.3.1 release posture | Documentation scan |
| `basis/README.md` | Documentation-only basis layer claim | Documented actual metadata API | Basis tests |
| `spectroscopy/README.md` | Stale "reserved" phrasing | Documented implemented correction layer and research boundaries | Spectroscopy tests |

Cleanup result: no known code path intentionally returns a fake success for an unimplemented production feature. Remaining limitations are documented as scientific/product boundaries rather than hidden placeholders.
