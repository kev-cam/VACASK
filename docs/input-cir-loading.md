# Loading Devices

The `load` directive makes device models from compiled Verilog-A modules
available for use in the netlist. It must appear at the top level, outside
of subcircuit definitions.

## Syntax

```text
load "filename"
load "filename" param1=value1 param2=value2 ...
```

The filename is a string that points to an OSDI-format shared library
(`.osdi` file). Optional parameters are passed to the device loader.

## Device search

VACASK searches for the file in the following order:

1. The directory containing the input file.
2. The current working directory.
3. The device library path (configurable).

## Built-in devices

VACASK provides several built-in devices that do not need to be loaded:

- `vsource` — independent voltage source
- `isource` — independent current source
- `vccs` — voltage-controlled current source
- `vcvs` — voltage-controlled voltage source
- `cccs` — current-controlled current source
- `ccvs` — current-controlled voltage source
- `mutual` — mutual inductance (inductive coupling)

See [Builtin Devices](input-cir-builtin.md) for full parameter documentation.

## OSDI Verilog-A devices

Passive components (resistor, capacitor, inductor) and transistor models
(BSIM3, BSIM4, BSIMBulk, VBIC, PSP, etc.) are distributed as compiled OSDI
files. Load them before using the corresponding `model` statements:

```text
load "resistor.osdi"
load "bsim4v8.osdi"
```

## Examples

```text
load "resistor.osdi"
load "capacitor.osdi"
load "bsim4v8.osdi"

model r resistor
model c capacitor
model nch bsim4v8 type=1
```
