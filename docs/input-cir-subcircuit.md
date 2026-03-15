# Subcircuits

Subcircuits define reusable blocks of circuit that can be instantiated multiple
times with different connections and parameters. They enable hierarchical
circuit design.

## Definition

```text
subckt name (terminal1 terminal2 ... terminalN)
  parameters param1=default1 param2=default2 ...

  model ...
  instance ...
  ...
ends
```

- **name** — A unique identifier for the subcircuit definition.
- The terminal list declares the ports visible to the parent circuit.
- Parameters with defaults can be overridden when the subcircuit is
  instantiated.

## Instantiation

A subcircuit is instantiated like any other device. The model name matches the
subcircuit definition name:

```text
x1 (a b) mysubckt param1=value1
```

The number of nodes in the connection list must match the number of terminals
in the subcircuit definition.

## Hierarchy

Subcircuits can contain other subcircuit definitions and instances, enabling
arbitrary nesting. Node names inside a subcircuit are local to that level —
only terminal nodes and global nodes cross hierarchy boundaries.

```text
subckt inverter (in out)
  parameters wp=2u wn=1u l=180n
  mp (out in vdd vdd) pch w=wp l=l
  mn (out in vss vss) nch w=wn l=l
ends

subckt buffer (in out)
  x1 (in mid) inverter
  x2 (mid out) inverter
ends

x_buf (a b) buffer
```

## Conditional content

Subcircuit bodies can include conditional blocks to select different
topologies based on parameter values. See
[Conditional Netlist Blocks](input-cir-cond.md).

## Examples

**Simple resistive divider:**

```text
subckt divider (in out gnd)
  parameters r1=10k r2=10k
  rtop (in out) r r=r1
  rbot (out gnd) r r=r2
ends

x1 (vin vout 0) divider r1=20k r2=10k
```
