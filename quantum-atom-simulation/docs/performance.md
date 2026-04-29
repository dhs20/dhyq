# Performance Notes

## Implemented Measures

- cached cloud generation instead of per-frame recomputation
- GPU point-cloud upload via VBO/VAO
- optional volume-slice rendering from a cached 3D texture
- off-screen framebuffer for the scene viewport
- runtime LOD that reuses the same cached cloud and draws only the densest prefix of the sorted point set
- interaction LOD that reduces point and slice counts while the camera is actively orbiting/panning/dollying
- adaptive sampling presets (`Fast`, `Balanced`, `High Fidelity`) that scale candidate count, radial CDF resolution, angular scan resolution, and Monte Carlo normalization samples
- asynchronous cloud regeneration so heavy point/volume rebuilds no longer block the main UI/render loop
- GPU timestamp-query timing for whole-frame, point-pass, and volume-pass measurements when supported by the driver/context
- tracked GPU-memory estimates for the point VBO, volume 3D texture, and scene framebuffer
- runtime FPS and build-time statistics in the Performance panel
- deterministic release validation thresholds for non-GPU paths: solver execution, small cloud generation, report export, and package health checks

## Tunable Controls

- point count
- LOD enable/disable
- medium/far/interaction point budgets
- medium/far camera distance thresholds
- adaptive sampling toggle
- sampling quality preset
- background worker state and queue visibility
- volume resolution
- extent scale
- point size
- exposure
- orbit/grid toggles

## Expected Interactive Envelope

On the current implementation, the heavy cost is generation time, not draw time:

- cloud generation cost scales with candidate count and superposition complexity
- draw cost scales with the active LOD point budget and enabled volume slices
- interaction mode deliberately trades fidelity for responsiveness without regenerating the cloud

The UI exposes:

- current LOD level and camera distance
- cloud-worker running / queued state
- requested / accepted / rendered point counts
- point build time
- volume build time
- accepted/candidate counts and effective candidate multiplier
- CDF / angular / Monte Carlo sampling resolutions
- average frame time
- GPU frame / point / volume timing
- tracked GPU memory

## Validation Notes

- local Windows build passes and launches
- local test suite passes, including fixed and adaptive cloud-generation-stat checks
- numerical-radial cloud overrides are covered by a deterministic test with a synthetic radial cache
- Tier 2 mean-field, Tier 4 correlation, Tier 5 dynamics, validation report export, and package resources are included in release validation
- GPU timings are driver-dependent and fall back to zeroed values when timestamp queries are unavailable
- GPU memory is an application-side estimate of owned buffers/textures/framebuffer attachments, not a vendor-reported total
- OpenCppCoverage is used when available; otherwise the release process writes `docs/reports/coverage-matrix.md` and gates on critical module test evidence

## Recommended Next Steps

1. Replace CPU-side volume rebuilds with sparse brick updates or a compute path.
2. Add occlusion-aware or screen-space-error LOD selection instead of distance-only thresholds.
3. Integrate vendor-specific memory telemetry where available.
