v {xschem version=3.4.8RC file_version=1.3}
G {}
K {}
V {}
S {}
F {}
E {}
N 340 -240 340 -160 {lab=TAIL}
N 340 -320 340 -300 {lab=IN}
N 340 -160 370 -160 {lab=TAIL}
N 80 -320 340 -320 {lab=IN}
N 80 -160 180 -160 {lab=CTL}
N 80 -230 240 -230 {lab=VCC}
N 300 -160 340 -160 {lab=TAIL}
C {ipin.sym} 80 -320 0 0 {name=p1 lab=IN}
C {ipin.sym} 80 -230 0 0 {name=p2 lab=VCC}
C {ipin.sym} 80 -160 0 0 {name=p3 lab=CTL}
C {lab_pin.sym} 370 -160 0 1 {name=p5 lab=TAIL}
C {capa.sym} 340 -270 0 0 {name=CB
m=1
value='C'
footprint=1206
device="ceramic capacitor"}
C {/home/alberto/Scaricati/ngspice_verilog_cosim/vacask_cosim_hier/final_folder/ainv.sym} 240 -160 0 0 {name=x1}
