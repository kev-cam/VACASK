#!/usr/bin/env python3
"""A7 — Hierarchical addressing verification.

TRIG and FB are internal nodes two levels deep (xcore:xana:{TRIG,FB}),
not subcircuit terminals. The bridge reaches them with a literal
colon-separated path built by hand (see analog.scs and cosim_core.cpp).

If Instance::translate()'s toplevel passthrough didn't work as documented,
or if the bridge's D2A Norton silently created a second, disconnected
node instead of reusing the real one, `fb` would never cross vth_hi --
the D2A write would be driving a node nothing else is attached to. That
is the actual failure mode this test is built to catch (REBUILD.md:
"If it grew, getNode() created a second node and the D2A drives
nothing").

Analytical timings (see analog.scs for the derivation):
  trig crossing:  1000.667 ns
  fb crossing:    ~1.11 ns after the D2A write, plus up to one tstep
                  (50 ns) of "next step" latency -- same reasoning as
                  A2_d2a_propagation.
"""
import csv
import math
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'common'))
from vcd_parser import parse_vcd, get_first_value
from scoreboard import Scoreboard

TRIG_NS = 1000.667
TOLERANCE_NS = 1.0

VDD = 1.8
R_NORTON = 1000.0
C_NORTON = 1e-12
R_LOAD = 100000.0
TSTEP_NS = 50.0
V_FINAL = VDD * R_LOAD / (R_NORTON + R_LOAD)
TAU_EFF_NS = (R_NORTON * R_LOAD / (R_NORTON + R_LOAD)) * C_NORTON * 1e9
D2A_SETTLE_NS = -TAU_EFF_NS * math.log(1.0 - 1.2 / V_FINAL)
MAX_D2A_DELAY_NS = D2A_SETTLE_NS + TSTEP_NS

# Expected unknown count: solutionLength() includes index 0 for ground,
# one unknown per non-ground node, and one branch-current unknown per
# voltage source (cross-checked against A1_ramp_crossing, which has 4
# nodes + 3 vsources + ground = 8, matching a real run). Here: TRIG and
# FB (2 nodes) + v_trig's branch current (1) + ground (1) = 4. The D2A
# Norton (isource + resistor + capacitor) does not add unknowns of its
# own -- it only connects to existing nodes. A duplicate "FB" node from
# a translate() bug would add one more, to 5.
EXPECTED_UNKNOWNS = 4


def main():
    sb = Scoreboard('A7_hierarchical')

    trans = parse_vcd('cosim.vcd', ['trig', 'fb', 'd2a_out', 'fb_rose'])

    t_trig = get_first_value(trans, 'trig', '1')
    sb.check_bool(1, 'trig_crossing_detected', t_trig is not None,
                  detail='trig went high (A2D resolved xcore:xana:TRIG)')
    if t_trig is not None:
        sb.check_timing('trig_crossing_time', t_trig, TRIG_NS, TOLERANCE_NS)

    t_fb = get_first_value(trans, 'fb', '1')
    sb.check_bool(1, 'fb_loopback_detected', t_fb is not None,
                  detail=(f'fb went high at {t_fb:.1f} ns (D2A reached the '
                          f'real xcore:xana:FB node)') if t_fb is not None else
                         'fb never went high -- D2A likely drove a duplicate node')

    if t_fb is not None and t_trig is not None:
        delay = t_fb - t_trig
        sb.add(2, 'd2a_response_delay', 0 < delay < MAX_D2A_DELAY_NS,
               measured=delay, expected=D2A_SETTLE_NS, tolerance=TSTEP_NS,
               detail=f'fb responded {delay:.3f} ns after trig '
                      f'(limit: {MAX_D2A_DELAY_NS:.1f} ns)')

    if os.path.exists('cosim_trace.csv'):
        with open('cosim_trace.csv') as f:
            header = next(csv.reader(f))
        n_unknowns = len(header) - 2  # columns are t,hk,v0,v1,...
        sb.add(2, 'no_duplicate_node', n_unknowns == EXPECTED_UNKNOWNS,
               measured=n_unknowns, expected=EXPECTED_UNKNOWNS,
               detail=f'{n_unknowns} unknowns in the circuit '
                      f'(expected {EXPECTED_UNKNOWNS}: TRIG, FB)')

    sb.report_and_exit()


if __name__ == '__main__':
    main()
