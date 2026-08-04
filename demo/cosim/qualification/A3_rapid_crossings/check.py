#!/usr/bin/env python3
"""A3 — Rapid crossings verification.

Tests whether the bridge detects both edges of pulses shorter than tstep.

NARROW_30: 30 ns pulse at 2µs (rise=fall=1ns)
  Rising edge crosses vth_hi=1.2V at:
    t_rise = 2000 + (1.2/1.8) * 1 = 2000.667 ns
  Falling edge: pulse ends at 2000 + 1(rise) + 30(width) = 2031 ns
    Falls from 1.8→0 in 1ns.  Crosses vth_lo=0.6V at:
    t_fall = 2031 + (1 - 0.6/1.8) * 1 = 2031.667 ns
  Both edges 31 ns apart (< 50 ns tstep).

NARROW_10: 10 ns pulse at 4µs (rise=fall=1ns)
  Rising: t = 4000 + (1.2/1.8) * 1 = 4000.667 ns
  Falling: t = 4011 + (1 - 0.6/1.8) * 1 = 4011.667 ns
  Both edges 11 ns apart (< 50 ns tstep).

Tier-1: exactly 1 posedge + 1 negedge per pulse (no drops, no duplicates).
Tier-2: edge times within 1 ns of analytical values.
"""
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'common'))
from vcd_parser import parse_vcd, get_posedges, get_negedges
from scoreboard import Scoreboard


EXPECTED_30_RISE = 2000.667
EXPECTED_30_FALL = 2031.667
EXPECTED_10_RISE = 4000.667
EXPECTED_10_FALL = 4011.667

TOLERANCE_NS = 1.0


def main():
    sb = Scoreboard('A3_rapid_crossings')

    trans = parse_vcd('cosim.vcd',
                      ['narrow_30', 'narrow_10',
                       'pos30_seen', 'neg30_seen', 'pos10_seen', 'neg10_seen'])

    pos30 = get_posedges(trans, 'narrow_30')
    neg30 = get_negedges(trans, 'narrow_30')
    pos10 = get_posedges(trans, 'narrow_10')
    neg10 = get_negedges(trans, 'narrow_10')

    sb.check_event_count('narrow_30_posedges', len(pos30), 1)
    sb.check_event_count('narrow_30_negedges', len(neg30), 1)
    sb.check_event_count('narrow_10_posedges', len(pos10), 1)
    sb.check_event_count('narrow_10_negedges', len(neg10), 1)

    if pos30:
        sb.check_timing('narrow_30_rise_time', pos30[0], EXPECTED_30_RISE,
                        TOLERANCE_NS)
    if neg30:
        sb.check_timing('narrow_30_fall_time', neg30[0], EXPECTED_30_FALL,
                        TOLERANCE_NS)
    if pos10:
        sb.check_timing('narrow_10_rise_time', pos10[0], EXPECTED_10_RISE,
                        TOLERANCE_NS)
    if neg10:
        sb.check_timing('narrow_10_fall_time', neg10[0], EXPECTED_10_FALL,
                        TOLERANCE_NS)

    if pos30 and neg30:
        gap30 = neg30[0] - pos30[0]
        sb.check_bool(2, 'narrow_30_edge_separation',
                      0 < gap30 < 50.0,
                      detail=f'gap={gap30:.3f} ns (both within one tstep)')

    if pos10 and neg10:
        gap10 = neg10[0] - pos10[0]
        sb.check_bool(2, 'narrow_10_edge_separation',
                      0 < gap10 < 50.0,
                      detail=f'gap={gap10:.3f} ns (both within one tstep)')

    sb.report_and_exit()


if __name__ == '__main__':
    main()
