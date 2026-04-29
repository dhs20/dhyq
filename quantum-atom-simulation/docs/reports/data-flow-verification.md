# Data Flow Verification

- Generated: 2026-04-29
- Scope: UI state, physics recompute, data catalogs, rendering, reports, CSV exports, package runtime resources.

## Runtime Flow

1. User changes controls in `ui/AppUi.cpp`.
2. `SimulationState` records inputs and dirty flags.
3. `Application::recomputeDerived()` rebuilds derived physics outputs:
   - hydrogenic values and transitions
   - central-field numerical result
   - Tier 2 mean-field result
   - Tier 3 spectroscopy corrections
   - Tier 4 correlation result
   - Tier 5 dynamics result
   - nuclear teaching-model outputs
4. `SceneRenderer` receives cached cloud/volume resources only when dirty.
5. UI panels read the same `DerivedData` for Physics, Plots, Performance, Help, Log.
6. `ValidationReportWriter` and plot CSV export serialize the currently active method stamps, validation records, and plot-window data.

## Asset/Data Flow

| Asset | Loader | Fallback | Validation Point |
| --- | --- | --- | --- |
| `assets/data/elements.json` | `ElementDatabase::loadFromJson` | built-in light-element subset | health check, tests |
| `assets/data/reference_catalog.json` | `loadReferenceTransitions` | `nist_reference_lines.csv` with warning | tests, health check, package verification |
| `assets/data/isotopes.json` | `loadIsotopes` | reduced isotope/nuclear teaching anchors | tests, health check |
| `assets/scenarios/*.json` | demo loader | built-in steps where applicable | smoke/package verification |

## Import/Export Flow

| Flow | Path | Status |
| --- | --- | --- |
| Validation Markdown/JSON | `docs/reports/latest-validation-report.*` | Covered by regression test |
| Plot CSV export | UI current-window export | Implemented; output includes Tier 2/3/4/5 derived data when active |
| Release reports | `docs/reports/*.md` | Implemented |
| Package resources | `dist/windows/x64/release/quantum_atom_simulation/` | Verified by script |

## Consistency Checks

| Checkpoint | Mechanism | Result |
| --- | --- | --- |
| Atomic number / charge state filters | `referenceTransitions(atomicNumber, chargeState)` | Tested |
| Method provenance non-empty | `MethodStamp`, `SourceRecord`, `ProvenanceRecord` | Tested in physics/data/report paths |
| Time-dependent labeling | Tier 5 `MethodStamp::isTimeDependent` and UI disclosure | Hardened |
| Missing data behavior | explicit fallback and warnings | Hardened |
| Package resource integrity | `verify_package.ps1` | Hardened |
