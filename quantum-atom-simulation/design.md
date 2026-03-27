# 3D Atomic Visualization Refactor Design

## Goals

This refactor replaces the legacy `freeglut + fixed-function immediate mode` prototype with a modular, testable, data-driven system that satisfies the following engineering requirements:

- Modern OpenGL 3.3 Core rendering with shader-based pipelines and explicit GPU buffers.
- Dockable GUI with scene, inspector, physics, plots, performance, help, and log panels.
- Unified physics-facing unit system based on SI outputs and explicit display-unit conversion in the UI.
- Verifiable hydrogenic physics for H and hydrogen-like ions, plus approximate multi-electron teaching mode using Aufbau filling and Slater shielding.
- Probability-cloud visualization with explicit complex phase, superposition support, cached sampling, and volume-slice rendering.
- Numerical radial Schrödinger solver with convergence diagnostics and overlay against analytical hydrogenic references.
- Documentation, examples, and automated physics/math tests.

## Module Layout

The new codebase is split into five top-level implementation areas and a single public include tree:

- `app/`
  - Application bootstrapping, dependency wiring, asset-path resolution, and main update loop.
  - Owns the simulation state and coordinates physics recomputation, UI, and rendering.
- `core/`
  - Logging, timers, lightweight profiling, and cross-module utility code.
- `physics/`
  - CODATA constants, units, hydrogenic formulas, transition/spectrum generation, element database loading, electron configuration, Slater rules, analytical wavefunctions, cloud sampling, and numerical solver.
- `render/`
  - OpenGL objects, camera, framebuffers, shaders, scene rendering, point clouds, orbit lines, energy overlays, and volume slices.
- `ui/`
  - ImGui docking shell and all user-facing panels, charts, report widgets, and validation summaries.
- `tests/`
  - Standalone binary for constants, transitions, electron configuration, Slater-rule sanity checks, wavefunction normalization, and solver convergence assertions.

## Public Interfaces

Headers live under `include/quantum/...` and are grouped by subsystem:

- `include/quantum/core/*`
- `include/quantum/physics/*`
- `include/quantum/render/*`
- `include/quantum/ui/*`

The physics layer is intentionally renderer-independent. The rendering and UI layers consume immutable or cached physics outputs rather than performing analytical work every frame.

## Data Flow

1. `Application` loads `assets/data/elements.json`, demo scenarios, and reference spectrum CSV files.
2. GUI changes mutate `SimulationState`.
3. Dirty flags trigger physics recomputation:
   - Bohr metrics and spectrum lines
   - Electron configuration and Slater shielding
   - Analytical wavefunction caches
   - Probability cloud / volume texture caches
   - Numerical solver caches
4. `SceneRenderer` renders into an off-screen framebuffer.
5. The Scene panel displays the framebuffer texture; other panels read the same cached state to plot graphs and tables.
6. Performance counters and logs are persisted frame-to-frame for diagnostics.

## Rendering Pipeline

### Windowing and Context

- GLFW window
- OpenGL 3.3 Core Profile
- GLAD loader
- ImGui docking + ImPlot

### GPU Objects

- `ShaderProgram`
- `VertexArray`
- `VertexBuffer`
- `IndexBuffer`
- `Texture2D`
- `Texture3D`
- `Framebuffer`

### Scene Passes

- Opaque scene pass
  - nucleus mesh
  - orbit rings
  - energy-level markers
- Transparent point-cloud pass
  - phase-colored points using per-vertex position, density, and phase
- Transparent volume-slice pass
  - 3D density/phase texture sampled by stacked slices
- Overlay pass
  - optional axis/grid helpers

### Performance Strategy

- No legacy matrix stack or immediate mode calls
- Cloud/VBO uploads occur only when parameters change
- Volume textures are rebuilt only on cache invalidation
- Runtime performance panel reports FPS, frame time, point count, acceptance ratio, and cache sizes

## UI Information Architecture

### Scene

- 3D viewport
- Mouse orbit, pan, dolly
- Rendering mode toggles
- Screenshot-ready clean viewport

### Inspector

- Element and charge state
- Mode selection: Bohr / Wave-Particle / Schrödinger Cloud / Compare
- Quantum numbers and transition selection
- Approximation toggle: strict hydrogenic vs multi-electron approximate
- Sampling density, threshold, slice resolution, and LOD controls

### Physics

- Current constants and units
- Energy, radius, velocity, `Z_eff`, transition outputs, normalization error
- Explicit warning banner for approximate multi-electron mode

### Plots

- Energy level diagram
- Spectrum lines
- Radial probability distribution `P(r)`
- Sampling-error plot
- Numerical-solver convergence plot

### Performance

- FPS
- CPU frame time
- Sample count / accepted count
- Cloud regeneration time
- Volume build time

### Help

- Concepts and formulas
- Shortcut reference
- Demo-scenario launcher

### Log

- Load/build/runtime issues
- Validation messages
- Scenario execution notes

## Physics Model Hierarchy

### Strict Hydrogenic Mode

Applies to:

- H
- Hydrogen-like ions such as `He+`, `Li2+`, `Ne9+`

Models:

- Reduced-mass corrected Bohr metrics
- Analytical hydrogenic wavefunctions `psi = R_nl(r) Y_lm(theta, phi)`
- Dipole transition filter with at least `Delta l = +-1`
- Numerical radial solver against Coulomb potential

Validation targets:

- `E_n ~ -Z^2 / n^2`
- `r_n ~ n^2 / Z`
- Canonical lines such as Lyman-alpha and Balmer-alpha

### Multi-Electron Approximate Teaching Mode

Applies to:

- Neutral or weakly ionized multi-electron atoms such as Ne

Models:

- Aufbau filling order
- Pauli occupancy limits
- Hund-guided same-subshell filling summary
- Configurable exception whitelist
- Slater shielding `sigma`
- Effective charge `Z_eff = Z - sigma`
- Hydrogenic-like orbital radius/energy estimates using `Z -> Z_eff`

UI treatment:

- Always labeled as approximation
- Scope and limitations shown directly in Physics and Help panels

## Numerical Solver Strategy

`SchrodingerNumericalSolver` solves the radial equation for Coulomb or effective central potentials.

Planned implementation:

- Numerov shooting for bound-state search
- Node counting for bracket selection
- Normalization of `u(r)` and conversion to `R(r)`
- Convergence report versus radial grid step

Outputs:

- Eigenenergy
- Radial wavefunction samples
- Error versus analytical hydrogenic energy
- Cached curves for ImPlot

## Verification and Testing

Automated tests cover:

- CODATA constant conversions
- Hydrogenic energy and wavelength calculations
- Electron-configuration formatting
- Slater-rule `sigma` / `Z_eff` sanity checks
- Approximate wavefunction normalization and orthogonality checks
- Numerical solver convergence for hydrogen-like ions

Documentation deliverables live under `docs/`:

- `physics-model.md`
- `validation.md`
- `performance.md`

## Milestones

### M1

- xmake modernization
- GLFW + GLAD + ImGui/ImPlot shell
- Docking layout
- Scene framebuffer, camera, and shader-based rendering skeleton

### M2

- SI-based Bohr model
- Hydrogen-like scaling with reduced mass
- Transition selection and spectrum plot
- Element data, electron configuration, Slater rules

### M3

- Complex hydrogenic wavefunction
- Superposition normalization
- Cached point-cloud generation
- Phase rendering and volume slices

### M4

- Numerical radial solver
- Convergence plots
- Analytical-vs-numerical validation tools

### M5

- Documentation, reports, tests, and curated demo scenarios for H, He+, and Ne
