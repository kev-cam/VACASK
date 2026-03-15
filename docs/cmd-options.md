# Simulator Options

Simulator options control tolerances, convergence algorithms, output format,
and other global settings. Options are set in the control block and apply to
all subsequent analyses.

## Syntax

```text
options name1=value1 name2=value2 ...
```

Options accept values or expressions. They are stored when the `options`
command executes and applied before each analysis runs.

## Subsections

1. [Setting Simulator Options](cmd-options-set.md)
2. [Options Causing Circuit Reelaboration](cmd-options-elaborate.md)
3. [List of Simulator Options](cmd-options-list.md)
