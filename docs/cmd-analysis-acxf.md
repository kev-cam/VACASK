# AC Small-Signal Transfer Function Analysis

The AC small-signal transfer function analysis (`acxf`) computes
frequency-dependent transfer functions from every independent source in the
circuit to a selected output. It also computes the input impedance and input
admittance seen at each source's terminals, all as functions of frequency.

## Syntax

```text
analysis name acxf [parameters]
```

## How it works

1. VACASK solves for the DC operating point $x_0$ where $f(x_0) = 0$.
2. It linearizes the circuit by computing the resistive Jacobian $J_r$ and
   the reactive Jacobian $J_c$ at $x_0$.
3. For each independent source and for each frequency $f$ it injects a unit
   excitation (magnitude 1, phase 0) and solves

$$\left(J_r + j\omega\, J_c\right) X = U_{\text{source}}$$

4. The results are used to compute:
   - **Transfer function** from the source to the designated output node
     (or node pair).
   - **Input impedance** ($Z_{\text{in}}$) and **input admittance**
     ($Y_{\text{in}}$) seen by each source.

## Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `out` | string or vector | `""` | Output node or node pair. Specify as a single node name or a two-element list for a differential measurement. |
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
| `default` | Save all transfer functions and input impedances/admittances. |
| `tf(source)` | Save the transfer function from the given source to the output. |
| `zin(source)` | Save the input impedance seen by the given source. |
| `yin(source)` | Save the input admittance seen by the given source. |

ACXF analysis also supports all operating point save directives (`v(node)`,
`i(instance)`, `p(instance,outvar)`).

## Output

- A file `<analysis>.*` containing the transfer functions, impedances, and
  admittances across frequency.
- If `writeop=1`, an additional `<analysis>.op.*` file with the operating
  point solution.

## Examples

**Single-ended output:**

```text
analysis xf1 acxf out="out_node" from=1 to=10G mode="dec" points=20
```

**Differential output:**

```text
analysis xf1 acxf out=["outp", "outn"] from=1 to=1G mode="dec" points=10
```

**Saving specific transfer functions:**

```text
save tf(Vin)
save zin(Vin)
analysis xf1 acxf out="vout" from=1 to=1G mode="dec" points=20
```
