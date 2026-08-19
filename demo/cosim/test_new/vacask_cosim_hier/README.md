# VACASK digital cosimulation

Bridges VACASK (analog) and Icarus Verilog (digital). See
`docs/cosim-architecture.md` for the design.

## Build the bridge (once)

```sh
make VACASK_PREFIX=/path/to/vacask/install
```

This produces `cosim.vpi`, the only thing that needs a build step.
`VACASK_PREFIX` defaults to `../install`; override it to point at
wherever VACASK's headers, `libsimlib.a`, and Icarus Verilog are
installed.

## Run any example or test

Everything downstream of `cosim.vpi` is the same two commands,
run from the directory holding the netlist, the DUT Verilog, and a
`cosim_top.v` that binds them (see `example/` or any test under
`qualification/`):

```sh
iverilog -o t.vvp *.v
vvp -M<path to this directory> -mcosim t.vvp
```

`cosim_top.v` is the only Verilog file the bridge needs -- it calls
`$cosim_a2d`/`$cosim_d2a` to bind analog nodes to digital signals and
`$cosim_run` to start the analysis (see `docs/cosim-architecture.md`
Section 8). The netlist can come from a schematic capture tool or be
written by hand; the bridge only needs the resulting `.scs` file.

For example, from `example/`:

```sh
iverilog -o t.vvp *.v
vvp -M.. -mcosim t.vvp
```

## Automated qualification suite

`qualification/run_all.py` builds `cosim.vpi` and runs every test
under `qualification/` with the same two commands, then reports a
pass/fail summary (`python3 qualification/run_all.py`). There is no
Makefile there -- it is not needed for a two-command build+run.

## Verilator adapter (proof of concept, unverified)

`test_verilator_bridge/` has a second adapter, for Verilator instead
of Icarus Verilog, demonstrating that `cosim_core.cpp` is portable to
a different digital simulator. It has not been built or run against a
real Verilator install -- see the README in that directory before
relying on it.

## Standalone VACASK API test

`test_vacask_api/` runs the same kind of circuit as `example/`
directly against VACASK's own C++ API, with no bridge, no VPI, and no
Verilog at all -- useful for isolating whether a problem is in
VACASK's API or in the bridge. See the README in that directory.

## Stress test reference comparison

`test_performance/` also has a pure-Verilog reference testbench
(`tb_ref.sv`) with no analog engine, for comparing timing against:

```sh
cd test_performance
iverilog -o tb_ref.vvp tb_ref.sv perf_dut.v
vvp tb_ref.vvp
```
