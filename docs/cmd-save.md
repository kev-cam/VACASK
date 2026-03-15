# Saving Results

The `save` directive controls which quantities are written to the output file.
Save directives apply to all subsequent analyses until they are cleared with
`clear saves`.

## Syntax

```text
save type
save type(argument)
save type(argument1, argument2)
```

Multiple save directives can appear on separate lines.

## Common save directives

These directives are supported by most analyses:

| Directive | Description |
|-----------|-------------|
| `default` | Save all node voltages and branch currents. This is the behavior when no save directive is given. |
| `full` | Save all node voltages (analysis-dependent; may include collapsed nodes or detailed breakdowns). |
| `v(node)` | Save the voltage at the specified node. |
| `i(instance)` | Save the current through the specified instance (typically a voltage source or inductor). |
| `p(instance,outvar)` | Save a device output variable (operating point variable) from the given instance. |

## Analysis-specific directives

Some analyses define additional save types:

**AC and DC incremental analyses** (`ac`, `dcinc`):

| Directive | Description |
|-----------|-------------|
| `dv(node)` | Save the incremental (complex) voltage at the node. |
| `di(instance)` | Save the incremental (complex) current through the instance. |

**Transfer function analyses** (`dcxf`, `acxf`):

| Directive | Description |
|-----------|-------------|
| `tf(source)` | Save the transfer function from the source to the output. |
| `zin(source)` | Save the input impedance at the source. |
| `yin(source)` | Save the input admittance at the source. |

**Noise analysis** (`noise`):

| Directive | Description |
|-----------|-------------|
| `n(instance)` | Save the total noise contribution from the instance. |
| `nc(instance)` | Save the detailed per-source noise contributions from the instance. |

## Inheritance

Analyses that depend on an operating point core (AC, ACXF, noise, DC
incremental, DC transfer function) also accept all operating point save
directives (`v`, `i`, `p`).

## Clearing saves

```text
clear saves
```

Removes all previously specified save directives. Subsequent analyses revert
to their default output behavior.

## Examples

```text
save v(out)
save i(Vdd)
save p(M1, gm)
analysis op1 op
```

```text
save dv(out)
save di(Vin)
analysis ac1 ac from=1 to=1G mode="dec" points=20
```

```text
save n(M1)
save n(M2)
analysis n1 noise out="vout" in=Vin from=1 to=1G mode="dec" points=20
```
