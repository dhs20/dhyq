# Validation

## Automated Checks

The `quantum_atom_tests` target validates:

- CODATA-based constants and unit conversions
- hydrogen Lyman-alpha wavelength
- hydrogen Balmer-alpha wavelength
- forbidden `2s -> 1s` electric-dipole transition
- neon electron configuration
- neon `2p` Slater `Z_eff`
- approximate `1s` radial normalization
- analytical `1s`/`2s` orthogonality
- numerical hydrogen `1s` finite-difference solve
- convergence samples for grid refinement

## Current Reference Cases

| Case | Expected | Tolerance |
| --- | --- | --- |
| H Lyman-alpha (`2p -> 1s`) | `121.567 nm` | `0.15 nm` |
| H Balmer-alpha (`3p -> 2s`) | `656.279 nm` | `0.4 nm` |
| Ne `2p` `Z_eff` | `5.85` | `0.3` |
| `1s` radial normalization | `1.0` | `1e-2` |

## Test Result

Latest local run:

```text
All tests passed.
```

## Runtime Report Export

The application can export the current runtime validation snapshot to:

- `docs/reports/latest-validation-report.md`
- `docs/reports/latest-validation-report.json`

The exported report includes active methods, approximation warnings, and validation records for the current UI state.

## Numerical Solver Status

The current solver path is a true finite-difference tridiagonal eigen solve for hydrogenic validation mode.

The UI plots convergence as:

- radial grid step
- versus absolute energy error

This closes the earlier gap where the numerical path depended on an analytical fallback.

## Reference-Data Scope

Current spectrum comparison in the application uses a local example CSV file under `assets/data/`.

This is useful for UI-side spot checks and demonstration, but it is not a full NIST ASD mirror or complete external-database integration.
