# Behavioral Sources for VACASK: Converting RPN Expressions to Verilog-A

This document explains a planned VACASK feature from the ground up. It assumes
**no electronics background** — if you can read a little Python and don't mind
learning a few new words along the way, you can follow all of it.

The short version: today, if you want a component in VACASK whose behavior is
"do some custom math on a couple of signals," you have to hand-write a
Verilog-A file, a small program in its own language. For example, to make a
node's voltage track the product of two other signals minus a constant, you'd
write a whole module (ports, discipline declarations, parameter declarations,
an `analog begin ... end` block — a dozen-plus lines) just to express one line
of math. Most SPICE-family simulators instead let you type that math directly
into the circuit description (a "B source"):

```spice
B1 out 0 V = V(in1) * V(in2) - 0.5
```

One line, no separate file, no compilation step. We're building a tool that
takes that kind of typed-in math expression and automatically writes the
Verilog-A file for you — so you still get a real Verilog-A model underneath
(with everything VACASK needs from one), but you never have to write it by
hand. This document covers what that means, why it's needed, and exactly how
the tool is built, file by file.

If you haven't read it yet, `docs/netlist-converter-contribution-guide.md` in
this same folder covers the fundamentals of circuits, netlists, and VACASK more
slowly. This document assumes you either read that one or already know terms
like *node*, *netlist*, *instance*, *model*, *Verilog-A*, and *OSDI*. A quick
refresher of the essentials is in section 1 anyway.

---

## 1. Sixty-second refresher

- A **circuit** is a network of parts (resistors, transistors, sources, ...)
  connected at **nodes** (connection points). Ground, the reference point, is
  usually node `0`. Example: a resistor between node `out` and node `0` has
  one terminal at `out`'s voltage and one at ground.
- A **netlist** is the text file describing the circuit: what parts exist,
  which nodes they connect to, and what analyses to run. Example line: `r1 out
  0 1k` means "a resistor named `r1`, connected between nodes `out` and `0`,
  with resistance 1000 ohms."
- A **device model** is the math that says how a part behaves — e.g. "current
  through a resistor equals voltage across it divided by resistance" (Ohm's
  law: `I = V / R`).
- **Verilog-A** is a small programming language for writing device models.
  VACASK does not hand-write device physics into its own C++ source code the
  way old simulators do; instead, model authors write Verilog-A, and a
  separate compiler called **OpenVAF** turns that Verilog-A file into a
  compiled binary file with the extension `.osdi`. VACASK loads `.osdi` files
  at runtime and uses them as devices. Example: `devices/resistor.va` is the
  Verilog-A source for VACASK's built-in resistor; OpenVAF compiles it into
  `resistor.osdi`.
- **OSDI** (Open Simulation Device Interface) is just the technical contract
  that lets VACASK and an `.osdi` file talk to each other — function calls,
  data layout, that sort of thing. You don't need its details for this
  document.

That's enough background. Now, the actual feature.

---

## 2. The problem: "I just want to type a formula"

### 2.1 What a "B source" is

Classic SPICE-family simulators (Ngspice, HSPICE, Spectre, ...) let you define
a component whose value is *any formula* you write, instead of picking from a
fixed list of resistor/capacitor/diode/etc. This is usually called a
**behavioral source** or, because its instance names traditionally start with
the letter B, a **B source**. Example, in Ngspice-style syntax:

```spice
B1 out 0 V = V(in1) * V(in2) - 0.5
```

Reading this left to right:
- `B1` is the component's name.
- `out` and `0` are the two nodes it connects to.
- `V =` means "this component forces a *voltage* between `out` and `0`" (the
  alternative is `I =`, meaning it forces a *current*).
- `V(in1) * V(in2) - 0.5` is the formula. `V(in1)` means "the voltage at node
  `in1`, whatever it currently is." So this component's output tracks the
  product of two other signals, minus a constant — a simple analog multiplier
  with an offset. If, at some instant, `V(in1) = 2V` and `V(in2) = 3V`, then
  `V(out) = 2*3 - 0.5 = 5.5V` at that instant, and it keeps recomputing as
  `in1`/`in2` change.

Other things people commonly do with a B source, all equally awkward to write
in Verilog-A by hand today:
- A comparator: `B2 out 0 V = V(in) > 1.5 ? 5 : 0`
- A current source driven by a formula: `B3 out 0 I = 1m * sin(2*3.14159*1000*time)`
- Glue logic combining two control signals: `B4 ctrl 0 V = V(a) & V(b)`

This is extremely useful in practice: modeling a control-system block, a
signal-processing operation, a comparator, a lookup-table-driven source, or
just glue logic between other parts of a larger simulation — all without
writing a full device model by hand.

### 2.2 Why VACASK doesn't have this yet

VACASK has no B-source-equivalent built in. If you want this kind of
"arbitrary formula" component today, your only option is to write a Verilog-A
file by hand and compile it with OpenVAF. Concretely, getting that one-line
multiplier-with-offset example working today means writing something like
this yourself (compare `devices/resistor.va` or `devices/opamp.va` for what a
real one looks like):

