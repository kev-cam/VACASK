# Setting Simulator Options

Options are set inside the control block with the `options` command and persist
until explicitly cleared.

## Syntax

```text
options name1=value1 name2=value2 ...
```

Multiple options can be set in a single statement. Values can be numeric
literals, strings, or expressions that reference circuit variables.

## When options take effect

Options are stored when the `options` command executes. They are applied:

- Before each analysis runs.
- At elaboration time (when relevant options change).
- Before `print` and `alter` commands.

This means you can set options at the top of the control block and they apply
to all subsequent analyses.

## Clearing options

```text
clear options
```

Reverts all options to their built-in defaults.

## Viewing options

```text
print options
```

Shows the currently set user options (only those explicitly set).

```text
print options_state
```

Shows the full current option state including defaults.

## Examples

```text
options rawfile="ascii" reltol=1e-4
analysis op1 op
```

```text
options temp=85 tnom=25
analysis op1 op
clear options
options temp=-40
analysis op2 op
```
