# Verilator adapter (proof of concept, unverified)

`cosim_verilator_adapter.cpp` implements the same `DigitalSim`
interface as `cosim_vpi_adapter.cpp` (see `../cosim_core.h`), but for
Verilator instead of Icarus Verilog. It exists to demonstrate that the
core/adapter split is real -- the same `cosim_core.cpp`, unmodified,
can be driven by a different digital simulator -- not as a finished,
supported second front-end.

**Status: not built or run against a real Verilator install as part of
this project.** It was written and reasoned about against Verilator's
documented API, but nobody has actually compiled and executed it here.
Treat it as a well-informed starting point, not a working feature.

## Why it looks different from the Icarus adapter

Icarus Verilog is an interpreter with a plugin mechanism (VPI): `vvp`
is the host process, `cosim.vpi` is a plugin loaded into it.

Verilator is a translator, not an interpreter: it turns Verilog into a
C++ class and generates no runnable program itself. Whoever uses it
has to supply their own `main()` that drives that class -- there is
no host process to be a plugin for. Because of that, `main()` lives
*here*, in the adapter (see the bottom of `cosim_verilator_adapter.cpp`),
and it wires up the A2D/D2A bindings directly in C++
(`core.bindA2D("xcore:xana:CLK", ...)`) rather than through
`cosim_top.v`'s `$cosim_a2d`/`$cosim_d2a` calls the way the Icarus
path does -- Verilator has no equivalent system-task mechanism to call
those through. `cosim_top.v` is still compiled (its module becomes the
generated `Vcosim_top` C++ class), but those particular calls inside
it are not what does the binding when Verilator is the target; whether
Verilator's compiler tolerates them being present but unresolved has
not been checked.

## Build (as documented in the adapter's own reasoning, not yet run)

```sh
verilator --cc --build --exe \
    cosim_top.v verilog1.v dig.v \
    cosim_verilator_adapter.cpp ../cosim_core.cpp \
    -CFLAGS "-I.. -I$(VACASK_PREFIX)/include" \
    -LDFLAGS "$(VACASK_PREFIX)/lib64/libsimlib.a -L$(VACASK_PREFIX)/lib64 -lklu -lbtf -lamd -lcolamd -lcamd -lccolamd -lcholmod -lsuitesparseconfig -L$(VACASK_PREFIX)/lib -lboost_filesystem -lboost_system -lboost_process -ldl -lpthread -lm"
```

Run the resulting binary directly (no `vvp`, no `-mcosim` -- see the
top-level `README.md` for why that's specific to the Icarus path):

```sh
obj_dir/Vcosim_top
```