```verilog
`include "constants.vams"
`include "disciplines.vams"

module bsrc1(p, n, in1, in2);
    inout p, n, in1, in2;
    electrical p, n, in1, in2;

    parameter real offset = 0.5;

    analog begin
        V(p,n) <+ V(in1)*V(in2) - offset;
    end
endmodule
```

...then running OpenVAF on it, then loading the result. That works, but
Verilog-A has its own syntax and rules, and most people who just want to type
`V(in1)*V(in2) - offset` shouldn't have to learn a whole modeling language to
do it.

VACASK's maintainer (Arpad) flagged this as a good project: **write a tool
that takes a formula and automatically produces the Verilog-A file above**, so
users get B-source convenience without VACASK's core simulator needing to
change at all.

---

## 3. Why we can't just evaluate the formula directly (the "why" behind the whole design)

This is the part that isn't obvious, and it's the reason the solution is "write
a converter to Verilog-A" instead of the much simpler-sounding "just add a
formula-evaluating device directly in VACASK's C++ code."

### 3.1 VACASK already has a formula language — just not for this

VACASK already contains a small expression language, used today for things
like parameter values, sweeps, and conditionals in a netlist. Internally it's
called **RPN** (Reverse Polish Notation — a way of representing a formula as a
flat sequence of operations instead of a nested expression tree; the "RPN"
name refers to how it's stored internally, not to how you type it — you type
ordinary-looking math like `a + b * c`).

You can already write things like this in a `.sim` file today:
```
r = 1k + 2*vsupply
```
and VACASK's built-in evaluator (`lib/rpneval.cpp`, `lib/rpnbuiltin.cpp`) computes
the number — e.g. if `vsupply = 5`, then `r` becomes `1000 + 2*5 = 1010`. So...
why not let people write `V(in1)*V(in2)` the same way and just evaluate it
every timestep?

### 3.2 The missing ingredient: derivatives

Circuit simulation doesn't just need the *value* of each device's
current/voltage — it needs to know how that value *changes* as every node
voltage in the circuit changes, a little. This sensitivity information is
called a **derivative**, and the full table of "how much does every device's
output change for a small nudge to every node voltage" is called the
**Jacobian**.

Concretely, for our running example `V(out) = V(in1)*V(in2) - offset`, the
solver doesn't just need "what is `V(out)` right now" — it needs "if I nudge
`V(in1)` a little, how much does `V(out)` move," which here is `∂V(out)/∂V(in1)
= V(in2)`, and likewise `∂V(out)/∂V(in2) = V(in1)`. At the operating point
`V(in1)=2, V(in2)=3`: `V(out) = 5.5`, and its derivatives are `3` (with respect
to `in1`) and `2` (with respect to `in2`). VACASK's nonlinear solver
(`include/nrsolver.h`) uses exactly these numbers, every single iteration, to
decide how to update its guess of the circuit's node voltages. This is the
standard Newton-Raphson method used by every SPICE-family simulator. Without
correct derivatives, the solver simply does not converge to a correct answer
for anything nonlinear — and `V(in1)*V(in2)` is already nonlinear (it's a
product of two unknowns), so even this simple example needs them.

VACASK's RPN evaluator (the thing that runs `r = 1k + 2*vsupply`-style
expressions) only computes the plain value — for the example above it would
happily hand back `5.5`, but has no mechanism to also spit out `3` and `2`.
Building one — a full symbolic or automatic differentiation system bolted onto
the RPN engine — would be a much larger and riskier project than it sounds,
and it would duplicate machinery that already exists elsewhere in the
toolchain.

### 3.3 Verilog-A already solves this — for free

Here's the key fact that makes the whole feature tractable: **OpenVAF, the
compiler VACASK already uses for every device model, automatically computes
derivatives for any Verilog-A code you give it.** Hand OpenVAF the single line

```verilog
V(p,n) <+ V(in1)*V(in2) - offset;
```

and the compiled `.osdi` file it produces already contains code that computes
not just `5.5` but also `∂/∂V(in1) = 3` and `∂/∂V(in2) = 2` at that operating
point — without anyone having typed those derivatives anywhere. That's the
entire reason Verilog-A models work in VACASK's Newton-Raphson solver at all —
every `.osdi` file already carries the Jacobian-computing code, generated
automatically by OpenVAF from the original `.va` source, without the model's
author having to write a single derivative by hand.

### 3.4 The design conclusion

So instead of teaching VACASK's RPN engine how to differentiate formulas
(hard, risky, duplicative), we do this instead:

1. Take the user's formula, written in VACASK's familiar expression syntax
   (e.g. `v(in1)*v(in2) - offset`).
2. Translate it, automatically, into a tiny Verilog-A file that just
   contributes that formula's value to a voltage or current (the `bsrc1.va`
   file from section 2.2).
3. Compile that file with the OpenVAF compiler VACASK already uses — this is
   the step that produces working derivatives, for free, because it's
   OpenVAF's whole job.
4. Load the resulting `.osdi` file into VACASK exactly like any other device.

The user never has to learn Verilog-A. They type a formula in the same style
they already use elsewhere in VACASK netlists, and get a working, fully
differentiable device out the other end. This is the project.

---

## 4. What's being built (the "what")

A new, self-contained Python command-line tool, living alongside VACASK's
existing Python tools:

```
python/rpn2va.py          <- command you run
python/rpn2valib/         <- the tool's internals
    lexer.py
    parser.py
    semantics.py
    codegen.py
    emit.py
    exc.py
    tests/
