# Audit of feature/acsmsig-solve

Scope: `main...HEAD` (7 commits, `opsolve` parameter for small-signal analyses).
Verified with the Debug binary: `test_opsolve.sim` passes, plus ad-hoc netlists (missing solution, `store` no-op, acsp after a bias change).

## Bug

- [ ] 1. `writeop=1` with `opsolve=0` writes an empty op file
  - `ac2.op.raw` is created with `No. Points: 0` and a header only.
  - `OperatingPointCore::initializeOutputs()` (`lib/coreop.cpp:86`) opens the file and `finalizeOutputs()` (`lib/coreop.cpp:104`) closes it, but `OperatingPointCore::evaluate()` (`lib/coreop.cpp:408`) never adds a point.
  - The docs for ac, acsp, acstb, acxf, noise, dcinc, dcxf and `cmd-analysis-op.md` say `writeop` has no effect and no op results are written. A zero-point raw file may also fail to load in rawread.
  - Fix option A: skip file creation in `initializeOutputs()` when `!params.solve` (smaller change, docs become true).
  - Fix option B: write the evaluated point in `evaluate()` (then update the docs).

## Test coverage

- [ ] 2. acsp and acstb are not covered by `test/test_opsolve.sim`
  - The docs claim both support `opsolve=0`.
  - acsp was checked by hand: with the source moved to 0.4 V, `opsolve=0` matched the 0.8 V reference and differed from a re-solve.
  - acstb is unverified (same code pattern, `lib/coreacstb.cpp:265`).
  - Add both to the test, or at least acstb.

- [ ] 3. List-form `nodeset` with `opsolve=0` is untested
  - The `OpSolveNodesetType` check (`lib/coreop.cpp:170`) was only read, not run.

## Limitations to decide on (not bugs)

- [ ] 4. Stored solution is read at `rebuild()`, not at run time
  - The core is rebuilt only when the mapping changes, so a slot re-stored between sweep points would be stale.
  - Existing `nodeset` behaves the same way, but with `opsolve=0` it changes results, not just the initial guess.
  - Decide whether to document it or re-read the slot in `evaluate()`.

- [ ] 5. Unknowns missing from the stored solution are evaluated at 0 silently
  - Documented in `cmd-analysis-op.md`.
  - Consider a warning, or an error under `strictforce`.

## Housekeeping

- [ ] 6. `.vscode/launch.json` is committed with local debug-target changes (points at `test_opsolve.sim`, comments out `test_hbnoise3.sim`).
- [ ] 7. `test/test_ac.sim` and `test/test_dcinc.sim` have whitespace-only diffs.
- [ ] 8. `docs/cmd-analysis-op.md` says the solution slot name is given by the `write` parameter; it should say `store`. This predates the branch but sits next to the new paragraph.
- [ ] 9. `OperatingPointCore::evaluate(bool atNodeset, ...)` has no caller that passes `false`.
  - Keep as reserved, or drop the parameter.
