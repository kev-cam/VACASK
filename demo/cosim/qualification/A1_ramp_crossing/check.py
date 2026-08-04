#!/usr/bin/env python3
"""A1 — Ramp crossing verification.

Analytical expected crossing times (vth_hi = 1.2 V):

  RAMP_SLOW: linear 0→1.8V over 10µs
    t = (vth_hi / V_final) * T_ramp = (1.2/1.8) * 10000 = 6666.667 ns
    Linear interpolation on a linear ramp is exact.

  RAMP_FAST: linear 0→1.8V over 1µs
    t = (1.2/1.8) * 1000 = 666.667 ns

  RC_CHARGE: V(t) = 1.8 * (1 - exp(-t / τ)),  τ = R*C = 20kΩ * 100pF = 2µs
    Step at t ≈ 0 (0.1 ps rise)
    t = -τ * ln(1 - vth_hi/V_final) = -2000 * ln(1 - 1.2/1.8) = 2197.225 ns
    Linear interpolation on exponential curve has curvature error — this
    tests interpolation accuracy under non-linear conditions.
"""
import math
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'common'))
from vcd_parser import parse_vcd, get_first_value
from scoreboard import Scoreboard

VTH_HI = 1.2
V_FINAL = 1.8
TAU_US = 2.0

EXPECTED = {
    'ramp_slow': (VTH_HI / V_FINAL) * 10000.0,
    'ramp_fast': (VTH_HI / V_FINAL) * 1000.0,
    'rc_charge': -TAU_US * 1000.0 * math.log(1.0 - VTH_HI / V_FINAL),
}

TOLERANCE_NS = 1.0
# RC exponential curve error budget, measured on a clean run rather than
# assumed. cosim_trace.csv confirms the solver is held to a hard, exact
# 50 ns step throughout this interval (maxstep is correctly enforced —
# this is not a step-size ceiling defect). The residual error is NOT
# chord-interpolation curvature: recomputing the crossing from the exact
# analytic V(2150ns)/V(2200ns) instead of VACASK's simulated values gives
# only ~0.03 ns of curvature error (|V''|h^2/(8|V'|) = 6.25e4 * (50e-9)^2
# = 0.156 ns is itself an overestimate of that, since the crossing sits
# close to one end of the bracketing step, not at its midpoint).
#
# The dominant term is the transient solver's own numerical integration
# accuracy at this step size: VACASK's simulated RC_CHARGE values are
# ~0.0004 V below the exact exponential at both samples bracketing the
# crossing, and dividing by the local slope (~3e-4 V/ns) accounts for
# essentially all of the measured 1.19 ns.
#
# This is NOT gated by reltol, contrary to an earlier hypothesis in this
# project's audit notes: the measured error (1.1924 ns) is byte-identical
# whether the bridge explicitly sets reltol=1e-4 or leaves VACASK's
# default reltol=1e-3 in place -- tested directly, both give the same
# number. That makes sense once maxstep (50 ns) is the binding
# constraint: reltol only shrinks the *adaptive* step when the natural
# step would exceed what the tolerance allows, and here the natural step
# is already capped well below that by maxstep, so tightening reltol has
# nothing left to bite on. The true source is the integration method's
# own truncation error at a fixed 50 ns step -- not further isolated,
# and not going to be for a residual this small and this well-bounded.
# resistor.va / capacitor.va are ideal single-parameter models (r, c only,
# no tc1/tc2 or parasitics), so it is not a hidden device-model effect.
RC_TOLERANCE_NS = 1.5


def main():
    sb = Scoreboard('A1_ramp_crossing')

    trans = parse_vcd('cosim.vcd', list(EXPECTED.keys()) +
                      ['slow_seen', 'fast_seen', 'rc_seen'])

    for sig, expected_ns in EXPECTED.items():
        t = get_first_value(trans, sig, '1')
        if t is None:
            sb.add(2, f'{sig}_crossing_detected', False,
                   detail=f'{sig} never went high')
            continue

        tol = RC_TOLERANCE_NS if sig == 'rc_charge' else TOLERANCE_NS
        sb.check_timing(f'{sig}_crossing_time', t, expected_ns, tol)

    for latch, sig in [('slow_seen', 'ramp_slow'),
                       ('fast_seen', 'ramp_fast'),
                       ('rc_seen', 'rc_charge')]:
        t = get_first_value(trans, latch, '1')
        sb.check_bool(1, f'{latch}_latched', t is not None,
                      detail=f'DUT latched posedge of {sig}')

    sb.report_and_exit()


if __name__ == '__main__':
    main()
