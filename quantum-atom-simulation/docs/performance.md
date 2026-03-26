# Performance Notes

## Implemented Measures

- cached cloud generation instead of per-frame recomputation
- GPU point-cloud upload via VBO/VAO
- optional volume-slice rendering from a cached 3D texture
- off-screen framebuffer for the scene viewport
- runtime LOD that reuses the same cached cloud and draws only the densest prefix of the sorted point set
- interaction LOD that reduces point and slice counts while the camera is actively orbiting/panning/dollying
- adaptive sampling presets (`Fast`, `Balanced`, `High Fidelity`) that scale candidate count, radial CDF resolution, angular scan resolution, and Monte Carlo normalization samples
- GPU timestamp-query timing for whole-frame, point-pass, and volume-pass measurements when supported by the driver/context
- tracked GPU-memory estimates for the point VBO, volume 3D texture, and scene framebuffer
- runtime FPS and build-time statistics in the Performance panel

## Tunable Controls

- point count
- LOD enable/disable
- medium/far/interaction point budgets
- medium/far camera distance thresholds
- adaptive sampling toggle
- sampling quality preset
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
- GPU timings are driver-dependent and fall back to zeroed values when timestamp queries are unavailable
- GPU memory is an application-side estimate of owned buffers/textures/framebuffer attachments, not a vendor-reported total

## Recommended Next Steps

1. Move point and volume generation to a worker thread.
2. Replace CPU-side volume rebuilds with sparse brick updates or a compute path.
3. Add occlusion-aware or screen-space-error LOD selection instead of distance-only thresholds.
4. Integrate vendor-specific memory telemetry where available.
