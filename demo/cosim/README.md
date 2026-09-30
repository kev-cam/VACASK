# Mixed-signal co-simulation with nvc

VACASK can be the analog half of an analog-on-top mixed-signal simulation in
which the [nvc](https://github.com/nickg/nvc) VHDL simulator (fork with the
cosim runtime) runs the digital half. It is the same arrangement as the
nvc/Xyce co-simulation and uses the same boundary files and VHDL.

Pieces:

- **External sources** (`include/extsource.h`). A `vsource`/`isource` with
  `type="pwl"` and a `code:` URI in its `file` parameter takes its value from a
  function in a shared library:

  ```text
  v_d2a (n 0) vsrc type="pwl" file="code:libcosim_bridge.so:vacask_bridge_init:d2a:<signal>"
  i_a2d (n 0) isrc type="pwl" file="code:libcosim_bridge.so:vacask_bridge_init:a2d:<signal>"
  ```

  The vsource drives an analog node from a digital signal (D2A). The
  zero-current isource samples the node voltage at every accepted timepoint
  and hands it to the digital side (A2D). An A2D source can ask for the
  transient to pause at the current point, so the digital side sees a
  threshold crossing at the time it happens.

- **Transient sync hook** (`TranSync`, `lib/coretran.cpp`). A co-simulation
  master can add a breakpoint and pause the transient at any accepted point.
  The paused analysis resumes later without losing its state. When it
  resumes, the breakpoints that the external sources report are applied to
  the step that was already scheduled.

- **Command interpreter pause mode** (`simulator/cmd.h`). With
  `setPauseOnStop(true)`, `run()` returns `Paused` when an analysis stops, and
  the next `run()` resumes it.

- **C interface** (`cinterface/`, `libvacaskcinterface.so`). It provides
  `vacask_open`, `vacask_initialize`, `vacask_simulateUntil`,
  `vacask_simulationComplete`, `vacask_getTime` and `vacask_close`. These
  mirror Xyce's `libxycecinterface`. The library is built by default on Linux
  (`-DVACASK_CINTERFACE=ON`). Module and include paths come from
  `SIM_MODULE_PATH` and `SIM_INCLUDE_PATH`.

nvc selects VACASK with `--vacask-netlist=<deck.sim>`. The boundary map is
given with `--cosim-config=<file.boundary>`, which is an alias of
`--xyce-config`.

## Demos

`run.sh min`: a digital square wave drives an RC through a D2A source.

`run.sh a2d`: an analog step goes through an A2D source to a VHDL threshold
process, and back out through a D2A source.

```sh
NVCB=/usr/local/src/nvc-build VCB=/opt/build.VACASK/cosim ./run.sh a2d
```

SIMetrix/XSPICE designs go through the Xyce front end
(`xyce/utils/simetrix_cosim.pl`), then through `xyce/utils/cir2vacask.py`, which
translates the Xyce deck to VACASK. Run
`ENGINE=vacask xyce/utils/test_simetrix_cosim/gen_run.sh design.net`.

## Tuning

- `COSIM_A2D_DV` (in volts, default 0.1) sets the A2D resolution. A probe
  pauses the analog side once its node has moved this far since the probe's
  last pause, so a threshold crossing is seen within DV/slope. On `hier.net`
  (a 5 V/µs input ramp), 0.1 V gives edges 19 ns late and 0.01 V gives them
  0.5 ns late. On the flyback, going from 0.1 V to 0.01 V takes the run from
  17 s to 79 s.
- `options tran_lteimplicit=0` keeps LTE control off the implicit equations
  that OpenVAF adds for `ddt()` results in the SPICE-distilled models. Without
  it, the UC3844 flyback collapses its timestep at the first switching cycles,
  where a hard-switched BJT is involved. `cir2vacask.py` sets it.
- `VACASK_COSIM_TRACE=1` prints every `simulateUntil` call (from, target,
  reached). `options tran_debug=3` names the unknown that has the worst LTE.
