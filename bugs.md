# Audit of feature/acsmsig-solve

Scope: `main...HEAD` (7 commits, `opsolve` parameter for small-signal analyses).
Verified with the Debug binary: `test_opsolve.sim` passes, plus ad-hoc netlists (missing solution, `store` no-op, acsp after a bias change).

## Bug

- [x] 1. `writeop=1` with `opsolve=0` writes an empty op file
  - Fixed with option B: `OperatingPointCore::evaluate()` now writes a point, so the op file always has one point per analysis point.
  - Docs updated (7 analysis pages, `cmd-analysis-op.md`, `internals/anop.md`).
  - Tested by `test/test_opsolvewrite.sim` (ac, dcinc, acsp, acstb, and a sweep where `opsolve` changes between points). Verified on the Debug build only.
  - `ac2.op.raw` is created with `No. Points: 0` and a header only.
  - `OperatingPointCore::initializeOutputs()` (`lib/coreop.cpp:86`) opens the file and `finalizeOutputs()` (`lib/coreop.cpp:104`) closes it, but `OperatingPointCore::evaluate()` (`lib/coreop.cpp:408`) never adds a point.
  - The docs for ac, acsp, acstb, acxf, noise, dcinc, dcxf and `cmd-analysis-op.md` say `writeop` has no effect and no op results are written. A zero-point raw file may also fail to load in rawread.
  - Fix option A: skip file creation in `initializeOutputs()` when `!params.solve` (smaller change, docs become true).
  - Fix option B: write the evaluated point in `evaluate()` (then update the docs).

## Test coverage

- [x] 2. acsp and acstb are not covered by `test/test_opsolve.sim`
  - Fixed: both added to `test_opsolve.sim`, with a series probe `vp` in the netlist for acstb. The circuit has no feedback loop, so `wf`, `wr` and `w` are rounding noise, and the y-parameters carry the check.
  - The docs claim both support `opsolve=0`.
  - acsp was checked by hand: with the source moved to 0.4 V, `opsolve=0` matched the 0.8 V reference and differed from a re-solve.
  - acstb is unverified (same code pattern, `lib/coreacstb.cpp:265`).
  - Add both to the test, or at least acstb.

- [x] 3. List-form `nodeset` with `opsolve=0` is untested
  - Not a bug. Checked by hand: `nodeset={ "3", 0.5 } opsolve=0` fails with "Nodeset must be a string when the operating point is not solved." (`OpSolveNodesetType`, `lib/coreop.cpp:170`). No automated test.

## Limitations to decide on (not bugs)

- [x] 4. Stored solution is read at `rebuild()`, not at run time
  - Not an issue. `store` is written when the analysis finishes (`finalizeOutputs()`), and a sweep belongs to a single analysis, so no slot is re-stored between sweep points. Checked with a sweep: the slot held the last point.
  - Documented in the "Stored solutions" section of `docs/cmd-analysis-overview.md` (op and small-signal analyses only, PSS not checked).

- [x] 5. Unknowns missing from the stored solution are evaluated at 0 silently
  - Fixed for op and HB: `OperatingPointCore::evaluate()` and `HBCore::evaluate()` check that every unknown has a value. Missing unknown is an error with `strictforce`, otherwise a warning and it is evaluated at 0.
  - Tested by hand (extra node 5 not in the stored solution): error and warning both fire for op and HB. Regression tests pass. No automated test yet, docs not updated yet.

## Housekeeping

- [x] 6. `.vscode/launch.json` is committed with local debug-target changes (active target is now `test_hbsolvewrite.sim`, `test_hbnoise3.sim` is commented out).
  - Harmless, left as is.
- [x] 7. `test/test_ac.sim` and `test/test_dcinc.sim` have whitespace-only diffs.
  - Harmless, left as is.
- [x] 8. `docs/cmd-analysis-op.md` says the solution slot name is given by the `write` parameter; it should say `store`. This predates the branch but sits next to the new paragraph.
  - Fixed: it now says `store`. No other store/write mismatch found in the docs.
- [x] 9. `OperatingPointCore::evaluate(bool atNodeset, ...)` has no caller that passes `false`.
  - Not a bug. The parameter is kept on purpose, reserved for future use.