```

This mirrors how the existing Ngspice-to-VACASK netlist converter is laid out
(`python/ng2vc.py` + `python/ng2vclib/`), so it should feel like a natural
sibling tool to anyone already familiar with this repo.

You run it like this:

```bash
python3 python/rpn2va.py \
    --name bsrc1 \
    --type V \
    --port p,n \
    --expr "v(in1)*v(in2) - offset" \
    --param offset=0.5 \
    -o bsrc1.va
```

and it writes `bsrc1.va`, ready to be compiled by OpenVAF and loaded by
VACASK — the exact behavior of the SPICE `B1 out 0 V = V(in1)*V(in2) - 0.5`
example from section 2.1, but built out of a file VACASK can actually use.

A couple more examples of the kind of thing you'd run:

```bash
# A current source: 1mA times a sine wave at a tunable frequency
python3 python/rpn2va.py \
    --name isrc1 --type I --port p,n \
    --expr "1m * sin(2*M_PI*freq*time)" \
    --param freq=1k \
    -o isrc1.va

# A comparator with a tunable threshold
python3 python/rpn2va.py \
    --name cmp1 --type V --port p,n \
    --expr "v(sig) > thresh ? 5 : 0" \
    --param thresh=1.5 \
    -o cmp1.va
```

---

## 5. The pipeline (the "how", at a glance)

```
 "v(in1)*v(in2) - offset"
        │
        │  1. lexer.py — chop the text into tokens
        ▼
 [IDENT(v) LPAREN IDENT(in1) RPAREN TIMES IDENT(v) LPAREN IDENT(in2) RPAREN
  MINUS IDENT(offset)]
        │
        │  2. parser.py — group tokens into a tree, respecting
        │     precedence (* before -, etc.)
        ▼
 BinOp(-, BinOp(*, Call(v,[in1]), Call(v,[in2])), Ident(offset))
        │
        │  3. semantics.py — walk the tree:
        │     - Call(v, [in1]) and Call(v, [in2]) are node references
        │       → in1, in2 become extra module ports
        │     - Ident(offset) is a plain identifier → becomes a parameter
        │     - reject anything unsupported (vectors, strings, ...)
        ▼
 ports = {in1, in2}, params = {offset: 0.5}
        │
        │  4. codegen.py — turn the tree into Verilog-A text,
        │     applying the operator/function/constant mapping tables
        ▼
 "V(in1)*V(in2) - offset"
        │
        │  5. emit.py — wrap that expression in a full .va file
        ▼
 bsrc1.va  (a complete Verilog-A module)
        │
        │  6. OpenVAF (already part of the VACASK toolchain, not part
        │     of this new tool) compiles it
        ▼
 bsrc1.osdi
        │
        │  7. VACASK loads it like any other device:
        │       load "bsrc1.osdi"
        │       model bsrc1model bsrc1
        │       b1 (p n in1 in2) bsrc1model offset=0.5
        ▼
 Simulated like any built-in or Verilog-A device, with real derivatives.
