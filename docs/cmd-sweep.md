# Sweeping

The `sweep` statement wraps an analysis and varies one or more parameters
across a set of values. Each sweep re-runs the enclosed analysis (or nested
sweep) for every sweep point.

## Syntax

```text
sweep name [parameters]
  analysis ...
```

Sweeps can be nested:

```text
sweep outer [parameters]
  sweep inner [parameters]
    analysis ...
```

Every sweep must have a unique **name**. The name appears as a vector in the
output file holding the swept values.

## Sweep targets

A sweep modifies one of the following targets. Exactly one must be specified.

| Parameters | Description |
|------------|-------------|
| `instance="inst" parameter="param"` | Sweep an instance parameter. |
| `model="mod" parameter="param"` | Sweep a model parameter. |
| `parameter="param"` | Sweep a top-level instance parameter. |
| `option="optname"` | Sweep a simulator option. |
| `variable="varname"` | Sweep a circuit variable. |

## Sweep modes

How values are generated is controlled by one of the following parameter
combinations:

| Mode | Required Parameters | Description |
|------|---------------------|-------------|
| Stepped | `from`, `to`, `step` | Sweep from `from` to `to` with fixed increment `step`. |
| Linear | `from`, `to`, `points`, `mode="lin"` | Linear sweep with the given number of points. |
| Decade | `from`, `to`, `points`, `mode="dec"` | Logarithmic sweep, `points` per decade. |
| Octave | `from`, `to`, `points`, `mode="oct"` | Logarithmic sweep, `points` per octave. |
| Values | `values=[...]` | Sweep over an explicit list of values. |

## Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `name` | identifier | — | Sweep name (required, positional). |
| `instance` | identifier | — | Instance whose parameter is swept. |
| `model` | identifier | — | Model whose parameter is swept. |
| `parameter` | identifier | — | Parameter name to sweep. |
| `option` | identifier | — | Simulator option to sweep. |
| `variable` | identifier | — | Circuit variable to sweep. |
| `from` | real | `0` | Starting value. |
| `to` | real | `0` | End value. |
| `step` | real | `0` | Step size (stepped sweep). |
| `mode` | identifier | — | `"lin"`, `"dec"`, or `"oct"`. |
| `points` | integer | `0` | Number of points (lin/dec/oct). |
| `values` | vector | — | Explicit list of values. |
| `continuation` | boolean | `1` | Enable continuation mode. The previous sweep point's solution is used as initial guess for the next. Disable with `0`. |

## Examples

**1-D DC sweep:**

```text
sweep vds instance="Vdd" parameter="dc" from=0 to=1.8 mode="lin" points=100
  analysis dc1 op
```

**2-D DC sweep:**

```text
sweep vgs instance="Vgg" parameter="dc" values=[0, 0.4, 0.8, 1.2, 1.8]
  sweep vds instance="Vdd" parameter="dc" from=0 to=1.8 mode="lin" points=100
    analysis dc1 op
```

**Sweeping a simulator option:**

```text
sweep temp option="temp" values=[-40, 27, 125]
  analysis op1 op
```

**Sweeping a circuit variable:**

```text
sweep width variable="W" from=1u to=10u mode="lin" points=10
  analysis op1 op
```
