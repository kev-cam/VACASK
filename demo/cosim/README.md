# Mixed-signal co-simulation with nvc

VACASK can be the analog half of an analog-on-top mixed-signal simulation in
which the [nvc](https://github.com/nickg/nvc) VHDL simulator (fork with the
cosim runtime) runs the digital half. It is the same arrangement as the
nvc/Xyce co-simulation and uses the same boundary files, VHDL and bridge
library (`libcosim_bridge.so`).

## How it works

The transient analysis is the master schedule. The digital simulator never
sets a target time of its own; it is advanced from inside the analog step:

1. VACASK solves a candidate step `tk -> t`. When the step has passed its
   own error control, every external source gets `candidate()` with the
   voltage across its terminals at `tk` and at `t`, and the bridge library's
   `vacask_extsource_step(t)` runs.
2. The bridge advances nvc to `t`: one digital time point at a time, feeding
   the probes' trajectory (linear between `tk` and `t`, the same assumption
   the integrator makes) at enough evenly spaced times for no sample to move
   more than `COSIM_A2D_DV` volts. The digital therefore resolves threshold
   crossings finer than the analog step, at the cost of digital events only.
3. If the digital changes an analog input (a D2A source) at some `tEvt < t`,
   the bridge stops there and the step is not accepted: VACASK redoes it so
   that it ends exactly at `tEvt`, where the D2A source starts its 1 ns ramp.
   A change at `t` itself is accepted (the ramp starts at `t`); a change in
   reaction to the step-start values redoes the same step.
4. Otherwise the step is accepted and the next one begins. The analog never
   stops anywhere else, and `simulateUntil(stop)` is one call.

Pieces:

- **External sources** (`include/extsource.h`, ABI 2). A `vsource`/`isource`
  with `type="pwl"` and a `code:` URI in its `file` parameter takes its value
  from a function in a shared library:

  ```text
  v_d2a (n 0) vsrc type="pwl" file="code:libcosim_bridge.so:vacask_bridge_init:d2a:<signal>"
  i_a2d (n 0) isrc type="pwl" file="code:libcosim_bridge.so:vacask_bridge_init:a2d:<signal>"
  ```

  The vsource drives an analog node from a digital signal (D2A); the
  zero-current isource is a probe (A2D). Besides `value()`, a source may
  provide `candidate()`, and the library may export
  `vacask_extsource_step()` (`ExtSource::preAccept` in `lib/coretran.cpp`
  calls them at the acceptance fence and redoes the step on a veto).

- **Transient sync hook** (`TranSync`, `lib/coretran.cpp`) and the command
  interpreter's pause mode (`simulator/cmd.h`, `setPauseOnStop(true)`): a
  caller can add a breakpoint and pause the transient at an accepted point,
  then resume it. The C interface uses this for `simulateUntil`.

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

`run.sh glitch`: a D2A change arrives while the previous edge is still
ramping; the driven voltage must reverse from where the ramp is.

```sh
NVCB=/usr/local/src/nvc-build VCB=/opt/build.VACASK/Release ./run.sh a2d
```

SIMetrix/XSPICE designs go through the Xyce front end
(`xyce/utils/simetrix_cosim.pl`), then through `xyce/utils/cir2vacask.py`, which
translates the Xyce deck to VACASK. Run
`ENGINE=vacask xyce/utils/test_simetrix_cosim/gen_run.sh design.net`.

## Tuning

- `COSIM_A2D_DV` (volts, default 0.01): the A2D sampling resolution inside a
  step (at most 1000 samples per step). It costs digital events only; on the
  flyback 0.1, 0.01 and 0.001 V run in 11, 11.5 and 12.4 s.
- `options tran_lteimplicit=0` keeps LTE control off the implicit equations
  that OpenVAF adds for `ddt()` results in the SPICE-distilled models. Without
  it, the UC3844 flyback collapses its timestep at the first switching cycles,
  where a hard-switched BJT is involved. `cir2vacask.py` sets it.
- `COSIM_TRACE=1` traces the D2A changes and the vetoed steps (bridge) and
  every `simulateUntil` call (nvc); `VACASK_COSIM_TRACE=1` traces them on the
  VACASK side. `options tran_debug=1` reports the vetoed points, and
  `tran_debug=3` names the unknown that has the worst LTE.
