# Circuit Elaboration

Elaboration is the process that transforms a parsed netlist description into a
fully resolved circuit ready for simulation. VACASK performs elaboration
automatically before the first analysis, but it can also be triggered
explicitly with the `elaborate` command.

## Elaboration steps

1. **Create built-in devices.** Register the built-in voltage/current sources,
   controlled sources, and mutual inductance device.
2. **Load OSDI devices.** Execute all `load` directives to make Verilog-A
   device models available.
3. **Build definitions.** Create the default top-level subcircuit definition
   and any nested subcircuit definitions.
4. **Instantiate.** Expand the hierarchy by instantiating all instances at
   every level. Evaluate conditional blocks (`@if`) and parameterized
   expressions.
5. **Build entity lists.** Organize models under their parent devices and
   instances under their parent models.
6. **Order nodes.** Ground nodes are placed first, then remaining nodes are
   sorted alphabetically. This determines the order of unknowns in the
   system matrix.
7. **Device and model setup.** Call the setup routines of every model and
   instance:
   - Evaluate default parameter values.
   - Detect node collapsing (e.g. a zero-value resistor merges its two nodes).
   - Detect sparsity pattern changes.
8. **Map unknowns.** Assign matrix indices, accounting for collapsed nodes.
9. **Build sparsity pattern.** Allocate the system matrix and state vector
   based on the device contributions discovered during setup.

## When re-elaboration happens

VACASK tracks what changed and re-elaborates only the affected parts:

- **Parameter changes** (via `alter` or `var`) may require re-running device
  setup and re-mapping unknowns.
- **Option changes** (e.g. `temp`, `scale`) may trigger re-parameterization
  and re-mapping. See
  [Options Causing Circuit Reelaboration](cmd-options-elaborate.md).
- **Continuation mode** in sweeps skips re-elaboration when only a single
  parameter value changes and the topology is unaffected.

## See also

- [Circuit Elaboration command](cmd-elaborate.md)
