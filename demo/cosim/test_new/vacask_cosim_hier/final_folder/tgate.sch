v {xschem version=3.4.8RC file_version=1.3}
G {}
K {}
V {}
S {}
F {}
E {}
N 290 -240 290 -220 {lab=A}
N 290 -160 290 -140 {lab=B}
N 290 -190 370 -190 {lab=0}
C {lab_pin.sym} 250 -190 0 0 {name=p1 lab=CTL}
C {lab_pin.sym} 290 -240 0 1 {name=p2 lab=A}
C {lab_pin.sym} 290 -140 0 1 {name=p3 lab=B}
C {lab_pin.sym} 370 -190 0 1 {name=p4 lab=0}
C {ipin.sym} 110 -170 0 0 { name=p5 lab=CTL }
C {iopin.sym} 110 -190 0 1 { name=p6 lab=B }
C {iopin.sym} 110 -210 0 1 { name=p7 lab=A }
C {ipin.sym} 110 -150 0 0 { name=p8 lab=VDD }
C {noconn.sym} 110 -150 0 1 {name=l1}
C {sg13g2_pr/sg13_lv_nmos.sym} 270 -190 0 0 {name=M1
l=0.13u
w=5u
ng=1
m=1
model=sg13_lv_nmos
spiceprefix=X
}
