# Command Line Options and Startup Sequence

## Command Line Options

| Option | Long form | Effect |
|--------|-----------|--------|
| `-h` | `--help` | Print help and exit. |
| `-dp` | `--dump-paths` | Print the locations of all simulator components (binary, module path, include path, OpenVAF, Python). |
| `-df` | `--debug-files` | Print each file's path as it is loaded, compiled, or written. |
| `-se` | `--skip-embed` | Do not extract embedded files from the input file. |
| `-sp` | `--skip-postprocess` | Do not run `postprocess` steps defined in the control block. |
| `-qp` | `--quiet-progress` | Suppress progress messages. |
| `--no-output` | | Suppress writing of result files. |
| `-n <n>` | `--ncpu <n>` | Number of threads the multithreaded solvers (`superlu`, `basker`) spawn per factorization. Default `1`. `n <= 0` autodetects: it honors `OMP_NUM_THREADS` if set, otherwise uses all available CPUs. `basker` rounds this down to the nearest power of two (Basker's `SetThreads()` requires one). |
| `-b <n>` | `--blas-ncpu <n>` | Number of threads OpenBLAS may use for dense operations. Default `1`. `n <= 0` lets OpenBLAS autodetect. |

If no filename is given VACASK prints a hint and exits.

## Parallelism

VACASK runs single-threaded unless a multithreaded linear solver is selected.
Builds compiled with the SuperLU_MT and/or Trilinos backends additionally
provide the `superlu` and `basker` solvers; see [Linear Solver
Selection](cmd-options-solver.md) for how to pick one per analysis or option.
When the simulator is built with the multithreaded SuperLU backend, `superlu`
is the default for [harmonic balance](cmd-analysis-hb.md) and the
harmonic-balance-based [(quasi)periodic small-signal
analysis](cmd-analysis-hbac.md). Every other analysis defaults to the `klu`
solver, which ignores both options below - unless a multithreaded solver is
selected explicitly for it.

There are two independent thread counts:

- `--ncpu` (`-n`): threads a `superlu` or `basker` factorization spawns.
  Default `1`.
- `--blas-ncpu` (`-b`): threads OpenBLAS uses for dense operations, most of which
  occur in the small-signal analyses. Default `1`.

This section describes `superlu`'s use of the OpenMP pool in detail; it does
not apply to `basker`, which manages its own thread pool independently (see
below).

### Serial runs

When both `--ncpu` and `--blas-ncpu` are `1` (the defaults), VACASK also pins the
OpenMP pool to a single thread. SuperLU's factorization loop has no explicit
thread-count clause, so without this cap it would still create a full-size team
of worker threads that busy-wait through every factorization and burn CPU for
nothing. With the cap, a default run is genuinely serial.

### Sizing the thread pool

As soon as either `--ncpu` or `--blas-ncpu` exceeds `1`, VACASK stops managing the
OpenMP pool; its size then comes entirely from `OMP_NUM_THREADS`. Set it to match
the work requested.

The solver and OpenBLAS thread counts multiply: a factorization on `--ncpu`
threads, each of which may call into an `--blas-ncpu`-threaded OpenBLAS, needs
`ncpu * blas_ncpu` threads in the pool. With `--ncpu 4 --blas-ncpu 3` size it for
`4 * 3 = 12`:

```text
OMP_NUM_THREADS=12 vacask -n 4 -b 3 circuit.sim
```

An undersized pool oversubscribes the cores and is usually slower than a smaller
thread count that schedules cleanly. Measure with `print stats`.

More threads is not always faster: for small circuits the factorization is cheap
and thread startup dominates, so the serial default often wins.

Leave `--blas-ncpu` at `1` unless the circuit is very large. The dense operations
it parallelizes are a small share of the total work for typical circuits, so
raising it only adds thread-management overhead and inflates the required
`OMP_NUM_THREADS` (recall the `ncpu * blas_ncpu` product above). It pays off only
when the small-signal dense blocks are large enough to amortize that cost.

Builds without the SuperLU backend, without the Trilinos backend, or without
OpenMP run everything on one thread, and both options have no effect on the
analyses that would otherwise use `superlu` or `basker`.

### `basker`'s thread pool

`basker` runs on Kokkos's OpenMP backend, which is sized once, at process
startup, directly from `--ncpu` (rounded down to the nearest power of two) -
not from `OMP_NUM_THREADS` or the ambient pool described above. There is
nothing to size manually: just pick `--ncpu`, and expect it to be clamped down
to a power of two (`--ncpu 6` behaves like `--ncpu 4`).

## Startup Sequence

When VACASK is launched it performs the following steps in order:

1. Parse command line flags.
2. Resolve the thread counts from `--ncpu` / `--blas-ncpu` (and `OMP_NUM_THREADS`); cap the OpenMP pool to one thread when both are `1`. `basker`'s Kokkos pool is sized separately, from `--ncpu` alone, the first time `basker` is used.
3. Apply `SIM_MODULE_PATH`, `SIM_INCLUDE_PATH`, and `SIM_OPENVAF` environment variables if set.
4. Read [TOML configuration files](startup-paths.md#toml-configuration-files) in order. Later files override earlier ones.
5. Parse the input file.
6. Extract embedded files to the current working directory (unless `-se` is given).
7. Create the circuit object, compiling any Verilog-A files referenced by `load` directives.
8. Execute the control block.
