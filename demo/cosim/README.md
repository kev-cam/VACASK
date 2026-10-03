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
   that it ends exactly at `tEvt`, where the D2A source starts its ramp
   (1 ns unless the boundary line says otherwise, see below).
   A change at `t` itself is accepted (the ramp starts at `t`); a change in
   reaction to the step-start values redoes the same step.
4. If the digital has stopped (`$finish`, `std.env.stop`/`finish`, a failure
   report or a fatal error) at `tf`, the step is vetoed to `tf` once if it
   ends after `tf`, and then the bridge answers *finish*: VACASK accepts the
   point and ends the transient there, as it does for a Verilog-A `$finish`.
   The initial point (t=0, after the operating point) is offered too, so a
   digital that stops at t=0 (during its t=0 processes, or on the operating
   point's A2D values) ends the run right after the operating point.
5. Otherwise the step is accepted and the next one begins. The analog never
   stops anywhere else, and `simulateUntil(stop)` is one call.

The digital settles its own t=0 processes before the analog starts, so the
operating point sees the values the digital drives at t=0 (a clock that
starts at 1 gives no start-up edge).

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
  calls them at the acceptance fence). Its result is
  `VACASK_EXTSRC_ACCEPT` (0), `VACASK_EXTSRC_VETO` (1, with `tEvt >= 0`: the
  step is redone to end at `tEvt`) or `VACASK_EXTSRC_FINISH` (2: the point is
  accepted and the transient finishes; at t=0 too).

- **Transient sync hook** (`TranSync`, `lib/coretran.cpp`) and the command
  interpreter's pause mode (`simulator/cmd.h`, `setPauseOnStop(true)`): a
  caller can add a breakpoint and pause the transient at an accepted point,
  then resume it. The C interface uses this for `simulateUntil`.

- **C interface** (`cinterface/`, `libvacaskcinterface.so`). It provides
  `vacask_open`, `vacask_initialize`, `vacask_simulateUntil`,
  `vacask_simulationComplete`, `vacask_getTime` and `vacask_close`. These
  mirror Xyce's `libxycecinterface`. The library is built by default on Linux
  (`-DVACASK_CINTERFACE=ON`). Module and include paths come from
  `SIM_MODULE_PATH` and `SIM_INCLUDE_PATH`. `vacask_cosim_abi()` returns the
  co-simulation protocol version (2: veto and finish); nvc refuses an engine
  without it (`** Fatal: co-simulation ABI mismatch: ...`), because an older
  one reads a finish as an accepted step and never ends. nvc checks
  `cosim_bridge_abi()` in `libcosim_bridge.so` the same way.

nvc selects VACASK with `--vacask-netlist=<deck.sim>`. The boundary map is
given with `--cosim-config=<file.boundary>`, which is an alias of
`--xyce-config`. One mapping per line, `#` starts a comment:

```text
D2A|A2D <nvc path> <bridge name> [rise=<s>] [fall=<s>]
```

`rise=`/`fall=` set a D2A's ramp durations (default 1 ns each; values below
1 fs are clamped with a warning). Every new ramp, a reversal in mid-ramp
included, takes the full rise time when it goes up and the full fall time
when it goes down. A malformed line, an unknown column, a bad value, a
bridge name over 255 characters or a repeated bridge name is an error, and
the registry holds 8192 signals.

Every run ends with one of these nvc lines:

```text
** Note: co-simulation finished: digital stop at 0 s (before the first analog step)
** Note: co-simulation finished: digital stop at <t> s
** Note: co-simulation finished: analog end at <t> s
** Error: VACASK transient failed at <t> s
** Error: co-simulation stalled at <t> s
** Error: co-simulation interrupted at <t> s
```

`<t>` is a time on the digital's femtosecond clock, printed exactly (as
`%.15g` prints it whenever that is exact: `1.2e-07`, `0.002000000006`).
`<t>` of a digital stop is the digital's own stop time; the analog output
ends there, within 2 fs (the engine's time at the finish, rounded to the
digital's clock), or one step later when the stop came at the start of a
step. An interrupt (SIGINT) still lets the engine finish its output, but the
run fails: the interrupted line, exit status 130.

A D2A ramp shorter than the engine resolves at that time (1e-13 x t: 1 fs
from 10 ms on, 10 ps from 100 s on) takes that long instead, with a warning
naming the signal (`[cosim_bridge] warning: D2A '<name>': a <d> s ramp at
<t> s is shorter than VACASK resolves at that time; it takes <d'> s`).

## Demos

`run.sh min`: a digital square wave drives an RC through a D2A source.

`run.sh a2d`: an analog step goes through an A2D source to a VHDL threshold
process, and back out through a D2A source.

`run.sh glitch`: a D2A change arrives while the previous edge is still
ramping; the driven voltage must reverse from where the ramp is.

```sh
NVCB=/usr/local/src/nvc-build VCB=/opt/build.VACASK/Release ./run.sh a2d
```

The finish protocol, the ramps, the registry and the boundary parser have
their own tests (both engines) in sv2ghdl:
`tests/vamos/fixtures/ams/cside_engine/run_cside.py`.

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
- `COSIM_TRACE=1` traces the D2A changes, the vetoed and finishing steps
  (bridge) and every `simulateUntil` call, the scope tree and each boundary
  binding (nvc); `VACASK_COSIM_TRACE=1` traces them on the VACASK side.
  `options tran_debug=1` reports the vetoed points and a finish requested by
  the external sources, and `tran_debug=3` names the unknown that has the
  worst LTE.
