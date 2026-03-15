# Options Causing Circuit Reelaboration

Certain simulator options influence how the circuit is elaborated. Changing
them forces VACASK to repeat parts of the elaboration process before the next
analysis can run.

## Options affecting mapping

The following options are passed to device models through the OSDI interface.
Changing any of them triggers a re-mapping of the circuit (node collapsing,
sparsity pattern rebuild, and device re-setup):

- `temp` — operating temperature
- `tnom` — device model reference temperature
- `gmin` — minimum conductance shunted across nodes
- `minr` — minimum resistance
- `scale` — geometry scaling factor
- `reltol` — relative tolerance
- `vntol` — voltage tolerance
- `abstol` — absolute current tolerance
- `chgtol` — charge tolerance
- `fluxtol` — flux tolerance

## Options affecting parameterization

These options are available inside parameterized expressions (e.g. through
`$temp`). Changing them causes VACASK to re-evaluate parameterized expressions
and re-run device setup:

- `temp`
- `scale`

## Practical implications

When you sweep `temp` or `scale` in a `sweep` statement the circuit is
automatically re-elaborated at each sweep point. This is more expensive than
sweeping a simple instance parameter but necessary because the option change
can affect the entire circuit.

Options that do not appear in the above lists (e.g. `rawfile`, `reltol` when
it does not change the value, solver iteration limits) do not trigger
reelaboration.
