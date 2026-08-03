# VACASK C++ API Reference

Documentation reverse-engineered from the VACASK source headers (`parseroutput.h`, `an.h`, `circuit.h`, `value.h`) and the five demo programs (`demo/api/demo1..5.cpp`) at commit `764f693`.

All classes live in namespace `sim` (the macro `NAMESPACE` expands to `sim`).

---

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Fundamental Types](#fundamental-types)
3. [Netlist Description (Parser Output) Layer](#netlist-description-parser-output-layer)
4. [Parser](#parser)
5. [Circuit](#circuit)
6. [Analysis](#analysis)
7. [Simulator Setup](#simulator-setup)
8. [Workflow: Building and Running a Simulation](#workflow-building-and-running-a-simulation)
9. [Sweep API](#sweep-api)
10. [Partial Elaboration](#partial-elaboration)
11. [Embedded Files and Postprocessing](#embedded-files-and-postprocessing)
12. [Complete Examples](#complete-examples)

---

## Architecture Overview

VACASK separates circuit simulation into three distinct phases:

```
  Netlist Description          Circuit Object           Analysis
  (ParserTables)          (elaborate & build)        (create & run)
       |                        |                        |
  PTLoad, PTModel,         Circuit(tab, comp, s)    Analysis::create(desc, cir, s)
  PTInstance, ...          cir.elaborate(...)        tran->run(s)
  PTAnalysis, PTSweep      cir.setOption(...)
       |                        |                        |
       v                        v                        v
  "What the circuit is"   "Live circuit ready       "Run simulation,
                           for simulation"           produce output"
```

1. **Netlist Description** (`ParserTables` and `PT*` classes): a declarative, in-memory representation of the circuit, models, instances, parameters, analyses, and sweeps. This is what you build using the fluent API.

2. **Circuit** (`Circuit`): takes a `ParserTables` object and an OSDI compiler, loads device models, elaborates the hierarchy, sets up nodes, unknowns, and the Jacobian sparsity pattern.

3. **Analysis** (`Analysis`): takes a `PTAnalysis` description and a `Circuit`, creates a coroutine-based analysis engine (OP, transient, etc.), runs it, and writes output files.

---

## Fundamental Types

### `Value`

A variant-like type that can hold:

| Type constant    | C++ type              | Description          |
|------------------|-----------------------|----------------------|
| `Type::Int`      | `Int` (integer)       | Integer scalar       |
| `Type::Real`     | `Real` (`double`)     | Floating-point scalar|
| `Type::String`   | `String` (`std::string`) | String scalar     |
| `Type::IntVec`   | `IntVector`           | Vector of integers   |
| `Type::RealVec`  | `RealVector`          | Vector of doubles    |
| `Type::StringVec`| `StringVector`        | Vector of strings    |
| `Type::ValueVec` | `ValueVector`         | Vector of Values     |

**Construction** is implicit from any of the above types:
```cpp
Value v1(42);          // Int
Value v2(3.14);        // Real
Value v3("hello");     // String
Value v4(IntVector{0, 1, 2});  // IntVec
```

**Access**: use `val<T>()` template method:
```cpp
double x = v2.val<Real>();
```

**Querying**: `type()`, `isVector()`, `isNumeric()`, `str()`.

### `Id`

An interned identifier (string). Constructible from `const char*`. Used everywhere for names of models, instances, parameters, nodes, analyses, etc.

```cpp
Id name("resistor");
```

### `Loc`

Source location. `Loc::bad` is the default "no location" sentinel. Mostly for parser diagnostics; API users can ignore it.

### `Status`

Error/status reporter. Passed by reference throughout the API. Check return values (`bool`) and use `s.message()` for error text.

```cpp
Status s;
if (!someOperation(s)) {
    std::cerr << s.message() << "\n";
}
```

A static `Status::ignore` is available as a default argument when you don't need error details.

### `Rpn`

A parsed RPN (reverse Polish notation) expression. Created by `Parser::parseExpression()`. An empty `Rpn()` is used as the "else" condition in conditional blocks.

---

## Netlist Description (Parser Output) Layer

These classes (`PT*` prefix = "Parser Table") describe the circuit declaratively. They use a **fluent API** pattern: every `.add()` returns `*this` (by lvalue ref on lvalues, by rvalue ref on rvalues), so calls chain naturally.

All `PT*` classes are **move-only** (copy constructors deleted) except `PTParsedIdentifier` and `PTSave` which are copyable.

### `PTParsedIdentifier`

A named identifier with an optional source location.

```cpp
PTParsedIdentifier pid("mynode");
PTParsedIdentifier pid2("mynode", Loc::bad);
```

**Getters**: `name()`, `location()`.

### `PTIdentifierList`

Alias for `std::vector<PTParsedIdentifier>`. Used for terminal connections, global/ground node lists. Supports implicit construction from initializer lists of string literals:

```cpp
PTIdentifierList terms = {"1", "2", "0"};
```

### `PTParameterValue` (alias: `PV`)

A named parameter with a constant `Value`.

```cpp
PV{"r", 1000}           // parameter r = 1000
PV("dc", 10)            // parameter dc = 10
PV("type", "pulse")     // string parameter
PV("values", IntVector{0, 1})  // vector parameter
```

**Getters**: `name()`, `val()`, `location()`.

### `PTParameterExpression` (alias: `PE`)

A named parameter with an `Rpn` expression (evaluated at elaboration/runtime).

```cpp
PE{"c", parser.parseExpression("2*c0")}
```

**Getters**: `name()`, `rpn()`, `location()`.

### `PTParameters`

A collection of `PTParameterValue` and `PTParameterExpression` objects.

```cpp
// From vectors
PTParameters params(
    PVv(PV{"c0", 1e-6}, PV{"v0", 5}),  // constant values
    PEv()                                // expressions (empty)
);

// Built incrementally
PTParameters params;
params.add(PV{"r", 1000});
params.add(PE{"c", p.parseExpression("2*c0")});
```

**Helpers**: `PVv(...)` and `PEv(...)` are variadic template functions that construct `std::vector<PV>` and `std::vector<PE>` respectively.

**Getters**: `valueCount()`, `expressionCount()`, `count()`, `values()`, `expressions()`.

### `PTModel`

A device model declaration. Associates a name with a device type and optional parameters.

```cpp
PTModel("res", "resistor")                    // simple model
PTModel("dio", "diode")
    .add(p.parseParameters("is=1e-12 n=2 rs=1 eg=1.2 xti=2"))  // with parameters
```

**Constructor**: `PTModel(Id name, Id device, [PTParameters&&, ] [Loc])`.

**Getters**: `name()`, `device()`, `isParameterized()`, `parameters()`, `location()`.

**Fluent API**: `.add(PTParameters&&)`, `.add(PTParameterValue&&)`, `.add(PTParameterExpression&&)`.

### `PTInstance`

An instance of a model with terminal connections and optional parameters.

```cpp
PTInstance("r1", "res", {"1", "2"})       // instance r1 of model res, terminals 1 and 2
    .add(PV{"r", 1000})                   // constant parameter
    .add(PE{"c", p.parseExpression("2*c0")})  // expression parameter
```

**Constructor**: `PTInstance(Id name, Id master, PTIdentifierList&& terminals, [PTParameters&&, ] [Loc])`.

**Getters**: `name()`, `masterName()`, `connections()`, `isParameterized()`, `parameters()`, `location()`.

**Fluent API**: `.add(PTParameters&&)`, `.add(PTParameterValue&&)`, `.add(PTParameterExpression&&)`.

### `PTBlock`

A netlist block containing models, instances, and conditional block sequences.

```cpp
PTBlock()
    .add(PTInstance("r1", "res", {"p", "out"})
        .add(PE("r", p.parseExpression("r*(1-fact)")))
    )
    .add(PTInstance("r2", "res", {"out", "n"})
        .add(PE("r", p.parseExpression("r*fact")))
    )
```

**Getters**: `models()`, `instances()`, `hasBlockSequences()`, `blockSequences()`.

**Fluent API**: `.add(PTModel&&)`, `.add(PTInstance&&)`, `.add(PTBlockSequence&&)`.

### `PTBlockSequence`

An if/else-if/else chain of conditional blocks. Each entry is a tuple of `(Loc, Rpn condition, PTBlock)`. An empty `Rpn()` means "else" (unconditional).

```cpp
PTBlockSequence()
    .add(p.parseExpression("mode==0"), PTBlock()
        .add(PTInstance("r1", "res", {"p", "out"})
            .add(PE("r", p.parseExpression("r*(1-fact)")))
        )
    )
    .add(Rpn(), PTBlock()    // else block
        .add(PTInstance("r1", "res", {"p", "out"})
            .add(PE("r", p.parseExpression("r*fact")))
        )
    )
```

Conditional blocks are evaluated at elaboration time. Changing a variable that affects the condition and calling `elaborateChanges()` can change the circuit topology at runtime.

### `PTSubcircuitDefinition`

A subcircuit definition (extends `PTModel`). Contains terminals, a root block, nested subcircuit definitions, and parameters.

```cpp
// Toplevel definition (no name, no terminals)
PTSubcircuitDefinition()
    .add(PTModel("res", "resistor"))
    .add(PTInstance("r1", "res", {"1", "0"}).add(PV("r", 100)))

// Named subcircuit with terminals
PTSubcircuitDefinition("mysub", {"p", "out", "n"})
    .add(PV("r", 1000))        // default parameter values
    .add(PV("fact", 0.5))
    .add(PTBlockSequence()...)  // conditional blocks
```

**Constructors**:
- `PTSubcircuitDefinition()` — unnamed toplevel
- `PTSubcircuitDefinition(Id name, [PTIdentifierList&& terminals, ] [Loc])` — named

**Getters**: `terminals()`, `root()`, `subDefs()`. Inherits `name()`, `device()`, `parameters()` from `PTModel`.

**Fluent API**: `.add(PTSubcircuitDefinition&&)` (nested defs), `.add(PTModel&&)`, `.add(PTInstance&&)`, `.add(PTBlockSequence&&)`, `.add(PTParameters&&)`, `.add(PTParameterValue&&)`, `.add(PTParameterExpression&&)`.

### `PTLoad`

Loads an OSDI device model file.

```cpp
PTLoad("resistor.osdi")
PTLoad("capacitor.osdi")
```

### `PTSave`

A save directive specifying which quantities to record during analysis.

```cpp
PTSave("default")           // save default quantities (all node voltages)
PTSave("p", "r1", "i")      // save property "i" of instance "r1" (type "p")
```

**Constructors**:
- `PTSave(Id typeName)` — type-only
- `PTSave(Id typeName, Id id1)` — type + object
- `PTSave(Id typeName, Id id1, Id id2)` — type + object + sub-object

### `PTSaves`

A collection of `PTSave` objects with fluent `.add()`.

### `PTSweep`

Describes a parameter sweep within an analysis.

```cpp
// Sweep an instance parameter
PTSweep("mode")
    .add(PV("instance", "x1"))
    .add(PV("parameter", "mode"))
    .add(PV("values", IntVector{0, 1}))

// Sweep a circuit variable
PTSweep("temp")
    .add(PV("variable", "myvar"))
    .add(PV("from", -50))
    .add(PV("to", 100))
    .add(PV("step", 2))

// Sweep a simulator option directly
PTSweep("temp")
    .add(PV("option", "temp"))
    .add(PV("from", -50))
    .add(PV("to", 100))
    .add(PV("step", 2))
```

**Sweep targets** (specified via parameters):
- `"instance"` + `"parameter"`: sweep an instance parameter
- `"variable"`: sweep a circuit variable
- `"option"`: sweep a simulator option

**Sweep ranges**:
- `"from"`, `"to"`, `"step"`: linear sweep
- `"values"` (with `IntVector` or `RealVector`): discrete sweep points

### `PTAnalysis`

Describes an analysis to run.

```cpp
PTAnalysis("tran1", "tran")       // transient analysis named "tran1"
    .add(PV{"step", 1e-6})        // time step
    .add(PV("stop", 10e-3))       // stop time

PTAnalysis("dc1", "op")           // operating point (DC) analysis named "dc1"
    .add(PTSweep("mode")...)      // with sweeps
```

**Constructor**: `PTAnalysis(Id name, Id typeName, [Loc])`.

**Analysis types**: `"tran"` (transient), `"op"` (operating point/DC), and others.

**Getters**: `name()`, `typeName()`, `parameters()`, `sweeps()`, `location()`.

**Fluent API**: `.add(PTSweep&&)`, `.add(PTParameters&&)`, `.add(PTParameterValue&&)`, `.add(PTParameterExpression&&)`.

### `PTEmbed`

Embeds a file (typically a Python postprocessing script) in the circuit description.

```cpp
PTEmbed("runme.py", R"script(
from rawfile import rawread
import matplotlib.pyplot as plt
# ... processing code ...
)script")
```

### `PTCommand`

A control block command (used internally by the command interpreter). Not typically used by API users.

### `ParserTables`

The **top-level container** that holds the entire circuit description. This is the starting point for building a circuit.

```cpp
ParserTables tab("My Circuit Title");
tab
    .add(PTLoad("resistor.osdi"))
    .add(PTLoad("capacitor.osdi"))
    .defaultGround()
    .setDefaultSubDef(
        PTSubcircuitDefinition()
        .add(PTModel("res", "resistor"))
        .add(PTInstance("r1", "res", {"1", "0"}).add(PV("r", 100)))
    )
    .add(PTEmbed("postprocess.py", "..."));
```

**Key methods**:

| Method | Description |
|--------|-------------|
| `setTitle(string)` | Set circuit title |
| `add(PTLoad&&)` | Add a device model file to load |
| `defaultGround()` | Add default ground node "0" |
| `addGround(PTParsedIdentifier)` | Add a named ground node |
| `addGlobal(PTParsedIdentifier)` | Add a global node |
| `setDefaultSubDef(PTSubcircuitDefinition&&)` | Set the default toplevel subcircuit |
| `add(PTEmbed&&)` | Add an embedded file |
| `addCommand(PTAnalysis&&)` | Add an analysis to the control block |
| `addCommand(PTCommand&&)` | Add a command to the control block |
| `verify(Status&)` | Validate the tables (thorough, for API-built circuits) |
| `verifyAfterParse(Status&)` | Lightweight validation after parsing |
| `dump(indent, ostream)` | Print tables for debugging |
| `writeEmbedded(debug, Status&)` | Write embedded files to disk |

**Getters**: `title()`, `fileStack()`, `loads()`, `defaultSubDef()`, `groundNodes()`, `globalNodes()`, `control()`, `embed()`.

---

## Parser

The `Parser` class provides expression and parameter parsing from strings.

```cpp
ParserTables tab("title");
Parser p(tab);

// Parse a single expression
Rpn expr = p.parseExpression("2*c0");

// Parse parameter assignments (returns PTParameters)
PTParameters params = p.parseParameters("r=10*(1+($temp-tnominal)/100)");
PTParameters params2 = p.parseParameters("type=\"pulse\" val0=0 val1=v0 delay=1m");
```

The `Parser` needs the `ParserTables` reference because all interned strings are stored there. **Do not call parsing in a loop** — parse once and reuse the results.

Special variables in expressions:
- `$temp` — current simulation temperature
- Circuit variables (set via `Circuit::setVariable()`)
- Instance/model parameters in scope

---

## Circuit

The `Circuit` class is the live, elaborated representation of a circuit ready for simulation.

### Construction

```cpp
OpenvafCompiler comp;           // OSDI compiler
Status s;
Circuit cir(tab, &comp, s);    // takes ParserTables + compiler
if (!cir.isValid()) {
    std::cerr << s.message() << "\n";
}
```

### Elaboration

Elaboration processes the declarative `ParserTables` into a live circuit with nodes, instances, sparsity pattern, etc.

```cpp
// Basic elaboration (default toplevel subcircuit only)
cir.elaborate({}, "__topdef__", "__topinst__", nullptr, s);

// With additional toplevel subcircuit definitions
cir.elaborate({"sub1"}, "__topdef__", "__topinst__", nullptr, s);
```

**Parameters**:
- `toplevelDefinitions`: vector of additional subcircuit definition names to elaborate alongside the default
- `topDefName`, `topInstName`: prefixes for the toplevel subcircuit model and instance names
- `devReq`: device requests pointer (can be `nullptr`)
- `s`: status reporter

**Important**: Elaboration does NOT reset variables or options. Call `clearVariables()` / `clearOptions()` manually before re-elaboration if needed.

### Variables API

Circuit variables are named values that can be referenced in parameter expressions.

```cpp
cir.setVariable("myvar", 0);             // set variable
cir.setVariable("tnominal", 27);

const Value* v = cir.getVariable("myvar");  // get variable (nullptr if not found)
if (v) std::cout << v->str();

cir.clearVariables();                     // clear all variables
```

### Options API

Simulator options control tolerances, temperature, and other simulation parameters.

```cpp
// Set a single option
cir.setOption("reltol", 1e-4);

// Set options from an IStruct
IStruct<SimulatorOptions> opt;
opt.core().reltol = 1e-4;
cir.setOptions(opt);

// Set options from parsed parameters
auto parsedOpt = p.parseParameters("temp=var1");
cir.setOptions(parsedOpt);

// Read options
double reltol = cir.simulatorOptions().core().reltol;

// Clear all options back to defaults
cir.clearOptions();
```

### Instance/Model Parameter API

Read and write parameters of elaborated instances and models.

```cpp
// Set an instance parameter
cir.setInstanceParameter("v1", "dc", 20);

// Read an instance parameter
auto [ok, val] = cir.instanceParameter("v1", "dc");
if (ok) std::cout << val.str();

// Set a model parameter
cir.setModelParameter("dio", "is", 1e-14);

// Read a model parameter
auto [ok2, val2] = cir.modelParameter("dio", "is");
```

### Finding Circuit Elements

```cpp
const Node*     n = cir.findNode("1");
const Device*   d = cir.findDevice("resistor");
const Model*    m = cir.findModel("res");
const Instance* i = cir.findInstance("r1");
```

### Debugging Dumps

```cpp
cir.dumpHierarchy(0, Simulator::out());
cir.dumpDevices(0, Simulator::out());
cir.dumpModels(0, Simulator::out());
cir.dumpNodes(0, Simulator::out());
cir.dumpVariables(0, Simulator::out());
cir.dumpOptions(0, Simulator::out());
cir.dumpUnknowns(0, Simulator::out());
cir.dumpSparsity(0, Simulator::out());
```

---

## Analysis

### Creating an Analysis

Analyses are created through a factory pattern.

```cpp
// Describe the analysis
auto tranDesc = PTAnalysis("tran1", "tran");
tranDesc
    .add(PV{"step", 1e-6})
    .add(PV("stop", 10e-3));

// Create the analysis object
Status s;
Analysis* tran = Analysis::create(tranDesc, cir, s);
if (!tran) {
    std::cerr << s.message() << "\n";
}
```

### Adding Saves

Save directives tell the analysis which quantities to record.

```cpp
tran->add(PTSave("default"));          // save all default quantities
tran->add(PTSave("p", "r1", "i"));     // save current through r1
```

### Adding Parameterized Options

Analysis options that depend on expressions (e.g., temperature as a function of a circuit variable).

```cpp
auto anPar = p.parseParameters("temp=myvar");
analysis->add(anPar);
```

### Running

```cpp
// Simple run (blocks until completion)
auto [ok, canResume] = tran->run(s);
if (!ok) {
    std::cerr << s.message() << "\n";
}
```

### Coroutine API (Advanced)

For fine-grained control, analyses expose a coroutine interface.

```cpp
if (!tran->start(s)) { /* error */ }

bool running = true;
while (running) {
    AnalysisState state = tran->resume();
    switch (state) {
    case AnalysisState::SweepPoint:
        // do something between sweep points
        break;
    case AnalysisState::Finished:
    case AnalysisState::Aborted:
        running = false;
        break;
    case AnalysisState::Stopped:
        running = false;  // or resume later
        break;
    default:
        break;
    }
}

tran->finish(s);
```

**`AnalysisState` enum**:
- `Uninitilized` (sic) — not started
- `SweepPoint` — one sweep point completed
- `Finished` — analysis completed normally
- `Stopped` — analysis stopped (can resume)
- `Aborted` — analysis aborted (cannot resume)

### Cleanup

Analysis objects are heap-allocated and must be deleted.

```cpp
delete tran;
```

---

## Simulator Setup

Before any simulation, the simulator must be initialized.

```cpp
#include "simulator.h"
#include "openvafcomp.h"

Simulator::setup();                                    // initialize
Simulator::prependModulePath({"/path/to/osdi/files"}); // OSDI search paths

// I/O streams
Simulator::out() << "Info message\n";
Simulator::err() << "Error message\n";
```

---

## Object Lifetime and Ownership

The VACASK objects form a dependency chain that determines destruction
order:

- `Analysis` holds references to both `Circuit` and `PTAnalysis`.
- `Circuit` holds a reference to `ParserTables`.
- `ParserTables` owns the subcircuit definitions and load directives.

**Destruction order**: delete `Analysis` first, then `Circuit`, then
`ParserTables`.

When calling `analysis->add(anPar)` with an lvalue `PTAnalysis`, the
analysis stores non-owning parameter pointers.  The `PTAnalysis`
descriptor must remain alive for the lifetime of the analysis.  Use
`std::move(anPar)` to transfer ownership to the analysis instead.

---

## Workflow: Building and Running a Simulation

The complete sequence:

```cpp
#include "simulator.h"
#include "parser.h"
#include "openvafcomp.h"
#include "circuit.h"

using namespace sim;

int main() {
    Status s;

    // 1. Setup
    Simulator::setup();
    Simulator::prependModulePath({"path/to/mod"});

    // 2. Build netlist description
    ParserTables tab("My Circuit");
    Parser p(tab);

    tab
        .add(PTLoad("resistor.osdi"))
        .defaultGround()
        .setDefaultSubDef(
            PTSubcircuitDefinition()
            .add(PTModel("res", "resistor"))
            .add(PTModel("vsrc", "vsource"))
            .add(PTInstance("r1", "res", {"1", "0"}).add(PV("r", 100)))
            .add(PTInstance("v1", "vsrc", {"1", "0"}).add(PV("dc", 5)))
        );

    // 3. Verify
    if (!tab.verify(s)) { /* handle error */ }

    // 4. Write embedded files (if any)
    tab.writeEmbedded(1, s);

    // 5. Create circuit
    OpenvafCompiler comp;
    Circuit cir(tab, &comp, s);
    if (!cir.isValid()) { /* handle error */ }

    // 6. Set options (optional)
    cir.setOption("reltol", 1e-4);

    // 7. Elaborate
    if (!cir.elaborate({}, "__topdef__", "__topinst__", nullptr, s)) {
        /* handle error */
    }

    // 8. Define analysis
    auto desc = PTAnalysis("op1", "op");

    // 9. Create and run analysis
    auto an = Analysis::create(desc, cir, s);
    if (!an) { /* handle error */ return 1; }
    an->add(PTSave("default"));
    auto [ok, canResume] = an->run(s);

    // 10. Cleanup
    delete an;
    return 0;
}
```

---

## Sweep API

Sweeps are added to `PTAnalysis` objects via `PTSweep`. Multiple sweeps create nested loops (outermost first).

### Sweep over instance parameter (discrete values)

```cpp
PTAnalysis("dc1", "op")
    .add(PTSweep("mode")
        .add(PV("instance", "x1"))
        .add(PV("parameter", "mode"))
        .add(PV("values", IntVector{0, 1}))
    );
```

### Sweep over circuit variable (linear range)

```cpp
PTAnalysis("dc1", "op")
    .add(PTSweep("temp")
        .add(PV("variable", "myvar"))
        .add(PV("from", -50))
        .add(PV("to", 100))
        .add(PV("step", 2))
    );
```

When sweeping a variable, the analysis also needs parameterized options that reference that variable:

```cpp
auto anPar = p.parseParameters("temp=myvar");
analysis->add(anPar);
```

### Sweep over simulator option (direct)

```cpp
PTAnalysis("dc2", "op")
    .add(PTSweep("temp")
        .add(PV("option", "temp"))
        .add(PV("from", -50))
        .add(PV("to", 100))
        .add(PV("step", 2))
    );
```

---

## Partial Elaboration

After full elaboration, you can change variables, options, or instance parameters and partially re-elaborate without rebuilding the entire circuit. This is more efficient and used internally during sweeps.

```cpp
// Change something
cir.setOption("reltol", 1e-6);
cir.setVariable("tnominal", 40);
cir.setInstanceParameter("v1", "dc", 20);

// Partially re-elaborate
auto [ok, topologyChange, bindingNeeded] = cir.elaborateChanges(nullptr, s);
if (topologyChange) {
    // Topology changed (e.g., conditional block switched)
}
if (bindingNeeded) {
    // Analysis rebinding needed
}
```

**Important**: if a variable change triggers a conditional block switch, `elaborateChanges()` will rebuild the affected subcircuit, resetting instance parameters within it to their netlist values.

### Multiple topologies with full re-elaboration

To switch between completely different subcircuit topologies, use `elaborate()` again:

```cpp
// First topology
cir.elaborate({"sub1"}, "__topdef__", "__topinst__", nullptr, s);
auto dc1 = Analysis::create(dc1Desc, cir, s);
dc1->run(s);
delete dc1;

// Second topology (re-elaborates from scratch)
cir.elaborate({"sub2"}, "__topdef__", "__topinst__", nullptr, s);
auto dc2 = Analysis::create(dc2Desc, cir, s);
dc2->run(s);
delete dc2;
```

---

## Embedded Files and Postprocessing

Embedded files are written to disk by `tab.writeEmbedded()`. Typically used for Python postprocessing scripts.

```cpp
tab.add(PTEmbed("runme.py", R"script(
from rawfile import rawread
import matplotlib.pyplot as plt

data = rawread('tran1.raw').get()
plt.plot(data["time"], data["2"])
plt.show()
)script"));

// ...
tab.writeEmbedded(1, s);  // debug level 1

// After analysis completes, run postprocessing
runProcess(pythonBinary, {"runme.py"}, &pythonLibraryPath, nullptr, false, false);
```

Output files are in `.raw` format and can be read with the bundled `rawfile` Python module.

---

## Complete Examples

### Demo 1: RC Transient

Builds an RC circuit with a pulse voltage source, runs transient analysis, saves node voltages and branch current.

Key patterns:
- `PTLoad` for loading OSDI models
- `defaultGround()` for ground node "0"
- Expression parameters: `PE{"c", p.parseExpression("2*c0")}`
- Parsed parameters: `p.parseParameters("type=\"pulse\" val0=0 val1=v0 ...")`
- Save directives: `PTSave("default")` and `PTSave("p", "r1", "i")`

### Demo 2: Conditional Blocks + DC Sweep

Demonstrates if/else topology in a subcircuit and sweeping an instance parameter with discrete values.

Key patterns:
- `PTSubcircuitDefinition("mysub", {"p", "out", "n"})` with default parameters
- `PTBlockSequence` with condition `p.parseExpression("mode==0")` and else block `Rpn()`
- Instance parameter sweep: `PTSweep("mode").add(PV("instance","x1")).add(PV("parameter","mode")).add(PV("values", IntVector{0,1}))`
- `cir.setVariable("var1", 60)` + `cir.setOptions(p.parseParameters("temp=var1"))`

### Demo 3: Variables and Parameterized Options Sweep

Two approaches to temperature sweep: (a) sweeping a circuit variable with parameterized options, (b) sweeping the option directly.

Key patterns:
- `cir.setVariable("myvar", 0)` to define a circuit variable
- `PTSweep("temp").add(PV("variable","myvar"))` to sweep the variable
- `analysis->add(p.parseParameters("temp=myvar"))` to link option to variable
- `PTSweep("temp").add(PV("option","temp"))` for direct option sweep
- Circuit state is restored after each analysis finishes

### Demo 4: Re-elaboration with Different Topologies

Two subcircuit definitions (R-D vs D-R) with separate elaboration and analysis for each.

Key patterns:
- Multiple `PTSubcircuitDefinition` within the default subcircuit
- `cir.elaborate({"sub1"}, ...)` then `cir.elaborate({"sub2"}, ...)` to switch topologies
- Each elaboration is a full rebuild

### Demo 5: Partial Elaboration

Demonstrates changing variables, options, and instance parameters followed by `elaborateChanges()` instead of full re-elaboration.

Key patterns:
- `cir.setVariable("fixed", 1)` / `cir.setVariable("fixed", 0)` triggers topology change
- `cir.setOption("reltol", 1e-6)` and `cir.setInstanceParameter("v1", "dc", 20)`
- `cir.elaborateChanges(nullptr, s)` returns `(ok, topologyChange, bindingNeeded)`
- `cir.instanceParameter("r1", "r")` to read back instance parameters
- `cir.getVariable("tnominal")` to read back variables
- Topology change from conditional block resets subcircuit instance parameters

---

## Class Hierarchy Summary

```
ParserTables              (top-level container)
  ├── PTLoad              (OSDI model file)
  ├── PTSubcircuitDefinition  (extends PTModel)
  │     ├── PTModel       (device model)
  │     ├── PTInstance     (device instance)
  │     ├── PTBlockSequence   (conditional blocks)
  │     │     └── PTBlock     (models + instances + nested sequences)
  │     └── PTSubcircuitDefinition  (nested subcircuits)
  ├── PTEmbed             (embedded files)
  └── PTControl           (vector of PTAnalysis | PTCommand)
        ├── PTAnalysis    (analysis description)
        │     └── PTSweep (parameter sweep)
        └── PTCommand     (control command)

PTParameters              (parameter container)
  ├── PTParameterValue    (constant parameter, alias PV)
  └── PTParameterExpression (expression parameter, alias PE)

PTSaves                   (save directive container)
  └── PTSave              (single save directive)

Circuit                   (live circuit)
Analysis                  (simulation engine, factory-created)
```
