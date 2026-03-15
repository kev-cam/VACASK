# Harmonic Balance Analysis

The harmonic balance (HB) analysis finds the periodic steady-state response
of a circuit driven by one or more periodic signals. Instead of simulating
the circuit until it settles (as transient analysis would), HB solves directly
for the Fourier coefficients of the steady-state waveform.

## Syntax

```text
analysis name hb [parameters]
```

## How it works

1. **Frequency grid.** VACASK builds a set of spectral frequencies from the
   specified fundamental frequencies and the truncation scheme.
2. **Colocation points.** It selects a set of time-domain sampling points
   across one or more periods.
3. **APFT.** An Almost Periodic Fourier Transform maps between the
   frequency-domain (solution phasors) and the time-domain (values at
   colocation points).
4. **Newton-Raphson.** VACASK iterates in the frequency domain, evaluating
   device equations at each colocation point in the time domain and
   transforming residuals back to the frequency domain.

## Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `freq` | real vector | `{}` | Fundamental frequencies ($f_1, f_2, \ldots, f_d$). |
| `nharm` | integer or vector | `4` | Number of harmonics per fundamental. Scalar applies to all; vector components apply to corresponding fundamentals. |
| `immax` | integer | `0` | Maximal intermodulation product order. If ≤ 0, defaults to the largest component of `nharm`. |
| `truncate` | identifier | `"diamond"` | Truncation scheme: `"raw"`, `"box"`, or `"diamond"`. |
| `samplefac` | real | `2` | Sampling factor in the time domain (≥ 1). |
| `nper` | real | `3` | Number of periods across which colocation points are selected. |
| `sample` | identifier | `"random"` | Sampling mode: `"uniform"` or `"random"`. |
| `harmonic` | integer vector | `{}` | Annotation for raw mode: flags indicating which entries in `freq` are harmonics. |
| `imorder` | integer vector | `{}` | Annotation for raw mode: intermodulation order of each entry in `freq`. |
| `nodeset` | string | `""` | Stored solution slot to use as initial guess. |
| `store` | string | `""` | Store the computed solution for later use. |
| `write` | boolean | `1` | Write analysis results to a file. |

### Truncation schemes

| Scheme | Description |
|--------|-------------|
| `raw` | The values in `freq` are used directly as the spectrum frequencies. No harmonic generation is performed. |
| `box` | Box truncation. For $d$ fundamentals the index ranges are $k_j = 0 \ldots H_j$; the first nonzero index must be positive. |
| `diamond` | Diamond truncation (default). Keeps spectral components satisfying $\sum \lvert k_j\rvert \le \text{immax}$; the first nonzero index must be positive. Balances spectral content against computational cost. |

## Save directives

| Directive | Description |
|-----------|-------------|
| `default` | Save all node voltages and branch currents at colocation points. |
| `full` | Save all node voltages at colocation points. |
| `v(node)` | Save the voltage at the specified node. |
| `i(instance)` | Save the current through the specified instance. |

## Output

- A file `<analysis>.*` containing the spectral solution.

## Examples

**Single-tone, 7 harmonics:**

```text
analysis hb1 hb freq=[1G] nharm=7
```

**Two-tone with diamond truncation:**

```text
analysis hb1 hb freq=[900M, 901M] nharm=[5, 5] immax=3
```

**Box truncation with explicit harmonic counts:**

```text
analysis hb1 hb freq=[1G] nharm=10 truncate="box"
```

**Using a stored solution as initial guess:**

```text
analysis hb1 hb freq=[1G] nharm=4 store="hb_sol"
analysis hb2 hb freq=[1G] nharm=8 nodeset="hb_sol"
```

## Convergence

HB convergence is controlled by the `hb_itl` simulator option (iteration
limit, default 100). Homotopy algorithms can be configured via `hb_homotopy`.
The `hb_skipinitial` option (default 1) skips the initial guess computation
when a nodeset is provided.
