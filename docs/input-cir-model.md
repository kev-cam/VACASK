# Models

A model statement associates a name with a device type and optionally sets
device parameters. Instances reference models to inherit their parameter
values.

## Syntax

```text
model name devicetype
model name devicetype param1=value1 param2=value2 ...
```

- **name** — A unique identifier for the model.
- **devicetype** — The name of a loaded device or a built-in device.
- Parameters set on the model apply to all instances that reference it unless
  overridden at the instance level.

## Parameter expressions

Model parameters can be constants or expressions:

```text
parameters vdd=1.8
model nch bsim4v8 type=1 tnom=27 vth0=0.4+0.01*vdd
```

Expressions may reference circuit variables, the `$temp` built-in, and
previously defined parameters.

## Multiple models

Several models can reference the same device type with different parameter
settings:

```text
load "bsim4v8.osdi"
model nch bsim4v8 type=1 vth0=0.4
model pch bsim4v8 type=-1 vth0=-0.4
```

## Examples

```text
model r resistor
model c capacitor
model v vsource
model i isource
```

```text
load "bsim3v3.osdi"
model nch_3v3 bsim3v3 type=1 tnom=27 version=3.3
```
