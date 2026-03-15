# Conditional Netlist Blocks

Conditional blocks select which parts of the netlist are included based on
parameter values evaluated at elaboration time. They are commonly used for
automated model binning and design variants.

## Syntax

```text
@if expression
  statements
@elseif expression
  statements
@else
  statements
@end
```

- The condition expressions are evaluated during circuit elaboration.
- A nonzero result is treated as true.
- `@elseif` and `@else` branches are optional.
- Conditional blocks can be nested.

## Allowed content

The body of each branch can contain any netlist statement that is valid in the
current context: instances, models, parameter declarations, and nested
conditional blocks.

## Use cases

**Model binning** — Select a model variant based on geometry parameters:

```text
subckt nmos (d g s b)
  parameters w=1u l=180n

  @if l < 250n
    m1 (d g s b) nch_short w=w l=l
  @elseif l < 1u
    m1 (d g s b) nch_mid w=w l=l
  @else
    m1 (d g s b) nch_long w=w l=l
  @end
ends
```

**Optional features:**

```text
parameters add_esd=0

@if add_esd
  d_esd (pad 0) esd_diode
@end
```

## Examples

```text
parameters use_ideal=1

@if use_ideal
  model r resistor
@else
  load "resistor_nonlinear.osdi"
  model r resistor_nonlinear
@end
```
