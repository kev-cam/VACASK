# Bug audit — feature/pnoise (82d9f4c4^..HEAD)

## Correctness

1. [x] `lib/coretran.cpp:1017` — Stale comment fixed. Traced the reference chain (`TranCore::params` is a reference into `PssCore::shootParams`); pnoise already correctly gets `evalnoise=true` threaded to t=0 via `PssCore::evaluate(noiseModulation=true)`. PAC never reads noise data at all (grepped, no hits) so it never needed the flag — comment was just wrong, not a behavior bug.
2. [x] `lib/corepnoise.cpp:639` — Fixed. Widened the exact-zero DC guard to a relative-tolerance band (`freqAtSpur>freqTol*max(1,|h|)*f0`, `freqTol=1e-14`, matching the tolerance already used in `smsigFreqIndex`), so a near-harmonic sweep point is now excluded/warned the same as an exact one instead of overflowing `pow()`. Applied the same fix to `lib/corehbnoise.cpp` (pre-existing, same pattern, not originally in this audit's diff range), gated relative to `max(minFundamental, |smsigFreq[i]|)` (floor is the lowest tone frequency, `spurs_.fundamentals()`, rather than an arbitrary `1.0`) since HBNOISE uses a general mixing map rather than a single fundamental `h*f0`.

## Performance

3. [x] `lib/coretran.cpp:838` — Not a bug, by design. `PssTranCore::onTimestepAccepted`'s three capture passes (`lib/corepsstran.cpp:411-450`) all call `circuit.evalAndLoad(..., evalSetup=nullptr, ...)` — load-only, per `OsdiDevice::evalAndLoad` (`lib/osdidevice.cpp:404,473`): `evalCore()` (the actual OSDI model computation, gated by `CALC_NOISE` etc.) only runs when `evalSetup!=nullptr`; a null `evalSetup` just extracts already-cached results from the *last real eval*. Since which NR iteration ends up converged, and whether that step is then accepted, are both unknowable in advance, `CALC_NOISE` must already be set on every live iteration's eval so that whichever one turns out to be "the" one has noise data cached for the later load-only pass to read. There's no cheaper alternative that preserves correctness.
4. [x] `lib/corepnoise.cpp:626` — Fixed. `pssCore_.noiseExponents()` grabbed once as `noiseExp` right after `pssCore_.evaluate()` succeeds, instead of once per sideband x source. `inst->noiseModulationBase()` hoisted to once per instance (`noiseModBase`), reused to seed `modulatedNoiseSlot` in the sideband loop instead of re-calling the accessor per sideband. Same hoisting applied to `lib/corehbnoise.cpp` (pre-existing, same pattern, `hbCore_.noiseExponents()`/`inst->noiseModulationBase()`).
5. [x] `lib/corepnoise.cpp:568` — Fixed. Hoisted `noiseDensity`/`zr`/`wr` above the `do {} while` sweep loop, allocated once and reused across sweep points instead of reconstructed every iteration. Verified safe: every write path (`vectorPlusScaledVector`, `applyModulationAdjoint`, the Table-source branch) fully overwrites `zr`/`wr`, never accumulates, so reuse carries no stale-data risk. Same hoisting applied to `lib/corehbnoise.cpp` (pre-existing, same pattern).

## Design / duplication

6. [x] `lib/corepss.cpp:467` — Not a bug; original finding mischaracterized the relationship. Jacobian capture (`jacSamplesCollectionEnabled`, gated by `enableTdJacobianCapture`) is needed by both PAC and pnoise; noise capture is needed only by pnoise, and is gated by two genuinely separate live-eval sites that must both be on: `coretran.cpp:838` (`params.evalnoise`, regular per-timestep NR corrector evals) and `corepsstran.cpp:272` (`tdNoise`, monodromy-integration evals). These aren't the same state tracked twice — they're two distinct gates, both correctly derived from the same `noiseModulation` bool at their respective call sites. No desync risk, nothing to fix.
7. [~] `lib/corepnoise.cpp` — Known, planned future work (not fixing now): large duplication of `HBNoiseCore`/`PACCore` logic (`resolveOutputDescriptors`, the per-instance noise-accumulation loop, `computeOmega`, `applyModulationAdjoint`, factor/refactor/rcond-check boilerplate; `smsigFreqIndex` at `include/corepnoise.h:179` is explicitly commented as an "own copy"). Small-signal cores (PAC/HBNoise/PNoise) are planned to be merged to eliminate this duplication.

## Convention (CLAUDE.md: comments at most one line)

8. [~] `include/corepnoise.h:26-36` — Won't fix, staying as-is (user call).
9. [~] `lib/corepnoise.cpp:621-625` and `:634-638` — Won't fix, staying as-is (user call).
10. [~] `lib/coretran.cpp:835-837` — Won't fix, staying as-is (user call).

## Dead code

11. [x] `demo/gilbert/gilbert.sim:100` — Fixed. Removed the dead `fnvals` definition.
