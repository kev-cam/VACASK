# AC Small-Signal Analysis

The AC small-signal analysis computes the frequency-domain response of a
linearized circuit around its DC operating point. It sweeps across a range of
frequencies and produces complex-valued phasors that represent the magnitude
and phase of sinusoidal steady-state responses.

## Syntax

```text
analysis name ac [parameters]
```

## How it works

1. VACASK solves for the DC operating point $x_0$ where $f(x_0)=0$ ($f$ is
   the resistive residual).
2. It linearizes the circuit at $x_0$ by computing the resistive Jacobian
   $J_r$ (Jacobian of $f$) and the reactive Jacobian $J_c$ (Jacobian of the
   charge/flux vector $q$).
3. For each frequency $f$ the analysis solves

$$\left(J_r + j\omega\, J_c\right) X = U$$

   where $\omega = 2\pi f$ and $U$ contains the AC excitations.
4. The result $X$ is a vector of phasors. A sinusoidal signal
   $A\cos(\omega t + \varphi)$ corresponds to the phasor
   $A\exp(j\varphi)$.

AC excitations are set through the `mag` and `phase` parameters on independent
sources. Phase is given in degrees. A negative magnitude is equivalent to
adding 180° to the phase.

## Parameters

AC analysis exposes the operating point parameters and adds frequency sweep
controls.

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `from` | real | `0` | Start frequency (for step, lin, dec, oct sweeps). |
| `to` | real | `0` | Stop frequency (for step, lin, dec, oct sweeps). |
| `step` | real | `0` | Step size (linear stepped sweep when `mode` is not given). |
| `mode` | identifier | — | Sweep mode: `"lin"` (linear), `"dec"` (points per decade), or `"oct"` (points per octave). |
| `points` | integer | `0` | Number of points when `mode` is given. |
| `values` | vector | — | Explicit vector of frequency values; overrides other sweep settings. |
| `nodeset` | string or list | `""` | Initial guess for the operating point. Can be a stored solution name or explicit node voltages. |
| `store` | string | `""` | Save the computed operating point under the given name. |
| `writeop` | boolean | `0` | Write the operating point to `<analysis>.op.*`. |
| `write` | boolean | `1` | Write analysis results to a file. |

### Frequency sweep modes

| Mode | Description |
|------|-------------|
| *none* (`step` given) | Stepped sweep from `from` to `to` with increment `step`. |
| `lin` | Linear sweep from `from` to `to` with `points` evenly spaced points. |
| `dec` | Logarithmic sweep with `points` points per decade. |
| `oct` | Logarithmic sweep with `points` points per octave. |
| *none* (`values` given) | Sweep over an explicit list of frequency values. |

## Save directives

| Directive | Description |
|-----------|-------------|
| `default` | Save all node voltages and branch currents (default behavior). |
| `full` | Save all node voltages. |
| `dv(node)` | Save the complex voltage phasor at the specified node. |
| `di(instance)` | Save the complex current phasor through the specified instance. |

AC analysis also supports all operating point save directives (`v(node)`,
`i(instance)`, `p(instance,outvar)`) because it reuses the operating point
core.

## Output

- A file `<analysis>.*` containing the complex phasors across frequency.
- If `writeop=1`, an additional `<analysis>.op.*` file with the operating
  point solution.

## Examples

**Decade sweep from 1 Hz to 10 GHz, 20 points per decade:**

```text
analysis ac1 ac from=1 to=10G mode="dec" points=20
```

**Linear sweep:**

```text
analysis ac1 ac from=1k to=100k mode="lin" points=500
```

**Explicit frequency values:**

```text
analysis ac1 ac values=[100, 1k, 10k, 100k, 1M]
```

**With operating point output:**

```text
analysis ac1 ac from=1 to=1G mode="dec" points=10 writeop=1
```
