# SPICE / Sky130 PDK compatibility — status and open issues

Branch `rust-parser-integration`. Last revised **2026-08-24**, after rebasing onto
upstream `origin/main` (`03671b60`).

Baseline probe: all 33 device types in the Sky130 `tt` corner (`sky130.lib.spice`).
**33 pass, 0 fail.** No PDK edits kept — all fixes belong in VACASK.

| | PASS (33) | FAIL (0) |
|---|---|---|
| FETs | nfet_01v8, nfet_01v8_lvt, nfet_03v3_nvt, nfet_05v0_nvt, nfet_g5v0d10v5, nfet_g5v0d16v0(_base), pfet_01v8, pfet_01v8_hvt, pfet_01v8_lvt, pfet_g5v0d10v5, pfet_g5v0d16v0(_base), nfet_01v8_esd, esd_nfet_05v0_nvt, esd_nfet_g5v0d10v5, esd_pfet_g5v0d10v5, nfet_20v0(_iso/_nvt/_zvt), pfet_20v0 | — |
| Resistors | res_generic_nd/pd, res_high_po, res_generic_po, res_xhigh_po, res_iso_pw | — |
| Caps | cap_mim_m3_1/2 | — |
| BJTs | all 5 (3 npn, 2 pnp) | — |

The corner load needs `options unknownparam="warn"` (C1) for the `minr` cards.
A `.lib … section=tt` include plus the five 20 V FETs was verified today; the
other 28 are carried over from the previous revision, unchanged by this fix.

nfet_01v8 OP verified: `Id=0.4755 mA` (matches expected ~0.48 mA). DC sweep also works.

---

## Build prerequisite (changed 2026-08-17)

Upstream's `print device` reads OSDI 0.4 `module_flags` / `bound_step_offset`
(`lib/osdidevice.cpp:619`). Models compiled by an OpenVAF older than the
`module_flags` work (2026-06) **segfault** in `OsdiDevice::dump`, with no output.

Required: OpenVAF-reloaded `master` >= `0e83f1e` (reports
`OpenVAF-reloaded 20260616-3-g0e83f1e`). Not `branches/llvm18`, which lacks it.
After swapping the compiler, delete and rebuild every `.osdi` — a stale mix is
what produces the segfault, and an incremental CMake build will not notice.

Symptom of a stale build tree rather than a real regression: `test_behav*`,
`test_trannoise*`, `test_op` fail with SIGSEGV while a clean build of the same
commit passes.

## Test baseline

Clean build of this branch: **87/88 pass**, or 88/89 with the Sky130 PDK present.
The single failure, `test_pssosc2.sim`, also fails on a pristine `origin/main`
build — it is upstream's, not ours. All 32 SPICE/parser tests pass, plus the
Sky130 nfet E2E when the PDK is checked out.

---

## Open issues

Ordered by leverage. IDs are stable — reference them from commits and future sessions.

| ID | Issue | Where | Blocks |
|----|-------|-------|--------|
| ~~**A1**~~ | ~~Diode master dispatch keyed on `level`~~ | **DONE** — always `sp_diode` | — |
| ~~**A2**~~ | ~~B-sources warn-skipped instead of translated~~ | **DONE** — emits `PTBehavioral` | — |
| ~~**A3**~~ | ~~Expression-valued `r=` not translated~~ | **DONE** — emits `PTBehavioral` flow source | — |
| ~~**A4**~~ | ~~BJT master dispatch ignores `level`~~ | **DONE** — 0/1/2 → `sp_bjt` | — |
| ~~**C1**~~ | ~~Unknown model parameter is a hard error~~ | **DONE** — option `unknownparam` | — |
| ~~**A5**~~ | ~~Bin-guard condition uses subckt-scope `l`/`w`~~ | **DONE** — guard from the M-line's own `l`/`w` | — |
| ~~**C2**~~ | ~~`.model` and `.subckt` share one name scope~~ | **DONE** — `m_` prefix on `.model` names | — |
| ~~**C3**~~ | ~~`m=` not mapped to `$mfactor`~~ | **DONE** — adapter forwards `$mfactor` | — |
| ~~**A7**~~ | ~~`temper` not rewritten to `$temp`~~ | **DONE** — rewritten in every SPICE value | — |
| ~~**A8**~~ | ~~`pwr()` has no Verilog-A translation~~ | **DONE** — context-dependent rewrite + `sgn` translator | — |
| ~~**A6**~~ | ~~`version` ignored in BSIM4 dispatch~~ | **DONE** — selects and configures the BSIM4 master | — |
| ~~**P1**~~ | ~~`lang=` must precede `section=`~~ | **DONE** — include options are order-independent | — |

`A*` = adapter (`lib/netlistrs.cpp`), `C*` = core simulator, `P*` = parser/lexer.

---

### A1 — Diode master dispatch keyed on `level`  *(FIXED)*

Fixed: `spiceModelMaster` now returns `sp_diode` unconditionally for `mt == "d"`.
Regression test `test/test_spice_diode_params.sim` +
`test/spice_diode_params.cir`. Original writeup below, kept for the rationale.

`spiceModelMaster` routed `.model … D` to the **native** `diode` master unless an
explicit `level>0` is present, falling back to `sp_diode` only for levelled cards:

```cpp
if (mt == "d") {
    int dlevel = 0;
    try { dlevel = std::stoi(level_str); } catch (...) {}
    return (dlevel > 0) ? "sp_diode" : "diode";
}
```

Native `devices/diode.va` declares 18 parameters; `devices/spice/diode.va`
(`sp_diode`) declares 84. Any ordinary ngspice diode card therefore fails:

```
.model dmod d is=1e-14 n=1.5 ikf=1e-3 isr=1e-12 nr=2 cjo=1p tt=1n
→ Parameter 'ikf' not found.
```

Adding `level=1` makes the *identical* card simulate, because `sp_diode` has
`ikf`/`isr`/`nr`/…

**Fix:** an ngspice `.model … D` is an ngspice diode — always `sp_diode`. The
existing re-append of `level=` as a real model parameter (junction-cap selector,
`netlistrs.cpp:340`) stays. The native `diode` master remains reachable from
native `.sim` decks; it should not be a SPICE dispatch target.

The `sn` / `full` / default `sp_diode` variants differ only in noise model and
exposed output variables. Default (full noise, no implicit-equation-inducing
outputs) is the right target; no change needed.

### A2 — B-sources warn-skipped instead of translated  *(FIXED)*

Fixed: the `Behavioral` case in `addSpiceDevice` now emits a `PTBehavioral`.
Regression test `test/test_spice_bsource.sim` + `test/spice_bsource.cir` covers
`v=` and `i=`, `v(node)` / `v(a,b)` / `i(instance)` control, both power
spellings, `time`, and a B-source inside a `.subckt` driven by a subckt
parameter. `test/spice_warnonly.cir` was rewritten around `K` (mutual
inductor), which is still genuinely unsupported, so the warn-and-continue path
stays covered.

What the adapter does:

- `V=`/`I=` are picked out of the instance params case-insensitively; exactly
  one must be present (otherwise warn and skip). Any other B-line parameter
  (`tc1`, `tc2`, `noisy`, `dtemp`, `reciproctc`, …) has no behavioral-source
  counterpart and is named in a warning, not silently dropped.
