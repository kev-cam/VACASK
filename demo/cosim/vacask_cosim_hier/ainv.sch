v {xschem version=3.4.8RC file_version=1.3}
G {}
K {}
V {}
S {}
F {}
E {}
N 170 -180 170 -150 {lab=OUT}
N 170 -310 170 -240 {lab=VDD}
N 170 -120 170 -50 {lab=0}
N 130 -180 130 -120 {lab=IN}
N 170 -180 240 -180 {lab=OUT}
N 70 -180 130 -180 {lab=IN}
N 170 -210 170 -180 {lab=OUT}
N 130 -240 130 -180 {lab=IN}
C {ipin.sym} 70 -180 0 0 {name=p1 lab=IN}
C {opin.sym} 240 -180 0 0 {name=p2 lab=OUT}
C {lab_pin.sym} 170 -50 0 0 {name=p3 sig_type=std_logic lab=0}
C {ipin.sym} 170 -310 0 0 {name=p4 lab=VDD}
C {sg13g2_pr/sg13_lv_nmos.sym} 150 -120 0 0 {name=M1
l=0.13u
w=5u
ng=1
m=1
model=sg13_lv_nmos
spiceprefix=X
}
C {sg13g2_pr/sg13_lv_pmos.sym} 150 -240 0 0 {name=M2
l=0.13u
w=5u
ng=1
m=1
model=sg13_lv_pmos
spiceprefix=X
}
