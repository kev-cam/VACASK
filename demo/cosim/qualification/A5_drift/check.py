#!/usr/bin/env python3
"""A5 — Long-run drift verification.

Clock: 1 MHz pulse, delay=5ns, rise=2ns, width=498ns, period=1000ns.

Expected posedge crossing of vth_hi=1.2V:
  For each cycle N (0-indexed):
    t_expected = delay + N * period + (vth_hi / V_peak) * rise
               = 5 + N * 1000 + (1.2/1.8) * 2
               = 5 + N * 1000 + 1.333
               = 6.333 + N * 1000   [ns]

  For a linear ramp (pulse rise), interpolation is exact.  Any drift is
  due to accumulated floating-point error in the bridge's time tracking.

Tier-2 pass: accumulated drift < 10 ns over 2000 cycles.
"""
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'common'))
from vcd_parser import parse_vcd, get_posedges
from scoreboard import Scoreboard

DELAY_NS = 5.0
PERIOD_NS = 1000.0
RISE_NS = 2.0
VTH_HI = 1.2
V_PEAK = 1.8
NUM_CYCLES = 2000

CROSSING_OFFSET_NS = (VTH_HI / V_PEAK) * RISE_NS


def expected_posedge(n):
    return DELAY_NS + n * PERIOD_NS + CROSSING_OFFSET_NS


def main():
    sb = Scoreboard('A5_drift')

    trans = parse_vcd('cosim.vcd', ['clk', 'counting'])

    posedges = get_posedges(trans, 'clk')

    sb.check_bool(1, 'minimum_edges_detected',
                  len(posedges) >= NUM_CYCLES,
                  detail=f'detected {len(posedges)} posedges, need {NUM_CYCLES}')

    if len(posedges) < 10:
        sb.add(2, 'drift', False, detail='too few edges to measure drift')
        sb.report_and_exit()
        return

    n_edges = min(len(posedges), NUM_CYCLES)
    errors = []
    max_drift = 0.0
    max_drift_idx = 0

    for i in range(n_edges):
        measured = posedges[i]
        expected = expected_posedge(i)
        err = measured - expected
        errors.append(err)
        if abs(err) > abs(max_drift):
            max_drift = err
            max_drift_idx = i

    sb.check_drift('accumulated_drift', abs(max_drift), n_edges)

    sb.check_timing('first_edge', posedges[0], expected_posedge(0), 1.0)

    mid = n_edges // 2
    sb.check_timing(f'mid_edge_{mid}', posedges[mid],
                    expected_posedge(mid), 1.0)

    last = n_edges - 1
    sb.check_timing(f'last_edge_{last}', posedges[last],
                    expected_posedge(last), 1.0)

    drift_tol = Scoreboard.TIER2_DRIFT_TOL_NS_PER_1K * (n_edges / 1000.0)
    sb.add(2, 'max_drift', abs(max_drift) <= drift_tol,
           measured=abs(max_drift), expected=0.0,
           tolerance=drift_tol,
           detail=f'max drift {max_drift:.4f} ns at edge {max_drift_idx}')

    sb.report_and_exit()


if __name__ == '__main__':
    main()
