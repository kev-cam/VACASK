v {xschem version=3.4.8RC file_version=1.3}
G {}
K {}
V {}
S {}
F {}
E {}
T {OSDI wrapper for acomp.va (module comparator)} 120 -130 0 0 0.3 0.3 {}
C {noconn.sym} 230 -90 2 0 {name=n1}
C {noconn.sym} 290 -90 2 1 {name=n4}
C {netlist.sym} 230 -260 0 0 {name=Comparator_OSDI value="
model comparator_model comparator vout_low=0 vout_high=1.8
ACOMP (ain1 ain2 dout) comparator_model
"}
C {ipin.sym} 230 -90 0 0 {name=p2 lab=ain1}
C {opin.sym} 290 -90 0 0 {name=p3 lab=dout}
C {noconn.sym} 230 -70 2 0 {name=n2}
C {ipin.sym} 230 -70 0 0 {name=p1 lab=ain2}
