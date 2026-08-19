v {xschem version=3.4.8RC file_version=1.3}
G {}
K {}
V {}
S {}
F {}
E {}
T {Analog conversion for plotting} 720 -110 0 0 0.4 0.4 {}
N 420 -210 420 -190 {lab=0}
N 180 -540 180 -480 {lab=IIN}
N 180 -420 180 -380 {lab=TEST_V}
N 270 -380 360 -380 {lab=TEST_V}
N 180 -270 420 -270 {lab=TEST_V}
N 180 -380 180 -270 {lab=TEST_V}
N 270 -500 270 -380 {lab=TEST_V}
N 180 -380 270 -380 {lab=TEST_V}
N 270 -730 360 -730 {lab=TEST_V}
N 270 -610 360 -610 {lab=TEST_V}
N 270 -500 360 -500 {lab=TEST_V}
N 270 -610 270 -500 {lab=TEST_V}
N 270 -960 360 -960 {lab=TEST_V}
N 270 -730 270 -610 {lab=TEST_V}
N 270 -840 270 -730 {lab=TEST_V}
N 270 -840 360 -840 {lab=TEST_V}
N 270 -960 270 -840 {lab=TEST_V}
N 850 -250 850 -220 {lab=COMP}
N 850 -160 850 -140 {lab=0}
C {ipin.sym} 80 -180 0 0 {name=p1 lab=INPUT}
C {ipin.sym} 80 -160 0 0 {name=p2 lab=VREF}
C {ipin.sym} 80 -140 0 0 {name=p3 lab=START}
C {opin.sym} 170 -140 0 0 {name=p4 lab=VALID}
C {ipin.sym} 80 -100 0 0 {name=p5 lab=CLK}
C {opin.sym} 170 -120 0 0 {name=p6 lab=D[5:0]}
C {capa.sym} 420 -240 0 0 {name=CLAST
m=1
value=31.25f
footprint=1206
device="ceramic capacitor"}
C {lab_pin.sym} 420 -190 0 0 {name=p26 lab=0}
C {lab_pin.sym} 60 -540 0 0 {name=p29 lab=INPUT}
C {lab_pin.sym} 180 -540 0 1 {name=p30 lab=IIN}
C {lab_pin.sym} 60 -580 0 0 {name=p31 lab=SAMPLE}
C {lab_pin.sym} 60 -620 0 0 {name=p32 lab=VREF}
C {res.sym} 180 -450 0 0 {name=R1
value=1k
footprint=1206
device=resistor
m=1}
C {lab_pin.sym} 180 -400 0 1 {name=p33 lab=TEST_V}
C {lab_pin.sym} 830 -480 0 0 {name=p7 lab=CLK}
C {lab_pin.sym} 830 -440 0 0 {name=p10 lab=COMP}
C {lab_pin.sym} 1150 -440 0 1 {name=p13 lab=VALID}
C {lab_pin.sym} 1150 -400 0 1 {name=p16 lab=D[5:0]}
C {lab_pin.sym} 1150 -480 0 1 {name=p19 lab=SAMPLE}
C {lab_pin.sym} 830 -400 0 0 {name=p22 lab=START}
C {lab_pin.sym} 360 -360 0 0 {name=p36 lab=VREF}
C {lab_pin.sym} 360 -340 0 0 {name=p37 lab=D[0]}
C {lab_pin.sym} 360 -480 0 0 {name=p8 lab=VREF}
C {lab_pin.sym} 360 -590 0 0 {name=p9 lab=VREF}
C {lab_pin.sym} 360 -710 0 0 {name=p11 lab=VREF}
C {lab_pin.sym} 360 -460 0 0 {name=p12 lab=D[1]}
C {lab_pin.sym} 360 -570 0 0 {name=p14 lab=D[2]}
C {lab_pin.sym} 360 -690 0 0 {name=p15 lab=D[3]}
C {lab_pin.sym} 360 -820 0 0 {name=p17 lab=VREF}
C {lab_pin.sym} 360 -800 0 0 {name=p18 lab=D[4]}
C {lab_pin.sym} 360 -940 0 0 {name=p20 lab=VREF}
C {lab_pin.sym} 360 -920 0 0 {name=p21 lab=D[5]}
C {lab_pin.sym} 850 -250 0 1 {name=p28 lab=COMP}
C {dac_bridge.sym} 870 -70 0 0 {name=A1 dac_bridge_model= dac_buff
device_model=".model dac_buff dac_bridge input_load=1e-15 t_rise=10n t_fall=10n
+ out_low=0 out_high=3.3"}
C {lab_pin.sym} 840 -70 0 0 {name=p34 lab=COMP}
C {lab_pin.sym} 900 -70 0 1 {name=p35 lab=COMP_A}
C {vcvs.sym} 850 -190 0 0 {name=E1 value=1000}
C {lab_pin.sym} 810 -170 0 0 {name=p23 lab=VREF}
C {lab_pin.sym} 810 -210 0 0 {name=p24 lab=TEST_V}
C {lab_pin.sym} 850 -140 0 0 {name=p25 lab=0}
C {/home/alberto/Scaricati/ngspice_verilog_cosim/vacask_cosim_hier/final_folder/tgate.sym} 120 -540 0 0 {name=x1}
C {/home/alberto/Scaricati/ngspice_verilog_cosim/vacask_cosim_hier/final_folder/ccap.sym} 420 -360 0 0 {name=x2 C=1p/32}
C {/home/alberto/Scaricati/ngspice_verilog_cosim/vacask_cosim_hier/final_folder/ccap.sym} 420 -480 0 0 {name=x3 C=1p/16}
C {/home/alberto/Scaricati/ngspice_verilog_cosim/vacask_cosim_hier/final_folder/ccap.sym} 420 -590 0 0 {name=x4 C=1p/8}
C {/home/alberto/Scaricati/ngspice_verilog_cosim/vacask_cosim_hier/final_folder/ccap.sym} 420 -710 0 0 {name=x5 C=1p/4}
C {/home/alberto/Scaricati/ngspice_verilog_cosim/vacask_cosim_hier/final_folder/ccap.sym} 420 -820 0 0 {name=x6 C=1p/2}
C {/home/alberto/Scaricati/ngspice_verilog_cosim/vacask_cosim_hier/final_folder/ccap.sym} 420 -940 0 0 {name=x7 C=1p}
C {/home/alberto/Scaricati/ngspice_verilog_cosim/vacask_cosim_hier/final_folder/sar_adc_vlog.sym} 990 -440 0 0 {name=x8}
