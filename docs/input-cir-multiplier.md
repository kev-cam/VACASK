# Parallel Devices (`$mfactor`)

The `$mfactor` parameter scales an instance to represent multiple identical
devices connected in parallel. Instead of creating separate instances, a
single instance with `$mfactor=N` models $N$ parallel copies.

## Syntax

```text
name (nodes) model $mfactor=N
```

## Behavior

When `$mfactor` is set on an instance:

- **Currents and charges** are multiplied by `$mfactor`.
- **Conductances** are multiplied by `$mfactor`.
- **Voltages** are unchanged (parallel devices share the same terminals).

The default value is 1 (one device). Fractional values are permitted.

## Built-in support

All built-in devices (voltage sources, current sources, and controlled
sources) support `$mfactor`. OSDI devices compiled from Verilog-A also
support it if the model uses `$mfactor` in its equations.

## Examples

**Four parallel MOSFETs:**

```text
m1 (d g s b) nch w=1u l=180n $mfactor=4
```

This is equivalent to four instances of the same transistor connected in
parallel.

**Scaling a current source:**

```text
i1 (vdd 0) isource dc=1m $mfactor=10
```

The effective DC current is 10 mA.
