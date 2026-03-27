# Quantum Atom Simulation

Modernized 3D atomic visualization system with:

- OpenGL 3.3 Core rendering
- GLFW windowing
- ImGui Docking GUI
- SI-based hydrogenic physics
- data-driven element database
- Bohr transitions and spectrum view
- phase-colored Schrodinger probability clouds
- numerical radial cache can drive the cloud sampler and phase visualization
- Slater-rule multi-electron teaching approximation
- finite-difference radial solver with convergence reporting
- asynchronous cloud regeneration to keep the UI responsive during heavy rebuilds
- physics/math validation target

## Build

Prerequisites:

- Windows with MSVC and xmake

Commands:

```powershell
$env:XMAKE_GLOBALDIR='D:\project\dhyq\.xmake-global'
$env:XMAKE_PKG_CACHEDIR='D:\project\dhyq\.xmake-pkg-cache'
$env:XMAKE_PKG_INSTALLDIR='D:\project\dhyq\.xmake-pkg-install'
xmake f -c -y
xmake -y
```

Binary output:

- `build/windows/x64/release/quantum_atom_simulation.exe`
- `build/windows/x64/release/quantum_atom_tests.exe`

## Run

Application:

```powershell
build\windows\x64\release\quantum_atom_simulation.exe
```

Tests:

```powershell
build\windows\x64\release\quantum_atom_tests.exe
```

Direct Windows launchers:

- [run_release.bat](D:/project/dhyq/quantum-atom-simulation/run_release.bat): launch the release app directly
- [verify_project.bat](D:/project/dhyq/quantum-atom-simulation/verify_project.bat): run tests, then launch the app

The executable now resolves the project root automatically, so launching it from `build/windows/x64/release/` also works without manually setting the working directory.

## GUI Panels

- `Scene`: 3D viewport with orbit rings, point clouds, and volume slices
- `Inspector`: element, charge, model, transition, cloud, and solver controls
- `Physics`: current formulas, units, `Z_eff`, transition values, and solver status
- `Plots`: energy levels, spectra, radial distribution, and convergence
- `Performance`: FPS, CPU/GPU timing, async cloud-build state, LOD state, sampling stats, and tracked GPU memory
- `Help`: concept summary and demo-script export
- `Log`: runtime and validation messages

## Example Scenes

Quick buttons in the Inspector:

- `H`
- `He+`
- `Ne Approx`

Scene definitions are stored in [assets/scenarios/demo_scenes.json](D:/project/dhyq/quantum-atom-simulation/assets/scenarios/demo_scenes.json).

## Project Layout

- `app/`: bootstrapping and orchestration
- `core/`: logging, paths, frame stats
- `physics/`: constants, transitions, configurations, shielding, clouds, solver
- `render/`: camera, OpenGL objects, scene renderer
- `ui/`: docking panels and plots
- `docs/`: model notes, validation, performance guidance
- `tests/`: physics verification target
- `assets/data/`: element and reference-line data

## Validation

Current local verification:

- application target builds successfully
- test target passes
- application binary starts successfully in a local smoke test
- hydrogenic radial solver uses a finite-difference tridiagonal eigen solve
- convergence samples are exposed in the GUI and tests
- cloud generation tests cover fixed and adaptive sampling-stat paths
- cloud generation tests verify that numerical-radial overrides actually affect sampled clouds

See:

- [design.md](D:/project/dhyq/quantum-atom-simulation/design.md)
- [docs/physics-model.md](D:/project/dhyq/quantum-atom-simulation/docs/physics-model.md)
- [docs/validation.md](D:/project/dhyq/quantum-atom-simulation/docs/validation.md)
- [docs/performance.md](D:/project/dhyq/quantum-atom-simulation/docs/performance.md)
