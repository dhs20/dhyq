# Physics Model

## Units

- Internal public-facing quantities are stored and reported in SI.
- UI display converts SI values to common teaching units:
  - energy: eV
  - wavelength: nm
  - radius: nm
- Rendering uses a separate visualization scale to map meter-scale atomic dimensions into scene units.

## Hydrogenic Model

Strict hydrogenic mode uses the reduced-mass corrected one-electron formulas:

- `E_n = -Ry * (mu / m_e) * Z_eff^2 / n^2`
- `r_n = a0 * (m_e / mu) * n^2 / Z_eff`
- `v_n = alpha * c * Z_eff / n`

Here:

- `Ry = 2.1798723611035e-18 J`
- `a0 = 5.29177210903e-11 m`
- `mu` is the electron-nucleus reduced mass

The strict mode is physically valid for H and hydrogen-like ions such as He+.

## Selection Rules and Spectrum

Transition filtering supports:

- no rule filtering
- electric-dipole filter with `Delta l = +-1`
- optional `Delta m` bound `|Delta m| <= 1`

Each accepted or rejected transition still reports:

- `Delta E`
- frequency
- wavelength
- series classification

Series labeling:

- `n_f = 1`: Lyman
- `n_f = 2`: Balmer
- `n_f = 3`: Paschen
- `n_f = 4`: Brackett
- `n_f = 5`: Pfund

## Multi-Electron Approximation

The multi-electron mode is intentionally labeled as an approximation.

Implemented pieces:

- Aufbau filling order
- Pauli occupancy limits
- configurable neutral-atom exception whitelist
- Slater shielding
- `Z_eff = Z - sigma`

For a selected subshell, hydrogenic metrics are reused with `Z -> Z_eff`.

This is suitable for teaching trends and qualitative orbital size/energy shifts, but not for precision spectroscopy.

## Wavefunction and Phase

The cloud visualizer keeps the complex phase explicitly:

- `psi = R_nl(r) Y_lm(theta, phi)`
- point-cloud color maps `arg(psi)`
- point alpha/brightness maps `|psi|^2`
- superposition coefficients are renormalized before evaluation

Point generation uses:

- precomputed radial CDFs
- angular rejection sampling
- cached candidate filtering for 1e5-point clouds
- optional numerical-radial interpolation from the finite-difference solver when the selected `(n, l)` matches the cached solver state

Animation note:

- most current scene motion is illustrative or pedagogical
- superposition visuals show basis-state interference structure
- this is not a direct time-dependent TDSE / TD-CI propagation result

## Numerical Solver

The project includes a `SchrodingerNumericalSolver` for the radial Coulomb equation using a finite-difference tridiagonal eigen solve.

Implemented workflow:

- discretize `[-1/2 d^2/dr^2 + l(l+1)/(2r^2) - 1/r] u = epsilon u` on a radial grid
- construct the symmetric tridiagonal Hamiltonian
- isolate the requested bound-state eigenvalue with a Sturm-sequence bisection
- recover the eigenvector with inverse iteration
- normalize `u(r)` and convert to `R(r) = u(r) / r`
- report convergence samples as `grid step -> energy error`

This path is fully numerical for the hydrogenic validation mode.
