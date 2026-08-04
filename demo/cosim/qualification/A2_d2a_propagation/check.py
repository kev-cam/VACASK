#!/usr/bin/env python3
"""A2 — D2A propagation loopback verification.

Analytical expected propagation path:

  1. Analog trigger ramp crosses vth_hi=1.2V at t_trig.
     TRIG_ON: PWL step at 1µs with 1ns rise → crosses 1.2V at:
       t_trig = 1000 + (1.2/1.8) * 1 = 1000.667 ns

  2. DUT sets d2a_out=1. This takes effect at the current yield time
     (which is t_trig since crossing detection places the yield there).

  3. D2A Norton source injects current on D2A_NODE.
     R_norton=1kΩ, C_norton=1pF → τ = 1 ns.
     R_load=100kΩ → V_final = 1.8 * 100k/(1k+100k) = 1.782V
     V(t) = 1.782 * (1 - exp(-t/τ_eff))
     τ_eff = (R_norton || R_load) * C_norton ≈ 990Ω * 1pF ≈ 0.99 ns

  4. A2D crossing detection on D2A_NODE:
     Crosses vth_hi=1.2V at t_d2a_cross after Norton settles.
     However: the Norton changes at the current yield time but the
     analog solver only uses the new current at the NEXT step.
     So the A2D detection happens at least one analog timestep later.

  Expected delay from trig to readback:
     ~1 analog tstep (50ns max, likely less with adaptive stepping)
     + Norton RC settling to 1.2V: -0.99ns * ln(1-1.2/1.782) ≈ 1.11 ns
     + interpolation error

  Tier-2 pass criterion: D2A readback delay < 100 ns (one full tstep + settling).
  This is a structural test — we verify the D2A actually works and measure
  the propagation delay, which is inherent to the bridge's commit-then-solve design.

  Falling edge (TRIG_OFF at 3µs):
     t_trig_off = 3000 + (1.2/1.8) * 1 = 3000.667 ns
     D2A switches to 0V, A2D detects vth_lo=0.6V crossing on fall.
"""
import math
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'common'))
from vcd_parser import parse_vcd, get_first_value, get_posedges, get_negedges
from scoreboard import Scoreboard

TRIG_ON_NS = 1000.667
TRIG_OFF_NS = 3000.667

# Analytic D2A settling delay: the Norton source (r=1000, c=1pF, set in
# cosim_top.v) drives R_LOAD=100k (see analog.scs), so
# tau_eff = (r_norton || r_load) * c, and the readback crosses vth_hi
# after -tau_eff * ln(1 - vth_hi/V_final). The D2A change also only takes
# effect at the *next* analog step (see cosim-architecture.md, "sampled"
# synchronisation), so allow one more tstep of slack on top.
VDD = 1.8
VTH_HI = 1.2
R_NORTON = 1000.0
C_NORTON = 1e-12
R_LOAD = 100000.0
TSTEP_NS = 50.0
V_FINAL = VDD * R_LOAD / (R_NORTON + R_LOAD)
TAU_EFF_NS = (R_NORTON * R_LOAD / (R_NORTON + R_LOAD)) * C_NORTON * 1e9
D2A_SETTLE_NS = -TAU_EFF_NS * math.log(1.0 - VTH_HI / V_FINAL)
MAX_D2A_DELAY_NS = D2A_SETTLE_NS + TSTEP_NS


def main():
    sb = Scoreboard('A2_d2a_propagation')

    signals = ['trig_on', 'trig_off', 'd2a_out', 'd2a_fb',
               'fb_rose', 'fb_fell']
    trans = parse_vcd('cosim.vcd', signals)

    t_d2a_on = get_first_value(trans, 'd2a_out', '1')
    sb.check_bool(1, 'd2a_out_toggled_on', t_d2a_on is not None,
                  detail='d2a_out set to 1 after trigger')

    t_fb_rise = get_first_value(trans, 'd2a_fb', '1')
    if t_fb_rise is not None and t_d2a_on is not None:
        delay_rise = t_fb_rise - t_d2a_on
        sb.add(2, 'd2a_rising_delay', delay_rise >= 0 and delay_rise < MAX_D2A_DELAY_NS,
               measured=delay_rise, expected=0.0, tolerance=MAX_D2A_DELAY_NS,
               detail=f'd2a_out→d2a_fb rising delay = {delay_rise:.3f} ns')
    else:
        sb.add(2, 'd2a_rising_delay', False,
               detail='d2a_fb never went high')

    t_d2a_off_edges = get_negedges(trans, 'd2a_out')
    t_fb_negedges = get_negedges(trans, 'd2a_fb')
    if t_fb_negedges and t_d2a_off_edges:
        t_d2a_off = t_d2a_off_edges[0]
        t_fb_fall = t_fb_negedges[0]
        delay_fall = t_fb_fall - t_d2a_off
        sb.add(2, 'd2a_falling_delay', delay_fall >= 0 and delay_fall < MAX_D2A_DELAY_NS,
               measured=delay_fall, expected=0.0, tolerance=MAX_D2A_DELAY_NS,
               detail=f'd2a_out→d2a_fb falling delay = {delay_fall:.3f} ns')
    else:
        sb.add(2, 'd2a_falling_delay', False,
               detail='d2a_fb negedge or d2a_out negedge not found')

    sb.check_bool(1, 'fb_rose_latched',
                  get_first_value(trans, 'fb_rose', '1') is not None,
                  detail='DUT latched D2A readback posedge')

    sb.check_bool(1, 'fb_fell_latched',
                  get_first_value(trans, 'fb_fell', '1') is not None,
                  detail='DUT latched D2A readback negedge')

    sb.report_and_exit()


if __name__ == '__main__':
    main()
