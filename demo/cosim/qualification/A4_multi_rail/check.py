#!/usr/bin/env python3
"""A4 — Multi-rail simultaneous crossing verification.

Two analog signals crossing vth_hi=1.2V within the same analog timestep.

RAIL_A: PWL step at 2.000µs with 10ns rise
  Crosses 1.2V at t = 2000 + (1.2/1.8) * 10 = 2006.667 ns

RAIL_B: PWL step at 2.005µs with 10ns rise
  Crosses 1.2V at t = 2005 + (1.2/1.8) * 10 = 2011.667 ns

Separation = 5 ns (< 50 ns tstep).

Tier-1: both rails detected, rail_a goes first.
Tier-2: crossing times within 1 ns of analytical values.
         No cross-rail attribution error (rail_a edge at rail_a time,
         rail_b edge at rail_b time).
"""
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'common'))
from vcd_parser import parse_vcd, get_first_value, get_posedges
from scoreboard import Scoreboard

EXPECTED_A_NS = 2006.667
EXPECTED_B_NS = 2011.667
TOLERANCE_NS = 1.0


def main():
    sb = Scoreboard('A4_multi_rail')

    trans = parse_vcd('cosim.vcd',
                      ['rail_a', 'rail_b', 'a_seen', 'b_seen', 'a_first'])

    t_a = get_first_value(trans, 'rail_a', '1')
    t_b = get_first_value(trans, 'rail_b', '1')

    sb.check_bool(1, 'rail_a_detected', t_a is not None,
                  detail='A2D on RAIL_A went high')
    sb.check_bool(1, 'rail_b_detected', t_b is not None,
                  detail='A2D on RAIL_B went high')

    if t_a is not None:
        sb.check_timing('rail_a_crossing_time', t_a, EXPECTED_A_NS,
                        TOLERANCE_NS)
    if t_b is not None:
        sb.check_timing('rail_b_crossing_time', t_b, EXPECTED_B_NS,
                        TOLERANCE_NS)

    if t_a is not None and t_b is not None:
        sb.check_bool(2, 'rail_a_before_rail_b', t_a < t_b,
                      detail=f'rail_a at {t_a:.3f} ns, rail_b at {t_b:.3f} ns')

        sb.check_bool(2, 'no_cross_rail_attribution',
                      abs(t_a - EXPECTED_A_NS) < TOLERANCE_NS and
                      abs(t_b - EXPECTED_B_NS) < TOLERANCE_NS,
                      detail='each crossing attributed to correct rail')

    a_first = get_first_value(trans, 'a_first', '1')
    sb.check_bool(1, 'dut_saw_a_first', a_first is not None,
                  detail='DUT latched rail_a as first crossing')

    sb.report_and_exit()


if __name__ == '__main__':
    main()