```

Steps 1–5 are what this project builds. Steps 6–7 already exist in VACASK
today and require no changes.

---

## 6. The new bit of syntax: `v(...)` and `i(...)`

VACASK's existing expression language (used for parameters, sweeps, etc.) has
no way to refer to "the voltage at this node right now" — it only computes
things that are known before simulation starts (like `1k + 2*vsupply` where
`vsupply` is a parameter, not a live simulated signal). A behavioral source,
by contrast, *must* reference live node voltages and branch currents — that's
the entire point.

So this tool defines two special names, recognized specifically by this
converter (not by VACASK's own expression grammar, and requiring no changes to
it):

| You write     | Meaning                                             | Becomes in Verilog-A |
|---------------|------------------------------------------------------|-----------------------|
| `v(node)`     | Voltage at `node`, relative to ground                | `V(node)`             |
| `v(n1, n2)`   | Voltage difference between `n1` and `n2`             | `V(n1, n2)`           |
| `i(branch)`   | Current flowing through named branch `branch`        | `I(branch)`           |

Examples:
- `v(in1)` → the voltage at node `in1` relative to ground → `V(in1)`
- `v(out, in1)` → the voltage difference between `out` and `in1` → `V(out,
  in1)` — useful for e.g. `--expr "gain * v(out, in1)"`, a formula about a
  voltage *difference* rather than an absolute node voltage
- `i(rsense)` → the current through a branch named `rsense` → `I(rsense)` —
  e.g. `--expr "1k * i(rsense)"` to build a voltage proportional to a sensed
  current

These look exactly like ordinary function calls, which VACASK's expression
grammar already supports — so no new grammar rules are needed at all. The
converter just treats calls to the specific names `v` and `i` differently
from calls to math functions like `sin` or `sqrt`: instead of translating them
into a Verilog-A function call, it turns them into a Verilog-A node/branch
*probe*, and it remembers every node name mentioned this way so it can declare
the right ports on the generated module. In the `v(in1)*v(in2) - offset`
example, this is exactly how the converter figures out that `in1` and `in2`
need to become module ports, while `offset` (below) doesn't.

Any other bare word in the expression is assumed to be one of three things,
checked in this order:
1. one of VACASK's named constants (section 9.3), e.g. `M_PI` — becomes the
   matching Verilog-A macro, not a parameter;
2. the special name `time` — VACASK's own RPN expression language has no
   live simulation-time value (it only evaluates static formulas, per
   section 3.1), but a time-varying behavioral source (a sine or pulse
   source, say) needs one, so `time` is recognized here too and becomes
   Verilog-A's standard `$abstime`;
3. otherwise, a **parameter** — a tunable number with a default value you
   supply on the command line, not a live circuit signal (like `offset` in
   the running example, or `freq` in `1m * sin(2*M_PI*freq*time)`).

---

## 7. Detailed file-by-file codemap

### `python/rpn2va.py` — the command you run
A small script, no `argparse` (matching the style of the existing
`python/ng2vc.py`, which hand-parses `sys.argv`). Reads `--name`, `--type`,
`--port`, `--expr`, repeated `--param name=value`, and `-o`/`--output`. Builds
the pieces, calls into `rpn2valib`, catches `RpnToVaError` and prints a clean
error message instead of a Python traceback. Example: running it with a typo
like `--expr "v(in1)*v(in2) - offst"` (missing the `e`) prints something like
`error: undefined parameter 'offst' (did you mean to pass --param offst=...?)`
rather than a stack trace.

### `python/rpn2valib/exc.py` — one error type
Defines `RpnToVaError`, this tool's only custom exception (mirrors
`ng2vclib/exc.py`'s `ConverterError`). Every place that detects something the
converter can't handle raises this, with a message that says exactly what's
wrong and where — no silent guessing. Example: `--expr "vector(1,2,3)"` raises
`RpnToVaError("unsupported function 'vector': operates on vector values, not "
"expressible in a scalar Verilog-A contribution")` rather than emitting
broken `.va` text. This matches a philosophy already written down in
`docs/netlist-converter-contribution-guide.md` (§20): "a converter that
refuses unsupported constructs is much safer than one that guesses
incorrectly."

### `python/rpn2valib/lexer.py` — text to tokens
Turns the raw expression string into a flat list of tokens: numbers
(including VACASK's SI-suffix notation like `1k`, `2.5u`, `10meg` — see the
full suffix table in section 9.4), identifiers, operators (`+ - * / ** == !=
< <= > >= & | ^ ~ << >> && || ! ? :`), parentheses, and commas. Example: the
input `"v(in1)*v(in2) - offset"` becomes the token list `IDENT(v) LPAREN
IDENT(in1) RPAREN TIMES IDENT(v) LPAREN IDENT(in2) RPAREN MINUS
IDENT(offset)`. This mirrors the token set of VACASK's own expression
tokenizer (`lib/dfllexer.l`), restricted to just what a standalone expression
needs (no whole-netlist-file syntax like `.model` or `control` blocks).

### `python/rpn2valib/parser.py` — tokens to a tree
Reads the token list and builds an **AST** (Abstract Syntax Tree — just a
tree of small Python objects representing "this operation applies to these
sub-expressions"). Uses a standard technique (a Pratt / recursive-descent
parser) that respects the same operator precedence and associativity as
VACASK's own grammar (`lib/dflparser.y`), so `a + b * c` parses as `a + (b *
c)` (multiplication binding tighter than addition) the same way VACASK would
parse it, `a ? b : c` works as a ternary, and so on. The tree node types are
plain dataclasses:

- `Num(value)` — a numeric literal, already resolved to a plain number (SI
  suffixes are expanded here, so the rest of the pipeline never has to think
  about them again). Example: `1k` becomes `Num(1000.0)`.
- `Ident(name)` — a bare word (a parameter, once semantics.py classifies it).
  Example: `offset` becomes `Ident("offset")`.
- `Call(name, args)` — a function call, e.g. `sin(x)` becomes `Call("sin",
  [Ident("x")])`, and `v(in1)` becomes `Call("v", [Ident("in1")])`.
- `BinOp(op, left, right)` — a two-input operation. Example: `a + b * c`
  becomes `BinOp("+", Ident("a"), BinOp("*", Ident("b"), Ident("c")))`.
- `UnaryOp(op, operand)` — a one-input operation, e.g. unary minus, `!`, `~`.
  Example: `-x` becomes `UnaryOp("-", Ident("x"))`.
- `Ternary(cond, if_true, if_false)` — the `?:` operator. Example: `v(sig) >
  thresh ? 5 : 0` becomes `Ternary(BinOp(">", Call("v",[Ident("sig")]),
  Ident("thresh")), Num(5.0), Num(0.0))`.

### `python/rpn2valib/semantics.py` — understanding the tree
Walks the AST once and:
1. Finds every `Call("v", ...)` and `Call("i", ...)`, validates the number of
   arguments (`v` takes 1 or 2, `i` takes 1), and records the node/branch
   names mentioned — these become the module's extra ports. Example: for
   `v(in1)*v(in2) - offset`, this step records `{in1, in2}` as ports.
2. Finds every remaining `Ident(name)`, and requires the caller to have
   supplied a default value for it via `--param`; if one's missing, raises
   `RpnToVaError` listing exactly which parameter names need defaults.
   Example: running with `--expr "v(in1)*gain"` but no `--param gain=...`
   raises `RpnToVaError("missing default for parameter 'gain'; pass --param
   gain=<value>")`.
3. Checks every `Call` against the supported-function table (section 9.2);
   anything not on the list — e.g. `vector(...)`, `sum(...)`, `isnan(...)` —
   raises `RpnToVaError` immediately, naming the unsupported function. These
   are rejected rather than guessed at because they're operations on lists,
   vectors, or types that simply don't make sense inside a single scalar
   Verilog-A contribution statement.

### `python/rpn2valib/codegen.py` — tree to Verilog-A text
Turns the (now validated) AST into a plain string of Verilog-A syntax,
applying three mapping tables (fully spelled out in section 9): operators,
functions, and constants. This is where, for example, VACASK's `**` power
operator turns `a ** b` into a Verilog-A `pow(a, b)` call, and VACASK's
`round(x)` becomes an expanded ternary expression, because Verilog-A has no
`round` built in. Concretely, `round(v(x))` becomes `((V(x)>=0) ?
floor(V(x)+0.5) : ceil(V(x)-0.5))`.

### `python/rpn2valib/emit.py` — text to a full `.va` file
Wraps the generated expression string in the rest of a real Verilog-A module:
the `` `include`` lines, the `module` declaration with all ports (the main
output pair plus every node/branch found by `semantics.py`), a `parameter
real` line per parameter (with its default and a `(*desc=...*)` annotation,
matching the style of every existing file under `devices/`), and the
`analog begin ... end` block with the single contribution statement
(`V(p,n) <+ expr;` for `--type V`, or `I(p,n) <+ expr;` for `--type I`). The
full worked example in section 8 shows exactly what this produces.

### `python/rpn2valib/tests/`
Small hand-written fixtures: an expression string in, the expected `.va` text
out, for one case per interesting rule (see section 10, "Testing strategy").

---

## 8. Worked example, start to finish

Command:

```bash
python3 python/rpn2va.py \
    --name bsrc1 --type V --port p,n \
    --expr "v(in1)*v(in2) - offset" \
    --param offset=0.5 \
    -o bsrc1.va
```

What each stage produces, following the pipeline in section 5:

1. **Tokens**: `v ( in1 ) * v ( in2 ) - offset`
2. **AST**: `BinOp(-, BinOp(*, Call(v,[in1]), Call(v,[in2])), Ident(offset))`
3. **Semantics**: ports found = `{in1, in2}`; parameters found = `{offset}`,
   default `0.5` supplied on the command line — OK, nothing missing.
4. **Codegen** produces the expression text: `V(in1)*V(in2) - offset`
5. **Emit** produces the full file, something close to:

```verilog
`include "constants.vams"
`include "disciplines.vams"

module bsrc1(p, n, in1, in2);
    inout p, n, in1, in2;
    electrical p, n, in1, in2;

    (*desc="Generated parameter", type="instance", units="1"*)
    parameter real offset = 0.5;

    analog begin
        V(p,n) <+ V(in1)*V(in2) - offset;
    end
endmodule
```

6. This file is compiled the same way any other VACASK Verilog-A model is:
   OpenVAF turns `bsrc1.va` into `bsrc1.osdi`.
7. It's used in a `.sim` netlist exactly like any other device
   (compare to `test/resonator.sim:5-10`):

```
load "bsrc1.osdi"
model bsrc1model bsrc1

b1 (out 0 in1 in2) bsrc1model offset=0.5
```

Now `out` tracks `V(in1)*V(in2) - 0.5` at every simulation timestep, with
fully correct derivatives, indistinguishable from a hand-written Verilog-A
device as far as VACASK's solver is concerned. For instance, if a DC sweep
drives `in1` from 0V to 5V while `in2` is held at 2V, `out` traces
`2*in1 - 0.5`, i.e. a straight line from `-0.5V` to `9.5V` — and the solver
converges on each point using the derivative `∂out/∂in1 = V(in2) = 2` that
OpenVAF generated automatically.

---

## 9. The mapping tables (the fiddly "why this exact translation" details)

These were pinned down by reading VACASK's actual source, not guessed — file
references included so they can be checked directly.

### 9.1 Operators

| VACASK | Verilog-A | Note |
|---|---|---|
| `+ - * /` | `+ - * /` | direct — e.g. `a + b*c` → `a + b*c` |
| `== != < <= > >=` | same | direct — e.g. `v(sig) > thresh` → `V(sig) > thresh` |
| `& | ^ ~ << >>` (bitwise) | same | direct — e.g. `flags & mask` → `flags & mask` |
| `&& || !` | same | direct — e.g. `v(a) > 0 && v(b) > 0` → `V(a) > 0 && V(b) > 0` |
| `?:` | same | direct — e.g. `v(sig) > thresh ? 5 : 0` → `V(sig) > thresh ? 5 : 0` |
| unary `-` | unary `-` | direct — e.g. `-v(in1)` → `-V(in1)` |
| `**` (power) | `pow(a, b)` | **not** kept as an infix operator — checked every `.va` file under `devices/` and infix `**` is never used (only appears inside `/* ... */` comment banners), while `pow(...)` is used 200+ times. So VACASK's `a ** b` becomes Verilog-A `pow(a, b)`, matching house style. Example: `v(in1) ** 2` → `pow(V(in1), 2)`. |

### 9.2 Functions

Straight across, one-to-one (VACASK name = Verilog-A name):
`sin cos tan asin acos atan sinh cosh tanh asinh acosh atanh exp sqrt
hypot abs ceil floor` — e.g. `sqrt(v(in1))` → `sqrt(V(in1))` — and `min`/`max`,
but only with exactly two arguments (VACASK may allow more; Verilog-A's
built-ins take two) — e.g. `min(v(a), v(b))` → `min(V(a), V(b))`.

Needing translation:

| VACASK | Verilog-A | Why |
|---|---|---|
| `ln(x)` | `ln(x)` | direct — e.g. `ln(v(in1))` → `ln(V(in1))` |
| `log(x)` | `ln(x)` | **VACASK's `log` is natural log**, not base-10 — confirmed in `lib/context.cpp:52-53`, where both `"log"` and `"ln"` are registered to the exact same underlying function (`FwLn`). Verilog-A's `log(x)` is base-10, so naively passing `log` straight through would silently compute the wrong thing. Example: VACASK's `log(v(in1))` must become Verilog-A `ln(V(in1))`, *not* `log(V(in1))`. This is flagged as the single most important gotcha in the whole mapping table. |
| `log10(x)` | `log(x)` | Verilog-A's `log` *is* base-10, so this is where it actually gets used. Example: `log10(v(in1))` → `log(V(in1))`. |
| `round(x)` | `(x>=0) ? floor(x+0.5) : ceil(x-0.5)` | Verilog-A has no `round` built in; this is the standard round-half-away-from-zero expansion. Example: `round(v(in1))` → `(V(in1)>=0) ? floor(V(in1)+0.5) : ceil(V(in1)-0.5)`. |
| `sgn(x)` | `(x>=0) ? 1 : -1` | **`sgn` and `sign` are two different functions**, not the same one under two names (an earlier draft of this doc conflated them). Per `include/rpnfunctor.h:298-303` (`FwSgn`), `sgn` takes exactly **1** argument and has no third "zero" case. Example: `sgn(v(in1))` → `(V(in1)>=0) ? 1 : -1`. |
| `sign(x1, x2)` | `(x2>=0) ? abs(x1) : -abs(x1)` | A **different, 2-argument, copysign-style** function — per `include/rpnfunctor.h:346-349` (`FwSign`), it returns `x1` with the sign of `x2`. No Verilog-A built-in equivalent either. Example: `sign(v(a), v(b))` → `(V(b)>=0) ? abs(V(a)) : -abs(V(a))`. |
| `atan2(x1, x2)` | `atan(x1/x2)` | VACASK's own `atan2` is **not** a real, quadrant-correct atan2 — per `include/rpnfunctor.h:341-344` (`FwAtan2`), it's implemented as plain `atan(x1/x2)`, ignoring the sign of `x2` entirely (wrong in quadrants II/III, undefined at `x2=0`). Verilog-A's built-in `atan2` *is* mathematically correct, but this converter deliberately replicates VACASK's actual (non-standard) behavior instead — so a formula means the same thing here as it would anywhere else in a VACASK netlist — emitting a comment in the generated file explaining why. Example: `atan2(v(a), v(b))` → `atan(V(a)/V(b))`. |
| `int(x)` / `integer(x)` | `$rtoi(x)` | truncate to integer. Example: `int(v(in1))` → `$rtoi(V(in1))`. |
| `real(x)` | *(dropped, no-op)* | everything is already a real number in this context. Example: `real(v(in1))` → `V(in1)`. |

Rejected outright, with a clear error naming the function (these operate on
vectors, lists, or types — concepts that don't exist inside a single scalar
Verilog-A contribution):
`string vector join split range sum prod where any all len isvector islist
isstring isint isreal isnan isinf isfinite`

Example: `--expr "sum(v(a), v(b), v(c))"` fails immediately with
`RpnToVaError("unsupported function 'sum'")` rather than producing something
that looks plausible but doesn't compile or means something different.

### 9.3 Constants

VACASK defines a set of named constants (`lib/context.cpp`) for use in
expressions. Verilog-A has its own standard constants file, `constants.vams`
(a copy ships in this repo at `devices/vbic/constants.vams` — the actual
Accellera-standard file, not VACASK-specific). Most match by name, but four
don't, which is worth knowing before assuming a plain find-and-replace works:

| VACASK name | Verilog-A macro | Note |
|---|---|---|
| `M_E M_LOG2E M_LOG10E M_LN2 M_LN10 M_PI M_TWO_PI M_PI_2 M_PI_4 M_1_PI M_2_PI M_2_SQRTPI M_SQRT2 M_SQRT1_2` | same name, backtick-prefixed (e.g. `` `M_PI``) | direct match, values checked. Example: `2*M_PI*freq*time` → `2*`M_PI`*freq*time`. |
| `P_C` | `` `P_C`` | direct |
| `P_U0` | `` `P_U0`` | direct |
| `P_CELSIUS0` | `` `P_CELSIUS0`` | direct |
| `P_Q` | `` `P_Q_OLD`` | `constants.vams` only defines `P_Q_SPICE`/`P_Q_OLD`/`P_Q_NIST1998`/`P_Q_NIST2010` — there is no bare `P_Q`. VACASK's own numeric value for `P_Q` was compared digit-for-digit against each variant and matches `P_Q_OLD` exactly. Example: `P_Q * n` → `` `P_Q_OLD * n``. |
| `P_K` | `` `P_K_OLD`` | same situation, matches `P_K_OLD` |
| `P_H` | `` `P_H_OLD`` | same situation, matches `P_H_OLD` |
| `P_EPS0` | `` `P_EPS0_OLD`` | same situation, matches `P_EPS0_OLD` |
| `M_DEGPERRAD` | *(none — inline the number)* | this constant (180/π, for converting radians to degrees) doesn't exist in `constants.vams` at all, under any name, so the converter emits its literal decimal value with a comment explaining what it is. Example: `v(in1) * M_DEGPERRAD` → `V(in1) * 57.29577951308232 /* M_DEGPERRAD: radians-to-degrees */`. |

### 9.4 Numeric literal suffixes

VACASK lets you write numbers with a scale suffix instead of writing out all
the zeros — e.g. `1k` for `1000`, `2.5u` for `0.0000025`. Exact table, taken
directly from VACASK's own tokenizer (`lib/dfllexer.l:532-558`):

| Suffix | Multiplier | Suffix | Multiplier |
|---|---|---|---|
| `f` | 10⁻¹⁵ | `k` / `K` | 10³ |
| `p` | 10⁻¹² | `M` | 10⁶ |
| `n` | 10⁻⁹ | `x` / `X` | 10⁶ |
| `u` | 10⁻⁶ | `meg` | 10⁶ |
| `m` | 10⁻³ | `G` | 10⁹ |
| `mil` | 25.4×10⁻⁶ | `T` | 10¹² |
| `a` | 10⁻¹⁸ | | |

(An earlier draft of this table omitted `a` — confirmed present, lowercase
only, no `A` variant, in `lib/dfllexer.l:519-558`'s suffix character class
and its `case 'a': s=1e-18;` branch.)

Two easy-to-miss traps, worth calling out explicitly since they're the kind of
thing that silently produces a wrong number instead of an error:
- **`m` and `M` are different** (`m` = milli = ÷1000, `M` = mega = ×1,000,000)
  — this is case-sensitive, unlike some other SPICE dialects. Example: `1m` is
  `0.001`, but `1M` is `1000000`.
  `x`/`X` also both mean mega, matching HSPICE-style `mil`/`meg` conventions.
- `meg` and `mil` are checked as whole three-letter suffixes *before* falling
  back to plain `m`, so `1meg` is a million (`1000000`), not "1 milli,
  followed by garbage `eg`," and `1mil` is `0.0000254`, not "1 milli followed
  by garbage `il`."

Because Verilog-A's own suffix conventions differ, the converter always
**resolves these to a plain decimal number** while parsing, rather than
trying to carry a suffix through into the generated `.va` file. Example:
`--param offset=2.5u` is resolved to `0.0000025` before it ever reaches
`emit.py`, so the generated `.va` file contains `parameter real offset =
0.0000025;`, not `2.5u`.

---

## 10. Testing strategy

Two levels, matching what's realistic to check without needing special
hardware:

1. **Text-level fixture tests** (`python/rpn2valib/tests/`): feed a small
   expression string in, compare the generated `.va` text against a
   hand-written expected file. For example, a fixture for the `log`/`ln` rule
   might look like:

   ```python
   def test_log_is_natural_log():
       result = convert(expr="log(v(in1))", name="t1", type_="V",
                         ports=["p", "n"], params={})
       assert "ln(V(in1))" in result
       assert "log(V(in1))" not in result
   ```

   One fixture per interesting rule: basic arithmetic and precedence, the
   ternary operator, `&&`/`||`, `v()`/`i()` port extraction, each
   function-mapping row in section 9.2 (especially the `log`/`ln`/`log10`
   trio and the `round`/`sgn` expansions), each constant in section 9.3
   (especially the four renamed ones and `M_DEGPERRAD`), an SI-suffix numeric
   literal, and at least one deliberately-unsupported construct (e.g.
   `vector(1,2,3)`) to confirm the tool fails loudly with a clear message
   instead of producing subtly-wrong Verilog-A.

2. **One true end-to-end test**: generate a `.va` file for a simple
   two-input multiplier (the `bsrc1` example from section 8), actually
   compile it with OpenVAF into an `.osdi` file, and simulate it using a tiny
   `.sim` netlist through VACASK's existing test mechanism
   (`test/CMakeLists.txt`'s `prepare_netlist_test` pattern, the same one used
   for every other `.sim`-based test in this repo) — e.g. a DC sweep of `in1`
   from 0 to 5V with `in2` fixed at 2V, checking that `out` comes out as
   `2*in1 - 0.5` at every swept point. This is the step that proves the whole
   chain — not just that the text looks plausible, but that VACASK genuinely
   loads it and computes the right numbers.

---

## 11. Deliberately left out of the first version (and why)

Keeping the first contribution narrow and reviewable (the same philosophy
already written down in `docs/netlist-converter-contribution-guide.md`):

- **Automatic netlist-level `bsource` directive.** Right now, using this
  feature is a two-step manual process: run `rpn2va.py`, then run OpenVAF,
  then add `load`/`model`/instance lines to your `.sim` file by hand. A great
  follow-up project is teaching VACASK's netlist reader to recognize a new
  `bsource` line directly in a `.sim` file — something like
  `bsource b1 (out 0 in1 in2) V = v(in1)*v(in2) - offset` in one step instead
  of the current four — and auto-run this whole chain. Deferred because it
  touches the netlist parser/loader itself, which is a bigger and riskier
  change than a standalone Python tool.
- **Vector-valued behavioral sources** (a single source producing many
  outputs at once, e.g. a 4-bit bus of voltages from one expression).
- **Noise contributions** in generated behavioral sources (real devices like
  `devices/resistor.va` also contribute thermal noise; a first-cut behavioral
  source doesn't need to).
- **Multi-branch outputs** (a single generated module driving more than one
  `V(...) <+` / `I(...) <+` statement — e.g. one source that sets both
  `V(out1,0)` and `V(out2,0)` from related formulas).

None of these are hard *in principle* — they're excluded so the first pull
request stays small enough to review quickly and land safely.

---

## 12. Glossary

**AST (Abstract Syntax Tree)**: A tree-shaped representation of a formula,
where each node is one operation (like "add these two things") and its
children are the things being operated on. Easier to translate to another
language than a flat token list. Example: `a + b*c` becomes `BinOp("+",
Ident("a"), BinOp("*", Ident("b"), Ident("c")))`.

**B source**: SPICE-family term for a component whose value is an arbitrary
user-written formula, rather than a fixed device type. Named for the `B`
prefix used in Ngspice/HSPICE instance names. Example: `B1 out 0 V =
V(in1)*V(in2) - 0.5`.

**Branch**: A named current path between two specific nodes, so you can refer
to "the current through this specific path" even if multiple components
connect the same two nodes. Example: `i(rsense)` refers to the current
through a branch named `rsense`.

**Derivative / Jacobian**: How much a value changes in response to a small
change in something else. The simulator's solver needs a full table of these
(the Jacobian) to iteratively home in on the correct circuit voltages.
Example: for `out = V(in1)*V(in2)`, the derivative of `out` with respect to
`in1` is `V(in2)`.

**Discipline**: In Verilog-A, a declaration of what "kind" of physical signal
a node carries (electrical, in this case) — determines whether `V()`/`I()`
make sense on it. Example: `electrical p, n, in1, in2;` in section 8's
generated module.

**Lexer / Tokenizer**: The first parsing stage — chops raw text into a flat
list of meaningful chunks (numbers, names, operators) without yet
understanding how they combine. Example: `"v(in1)*offset"` becomes `IDENT(v)
LPAREN IDENT(in1) RPAREN TIMES IDENT(offset)`.

**Newton-Raphson**: The standard iterative numerical method circuit simulators
use to solve the nonlinear equations describing a circuit; it repeatedly uses
derivative information to improve its guess.

**OpenVAF**: The compiler that turns a `.va` (Verilog-A) file into a `.osdi`
shared library VACASK can load. Also the component that automatically works
out derivatives for whatever formula the `.va` file contains. Example: turns
`bsrc1.va` into `bsrc1.osdi`.

**OSDI**: The technical interface/contract between VACASK and a compiled
`.osdi` device file.

**Parser**: The parsing stage after tokenizing — groups a flat token list into
a tree (the AST) that reflects operator precedence and nesting. Example: the
token list from the Lexer entry above becomes `BinOp("*", Call("v",
[Ident("in1")]), Ident("offset"))`.

**RPN (Reverse Polish Notation)**: The internal, flattened representation
VACASK uses to store already-parsed expressions. Not something users type
directly — you type ordinary math like `1k + 2*vsupply` and VACASK converts
it internally.

**Verilog-A**: The modeling language used to describe how a device behaves,
compiled by OpenVAF into something VACASK can load. Example:
`V(p,n) <+ V(in1)*V(in2) - offset;` inside an `analog begin ... end` block.

---

## 13. Immediate next steps if picking this up

1. Read `lib/context.cpp` (constants and function registrations) and
   `lib/dflparser.y`/`lib/dfllexer.l` (grammar/tokens) once, side by side with
   section 9 of this document, to sanity-check the mapping tables before
   writing code against them.
2. Build `lexer.py` and `parser.py` first, with unit tests that just check
   the AST shape (no Verilog-A output yet) — get parsing solid in isolation.
   Start with something trivial like `"1 + 2"`, then work up to the running
   `"v(in1)*v(in2) - offset"` example.
3. Add `semantics.py`'s port/parameter extraction and rejection rules.
4. Add `codegen.py` + `emit.py`, then the fixture tests from section 10.1.
5. Only after all of that works, attempt the OpenVAF end-to-end test from
   section 10.2 — it depends on having OpenVAF installed/available, which may
   not be true in every development environment.
