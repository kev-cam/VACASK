v {xschem version=3.4.8RC file_version=1.3}
G {}
K {}
V {}
S {}
F {}
E {}
T {Every pin gets a 1G stub resistor to ground -- just enough to keep
its node from collapsing away during VACASK elaboration} -340 -230 0 0 0.35 0.35 {layer=4}
C {ipin.sym} -300 -20 0 0 {name=p1 lab=Clk}
C {res.sym} -380 -100 0 0 {name=Rclk value=1G footprint=1206 device=resistor m=1}
C {lab_pin.sym} -380 -130 0 0 {name=p2 lab=0}
C {lab_pin.sym} -380 -70 0 0 {name=p3 lab=Clk}
C {opin.sym} 20 -10 0 0 {name=p4 lab=Sample}
C {res.sym} -30 -100 0 0 {name=R3 value=1G footprint=1206 device=resistor m=1}
C {lab_pin.sym} -30 -130 0 0 {name=p5 lab=0}
C {lab_pin.sym} -30 -70 0 0 {name=p6 lab=Sample}
C {res.sym} 60 -100 0 0 {name=R2 value=1G footprint=1206 device=resistor m=1}
C {lab_pin.sym} 60 -130 0 0 {name=p8 lab=0}
C {lab_pin.sym} 60 -70 0 0 {name=p9 lab=Done}
C {res.sym} 180 -100 0 0 {name=R1 value=1G footprint=1206 device=resistor m=1}
C {lab_pin.sym} 180 -130 0 0 {name=p11 lab=0}
C {lab_pin.sym} 180 -70 0 0 {name=p12 lab=Result[5:0]}
C {res.sym} -300 -100 0 0 {name=Rclk1 value=1G footprint=1206 device=resistor m=1}
C {lab_pin.sym} -300 -130 0 0 {name=p16 lab=0}
C {lab_pin.sym} -300 -70 0 0 {name=p17 lab=Comp}
C {res.sym} -220 -100 0 0 {name=Rclk2 value=1G footprint=1206 device=resistor m=1}
C {lab_pin.sym} -220 -130 0 0 {name=p18 lab=0}
C {lab_pin.sym} -220 -70 0 0 {name=p19 lab=Start}
C {ipin.sym} -300 0 0 0 {name=p20 lab=Comp}
C {ipin.sym} -300 20 0 0 {name=p21 lab=Start}
C {opin.sym} 20 10 0 0 {name=p7 lab=Done}
C {opin.sym} 20 30 0 0 {name=p10 lab=Result[5:0]}