- `currentSource = (I= was given)`.
- The expression goes through `spiceBehavioralExpr()` before
  `Parser::parseExpression`. Two ngspice spellings are rewritten:
  `^` → `**` (in VACASK `^` is bitwise XOR, which has no Verilog-A equivalent,
  so leaving it would fail rather than miscompute — but the right answer is
  known) and `time` → `$abstime` (available only inside behavioral source
  expressions, which is exactly where this lands). Identifiers inside a
  `v(...)`/`i(...)` argument list name nodes and instances, so the rewrite skips
  them. `temper` is deliberately left to A7, whose scope is all parameter
  expressions, not just B-sources; `hertz` has no VACASK equivalent.
- A parse failure sets `Status` and aborts the merge rather than warning —
  a B-source that cannot be translated is a real error, not a skippable device.

Still unsupported by construction, inherited from `rpnexprva`: ngspice's `%`,
`\`, `pwr`/`pwrs`/`u`/`uramp`/`sgn`/`if`, and vector/list literals. These now
fail with a translation error instead of being silently skipped, which is the
intended trade.

Original writeup below, kept for the rationale.

Upstream landed **behavioral sources** (`lib/rpnexprva.cpp`, 490 lines — an
RPN→Verilog-A translator; see `docs/dev-builtin-behavioral.md`). Syntax:

```
<name> (p n) v=<expr>      // potential-defining
<name> (p n) i=<expr>      // flow-defining
```

`v(node)`, `v(nodeA,nodeB)` and `i(instance)` inside the expression are recognised
and silently wired as extra terminals. The expression is compiled once into
`__behavioral_<name>` and instantiated as an ordinary OSDI device.

Our adapter still emits:

```cpp
case netlist::SpiceDeviceKind::Behavioral:
    Simulator::err() << "WARNING: SPICE device '" << name
                     << "' (Behavioral/B-source) has no VACASK equivalent; skipped\n";
