# Error Handling

The `abort` command controls how VACASK responds to errors in control block
commands. By default VACASK aborts on most errors except analysis failures.

## Syntax

```text
abort always
abort never
abort on cmd1 cmd2 ...
abort except cmd1 cmd2 ...
```

## Modes

| Form | Behavior |
|------|----------|
| `abort always` | Abort on any command error. |
| `abort never` | Never abort; continue past all errors. |
| `abort on cmd1 cmd2 ...` | Abort only when one of the listed commands fails. |
| `abort except cmd1 cmd2 ...` | Abort on all errors **except** those from the listed commands. |

The default mode is `abort except analysis`, which means the simulator
continues past analysis failures but aborts on other command errors.

## Valid command names

The following names can appear after `on` or `except`:

`analysis`, `abort`, `clear`, `save`, `var`, `options`, `alter`,
`elaborate`, `print`, `postprocess`.

## Examples

**Abort on all errors:**

```text
abort always
```

**Continue past everything:**

```text
abort never
```

**Abort only on elaboration or analysis failures:**

```text
abort on analysis elaborate
```

**Abort on everything except analysis failures (default):**

```text
abort except analysis
```
