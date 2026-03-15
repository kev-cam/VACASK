# List of Simulator Options

A complete reference of every simulator option, grouped by category.

## Temperature and scaling

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `temp` | real | `27` | Operating temperature in °C. Available as `$temp` in expressions. |
| `tnom` | real | `27` | Device model reference temperature in °C. |
| `scale` | real | `1.0` | Geometry scaling factor. All dimensions are multiplied by this value. |

## Device conductances

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `gmin` | real | `1e-12` | Minimum conductance (S) shunted across device junctions. Must be ≥ 0. |
| `gshunt` | real | `0` | Additional shunt conductance (S) from potential nodes to ground. 0 = off. |
| `minr` | real | `0` | Minimum resistance (Ω). Prevents singularities in diode-like devices. |

## Tolerances

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `tolmode` | identifier | `"spice"` | Tolerance mode: `"spice"`, `"va"` (Verilog-A natures), or `"mixed"`. |
| `tolscale` | real | `1.0` | Global scaling factor for absolute tolerances. |
| `reltol` | real | `1e-3` | Relative tolerance for convergence. 0 < reltol < 1. |
| `abstol` | real | `1e-12` | Absolute current tolerance in A. |
| `vntol` | real | `1e-6` | Absolute voltage tolerance in V. |
| `chgtol` | real | `1e-15` | Charge tolerance in As. Default corresponds to 1 mV across 1 pF. |
| `fluxtol` | real | `1e-14` | Flux tolerance in Vs. Default corresponds to 1 µA across 10 nH. |

### Tolerance modes

- **`spice`** — Uses `vntol` for non-flow unknowns, `abstol` for flow
  unknowns, and corresponding charge/flux tolerances for reactive residuals.
- **`va`** — Uses Verilog-A natures and disciplines where available; does not
  enforce tolerances otherwise.
- **`mixed`** — Uses Verilog-A natures where available; falls back to SPICE
  tolerances otherwise.

## Relative reference

Controls how the reference value for relative tolerance is computed.

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `relref` | identifier | `"alllocal"` | Overall relative reference policy. |
| `relrefsol` | identifier | `"relref"` | Reference for solution convergence. |
| `relrefres` | identifier | `"relref"` | Reference for residual convergence. |
| `relreflte` | identifier | `"relref"` | Reference for LTE error. |

When set to `"relref"`, each sub-option inherits the value of `relref`.
Specific values are:

| Value | Description |
|-------|-------------|
| `"pointlocal"` | Separate for each unknown, at the current timepoint. |
| `"local"` | Separate for each unknown, maximum over past timepoints. |
| `"pointglobal"` | Maximum over all unknowns, at the current timepoint. |
| `"global"` | Maximum over all unknowns and past timepoints. |

The `relref` option accepts summary values:

| Value | Description |
|-------|-------------|
| `"alllocal"` | All sub-options use `"local"`. |
| `"sigglobal"` | Mixed: signal-level uses global reference. |
| `"allglobal"` | All sub-options use `"global"`. |

## Matrix and solution checks

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `matrixcheck` | integer | `0` | Check matrix for inf/NaN. 0 = off, 1 = warn, 2 = error. |
| `rhscheck` | integer | `1` | Check right-hand side vector for inf/NaN. |
| `solutioncheck` | integer | `1` | Check solution vector for inf/NaN. |
| `rcondcheck` | real | `0` | If > 0, check matrix reciprocal condition number; fail if below this threshold. Used in small-signal analyses. |

## Newton-Raphson solver

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `nr_debug` | integer | `0` | Debug verbosity. ≥ 1 messages, ≥ 2 new solution, ≥ 3 old solution, ≥ 4 linear system. |
| `nr_bypass` | integer | `0` | Enable bypassing converged instances. 1 = on. |
| `nr_convtol` | real | `0.01` | Tolerance factor for instance convergence check. < 1 is stricter. |
| `nr_bypasstol` | real | `0.01` | Tolerance factor for instance bypass check. < 1 is stricter. |
| `nr_conviter` | integer | `1` | Number of consecutive convergent iterations required to confirm convergence. |
| `nr_residualcheck` | integer | `1` | Check residual (not only solution change) for convergence. |
| `nr_damping` | real | `1.0` | Newton-Raphson damping factor. 0 < damping ≤ 1. |
| `nr_force` | real | `1e5` | Forcing factor for nodesets and initial conditions. |
| `nr_contbypass` | integer | `1` | Allow forced bypass in first NR iteration when continuation mode is enabled. |

## Homotopy

### Gmin stepping

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `homotopy_debug` | integer | `0` | Debug verbosity for homotopy algorithms. |
| `homotopy_gminsteps` | integer | `100` | Maximum gmin stepping steps. ≤ 0 disables gmin stepping. |
| `homotopy_gminfactor` | real | `10.0` | Initial gmin stepping factor. |
| `homotopy_maxgminfactor` | real | `10.0` | Maximum gmin stepping factor. |
| `homotopy_mingminfactor` | real | `1.00005` | Minimum gmin stepping factor; give up when factor falls below this. |
| `homotopy_startgmin` | real | `1e-3` | Gmin value at which dynamic stepping starts. |
| `homotopy_maxgmin` | real | `1e2` | Gmin value above which the algorithm fails. |
| `homotopy_mingmin` | real | `1e-15` | Gmin value at which stepping stops (when gmin/gshunt are 0). |