```

That is now simply obsolete. The pieces needed are all in place:

- `Parser::parseExpression(std::string) -> Rpn` — `include/parser.h:38`
- `PTBehavioral(Id name, PTIdentifierList&& terms, Rpn&& expr, bool currentSource, …)` — `include/parseroutput.h:353`
- `PTBlock::add(PTBehavioral&&)` — `include/parseroutput.h:420`, and the same on `PTSubcircuitDefinition` (`:516`)

Work is: map ngspice `B… V=<expr>` → `currentSource=false`, `I=<expr>` →
`currentSource=true`, and translate ngspice expression spellings into VACASK's
expression language before `parseExpression`.

`test/spice_warnonly.cir` currently *asserts* the warn-and-skip behaviour and will
need rewriting into a real B-source E2E when this lands. Keep a warn-only test
alive for genuinely unsupported kinds (Switch, Osdi). *(Done — it now warns on
`K`, which needs no `.model` card and so leaves the divider assertion intact.)*

Note the limitations in `docs/dev-builtin-behavioral.md`: only real/integer/string
literals translate; vector and list literals are rejected.

### A3 — Expression-valued `r=` not translated  *(FIXED)*

Fixed: the Resistor case in `addSpiceDevice` now detects a resistance that
probes the solution and emits a `PTBehavioral` flow source instead of an
`sp_resistor` instance. Regression test `test/test_spice_behavioral_res.sim` +
`test/spice_behavioral_res.cir`. Original writeup below, kept for the rationale.

What the adapter does:

- `spiceExprHasProbe()` scans the effective resistance expression — the
  positional token, or `r=` when the positional token is a model name (the
  existing value-vs-model disambiguation, now hoisted into a `valIsModel` flag
  and shared by both paths) — for a `v(`/`i(` access call. Identifiers are
  scanned whole, so `vth`, `iref` and `div(...)` do not match.
- On a hit it emits `i = v(p,n)/max(<r>, 1e-12)` across the resistor's own two
  nodes. `1e-12` is the same too-small-resistance floor `sp_resistor` itself
  clamps to (`devices/spice/resistor.va:191`), so the non-behavioral and
  behavioral paths degenerate the same way instead of dividing by zero.
- Everything else on the card — a `.model` reference, `tc1`/`tc2`/`w`/`l` — has
  nowhere to go on a behavioral source and is named in a warning, not silently
  dropped.
- Expression translation and the parse-failure-is-fatal policy are shared with
  A2 through the new `addSpiceBehavioral()` helper, which the B-source case now
  also calls.

Limitation worth stating: `max()` clamps a *negative* resistance expression to
`+1e-12` rather than `-1e-12`, so a deliberately negative behavioral resistor
flips sign at the clamp. VACASK's expression language has no translatable
sign-preserving floor (`min`/`max` are the 2-argument translators available,
`rpnexprva.cpp:139-140`), and the PDK expressions are `abs()`-wrapped, so this
is accepted rather than worked around.

Verified against the real PDK: `sky130_fd_pr__res_xhigh_po` (w=0.35, l=10) now
solves an operating point — 13.6 µA at 1 V, i.e. ~73.5 kΩ, matching `rbody0` +
two ~110 Ω heads computed by hand. It needed C3 (`m=` on the parasitic-cap
X-calls) worked around to get there. `res_iso_pw` now reaches expression
translation and stops on A7 (`temper`). The five 20 V FETs reach it and stop on
A8 (`pwr()`).

Several PDK subckts define resistors whose `r=` references node voltages:

```
rldd d d1 r='abs((1/w)*(rdrift/(1+vgdep*(v(g,s)-vth-vbdep*v(b,s))))*...)'
```

A resistor `r=f(...)` between `p` and `n` becomes a behavioral **flow** source
`i = v(p,n)/f(...)`. Affects all 5 × 20V FETs + res_xhigh_po + res_iso_pw
(7 of the 15 failing devices).

Detection: the Resistor case must distinguish a numeric/parametric `r=` (keep the
current `sp_resistor` path) from one referencing `v(...)`/`i(...)` (route to
`PTBehavioral`).

**Why flow and not potential.** The obvious alternative — a potential source
`v = i_self * f`, which avoids the reciprocal — is *not expressible*.
`lib/rpnexprva.cpp:483` hardcodes the contribution as
`(currentSource ? flowAccess : potAccess) + "(br) <+ " + resultExpr`, and the
translated RPN can only yield node potentials, parameters/literals, or
`i(instance)` — which at `rpnexprva.cpp:257` becomes `V(__iN_inst)`, the potential
of *another* instance's internal flow node. Nothing produces the device's own
branch current, so the canonical Verilog-A idiom `V(br) <+ I(br)*f` cannot be
emitted. `i(<own name>)` does not substitute: control-node references are rebound
to *existing* nodes at elaboration, while `br` is internal to the module being
synthesized.

Two reasons the flow form is the right target regardless:

- `I(br) <+ V(br)/f` is an explicit conductance stamp — zero added unknowns.
  `V(br) <+ I(br)*f` is a potential-contributed branch, adding one unknown per
  instance.
- ngspice's behavioral resistor evaluates R per iteration and stamps 1/R, so the
  reciprocal is parity with the reference implementation.

**Division hazard, precisely.** For the Sky130 expression above, `r→∞` (denominator
`(1+vgdep*(…))→0`) is the *benign* direction — `i→0`. The hazard is `r→0`, which
requires that denominator to blow up, i.e. large gate overdrive during an NR
excursion. Mitigate inside the emitted expression: `max` is in the translatable
function list (2-argument form), so emit `i = v(p,n)/max(f, rmin)`. No new
machinery needed.

### A4 — BJT master dispatch ignores `level`  *(FIXED)*

Fixed: `spiceModelMaster` now dispatches `npn`/`pnp` on `level`, and
`{"sp_bjt", "spice/bjt.osdi"}` was added to `osdiFileForMaster()`. Regression
test `test/test_spice_bjt.sim` + `test/spice_bjt.cir`. Original writeup below,
kept for the rationale.

The mapping mirrors ngspice's `inpdomod.c:46-79`:

| level | master |
|---|---|
| absent / 0 / 1 / 2 | `sp_bjt` (Gummel-Poon) |
| 4 / 9 | `vbic13` |
| 8 | HICUM2 — not shipped; warn, fall back to `sp_bjt` |
| other | ngspice errors; we warn and fall back to `sp_bjt` |

An absent `level=` means level 1 in ngspice, so an ordinary unlevelled
`.model … NPN` card is Gummel-Poon — the same reasoning as A1's diodes.

**The substrate node needed no adapter work.** `sp_bjt(c, b, e, sub)` has 4
terminals and an ngspice `Q` card may name only 3, but an unconnected trailing
OSDI terminal already resolves to ground in VACASK, which is exactly ngspice's
rule (`inp2q.c:80-82` ties missing ports to `gnode`). Verified, not assumed: with
`iss` raised until the substrate junction dominates the bias point, `Q c b e` and
`Q c b e 0` give bit-identical operating points, both differing by >0.5 V from a
substrate tied to the emitter. The regression test asserts all three, so the
contract is pinned if `osdiinstance.cpp:447`'s "TODO: … resolve this with BJT and
BSIM3SOI models" is ever resolved the other way.

The test's discriminator for the dispatch itself is `bf`: it is a Gummel-Poon
parameter VBIC 1.3 does not declare at all (VBIC derives beta from `ibei`/`ibci`),
so before the fix the deck failed to elaborate with `Parameter 'bf' not found`.
Afterwards `Ic = bf*Ib` holds to 5 digits.

`vbic13` is still the 3-terminal `vbic13(c, b, e)`, so a level=4/9 card with a
substrate node is rejected for too many terminals until `vbic13_4t` is wired up.
That is unchanged by this fix and still deferred.

**Sky130 status: the 5 BJTs now stop on C1, not on dispatch.** A card carrying
the Sky130 shape (`tref=30 subs=1 tlev=0 is=… bf=…`) loads and solves; adding
back `dcap=2 gap1=0 gap2=0` fails with `Parameter 'dcap' not found`. Note `tref`
needs no adapter rename here — `devices/spice/bjt.va:91` really does declare
`aliasparam tref = tnom`, unlike `resistor.va`/`capacitor.va`.

### C1 — Unknown model parameter is a hard error  *(FIXED)*

Fixed by simulator option `unknownparam` (`error` | `warn` | `ignore`, default
`error`). Regression tests `test/test_spice_unknown_param{,_warn,_error}.sim`
share `test/spice_unknown_param.cir` and pin all three values: `ignore`
elaborates and leaves the operating point bit-identical to a card without the
undeclared parameters, `warn` elaborates and reports each one with its source
location, and the default is still a hard stop (`WILL_FAIL`). Documented in
`docs/cmd-options-params.md`. Original writeup below, kept for the rationale.

What landed:

- `Parameterized::UnknownParam { Error, Warn, Ignore }`
  (`include/parameterized.h`) and a trailing `UnknownParam` argument on the three
  `setParameters` overloads that device paths use. It trails `Status& s` rather
  than preceding it — against the file's convention, but the majority of callers
  (options, analysis parameters, sweeps, `alter`) have no policy to pass, and
  the alternative was `UnknownParam::Error` spelled out at a dozen call sites
  that do not care.
- `Parameterized::skipUnknownParameter()` does its own `parameterIndex()` lookup
  **before** calling `setParameter`, and only when the policy is relaxed. That is
  what confines the downgrade to a name miss: `Status` stores no error code
  (`lib/status.cpp` keeps a message and a bool), so a `NotFound` cannot be told
  apart from a type mismatch after the fact. It also means the `error` path is
  byte-for-byte what it was before. An expression parameter is skipped before it
  is evaluated, so a dropped parameter cannot fail the run through its
  expression either.
- Threaded from `circuit.simulatorOptions().core().unknownParameterPolicy()` at
  `osdidevice.cpp` (model), `osdimodel.cpp` (instance), `devbuiltin.h` (both),
  `hierdevice.cpp:241` (subcircuit instance), and both loops of
  `HierarchicalInstance::propagateParameters` — the last so that a parameter
  dropped at creation stays dropped instead of reappearing as an error when
  expressions are re-propagated.
- Classified in `parametrizationAffectingOptions` (`lib/options.cpp`), so
  changing the option re-parametrizes the hierarchy.
- Any value other than `warn`/`ignore` resolves to `Error`, so a typo in the
  option value cannot quietly disable the check. There is no options validation
  infrastructure to hook into (`lib/options.cpp:40` still says
  `TODO: options validation`), so failing strict is the substitute.

Not covered, deliberately: simulator options, analysis parameters, sweeps, and
`alter`. A name the user typed directly should not resolve to nothing quietly.

**Sky130 payoff: all 5 BJTs now solve an operating point.** Verified against the
real PDK by assembling the `tt` chain by hand (the `.lib` path is still blocked
by C2): `models_global` + `parameters_fet_tt` + `parameters_res_nom` +
`parameters_cap_nom` + `models_diodes` + `models_bjt`, with the corner's five
`.param` switches. At `Ib = 1 uA` the three npn devices sit at
`Vbe = 0.800 / 0.817 / 0.824 V` and the two pnp at `Veb = 0.752 / 0.818 V`.

Under `warn` the run reports exactly four names: `dcap`, `gap1`, `gap2`, and
`tsky130_fd_pr__res_generic_m1` / `_m2`. The last two are the **upstream PDK
bug** recorded further down (a botched search-and-replace for `trm1`/`trm2`), so
C1 absorbs that too — without it the PDK's own typo is an unconditional stop.

Machine-checked the whole card while there: extracting every `name=` from the
`.model` cards in `models_bjt.spice` and diffing against `bjt.va`'s
`parameter`/`aliasparam` declarations leaves only those five plus `level`, which
the adapter handles. Nothing else in a Sky130 bipolar card is undeclared.

**This supersedes the per-master strip table in the previous revision.** Rather
than curating a hardcoded list per master, make it an option.

Every path funnels through `Parameterized::setParameter(Id name, …)`
(`lib/parameterized.cpp:27`), which sets `Status::NotFound`; `setParameters` then
bails on the first failure. `Circuit::simulatorOptions()` (`include/circuit.h:473`)
is reachable from both `OsdiDevice::createModel` (`lib/osdidevice.cpp:55`) and
`OsdiModel::createInstance` (`lib/osdimodel.cpp:85`), so threading a policy down
is mechanical.

Sketch: add a `SimulatorOptions` member (e.g. `Id unknownparam`, values
`error` | `warn` | `ignore`, default `error`), register it in
`Introspection<SimulatorOptions>::setup()` (`lib/options.cpp:223`), and thread the
policy into `setParameters`.

Two things to get right:

- Downgrade **only** `NotFound` arising from name lookup. Evaluation failures and
  type mismatches must still be fatal.
- Classify the new option in `parametrizationAffectingOptionsChanged`
  (`include/circuit.h:503`) so rebuild detection stays correct.

Why an option beats the strip table: a curated list needs a code change per PDK; a
general warn-and-continue does not. It also matches ngspice, which prints
`unrecognized parameter (X) - ignored` and carries on.

**Caveat worth stating plainly:** this is a compatibility fallback, not a
correctness one. A silently dropped parameter that *does* matter gives wrong
numbers instead of a stop. Hence: default to `error`, and keep fixing aliasable
renames (`tref`, below) by name in the adapter rather than swallowing them.

Since A4 landed this is the **sole** remaining blocker for all 5 Sky130 BJTs:
their cards load and solve once `dcap`/`gap1`/`gap2` are removed by hand.

Parameters known to need it, and why they are safe to drop:

| master | parameter |
|---|---|
| `sp_bjt` | `dcap`, `gap1`, `gap2` |
| `sp_bsim4v8`, `sp_diode` | `minr` |

`minr` appears nowhere in any BSIM4 version (case-insensitive grep of the whole
tree), `dcap` would need depletion-cap equations ngspice does not contain, and
`gap1`/`gap2` feed Eg(T) only under `tlev=1` while every Sky130 BJT card sets
`tlev=0` — inert in these cards regardless.

### A5 — Bin-guard condition uses subckt-scope `l`/`w`  *(FIXED)*

Fixed, in two parts. Regression test `test/test_spice_binned_mos_indirect.sim` +
`test/spice_binned_mos_indirect.cir` covers both; the original
`test_spice_binned_mos` stays as the plain-`l={l}` case. Original writeup below,
kept for the rationale.

**Part 1 — guard from the M-line's own geometry.** `collectBinGeometry()` walks
the block's devices and records, per referenced model name, the `l=`/`w=`
expressions of the first M-line naming it; `emitBinnedModelGroup` substitutes
those (parenthesized) into the guard instead of bare `l`/`w`, falling back to
`l`/`w` when no M-line is found. This is what ngspice does — it bins on the
instance line's `l`/`w` (`inpgmod.c:288-291`), which need not be subcircuit
parameters at all. Only the *first* M-line per model counts: two M-lines with
different geometries would need one model per instance, which a single
@if-guarded definition cannot express. Not a real shape — a Sky130 FET subckt
has exactly one M-line.

**Part 2 — a card is a bin only if its name says so.** The ESD diagnosis in the
original writeup was wrong: all four ESD subckts *do* declare `.param l=1 w=1`
and write `l={l} w={w}`, so Part 1 changes nothing for them. The real cause is
that `isBinnedModel` keyed on the mere presence of `lmin`/`lmax`/`wmin`/`wmax`.
ngspice resolves an M-line's model reference by exact name first
(`inp2m.c:81`) and only falls through to the binning search on a miss; that
search accepts a candidate whose name is the reference plus a `.<digits>`
extension (`model_name_match`, `string.c:1015`). So `.model nshortesd_model`
with binning bounds is an ordinary card whose bounds are inert, while
`.model …_base.0` is a one-bin group. `binBaseName` now returns `nullopt`
without a `.<digits>` suffix and `isBinnedModel` requires one. The old
underscore spelling (`_N`) was dropped with it — ngspice has no such rule, and
no Sky130 `.model` name ends in `_<digits>`.

**Verified against the real PDK.** All four ESD FETs now solve an operating
point (`nfet_01v8_esd`, `esd_nfet_05v0_nvt`, `esd_nfet_g5v0d10v5`,
`esd_pfet_g5v0d10v5`). Note `esd_nfet_g5v0d10v5` is the one that keeps a `.0`
suffix, so it stays binned and needs an in-range geometry — `w >= 17.5u` — just
as it would in ngspice. The five 20 V FETs now get past model emission and stop
on A8 (`pwr()`), which is the next blocker, not this one.

**Known remaining deviation, deliberately not fixed here.** ngspice's `in_range`
(`inpgmod.c:234-239`) is `min <= v <= max` with a 1e-9 *absolute* tolerance at
both ends, despite its own comment claiming `min <= v < max`. Our guard is the
documented `>= lmin && < lmax`. Consequence: a geometry sitting exactly on a
group's outermost `lmax`/`wmax` binds in ngspice and does not here. Replicating
it would make Sky130's contiguous bins overlap at every boundary (their `lmax`
is the next bin's `lmin`), and the 1e-9 absolute slop is ~0.6% at a 1.6e-7
bound — it looks like a bug in ngspice, not a rule worth matching. Left as-is;
revisit if a real deck trips on it.

Original writeup below.

`emitBinnedModelGroup` (`netlistrs.cpp:389`) generates `@if l*$scale >= lmin …`
referencing subckt-scope `l`/`w`. The 20V subckts do not declare `l` — the
M-instance sets `l=hvnel_sky130_fd_pr__nfet_20v0` (an internal expression).

Same root cause for the ESD "Master not found" failures: those are single-bin
models where the condition never matched.

**Fix:** compile the bin-guard condition from the **actual `l`/`w` expressions on
the M-instance**, not from bare subckt-scope variables.

### C2 — `.model` and `.subckt` share one name scope  *(FIXED)*

Fixed adapter-side, as recommended below: every name originating from a SPICE
`.model` card is registered — and referenced — as `m_<name>`, unconditionally.
Regression test `test/test_spice_model_subckt_namespace.sim` +
`test/spice_model_subckt_namespace.cir` defines one name as both a `.model` and
a `.subckt` and instantiates both meanings in the same circuit, so it pins the
binding as well as the absence of the collision. Documented in
`docs/input-include.md`. Original writeup below, kept for the rationale.

What the adapter does:

- `spiceModelName()`/`spiceModelId()` (`lib/netlistrs.cpp`) prepend `m_` to the
  lowercased name. Applied at the one definition site (`buildSpiceModelCard`,
  which also covers the collapsed base name of a binned group) and at every
  model *reference*: R (both the explicit `mdl` and the value-is-a-model-name
  branch), C (likewise, including the `hasSpiceModel` probe that decides which
  branch to take), L, D, M, Q. **Not** X-calls, and **not** the masters
  `ensureSpiceModel` synthesizes (`sp_resistor`, `vsource`, …) — those are
  VACASK master names, not PDK identifiers.
- `collectBinGeometry` / `emitSpiceModels` keep working on raw SPICE names on
  both sides of their lookup, so binning needed no change.

**Cost, larger than the writeup below anticipated.** It is not only `print`/`save`
that sees the prefix: a **native** deck naming a model card defined in an
included SPICE file must spell it `m_<name>` too. Three existing tests did
exactly that and were updated — `test_lang_include.sim`, `test_lang_override.sim`
(`resr` → `m_resr`) and `test_spice_include_section.sim` (`rtt`/`rff`). That is
the user-visible behaviour change in this commit. Subcircuit names, and native
references to them, are untouched.

**Verified against the real PDK.** The tt chain with `models_fet.spice` omitted
(the 20 V FETs still stop on A8, and `models_fet` is included *before*
`models_diodes`, so a full-corner run never reaches the collisions) loads and
elaborates clean — all 9 collisions gone. Then, against that loaded chain, an
`Xsub … sky130_fd_pr__diode_pd2nw_11v0` and a
`Dmod … sky130_fd_pr__diode_pd2nw_11v0` in one deck both elaborate and solve,
i.e. the subckt and the model card coexist and each reference binds to the right
one. The full `section=tt` include now runs to A8 (`pwr()`) instead of stopping
at the first name clash.

**The diagnostic gap below was not closed.** `netlist::SpiceModel` carries only
`name`/`model_type`/`level`/`params` — no file or line — so there is nothing to
put in `PTModel::location()` without extending the Rust bridge's projection.
Moot for SPICE-origin names now that they cannot collide; still open if a native
deck collides with itself.

Original writeup below.

ngspice keeps them in separate namespaces; VACASK shares one, so a whole-corner
load fails at `lib/circuit.cpp:578`:

```
include ".../sky130.lib.spice" lang=ngspice section=tt
→ A model/subcircuit with name 'sky130_fd_pr__diode_pd2nw_11v0' already exists.
```

Both definitions really are in the `tt` chain:

```
continuous/models_diodes.spice:1185       .subckt  sky130_fd_pr__diode_pd2nw_11v0 a c mult=1
cells/diode_pd2nw_11v0/….model.spice:16   .model sky130_fd_pr__diode_pd2nw_11v0 d
```

the second via `corners/tt.spice:6` → `all.spice:56`.

**Scale: 9 collisions, not 3.** Intersecting every `.model` and `.subckt` name
across the PDK: `short`, plus 8 diodes (`diode_pd2nw_05v5{,_hvt,_lvt}`,
`diode_pd2nw_11v0`, `diode_pw2nd_05v5{,_lvt,_nvt}`, `diode_pw2nd_11v0`). The
earlier count of 3 was just what surfaced before the first error aborted the load.

**Mechanism.** A subckt definition becomes a `HierarchicalModel`
(`include/hierdevice.h:41`, `: public Model`), registered into `Circuit::modelMap`
at `lib/circuit.cpp:766`. An OSDI model card becomes an `OsdiModel`, registered
into the *same* map at `lib/osdidevice.cpp:48`. One
`std::unordered_map<Id,std::unique_ptr<Model>>` (`include/circuit.h:536`) — so the
second insert fails.

**A previous revision proposed "separate name scopes in `PTSubcircuitDefinition`".
That is the wrong layer.** `PTSubcircuitDefinition` is a parse-tree container;
scoping names there does not affect `Circuit::modelMap`, and elaboration would
flatten straight back into the same collision unless elaboration were also made
scope-aware.

**Options.** The disambiguating fact — subckt call vs device-with-model — exists in
the adapter (`SpiceDeviceKind::SubcktCall` vs `::Diode`) and is destroyed when we
emit a `PTInstance` carrying a bare master `Id`. Core never sees it.

- **Adapter-side `m_` prefix on model names (recommended).** Follow the convention
  already established by the Cadnip.jl converter
  (`SpiceArmyKnife.jl/src/Convert.jl:160`):

  ```julia
  :model_prefix => "m_",
  :ckt_prefix   => ""
  ```

  Applied **unconditionally** — every `.model` definition (`cg_spectre.jl:455`,
  and `:530` for the binned-group base name) and every model reference on an
  R/C/D/M/Q instance (`:604`, `:634`, `:664`, `:687`, `:713`, `:729`). Subckt
  names and X-call references stay bare.

  Preferred over collision-conditional mangling because it needs no collision
  detection at all, is predictable (`.model foo` is *always* `m_foo`, never
  contingent on what some other file defines), and keeps the direct-include path
  name-compatible with decks already converted through Cadnip / CedarSim.

  Zero core changes; sits alongside the case canonicalization and binning lowering
  already done in the adapter. Composes with binning as-is, since
  `emitBinnedModelGroup` already emits both the collapsed base name and the
  M-instance reference.

  Scope note: the prefix applies to names originating from `.model` cards only —
  **not** to the master/model names `ensureSpiceModel` synthesizes for builtins
  (`sp_resistor`, `vcvs`, …), which are VACASK master names, not PDK identifiers.

  Cost: `m_`-prefixed names appear in `print device(...)` and any `save`/`print`
  written against a model name — but uniformly, and matching what Cadnip users
  already see.
- **Split the map in core.** Correct and general, and would serve native decks too
  — but VACASK *deliberately* unifies "master" (a model and a subckt are both
  things an instance can name). Splitting it is a semantics change, needs a kind
  tag on `PTInstance`, and is upstream's call.

**Diagnostic gap, fix while in there:** `Circuit::add` prints "The existing
model/subcircuit was defined here" only when `mod->location()` is set.
Adapter-synthesized `PTModel`s carry no location, so this error currently comes out
as a bare one-liner with no way to find either definition.

This is what stops a plain "load the tt corner and go" workflow, so it gates any
realistic PDK use even once A1–A5 land.

### C3 — `m=` not mapped to `$mfactor`  *(FIXED)*

Fixed entirely in the adapter — **it is not a core issue**, despite the label it
carried in earlier revisions. Regression test `test/test_spice_mfactor.sim` +
`test/spice_mfactor.cir` pins all eleven cases below. Documented in
`docs/input-include.md`. Original writeup at the end.

**ngspice does this as a source-to-source rewrite, and so do we.**
`inp_fix_subckt_multiplier` (`inpcom.c:4118`) is the whole mechanism: when an
X-line carries `m=`, ngspice appends `m=1` to the called `.subckt` line and
`m={m}` to every device line inside it. There is no subcircuit-multiplier
concept in the ngspice solver at all. VACASK's `$mfactor` is upstream's
(`docs/cir-mfactor.md`, `edeaea0a`), and its "Passing `$mfactor` through
subcircuits" section says a subcircuit must declare `$mfactor` and forward it by
hand. So the fix is to write that forwarding from the adapter:

- Every SPICE `.subckt` gains `$mfactor=1` in its parameter list
  (`fillSpiceSubDef`).
- Every device inside receives `$mfactor=<inherited>` — or
  `$mfactor=(<inherited>)*(<own m>)` when the line has its own `m=`
  (`spiceMfactorExpr`, `paramsWithMfactor`). `m` itself is stripped: it is not a
  parameter of any VACASK master.
- An X-call is a device like any other, so nesting composes for free.
- At the top level `<inherited>` is empty, so a deck with no `m=` anywhere emits
  no `$mfactor` at all — including on X-calls, which keeps a SPICE block calling
  a *native* subcircuit (which would not declare `$mfactor`) working.

**Which devices take the multiplier is ngspice's list, not a guess.**
`inp_fix_subckt_multiplier` skips lines starting with `*vehaknopstuwy` and skips
a `B` source whose expression is `v=`. The common thread is that all of them
impose a *potential*: replicating an ideal voltage source in parallel changes
neither the imposed voltage nor any node current, only the per-instance branch
current that gets reported. So V/E/H and `B … v=` get none, and an explicit `m=`
on such a line is dropped with a warning. Everything else does — including
`isource`, F and G, which are flow-defining.

**Behavioral sources needed their own answer.** `PTBehavioral`
(`include/parseroutput.h:350`) carries no parameter list — the grammar rejects
anything but `v`/`i`/`potential`/`flow`/`discipline` (`dflparser.y:943`) — so
there is nowhere to put `$mfactor` on one. But a *flow* contribution scales
linearly, so the multiplier is folded into the expression instead:
`i = (v(p,n)/max(r,1e-12))*($mfactor)`. Verified empirically before relying on
it: `docs/dev-builtin-behavioral.md:100`'s "`$mfactor` is not supported for
behavioral sources" refers to the instance parameter, not the identifier — a
behavioral expression may reference an enclosing subcircuit's `$mfactor`
parameter like any other free identifier, and it resolves correctly. This is
what makes the multiplier reach A3's behavioral resistors, i.e. Sky130's
`rldd … m={m}`.

**Two deliberate departures from ngspice, both stated in the code comment.**

- *Unconditional.* ngspice rewrites only a `.subckt` that some X-line actually
  multiplies. Sky130 calls its parasitic subcircuits from a different file than
  the one defining them (`models_resistors.spice:188` calls a subckt defined in
  `models_capacitors.spice:135`) and this adapter resolves includes lazily, so
  "is this subcircuit ever multiplied?" is not knowable when the call site is
  translated. A `$mfactor` that stays 1 costs one instance parameter and
  nothing else.
- *An X-line's `m=` always becomes the multiplier*, even when the called
  subcircuit declares a parameter of its own named `m` — the case in which
  ngspice hands the value down as that parameter and does **not** multiply.
  Both routes land on the same answer for the only shape this occurs in,
  Sky130's 20 V FETs, which declare `m=1` and forward it by hand to their
  devices: ngspice reaches those devices through the parameter, we reach the
  same devices through the multiplier. They differ only if a subcircuit uses `m`
  for something that is not device multiplicity, which nothing in Sky130 does.
  The regression test pins this shape (`dblk`).

**Verified against the real PDK.** `sky130_fd_pr__res_xhigh_po` (w=0.35, l=10)
solves an operating point at 13.597 uA / 1 V — the same number A3 recorded, but
now **without the hand-editing of the `m=0.5` X-calls that A3 needed**. Dumping
the parasitic instance confirms the value actually arrives rather than merely
being accepted:

```
Osdi device instance x1:xc0:c0 of model x1:xc0:sp_capacitor
    $mfactor = 0.5 (real)
```

The five 20 V FETs still stop at A8 (`pwr()`), unchanged — but `rldd`'s
dropped-parameter warning no longer names `m`, so the multiplier is now consumed
rather than discarded there too.

Test baseline after the fix: **85/86**, the single failure still the upstream
`test_pssosc2.sim`.

Not fixed, and worth stating: `models_capacitors.spice` references an undefined
`cp1f`/`cp1fsw` (defined only under `combined_models/rescap/`, which the `tt`
chain does not include). That is a separate, pre-existing PDK/eager-evaluation
issue; the verification above supplies them from `rescap/res_typical__cap_typical.spice`.

Original writeup below.

`m` is ngspice's instance multiplier on M/D/Q instances and X-calls. The
vadistiller replaced `m` with the Verilog-A builtin `$mfactor` (correct — `m` is
not a model parameter). VACASK should set `$mfactor` from an instance's `m=`.
Affects every device type using `m=` (18 subckt calls + any M/D/Q instance).

### A7 — `temper` not translated  *(FIXED)*

Fixed in the adapter: `temper` → `$temp`, in every expression a SPICE block
contributes. Regression tests `test/test_spice_temper.sim` +
`test/spice_temper.cir` (seven shapes, each checked at 27 °C **and** 127 °C so
the `$temp` re-evaluation is pinned as well as the value) and
`test/test_spice_temper_shadow.sim` + `test/spice_temper_shadow.cir` (the
declared-`temper` deviation below). Documented in `docs/input-include.md`.
Original writeup at the end.

What landed:

- The B-source expression rewriter grew a flag and became the single SPICE
  expression rewriter, `spiceExpr(in, behavioral)`: `temper` → `$temp` always,
  `time` → `$abstime` and `^` → `**` only for behavioral sources. It already
  scanned whole identifiers and skipped `v(...)`/`i(...)` argument lists (those
  name nodes and instances), so `temperature`, `mytemper` and a node named
  `temper` all survive for free. Matching is case-insensitive — ngspice writes
  `TEMPER` freely, and at the point a value is read the adapter has not
  lowercased it yet.
- `spiceValue(v)` = `spiceExpr(stripExprQuoting(v), false)` replaced
  `stripExprQuoting` at every SPICE-origin value site: `.param` cards and
  `.subckt` parameter lists (`paramString`'s new `spiceValues` flag), device
  instance params, `.model` card params, `spiceParamValue`, the positional
  value token, source function arguments, and B-source `v=`/`i=`. Native and
  Spectre-origin values still use `stripExprQuoting` alone: a native deck may
  legitimately define its own `temper`, and Spectre is out of scope.
  Applying it at the *value* level rather than to the assembled
  `name=value …` string is what keeps a parameter *named* `temper` from being
  rewritten into `$temp=`.
- Because the behavioral path reads its expression through `spiceValue` too, the
  behavioral rewrite runs over an already-rewritten string. That is harmless
  (`$temp` is not `temper`) and keeps one code path instead of two.

**Deliberate deviation: the rewrite is unconditional.** `$temp` is shadowable by
a same-named parameter, so the ideal is to defer to a deck that declares its own
`temper` — but the declared-name set is known only in `spiceBlockToTables` /
`fillSpiceSubDef`, and honouring it would mean threading a scope-aware name set
through every param-string builder in the adapter (~15 signatures) for a name
ngspice itself reserves. Instead, those two functions call
`warnIfTemperDeclared()`, so a deck declaring `temper` gets a warning naming the
scope and saying the declaration has no effect on expressions that use it. The
alternative — silently substituting the simulator temperature — is the failure
mode C1's caveat warns about.

**Rejected alternative: declare `temper=$temp` as a parameter** in every SPICE
scope, the way C3 declares `$mfactor=1`. It would make shadowing automatic, but
a top-level SPICE block is merged into the top-level definition once *per
included file*, so the whole Sky130 chain would declare `temper` dozens of times
over.

**Verified against the real PDK: `res_iso_pw` now solves, and it is the last
device A7 was blocking.** `sky130_fd_pr__res_iso_pw` (l=10, w=2.65) — whose
behavioral `r=` carries `(1+vvt*(temper-tref)+ut*(temper-tref)^2)` — gives
14987.3 Ω at 27 °C and 21365.9 Ω at 127 °C, against 14989.3 / 21369.0 computed
by hand from `sw_pw_rs=3816`; the residual is the `av`/`bv` bias terms at the
10 mV probe bias. The 27→127 °C ratio matches the analytic tempco factor
1.425618 to six digits, which is the part A7 is actually about. The five 20 V
FETs now translate their `vth`/`rdrift`/`hvvsat` tempco `.param`s and stop on A8
(`pwr()`), as they did before this fix — `temper` was never their blocker, only
`res_iso_pw`'s.

Original writeup below.

ngspice's simulation-temperature variable (°C), used in 20V device `.param`
expressions.

**No core change needed.** VACASK already exposes `$temp` — ambient temperature in
°C, backed by option `temp`, available in all parameter expressions, and
re-evaluated automatically when `temp` changes (`docs/expr-special.md`). Same
quantity, same units as ngspice's `temper`.

So this is a pure adapter rewrite: `temper` → `$temp`. A previous revision
proposed "expose `temper` aliasing `opt.temp`", i.e. building something that
already exists.

Implementation notes:
- Match on identifier boundaries, not substrings — `temperature`, `mytemper` must
  survive.
- Match case-insensitively; ngspice writes `TEMPER` freely and the adapter already
  canonicalizes SPICE identifier case.
- `$temp` is shadowable by a same-named parameter or circuit variable, so if a deck
  defines its own `temper`, defer to it rather than rewriting.

### A6 — `version` ignored in BSIM4 dispatch  *(FIXED)*

Fixed: BSIM4 `version` now participates in level 54 dispatch. An absent selector
uses the native `bsim4` master; an explicit selector uses the ngspice-compatible
`sp_bsim4v8` master. Supported 4.8.x numeric selectors are re-emitted as strings,
matching the OSDI parameter type instead of failing elaboration.

Sky130's legacy `version=4.5` and `version=4.62` cards keep the established
`sp_bsim4v8` 4.8.3 fallback. Other unsupported selectors produce a warning before
using that fallback, so a typo cannot silently change equations. Regression test
`test/test_spice_bsim4_version.sim` + `test/spice_bsim4_version.cir` pins both
masters, supported-version propagation through model introspection, both Sky130
fallback selectors, and the unknown-version warning. The real Sky130 nfet include
test remains passing.

### A8 — `pwr()` has no Verilog-A translation  *(FIXED)*

Fixed: `spiceExpr()` expands `pwr()` in every SPICE expression, differently in
the two contexts ngspice itself distinguishes. Regression test
`test/test_spice_pwr.sim` + `test/spice_pwr.cir` (nine shapes: both contexts,
positive/negative bases, integer and fractional exponents, nesting, the
identifier guard, and `pwr()` inside a behavioral resistance). Original writeup
at the end.

**The previous writeup's premise was half wrong, and the half that was wrong is
the half Sky130 needs.** `pwr` is not one function in ngspice — it is two,
selected by which expression parser reads it:

| context | ngspice implementation | meaning |
|---|---|---|
| `.param` / `.model` values | `frontend/numparam/xpressn.c:134`, `XFU_PWR` → `pow(fabs(z), x)` | `\|x\|**y` |
| behavioral (B-source, `r=`) | `spicelib/parser/ptfuncs.c:130`, `PTpwr` → `a<0 ? -pow(-a,b) : pow(a,b)` | `sgn(x)*\|x\|**y` |

Both verified against `ngspice-45.2` sources *and* by running it: in `.param`,
`pwr(-2,3)` is `+8` and `pwr(-4,0.5)` is `+2`; in a B-source, `pwr(-2,0.5)` is
`-1.41421`. VACASK's `sgn()` is `(x>=0) ? 1 : -1` (`rpnfunctor.h` `FwSgn`),
which is `PTpwr`'s zero-counts-as-positive convention exactly.

So the rewrite is `pow(abs(x), y)` for ordinary values and
`sgn(x)*pow(abs(x), y)` for behavioral ones. Both duplicate `x`; SPICE
expressions are side-effect free, so that costs only text.

**`pwrs` is not implemented, on purpose: ngspice does not have it.** It exists
solely as a PSPICE-compatibility `.func` injected by `frontend/inpcompat.c:699`
(`.func pwrs(x, a) { sgn(x) * pow(x, a) }` — note the *signed* `pow`, and note
that the same block redefines `pwr` as plain `pow` too, so PSPICE mode disagrees
with both native meanings). Stock ngspice rejects `pwrs` outright — verified:
`Error: no such function 'pwrs'`. This adapter does not implement PSPICE
compatibility mode, so `pwrs` is left alone and fails as an unknown function,
which is parity. It appears nowhere in Sky130 (0 occurrences; `pwr` has 19).

**Reading order matters, and it bit.** Every SPICE value already flowed through
`spiceValue()` → `spiceExpr(…, behavioral=false)`, and the behavioral paths then
ran `spiceExpr(…, behavioral=true)` over that *already rewritten* string — which
A7 explicitly recorded as harmless. It is no longer: the first pass consumes the
`pwr` call, so a behavioral expression would silently get the `.param` meaning.
`spiceValue()`/`spiceParamValue()` grew a `behavioral` flag and the two
behavioral sites (the A3 resistance, the B-source `v=`/`i=`) now read their
expression once, with the right flag. The A3 resistance is still read
non-behaviorally first for the `spiceExprHasProbe()` test, then re-read.

**One core change: `sgn` (arity 1) added to `vaFuncMap`** (`lib/rpnexprva.cpp`).
Verilog-A has no sign function, so the translator emits the conditional
`((x)>=0.0 ? 1.0 : -1.0)`; the ternary operator was already translatable
(`rpnexprva.cpp` `OpQuestion`). This is what lets the behavioral form of `pwr`
exist at all, and it also makes a plain `sgn()` usable in any behavioral source —
one item off the `rpnexprva` gap list A2 recorded.

**Implementation note.** `spiceExpr()`'s single-pass scanner grew a frame stack
(one entry per open paren) in place of the `vector<bool>` access flags. A
`pwr(` frame emits none of its own text; the whole call is rewritten in place at
its closing paren, when both arguments are known. Nesting composes because an
inner call closes first and only ever rewrites text past the outer frame's
recorded argument-start and comma offsets. A `pwr` that is not the 2-argument
form ngspice defines is put back verbatim for the parser to reject, and a `pwr`
inside a `v(...)`/`i(...)` argument list is a node name and is left alone.

**Verified against the real PDK: all five 20 V FETs now solve an operating
point** from the full `include … lang=ngspice section=tt` chain (plus
`unknownparam="warn"` for `minr`, C1). At `Vgs = Vds = 5 V`:
`nfet_20v0` 7.85 mA, `_iso` 8.05 mA, `_nvt` 7.95 mA, `_zvt` 9.92 mA (w = 60 µm),
and `pfet_20v0` 8.31 mA at `Vsg = Vsd = 5 V` (w = 50 µm) — the zero-Vt variant
drawing the most current, as it should. The generated behavioral module carries
exactly five `pow(abs(` sites, one per `rldd`. `_iso` has five terminals
(`d g s b sub`); leaving `sub` floating makes the operating point singular, which
is a deck error, not an adapter one.

This closes the last of the five failing device types. Original writeup below,
kept for the rationale.

Surfaced by A3. `sky130_fd_pr__nfet_20v0`'s behavioral `rldd` uses ngspice's
`pwr()`:

```
rldd d d1 r='abs((1/w)*(rdrift/(1+…))*(1+pwr((abs(v(d,s)+…)/(…)),avsat)))'
```

which now reaches the translator and fails cleanly with
`Function pwr() with arity 2 cannot be translated to Verilog-A.`
This is the trade A2 recorded — the `rpnexprva` gap list (`%`, `\`, `pwr`,
`pwrs`, `u`, `uramp`, `sgn`, `if`) fails loudly rather than skipping.

`pwr` and `pwrs` are the two that actually appear in Sky130 and both have exact
rewrites: ngspice defines `pwr(x,y) = |x|**y` and `pwrs(x,y) = sgn(x)*|x|**y`.
So `pwr` is a pure adapter rewrite in `spiceBehavioralExpr()` (needs
paren-matching + a depth-1 comma split to reach the two arguments);
`pwrs` additionally needs a `sgn` equivalent, which is not in the translator's
function table either. Doing `pwr` alone unblocks the 20 V FETs.

Deliberately *not* done as part of A3: A2 recorded failing loudly on these as
the intended behaviour, so reversing that is a separate decision.

### P1 — `lang=` must precede `section=`  *(FIXED)*

Fixed: `lang=` now pushes its value state from either `INCEND` or `LIBEND`, and
the value rule returns to the state that requested it. Therefore both option
orders below preserve the selected language and library section. Regression
coverage in `test/test_spice_include_section.sim` loads sections using each order
and checks their combined result. Existing behavior is unchanged for repeats:
the last `lang=` wins, while a repeated `section=` is rejected.

```
include "x.lib" section=tt lang=ngspice
include "x.lib" lang=ngspice section=tt
```

---

## Resolved / superseded since the previous revision

**Resistor/capacitor `tref` — the VADistiller patch was REJECTED upstream.**
The previous revision recorded `aliasparam tref = tnom` as *done* in
`devices/spice/resistor.va` / `capacitor.va`, and concluded the adapter hack was
"redundant but harmless; removing it is cleanup". **Both conclusions are now
wrong.** Confirmed against the current distilled sources: `spice/resistor.va` has
`aliasparam` entries for `r`, `tc1`, `noise`, `dw`, `dlr`, `tc1r`, `tc2r`,
`model_w`, `res` — and **nothing for `tref`**.

The adapter rename at `netlistrs.cpp:333-346` (strip `tref`, re-append as `tnom=`)
is therefore **load-bearing and must stay**. Verified working: a Sky130-style
`.model … r tc1=… tc2=… rsh=… dw=… tref=25` card simulates correctly today.

The reasoning for why `tref` deserved a fix at all is still sound and worth
keeping for the record — see below.

**"Behavioral resistor `v(node)`" is no longer a wait on upstream.** Reclassified
as A3, downstream of A2, now that `rpnexprva` has landed.

**Per-master unknown-parameter strip table** superseded by C1.

### Why `tref` was in scope and `minr`/`dcap` are not

ngspice has no explicit accept-list; anything missing from a parameter table hits
the catch-all at `inpgmod.c:163-178`. Verified against `ngspice-45.2`, `tref` on
`r`/`c` draws the *same* complaint as `minr` and `dcap`:

```
Warning: Model issue on line 2 :
  .model myres r tref=30 tc1=1e-3 rsh=100 ...
unrecognized parameter (tref) - ignored
```

(Only `level` and `m` are ignored by name, `inpgmod.c:139-145`. The warning fires
only for *instantiated* model cards, which is plausibly why this went unnoticed.)

So the dividing line is not ngspice's parser but **whether correct behaviour is
knowable**. `tref` is an unambiguous rename of a quantity the same model already
implements — `res.c:61` and `cap.c:54` both declare `IOPXU("tnom", …)`, and
`dio.c:56` spells out that this exact alias is what `tref` means. Nothing to
invent. The others would require inventing equations or a clamp site.

Consequence worth stating plainly: stock ngspice **silently mis-simulates Sky130
resistors and capacitors**, evaluating them at `tnom=27` and computing `tc1`/`tc2`
drift from the wrong reference temperature. Handling the alias makes VACASK
deliberately *more correct* than ngspice — it is not parity with it. With the
VADistiller patch rejected, the adapter is where that correctness now lives.

Note this is also why VADistiller's own validator cannot cover `tref`: it runs the
*same* deck through ngspice and OSDI, so a deck setting `tref` would fail by
construction (ngspice ignores it, OSDI honours it). Any deliberate deviation from
ngspice must be checked VACASK-side.

## Verified gap list

Machine-checked by extracting every `name=` token from the Sky130 `.model` cards
and diffing against the `parameter`/`aliasparam` declarations in each generated
`.va`:

| `.va` | Missing | Disposition |
|---|---|---|
| `resistor.va`, `capacitor.va` | `tref` | adapter rename, `netlistrs.cpp:333-346` (VADistiller patch rejected) |
| `bjt.va` | `dcap`, `gap1`, `gap2` | C1 — `options unknownparam="warn"` |
| `bsim4v8.va`, `diode.va` | `minr` | C1 — `options unknownparam="warn"` |

`subs` and `cjs` are **not** missing — both are already in `bjt.va` (`bjt.va:89`,
`bjt.va:134-136`, including the `csub`/`ccs` aliases). A separate check confirms
every `IF_REDUNDANT` alias in ngspice's `res`/`cap`/`dio`/`bjt`/`bsim3`/`bsim4`
parameter tables already reaches the generated `.va`, so the alias pipeline is
sound — `tref` was a gap in ngspice, not in VADistiller.

Note on `tc1`: `sp_resistor` declares `aliasparam tc1 = tc`, i.e. an ngspice
`.model … R tc1=` sets the *instance-default* `tc`, not the model-level
`model_tc1` (whose alias is `tc1r`). This currently gives the intended behaviour
because VACASK lets a model card seed instance-parameter defaults, but it is a
coincidence worth remembering if the temperature path is ever revisited.

## Upstream VADistiller bug — `ikf`/`ikr`/`ikp` inert in `sp_diode`

Found while writing the A1 regression test. `devices/spice/diode.va` declares
`DIOforwardKneeCurrentGiven`, `DIOreverseKneeCurrentGiven` and
`DIOforwardSWKneeCurrentGiven` (`:235-237`), initialises all three to `0`, and
reads them at `:512`, `:518`, `:524`, `:915`, `:921`, `:927` — but **never assigns
`1`** to any of them. Compare `DIOtempGiven` / `DIOnomTempGiven` /
`DIOmetalOxideThickGiven` / `DIOpolyOxideThickGiven`, which do get set
(`:472`, `:487`, `:497`, `:501`).

Consequence: high-injection knee current is silently disabled for every
`sp_diode` instance, whatever `ikf=` says. Verified empirically — two otherwise
identical cards differing only by `ikf=1e-6` at ~0.43 mA bias give bit-identical
operating points (`v = 0.6336802179081024` both).

Sky130's `sky130_fd_pr__diode_*` cards do not set `ikf`, so this does not affect
the corner probe. Fix belongs in VADistiller's `$param_given` lowering, not in
VACASK. Contrast `isr`, which is correctly guarded by `$param_given(isr)`
(`:855`, `:870`, `:886`) and does take effect.

## Upstream PDK bug

`models_bjt.spice:84,167,224` emit `tsky130_fd_pr__res_generic_m1` where `trm1`
was intended — a botched search-and-replace in the combined-models generator.

## Already handled (no action needed)

- Diode `tref` → OSDI `aliasparam tref = tnom` in `diode.va`
- BSIM4 `tnom` → correct name already, `$simparam("tnom")` runtime path works
- Controlled sources E/F/G/H → mapped to builtin `vcvs`/`vccs`/`cccs`/`ccvs`
  (`netlistrs.cpp:744-841`). These builtins predate the rebase; the *new* upstream
  capability is behavioral sources (A2), not these.

## Reproduction

```
include ".../combined_models/sky130.lib.spice" lang=ngspice section=tt
```

Note the `lang=` before `section=` ordering (P1).
