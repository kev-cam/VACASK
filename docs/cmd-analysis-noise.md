# Small-Signal Noise Analysis

The noise analysis computes the output-referred noise power spectral density
of a circuit as a function of frequency. It identifies the contribution of
every noise source in every device instance and reports both individual
contributions and the total output noise. When an input source is specified
the analysis also computes the power gain from input to output.

## Syntax

```text
analysis name noise [parameters]
```

## How it works

1. VACASK solves for the DC operating point $x_0$ where $f(x_0) = 0$.
2. It linearizes the circuit at $x_0$ by computing $J_r$ and $J_c$.
3. For each noise source the analysis solves

$$\left(J_r + j\omega\, J_c\right) X = U_{\text{noise}}$$

   and accumulates the contribution to the output power spectral density.
4. If an input source is given, the same linear system is solved with a unit
   excitation at the input to compute the power gain from input to output.
5. Contributions are aggregated per device instance and summed to produce
   the total output noise.

## Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `out` | string or vector | `""` | Output node or node pair. |
| `in` | identifier | `""` | Input source used for gain computation. |
| `from` | real | `0` | Start frequency. |
| `to` | real | `0` | Stop frequency. |
| `step` | real | `0` | Step size (linear stepped sweep when `mode` is not given). |
| `mode` | identifier | — | Sweep mode: `"lin"`, `"dec"`, or `"oct"`. |
| `points` | integer | `0` | Number of points when `mode` is given. |
| `values` | vector | — | Explicit vector of frequency values. |
| `nodeset` | string or list | `""` | Initial guess for the operating point. |
| `store` | string | `""` | Save the computed operating point under the given name. |
| `writeop` | boolean | `0` | Write the operating point to `<analysis>.op.*`. |
| `write` | boolean | `1` | Write analysis results to a file. |

See the [AC analysis](cmd-analysis-ac.md) documentation for details on the
frequency sweep modes.

## Save directives

| Directive | Description |
|-----------|-------------|
| `default` | Save all noise contributions per instance (totals only). |
| `full` | Save all noise contributions per instance with detailed per-source breakdown. |
| `n(instance)` | Save the total noise contribution from the given instance. |
| `nc(instance)` | Save the detailed noise contributions from the given instance (per source). |

Noise analysis also supports all operating point save directives (`v(node)`,
`i(instance)`, `p(instance,outvar)`).

## Output

- A file `<analysis>.*` containing the noise contributions and total output
  noise across frequency.
- If `writeop=1`, an additional `<analysis>.op.*` file with the operating
  point solution.

## Examples

**Basic noise analysis, 1 Hz to 10 MHz:**

```text
analysis n1 noise out="vout" in=Vin from=1 to=10M mode="dec" points=20
```

**Differential output with detailed contributions:**

```text
save full
analysis n1 noise out=["outp", "outn"] in=Vin from=10 to=1G mode="dec" points=10
```

**Saving a specific instance contribution:**

```text
save n(M1)
analysis n1 noise out="vout" in=Vin from=1 to=100M mode="dec" points=20
```
