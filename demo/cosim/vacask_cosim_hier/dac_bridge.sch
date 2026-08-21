v {xschem version=3.4.8RC file_version=1.3}
G {}
K {}
V {}
S {}
F {}
E {}
T {OSDI wrapper for dac_bridge.va (module dac)} 120 -130 0 0 0.3 0.3 {}
C {netlist.sym} 230 -260 0 0 {name=DAC_OSDI value="
model dac_model dac vth=0.9 vsoft=10m vout_low=0 vout_high=1.8
DAC (din aout) dac_model
"}
C {opin.sym} 310 -60 0 0 {name=p2 lab=aout}
C {ipin.sym} 210 -60 0 0 {name=p3 lab=din}
