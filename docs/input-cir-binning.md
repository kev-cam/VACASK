# Automated Binning

Automated binning uses [conditional netlist blocks](input-cir-cond.md) to
select the appropriate device model based on instance geometry or other
parameters. This is a common technique in foundry PDKs where different model
parameter sets cover different device geometries.

## How it works

A subcircuit wraps the device and uses `@if`/`@elseif`/`@else` to choose the
model at elaboration time. The conditions test instance parameters (typically
width and length) against bin boundaries.

## Example

```text
subckt nmos_binned (d g s b)
  parameters w=1u l=180n

  @if w < 500n && l < 250n
    m1 (d g s b) nch_bin1 w=w l=l
  @elseif w < 500n
    m1 (d g s b) nch_bin2 w=w l=l
  @elseif l < 250n
    m1 (d g s b) nch_bin3 w=w l=l
  @else
    m1 (d g s b) nch_bin4 w=w l=l
  @end
ends
```

Each `nch_binN` model is declared separately with bin-specific parameters.
When an instance of `nmos_binned` is elaborated, the parameters `w` and `l`
determine which branch is taken and therefore which model is used.

## Bin parameter sources

Bin boundaries and model parameters typically come from foundry-supplied
include files. Use the `include` directive with section selection to pull in
the relevant data:

```text
include "models.lib" section=tt
```

See [Including a File](input-include.md) for details on library sections.
