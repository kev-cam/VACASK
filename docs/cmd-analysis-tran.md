# Transient Analysis

The transient analysis simulates the time-domain behavior of a circuit. It
solves the nonlinear differential-algebraic system

$$f(x(t)) + \frac{d}{dt}\,q(x(t)) = 0$$

from a set of initial conditions to a specified stop time using implicit
multistep integration methods with adaptive timestep control.

## Syntax

```text
analysis name tran [parameters]
```

## How it works

1. **Initial conditions.** Two modes are available:
   - `icmode="op"` (default) — Performs an operating point analysis at $t=0$
     with initial conditions forced onto the circuit. The operating point
     solution provides consistent initial values for all state variables.
   - `icmode="uic"` — Uses the given initial conditions directly without
     computing an operating point. This is equivalent to the classic SPICE3
     UIC mode.
2. **Time integration.** VACASK advances the solution in time using a
   multistep integration method (selected via the `tran_method` simulator
   option). At each timestep a Newton-Raphson iteration solves the nonlinear
   algebraic system that results from the time discretization.
3. **Adaptive timestep.** The timestep is controlled by the local truncation
   error (LTE). The step is reduced when the LTE exceeds the tolerance and
   increased when the error is comfortably below it. Source breakpoints
   (discontinuities in time-varying stimuli) are respected automatically.

## Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `step` | real | `0` | Initial timestep. |
| `stop` | real | `0` | Simulation stop time. |
| `start` | real | `0` | Time at which results start being recorded. Simulation still runs from $t=0$. |
| `maxstep` | real | `0` | Maximum allowed timestep (0 = no limit). |
| `icmode` | identifier | `"op"` | Initial condition mode: `"op"` or `"uic"`. |
| `ic` | string or list | `""` | Initial conditions. Can be a stored solution slot name or a list with alternating node names and values. |
| `nodeset` | string or list | `""` | Initial guess for the operating point (used when `icmode="op"`). |
| `store` | string | `""` | Save the computed operating point or final state under the given name. |
| `write` | boolean | `1` | Write analysis results to a file. |

Initial conditions follow the same format as nodesets (see the
[operating point analysis](cmd-analysis-op.md) documentation).

## Save directives

| Directive | Description |
|-----------|-------------|
| `default` | Save all node voltages and branch currents (default). |
| `full` | Save all node voltages. |
| `v(node)` | Save the voltage at the specified node. |
| `i(instance)` | Save the current through the specified instance. |
| `p(instance,outvar)` | Save the specified output variable from the given instance. |

## Output

- A file `<analysis>.*` containing the time-domain waveforms.

## Examples

**Basic transient, 10 ns stop time:**

```text
analysis t1 tran step=10p stop=10n
```

**With initial conditions from a stored operating point:**

```text
analysis op1 op store="myop" write=0
analysis t1 tran step=1p stop=100n ic="myop"
```

**Record only the steady-state portion:**

```text
analysis t1 tran step=1n stop=1m start=900u
```

**UIC mode with explicit initial conditions:**

```text
analysis t1 tran step=10p stop=100n icmode="uic" ic=["node1"; 1.0; "node2"; 0.5]
```

**Limiting the maximum timestep:**

```text
analysis t1 tran step=1p stop=10n maxstep=100p
```

## Integration methods

The integration method is selected with the `tran_method` simulator option.
The maximum order is controlled by `tran_maxord`.

| Method | Option value | Description |
|--------|-------------|-------------|
| Trapezoidal | `"trap"` | Second-order implicit method (default). |
| Adams-Moulton | `"am"` | Implicit multistep predictor-corrector. |
| BDF (Gear) | `"gear"` | Backward differentiation formula; good for stiff circuits. |

The `tran_xmu` option (default 0.5) controls the blending coefficient for the
trapezoidal method. A value of 0.5 gives pure trapezoidal; values closer to 1
shift toward backward Euler.
