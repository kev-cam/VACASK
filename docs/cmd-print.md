# Printing

The `print` command displays information about the circuit, variables,
options, or the result of expressions. It is primarily a debugging and
inspection tool.

## Syntax

**Evaluate and print expressions:**

```text
print expr1 expr2 ...
```

**Print circuit information:**

```text
print keyword ["name1" "name2" ...]
```

## Keywords

| Keyword | Description |
|---------|-------------|
| `device_files` | List all loaded device files. |
| `device_file` | Search device files matching a substring pattern. Patterns follow as string arguments. |
| `counts` | Print device complexity counts. |
| `devices` | Print all elaborated devices. |
| `models` | Print all elaborated models. |
| `variables` | Print all circuit variables. |
| `saves` | Print active save directives. |
| `options` | Print user-specified options (only those explicitly set). |
| `options_state` | Print the full current option state including defaults. |
| `hierarchy` | Print the circuit hierarchy tree. |
| `nodes` | Print all circuit nodes. |
| `unknowns` | Print the unknown variables in the system. |
| `tolerances` | Print tolerances assigned to unknowns. |
| `sparsity` | Print the system matrix sparsity pattern. |
| `instance` | Print details of named instances. Instance names follow as string arguments. |
| `model` | Print details of named models. Model names follow as string arguments. |
| `device` | Print details of named devices. Device names follow as string arguments. |
| `stats` | Print system statistics and timing. |
| `rpn` | Print the RPN (reverse Polish notation) representation of expressions. |

When called without a keyword, `print` evaluates its arguments as expressions
and writes the results to standard output. String values are printed without
quotes.

## Examples

```text
print "Temperature is " temp " degrees"
```

```text
print devices
print instance "M1" "M2"
print nodes
```

```text
print options
print hierarchy
```