### Source stepping

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `homotopy_srcsteps` | integer | `100` | Maximum source stepping steps. ≤ 0 disables source stepping. |
| `homotopy_srcstep` | real | `0.01` | Initial source step for dynamic source stepping. |
| `homotopy_srcscale` | real | `3.0` | Source step scaling factor (multiplied on success, divided on failure). |
| `homotopy_minsrcstep` | real | `1e-7` | Source step at which stepping gives up. |
| `homotopy_sourcefactor` | real | `1.0` | Scaling factor for all independent sources. Use 1.0 for normal simulation. |

## Operating point

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `op_debug` | integer | `0` | Debug verbosity. ≥ 1 convergence report, ≥ 2 continuation info. |
| `op_itl` | integer | `100` | Maximum NR iterations (non-continuation mode). |
| `op_itlcont` | integer | `50` | Maximum NR iterations (continuation mode). |
| `op_skipinitial` | integer | `0` | Skip initial OP attempt; go straight to homotopy. |
| `op_homotopy` | list | `["gdev","gshunt","src"]` | Homotopy algorithms to try, in order. |
| `op_srchomotopy` | list | `["gdev","gshunt"]` | Homotopy algorithms to try when source stepping fails at factor 0. |
| `op_nsiter` | integer | `1` | Number of NR iterations during which nodesets are applied. |

## Small-signal

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `smsig_debug` | integer | `0` | Debug verbosity. ≥ 1 for messages, ≥ 100 print linear system, ≥ 101 print matrix before checks. |

## Transient

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `tran_debug` | integer | `0` | Debug verbosity. 1 = steps, 2 = solver. |
| `tran_method` | identifier | `"trap"` | Integration method: `"trap"`, `"am"`, `"am2"`, `"gear"`, `"bdf"`, `"gear2"`, `"bdf2"`, `"euler"`. |
| `tran_maxord` | integer | `2` | Maximum integration order (for `"am"` and `"gear"`/`"bdf"` methods). |
| `tran_itl` | integer | `10` | Maximum NR iterations per timepoint. |
| `tran_fs` | real | `0.25` | Fraction of `step` used to compute the first timestep. 0 < fs ≤ 0.5. |
| `tran_ffmax` | real | `0.25` | Fraction of the fastest source period that limits the initial timestep. 0 = off. |
| `tran_fbr` | real | `0.2501` | Fraction of breakpoint interval for maximum timestep between breakpoints. 0 < fbr ≤ 1/3. |
| `tran_rmax` | real | `0` | Ratio of timestep to `step` upper limit. < 1 disables this limit. |
| `tran_minpts` | integer | `50` | Minimum number of timepoints from `start` to `stop`. < 1 disables this limit. |
| `tran_ft` | real | `0.25` | Factor for reducing the timestep when iterations exceed `tran_itl`. |
| `tran_predictor` | integer | `0` | 1 = use predictor for the initial guess, 0 = use previous solution. |
| `tran_redofactor` | real | `2.5` | Reject a timepoint if timestep / LTE-computed timestep exceeds this. 0 = no rejection. |
| `tran_lteratio` | real | `3.5` | LTE overestimation factor. Greater values yield looser LTE tolerance. |
| `tran_spicelte` | integer | `0` | 0 = correct LTE handling, 1 = SPICE-like (less accurate) LTE. |
| `tran_xmu` | real | `0.5` | Trapezoidal blending coefficient. 0 = backward Euler, 0.5 = pure trapezoidal. |
| `tran_trapltefilter` | integer | `1` | Enable trap ringing filter for predictor and LTE computation. Only for AM order 2. |

### Integration method shortcuts

| Value | Equivalent |
|-------|------------|
| `"trap"` | Adams-Moulton, max order 2 (trapezoidal). |
| `"am2"` | Same as `"trap"`. |
| `"am"` | Adams-Moulton, max order set by `tran_maxord`. |
| `"euler"` | Adams-Moulton, max order 1 (backward Euler). |
| `"gear"` | BDF, max order set by `tran_maxord`. |
| `"bdf"` | Same as `"gear"`. |
| `"gear2"` | BDF, max order 2. |
| `"bdf2"` | Same as `"gear2"`. |

## Harmonic balance

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `hb_debug` | integer | `0` | Debug verbosity. ≥ 1 convergence, ≥ 2 continuation, ≥ 3 spectrum. |
| `hb_itl` | integer | `100` | Maximum NR iterations (non-continuation mode). |
| `hb_itlcont` | integer | `50` | Maximum NR iterations (continuation mode). |
| `hb_skipinitial` | integer | `1` | Skip initial HB attempt; go straight to homotopy. |
| `hb_homotopy` | list | `["src"]` | Homotopy algorithms to try. |

## Output

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `rawfile` | identifier | `"binary"` | Output format: `"ascii"` or `"binary"`. |
| `strictoutput` | integer | `2` | 0 = keep files on error, 1 = delete on error, 2 = delete before analysis. |
| `strictsave` | integer | `1` | 0 = bind failures produce zeros, 1 = error on first bind only, 2 = always error. |
| `strictforce` | integer | `1` | 0 = warn on nodeset/IC conflicts, 1 = abort on conflicts. |
| `accounting` | integer | `0` | 0 = statistics outside analyses, 1 = also inside analyses. |

## Sweep

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `sweep_pointmarker` | integer | `0` | 1 = yield before each sweep point (for co-simulation). |
| `sweep_debug` | integer | `0` | Debug verbosity. ≥ 1 messages, ≥ 2 details. |
