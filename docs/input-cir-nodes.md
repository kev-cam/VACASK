# Nodes

Nodes represent electrical connection points in the circuit. Every terminal of
every instance connects to a node, and VACASK solves for the voltage at each
node relative to the ground reference.

## Ground nodes

At least one ground node must be declared. The ground node serves as the
voltage reference (0 V). The first name in the list is the primary name;
additional names are aliases.

```text
ground 0
ground gnd 0
```

## Global nodes

Global nodes are accessible across the entire hierarchy — including inside
subcircuit instances — without being passed through terminal lists.

```text
global vdd vss
```

## Node names

Node names follow the same rules as identifiers: they start with a letter,
underscore, or dollar sign and may contain letters, digits, underscores, and
dollar signs. Integer names (e.g. `0`, `1`) are also allowed in connection
lists.

Quoted node names (single quotes) permit additional characters:

```text
ground '0'
v1 ('3.3V' 0) vsource dc=3.3
```

## Internal nodes

Devices may create internal nodes that are not directly accessible in the
netlist. For example, voltage sources create an internal flow node for the
branch current. The naming convention for internal nodes is
`instance:flow(br)`.

## Hierarchy

Inside subcircuit instances, node names are scoped to that level of the
hierarchy. Only terminals listed in the `subckt` definition and global nodes
cross hierarchy boundaries. See [Subcircuits](input-cir-subcircuit.md).
