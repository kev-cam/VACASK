# Circuit Elaboration

The `elaborate` command explicitly triggers circuit elaboration. Normally
VACASK elaborates the circuit automatically before the first analysis, but
`elaborate` is useful when you need to control the process — for example after
altering parameters or when working with multiple subcircuit definitions.

## Syntax

**Elaborate specific subcircuit definitions:**

```text
elaborate circuit("def1", "def2", ...) topdef="name" topinst="name"
```

**Re-elaborate only what changed:**

```text
elaborate changes
```

## Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| first argument(s) | strings | — | Names of subcircuit definitions to elaborate. If empty, elaborates all definitions in the netlist. |
| `topdef` | string | `"__topdef__"` | Name assigned to the top-level subcircuit definition. |
| `topinst` | string | `"__topinst__"` | Name assigned to the top-level instance. |

## Elaboration process

Elaboration transforms the parsed netlist into a fully resolved circuit:

1. Create built-in devices (voltage and current sources, controlled sources).
2. Load OSDI devices specified by `load` directives.
3. Instantiate top-level instances and expand subcircuit hierarchy.
4. Order nodes (ground first, then alphabetically).
5. Run device and model setup (detect collapsed nodes, set defaults).
6. Map unknowns (assign matrix indices accounting for collapsed nodes).
7. Build the sparsity pattern and allocate the state vector.

## Examples

**Default elaboration:**

```text
elaborate circuit()
```

**Elaborate and re-elaborate after alterations:**

```text
elaborate circuit()
alter instance("R1") r=20k
elaborate changes
analysis op1 op
```
