# Modifying Parameters

The `alter` command changes instance or model parameters during simulation
without reparsing the netlist. It is useful for running multiple analyses with
different parameter values.

## Syntax

```text
alter instance("name1", "name2", ...) param1=value1 param2=value2 ...
alter model("name1", "name2", ...) param1=value1 param2=value2 ...
```

- **`instance`** or **`model`** — Selects the target type.
- The parenthesized list contains one or more entity names (as strings).
- Keyword arguments specify the parameters to change and their new values.
  Values can be expressions.

## Behavior

- If the circuit has not been elaborated yet, VACASK performs a default
  elaboration first.
- After parameters are modified, VACASK marks the affected devices for
  re-setup. The changes take effect at the next analysis.
- Altering a parameter that affects node collapsing or device topology may
  trigger a re-mapping of the circuit.

## Examples

**Change an instance parameter:**

```text
alter instance("R1") r=20k
```

**Change multiple instances at once:**

```text
alter instance("M1", "M2") w=2u l=180n
```

**Change a model parameter:**

```text
alter model("nch") vth0=0.4
```
