# Instances

An instance places a device into the circuit by connecting its terminals to
nodes and associating it with a model.

## Syntax

```text
name (node1 node2 ... nodeN) modelname
name (node1 node2 ... nodeN) modelname param1=value1 param2=value2 ...
```

- **name** — A unique identifier for the instance.
- The parenthesized list contains the nodes to which the device terminals
  connect, separated by spaces.
- **modelname** — The name of a previously declared model.
- Parameters override the values set on the model.

## Connection list

The number of nodes must match the number of terminals the device expects.
For example, a two-terminal resistor needs exactly two nodes:

```text
r1 (a b) r r=10k
```

A four-terminal MOSFET needs four nodes (drain, gate, source, bulk):

```text
m1 (d g s b) nch w=1u l=180n
```

## Instance parameters

Instance-level parameters take precedence over model-level parameters.
Parameters can be constants or expressions:

```text
parameters width=1u
m1 (d g s b) nch w=width l=180n
```

## Examples

```text
// Voltage source between nodes 1 and 0
v1 (1 0) vsource dc=1.8

// Resistor
r1 (1 2) r r=1k

// Capacitor
c1 (2 0) c c=10p

// MOSFET
m1 (out in vss vss) nch w=2u l=180n
```
