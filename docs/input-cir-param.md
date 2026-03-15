# Parameterization

The `parameters` keyword declares named constants and expressions that can be
referenced throughout the netlist. Parameters enable flexible, reusable circuit
descriptions.

## Syntax

```text
parameters name1=value1 name2=expr2 ...
```

Multiple parameters are separated by whitespace. Values can be numeric
literals, strings, or expressions.

## Scope

- **Top-level parameters** are visible everywhere in the netlist.
- **Subcircuit parameters** are visible inside the subcircuit and can be
  overridden when the subcircuit is instantiated.
- **Instance parameters** override model parameters for that instance only.

Parameter lookup follows this chain:

1. Instance context
2. Parent subcircuit
3. Top-level instance
4. Global parameters
5. Built-in constants (`pi`, `e`, `q`, `k`)

## Expressions

Parameter values can be arbitrary expressions combining variables, constants,
operators, and function calls:

```text
parameters width=1u length=180n area=width*length
```

Expressions are evaluated during circuit elaboration, so all referenced
variables must be defined by that point.

## Overriding

Subcircuit parameters define defaults that callers can override:

```text
subckt amp (in out)
  parameters gm=1m rl=10k
  ...
ends

x1 (a b) amp gm=2m          // rl stays at 10k
x2 (c d) amp gm=1m rl=20k   // both overridden
```

## Examples

```text
parameters vdd=1.8 vss=0 ibias=10u

v1 (vdd_node 0) vsource dc=vdd
i1 (vdd_node bias) isource dc=ibias
```

```text
parameters r_val=10k
parameters c_val=1/(2*pi*1e6*r_val)  // RC filter at 1 MHz

r1 (in out) r r=r_val
c1 (out 0) c c=c_val
```
