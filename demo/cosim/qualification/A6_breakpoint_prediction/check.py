#!/usr/bin/env python3
"""A6 — Breakpoint prediction verification.

Tests that forward-prediction breakpoint injection causes the solver to
land at (or very near) a known crossing time, rather than overshooting
by a full adaptive timestep.

Analytical crossing: linear ramp 0→3.0V over 10µs, vth_hi=1.55V
  t_cross = (1.55/3.0) * 10000 = 5166.667 ns

This crossing is off the adaptive step grid (the solver uses ~200 ns
steps on this ramp), so it is unreachable by uniform stepping.

The discriminating assertion is the solver step location, not the
crossing time: interpolation on a linear ramp is exact, so crossing_time
passes with or without prediction.

We verify:
  1. The crossing was detected (ramp_in goes high)
  2. The detected crossing time is within 1 ns of 5166.667 ns
  3. The solver actually stepped near the crossing (cosim_trace.csv
     has an accepted step within 1 ns of 5166.667 ns)
  4. The breakpoint shortened the natural step (200 ns → ~192 ns),
     proving the step was injected, not natural
  5. The D2A loopback responded (ack_fb goes high on LOOPBACK)
"""
import csv
import math
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'common'))
from vcd_parser import parse_vcd, get_first_value
from scoreboard import Scoreboard

EXPECTED_CROSS_NS = 5166.667
CROSSING_TOL_NS = 1.0
SOLVER_STEP_TOL_NS = 1.0
# A consumed breakpoint lands the solver *exactly* on the predicted time
# (see the coretran.cpp patch: tSolveNew is set to externalBreakPoint_,
# not merely clamped). Grid alignment or interpolation luck cannot produce
# this; only breakpoint injection can. Falsifiability: comment out the
# core.setExternalBreakPoint() call in cosim_core.cpp and this must fail.
EXACT_LANDING_TOL_NS = 0.01

# Analytic D2A response bound: Norton r=1k c=1p (from cosim_top.v) into the
# high-impedance LOOPBACK load (r=1e12, negligible divider loss), settling
# to vth_hi=1.5V of vdd=3.0V, plus one tstep of "next step" latency (see
# A2_d2a_propagation/check.py for the same reasoning).
D2A_R_NS = 1000.0
D2A_C_F = 1e-12
VDD = 3.0
LOOPBACK_VTH_HI = 1.5
TSTEP_NS = 500.0
D2A_TAU_NS = D2A_R_NS * D2A_C_F * 1e9
D2A_SETTLE_NS = -D2A_TAU_NS * math.log(1.0 - LOOPBACK_VTH_HI / VDD)
MAX_D2A_RESPONSE_NS = D2A_SETTLE_NS + TSTEP_NS


def find_solver_steps_near(trace_file, target_s):
    """Find the closest step and its neighbours in cosim_trace.csv.

    Returns (closest_dist_s, prev_step_s, closest_step_s, next_step_s)
    or None if no step is close enough.
    """
    steps = []
    with open(trace_file) as f:
        reader = csv.reader(f)
        for row in reader:
            try:
                steps.append(float(row[0]))
            except (ValueError, IndexError):
                continue

    if not steps:
        return None

    best_idx = 0
    best_dist = abs(steps[0] - target_s)
    for i, t in enumerate(steps):
        d = abs(t - target_s)
        if d < best_dist:
            best_dist = d
            best_idx = i

    prev2_s = steps[best_idx - 2] if best_idx > 1 else None
    prev_s = steps[best_idx - 1] if best_idx > 0 else None
    return (best_dist, prev2_s, prev_s, steps[best_idx])


def main():
    sb = Scoreboard('A6_breakpoint_prediction')

    trans = parse_vcd('cosim.vcd',
                      ['ramp_in', 'crossed', 'ack_out', 'ack_fb'])

    # 1. Crossing detected
    t_cross = get_first_value(trans, 'ramp_in', '1')
    sb.check_bool(1, 'crossing_detected', t_cross is not None,
                  detail='ramp_in went high')

    if t_cross is not None:
        # 2. Crossing time accuracy
        sb.check_timing('crossing_time', t_cross, EXPECTED_CROSS_NS,
                        CROSSING_TOL_NS)

    # 3+4. Solver step near crossing + surrounding steps are grid-scale
    if os.path.exists('cosim_trace.csv'):
        target_s = EXPECTED_CROSS_NS * 1e-9
        result = find_solver_steps_near('cosim_trace.csv', target_s)
        if result is not None:
            closest_dist, prev2_s, prev_s, closest_s = result
            closest_ns = closest_dist * 1e9
            sb.add(2, 'solver_step_near_crossing',
                   closest_ns < SOLVER_STEP_TOL_NS,
                   detail=f'closest solver step to {EXPECTED_CROSS_NS:.3f} ns: '
                          f'{closest_ns:.3f} ns away '
                          f'(limit: {SOLVER_STEP_TOL_NS} ns)')

            # Only a consumed breakpoint lands exactly on the predicted
            # time; interpolation or grid alignment cannot.
            sb.add(2, 'solver_step_is_exact_breakpoint',
                   closest_ns < EXACT_LANDING_TOL_NS,
                   detail=f'accepted step is {closest_ns:.6f} ns from '
                          f'predicted crossing (limit: {EXACT_LANDING_TOL_NS} ns)')

            if prev2_s is not None and prev_s is not None:
                natural_gap_ns = (prev_s - prev2_s) * 1e9
                bp_gap_ns = (closest_s - prev_s) * 1e9
                shortened = bp_gap_ns < natural_gap_ns * 0.99
                sb.add(2, 'breakpoint_shortened_step',
                       shortened and natural_gap_ns > 10.0,
                       detail=f'natural step {natural_gap_ns:.1f} ns, '
                              f'breakpoint step {bp_gap_ns:.1f} ns '
                              f'(shortened by {natural_gap_ns - bp_gap_ns:.1f} ns)')
        else:
            sb.add(2, 'solver_step_near_crossing', False,
                   detail='no steps found in cosim_trace.csv')
    else:
        sb.add(2, 'solver_step_near_crossing', False,
               detail='cosim_trace.csv not found')

    # 5. D2A loopback response — ack_fb should go high after ramp_in
    t_fb = get_first_value(trans, 'ack_fb', '1')
    sb.check_bool(1, 'd2a_loopback_response', t_fb is not None,
                  detail=f'ack_fb went high at {t_fb:.1f} ns' if t_fb else
                         'ack_fb never went high')

    if t_fb is not None and t_cross is not None:
        d2a_delay = t_fb - t_cross
        sb.add(2, 'd2a_response_delay', 0 < d2a_delay < MAX_D2A_RESPONSE_NS,
               detail=f'D2A responded {d2a_delay:.1f} ns after crossing '
                      f'(limit: {MAX_D2A_RESPONSE_NS:.1f} ns)')

    sb.report_and_exit()


if __name__ == '__main__':
    main()
