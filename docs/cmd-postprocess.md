# Postprocessing

The `postprocess` command runs an external program after an analysis completes.
The most common use is to invoke a Python script that reads the output file and
performs plotting, measurement extraction, or result validation.

## Syntax

```text
postprocess("program", "arg1", "arg2", ...)
```

All arguments are expressions that must evaluate to strings. The first argument
is the program to execute; the remaining arguments are passed as command-line
arguments.

## Behavior

- VACASK evaluates each expression and checks that the result is a string.
- The program runs as a child process. VACASK waits for it to finish.
- The `PYTHON` circuit variable is available during evaluation and contains
  the path to the detected Python 3 interpreter.
- If the program exits with a nonzero status, VACASK reports an error.
- Postprocessing can be disabled at the simulator level; in that case the
  command is silently skipped.

## Examples

**Run a Python script:**

```text
postprocess(PYTHON, "postprocess.py")
```

**Pass arguments:**

```text
postprocess(PYTHON, "check.py", "--tolerance", "1e-3")
```

**Combine with embedded files:**

```text
control
  analysis op1 op
  postprocess(PYTHON, "analyze.py")
endc

embed "analyze.py" <<<PY
from rawfile import rawread
data = rawread("op1.raw").get()
for name in data.names:
    print(name, data[name])
>>>PY
```
