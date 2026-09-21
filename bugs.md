# Bug audit: f166ed0e..HEAD

Audit of the 20 commits from f166ed0e (inclusive) to 4daf69c9. Findings were probed with
the Debug binary built before the last "Cleanup" commit; ctest was not run.

## Bugs

1. [x] **hbnoise linearizes at the nodeset instead of the converged HB solution when `hbsolve=1`.**
   After `hbCore_.run()`, `HBNoiseCore::coroutine()` always calls `evaluateAtNodeset(true, ...)`
   (lib/corehbnoise.cpp:338). That function starts with
   `solution.vector() = nrSolver.forces(1).unknownValue_` (lib/corehb.cpp:318), which discards
   the solution HB just computed.
   - Verified: nodeset from an `ampl=0.5` HB solution, `hbsolve=1`, `ampl=2` gave onoise
     1.25e-24 against 2.0e-23 without a nodeset (ratio 16 = (2/0.5)^2).
   - Without a nodeset `forces(1)` is empty, so the assignment sets the vector size to 0 and
     later code reads the leftover buffer. This is undefined behavior that works only because
     `std::vector` keeps its capacity. `test_hbnoise2` passes for this reason;
     `test_hbnoise1` only uses `hbsolve=0`.
   - Also corrupts `solution` for the next point of a sweep (docs/cmd-sweep.md sweeps hbnoise).
   - Fix: skip the copy from `forces(1)` when the solution was just solved; keep it for `hbsolve=0`.

## Minor

2. [x] **hbac docs listed a `p(instance,outvar)` save that no longer exists.**
   The pre-refactor `HBAC::resolveHbSave` had an `idP` branch calling `hbCore.addInstanceOutvar`.
   It was dropped in 712b8fa1. Not a regression: HB stores no output variables, so the branch
   could only read whatever the last-evaluated collocation point left in the instance, and `HB`
   itself never supported `p` (TODO in lib/anhb.cpp). Only the docs were stale.
   - [x] Removed the row from docs/cmd-analysis-hbac.md and the `p` rows from
     docs/internals/anhbac.md and docs/internals/anhbnoise.md.
   - [ ] Delete the unused `idP` in include/anhbsmsig.h:182 (code, not done).

3. [x] **hbnoise doc example uses an invalid spur entry.**
   docs/cmd-analysis-hbnoise.md:132 has `spur={0} smag=[1]`. In hbac an integer scalar entry
   aborts with "Spur #0 ... not found". hbnoise ignores `spur`, so the example runs, but the
   attributes are meaningless. Drop them or use `0.0`.

4. [x] **`test_hbnoise1.sim` does not assert the transient-noise reference.**
   The Welch estimate (`ftr`, `Pxx`, line 164) is only plotted, so the "independent check"
   is never checked. Line 111 also has a leftover `print(nr1)`.

5. [x] **Dead error classes in hbnoise.**
   `HbNoiseSpurPruneFailed`, `HbNoiseMixingMapFailed`, `HbNoiseDelayBindFailed`
   (include/corehbnoise.h) were never pushed; `HBACCore::rebuildCore` pushes the `HbAc*`
   errors instead. Removed the three classes.
   - [x] `HbAcDelayBindFailed` said "HBAC matrix" when an hbnoise run failed to bind delay
     lines. Reworded to "small-signal conversion matrix" (include/corehbac.h:83).

6. [x] **Stray trailing quote in new error messages.**
   include/osdiinstance.h:52 (`OsdiNoiseExponentChangeDetected`) and include/coretran.h:128
   (`TranTableNoiseNotSupported`) ended in `'.'`. Fixed both.
   - [x] Four older messages in include/osdiinstance.h (lines 19, 27, 35, 43) had the same
     stray quote, and "instace" in line 19. Fixed.
