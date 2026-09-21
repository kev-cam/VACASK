# Bug audit: f166ed0e..HEAD

Audit of the 20 commits from f166ed0e (inclusive) to 4daf69c9. Findings were probed with
the Debug binary built before the last "Cleanup" commit; ctest was not run.

## Bugs

1. [ ] **hbnoise linearizes at the nodeset instead of the converged HB solution when `hbsolve=1`.**
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

2. [ ] **`p(instance,outvar)` save dropped from hbac in the refactor (712b8fa1).**
   The old `HBAC::resolveHbSave` had an `idP` branch calling `hbCore.addInstanceOutvar`.
   `HBSmallSignal::resolveHbSave` (include/anhbsmsig.h:182) declares `idP` but never uses it.
   - Verified: with `writehb=1`, `save p(v1,v)` produces nothing in `hbac1.hb.raw`, even with
     `strictsave=1`. docs/cmd-analysis-hbac.md:89 still documents it.
   - The pre-refactor build was not run, so the removed branch in the diff is the evidence.
   - May be intentional (`HB` itself never supported `p`, see the TODO in lib/anhb.cpp). If so,
     remove the hbac doc row and the unused `idP`. Otherwise restore the branch.

## Minor

3. [ ] **hbnoise doc example uses an invalid spur entry.**
   docs/cmd-analysis-hbnoise.md:132 has `spur={0} smag=[1]`. In hbac an integer scalar entry
   aborts with "Spur #0 ... not found". hbnoise ignores `spur`, so the example runs, but the
   attributes are meaningless. Drop them or use `0.0`.

4. [ ] **`test_hbnoise1.sim` does not assert the transient-noise reference.**
   The Welch estimate (`ftr`, `Pxx`, line 164) is only plotted, so the "independent check"
   is never checked. Line 111 also has a leftover `print(nr1)`.

5. [ ] **Dead error classes and misleading messages in hbnoise.**
   `HbNoiseSpurPruneFailed`, `HbNoiseMixingMapFailed`, `HbNoiseDelayBindFailed`
   (include/corehbnoise.h:80-88) are never pushed. `HBACCore::rebuildCore` pushes the `HbAc*`
   errors, so an hbnoise failure reports "HBAC matrix". Push the `HbNoise*` errors from
   `HBNoiseCore::rebuild()` or drop the dead classes.

6. [ ] **Stray trailing quote in new error messages.**
   include/osdiinstance.h:52 (`OsdiNoiseExponentChangeDetected`) and include/coretran.h:128
   (`TranTableNoiseNotSupported`) end in `'.'`. Copies an existing pattern (same in
   `OsdiDelayChangeDetected` and others), so cosmetic.
