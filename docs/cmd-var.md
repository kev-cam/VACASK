# Circuit Variables

Circuit variables store named values that can be referenced in expressions
throughout the control block. They are useful for parameterizing analyses
and for sharing computed values between commands.

## Syntax

```text
var name1=value1 name2=value2 ...
```

Variable names follow identifier rules. Values can be numeric literals,
strings, or expressions that reference other variables and constants.

All expressions in a `var` command are evaluated before any assignment takes
place, so the order of variables in the same `var` statement does not matter.

## Scope

Variables set with `var` are visible to all subsequent commands in the control
block. They can also be referenced inside parameterized expressions if the
circuit is re-elaborated after the variable is set.

The built-in variable `PYTHON` is automatically set to the path of the Python 3
interpreter (if found).

## Clearing variables

```text
clear variables
```

Removes all user-defined variables and resets `PYTHON` to the detected Python
path.

## Viewing variables

```text
print variables
```

## Examples

```text
var R=10k C=100p
var tau=R*C
```

```text
var vdd=1.8 vss=0
```
