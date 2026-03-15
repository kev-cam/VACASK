# Analysis Statements

Inside the control block every simulation is set up with the `analysis`
keyword. Analyses can be wrapped in one or more `sweep` statements to form
parameter sweeps.

## Syntax

```text
analysis name type [param1=value1 param2=value2 ...]
```

- **name** — A unique identifier for the analysis. The name appears in output
  file names and can be used to reference results in postprocessing.
- **type** — The analysis type (see below).
- Parameters are optional and depend on the analysis type.

## Analysis types

| Type | Description | Documentation |
|------|-------------|---------------|
| `op` | Operating point | [cmd-analysis-op.md](cmd-analysis-op.md) |
| `dcinc` | DC incremental small-signal | [cmd-analysis-dcinc.md](cmd-analysis-dcinc.md) |
| `dcxf` | DC small-signal transfer function | [cmd-analysis-dcxf.md](cmd-analysis-dcxf.md) |
| `ac` | AC small-signal | [cmd-analysis-ac.md](cmd-analysis-ac.md) |
| `acxf` | AC small-signal transfer function | [cmd-analysis-acxf.md](cmd-analysis-acxf.md) |
| `noise` | Small-signal noise | [cmd-analysis-noise.md](cmd-analysis-noise.md) |
| `tran` | Transient | [cmd-analysis-tran.md](cmd-analysis-tran.md) |
| `hb` | Harmonic balance | [cmd-analysis-hb.md](cmd-analysis-hb.md) |

## Wrapping in sweeps

One or more `sweep` statements can precede an analysis to create nested
parameter sweeps. See [Sweeping](cmd-sweep.md).

```text
sweep vgs instance="M1" parameter="vgs" from=0 to=1.8 mode="lin" points=19
  analysis op1 op
```

## Examples

```text
control
  analysis op1 op
  analysis ac1 ac from=1 to=10G mode="dec" points=20
  analysis t1 tran step=1p stop=100n
endc
```
