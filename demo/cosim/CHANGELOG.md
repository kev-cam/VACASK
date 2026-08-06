# Cosim bridge changelog

Chronological log of changes to the bridge itself (`cosim_core.cpp`,
`cosim_vpi_adapter.cpp`). For the bridge's design, see
`docs/cosim-architecture.md`; this file just tracks what changed and why.

## 2026-08-06

All five changes below came out of wiring a real production testbench
(GENESIS_AA) through the bridge for the first time — every qualification
test up to this point used hand-written, well-behaved netlists, so none
of these gaps had surfaced yet.

### 1. `$cosim_a2d` rejected bit-selects of vector regs

**File:** `cosim_vpi_adapter.cpp`, `a2d_compiletf`.

`reg [7:0] ui_in; ... $cosim_a2d("node", ui_in[3], ...)` failed with
`must be a reg, not a net`. A bit-select of a vector reg shows up over
VPI as `vpiPartSelect`, not `vpiRegBit` (`vpiRegBit` is only for bits of
a reg *memory* array, e.g. `reg mem[0:7]`). The check now also accepts
`vpiPartSelect` when its parent (`vpi_handle(vpiParent, ...)`) is a real
`vpiReg`/`vpiIntegerVar` — i.e. a slice of writable procedural storage,
not of a net (which `vpi_put_value` can't safely drive, hence the
original check existed at all).

### 2. No way to point the bridge at PDK model/module directories

**File:** `cosim_core.cpp`, `Core::start()`.

The bridge is deliberately PDK-agnostic, but a project netlist that
pulls in PDK libraries via bare filenames (`include
"sg13g2_vacask_common.lib"`, `load "mosvar.osdi"`) needs those
directories on VACASK's include/module search paths — normally supplied
by the `vacask` CLI wrapper's `.vacaskrc.toml` (`[Paths]
include_path_prefix` / `module_path_prefix`), which the bridge never
loads since it drives VACASK's API directly.

Added two optional environment variables, colon-separated like `$PATH`:
`VACASK_INCLUDE_PATH` and `VACASK_MODULE_PATH`. If set, their contents
are appended via `Simulator::appendIncludePath()` /
`appendModulePath()` right after `Simulator::setup()`. No PDK-specific
knowledge added to the bridge itself — the caller supplies the paths.

### 3. A2D/D2A validation stopped at the first missing node

**File:** `cosim_core.cpp`, `Core::start()`.

The node-existence check for `$cosim_a2d`/`$cosim_d2a` targets used to
`return false` on the first `findNode()` miss, hiding every other bad
binding until you fixed that one and re-ran. It now walks the full list,
reports every missing node, and only aborts after all of them have been
checked. Made diagnosing ~20 bad hierarchical paths at once tractable
instead of one-at-a-time.

### 4. Silent progress: no feedback until the run ends

**File:** `cosim_core.cpp`, `VacaskState`, `timestep_cb`, `Core::start()`.

The bridge runs a real transient silently — VACASK's own
`TranCore::install(ProgressReporter*)` exists but was never wired up.
Added `vs.tstop` and `vs.lastProgressPct` to `VacaskState`; `timestep_cb`
now prints `COSIM: progress N% (t=... / ... s)` whenever the integer
percentage advances (not every step, to avoid spam).

### 5. No wall-clock timing at the end of a run

**File:** `cosim_core.cpp`, `VacaskState`, `Core::start()`, `Core::pump()`.

Added `vs.startTime` (`std::chrono::steady_clock`), stamped right before
the first `pump()` call. The "transient complete" message now includes
elapsed wall time: `COSIM: transient complete, N yields, M crossings,
X.XXs wall`.

**Verification:** all five changes were checked against
`qualification/A1_ramp_crossing` (`check.py`, 6/6 tier-1/tier-2 checks)
after each edit — no regressions.
