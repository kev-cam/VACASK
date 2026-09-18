# Linear Solver Selection

These options choose the sparse linear solver used to factor and solve the
circuit Jacobian inside each analysis. VACASK always provides `klu` (the KLU
sparse solver). Builds compiled with the SuperLU_MT backend also provide
`superlu`, a multithreaded solver whose thread count is set with the
[`--ncpu` command line option](startup-options.md#parallelism). Builds compiled
with Trilinos also provide `basker`, the Trilinos ShyLU-Basker solver, likewise
multithreaded (OpenMP) and controlled by the same `--ncpu` option (rounded down
to the nearest power of two; see [Command Line
Options](startup-options.md#parallelism)).

| Name | Type | Default | Allowed | Description |
|------|------|---------|---------|-------------|
| `tdsolver` | string | `""` | `klu`, `superlu`, `basker`, `""` | Solver for the real Jacobian: operating point, `dcinc`, `dcxf`, transient, and the shooting loop of periodic steady-state analysis. The frequency-domain small-signal analyses also use it for the operating-point solve they run first. |
| `smsigsolver` | string | `""` | `klu`, `superlu`, `basker`, `""` | Solver for the complex Jacobian of the frequency-domain small-signal analyses: `ac`, `acxf`, `acstb`, `acsp`, and `noise`. |
| `hbsolver` | string | `""` | `klu`, `superlu`, `basker`, `""` | Solver for the real harmonic balance Jacobian, in both `hb` and the large-signal solve of `hbac`. |
| `qpsmsigsolver` | string | `""` | `klu`, `superlu`, `basker`, `""` | Solver for the complex conversion-matrix Jacobian of the harmonic-balance-based (quasi)periodic small-signal analysis (`hbac`). |

An empty string selects the built-in default: `klu` for `tdsolver` and
`smsigsolver`, and `superlu` for `hbsolver` and `qpsmsigsolver` when the
SuperLU_MT backend is present (`klu` otherwise). Harmonic balance defaults to
`superlu` because its Jacobian has dense spectral coupling that KLU handles
poorly.

Each analysis also accepts a `solver` parameter that overrides the matching
option for that one analysis. The frequency-domain small-signal analyses
additionally accept `opsolver`, which overrides `tdsolver` for their
operating-point solve.

## Choosing a solver

| Solver | Threading | Large circuits | HB / quasi-periodic small-signal |
|--------|-----------|-----------------|-----------------------------------|
| `klu` | Serial | Good - the generic, reliable default | Poor - the dense spectral coupling of block matrices defeats KLU's fill-reducing ordering |
| `superlu` | Parallel (SuperLU_MT) | Poor - overhead dominates on the small, sparse per-timepoint Jacobians of ordinary circuit simulation | Good - built for the denser block-sparse matrices HB and quasi-periodic small-signal analyses produce |
| `basker` | Parallel (OpenMP) | Good - same niche as KLU (block-triangular-form-aware sparse LU), but threaded | Not specifically targeted - behaves like KLU on these matrices |

`klu` is the safe default for ordinary (transient, DC, AC) analyses on large
circuits. For harmonic balance and `hbac`, prefer `superlu`. `basker` is worth
trying wherever `klu` would be used and multiple CPUs are available.

## Example

```text
control
  options hbsolver="klu"
  analysis hb1 hb freq=[1G] nharm=7
  analysis hb2 hb freq=[1G] nharm=7 solver="superlu"
endc
```
