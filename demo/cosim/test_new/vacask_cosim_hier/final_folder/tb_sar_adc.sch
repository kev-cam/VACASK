v {xschem version=3.4.8RC file_version=1.3}
G {}
K {}
V {}
S {}
F {}
E {}
B 2 80 -880 1140 -510 {flags=graph
y1=0
y2=3.3
ypos1=0.27131944
ypos2=4.2824898
divy=5
subdivy=1
unity=1
x1=-1.9097719e-06
x2=0.00032713555
divx=5
subdivx=1
xlabmag=1.0
ylabmag=1.0
node="d[5..0];d5,d4,d3,d2,d1,d0
valid
clk
start
d0
d1
d2
d3
d4
d5
x2.sample
vcc"
color="8 4 4 8 10 10 10 10 10 10 4 10"
dataset=-1
unitx=1
logx=0
logy=0
digital=1
linewidth_mult=1}
B 2 80 -1640 1140 -920 {flags=graph
y1=-1.2
y2=3
ypos1=0
ypos2=2
divy=5
subdivy=1
unity=1
x1=-1.9097719e-06
x2=0.00032713555
divx=5
subdivx=1
xlabmag=1.0
ylabmag=1.0
dataset=-1
unitx=1
logx=0
logy=0
digital=0
color="4 8 10 9"
node="INPUT
i(vamm)
xtest.sample 0.05 *
x2.test_v"
linewidth_mult=1
hcursor1_y=1.0550322}
T { A simple DAC so that the result may be compared to the input.} 800 -230 0 0 0.4 0.4 {}
T {Analog conversion for plotting} 220 -140 0 0 0.4 0.4 {}
T {This is an example of a true mixed mode
(analog + Digital) simulation using ngspice 
for the analog part and Icarus Verilog 
(or Verilator) for the verilog part.

Instructions
- You need Icarus verilog installed.
- Icarus verilog must be built with the --enable-libvvp option.
- Follow the launchers in order
} 1150 -1470 0 0 0.7 0.7 {}
N 1330 -100 1330 -80 {lab=SUM}
N 280 -290 280 -270 {lab=START}
N -30 -290 -30 -270 {lab=INPUT}
N 50 -290 50 -270 {lab=VCC}
N 840 -100 1330 -100 {lab=SUM}
N 840 -190 840 -160 {lab=D5}
N 920 -190 920 -160 {lab=D4}
N 1000 -190 1000 -160 {lab=D3}
N 1080 -190 1080 -160 {lab=D2}
N 1160 -190 1160 -160 {lab=D1}
N 1240 -190 1240 -160 {lab=D0}
N 40 -140 40 -120 {lab=CLK}
C {ammeter.sym} 1330 -50 0 0 {name=VAMM savecurrent=0 spice_ignore=0}
C {lab_pin.sym} 1330 -20 0 0 {name=p35 lab=0}
C {lab_pin.sym} 280 -210 0 0 {name=p37 lab=0}
C {lab_pin.sym} 280 -290 0 0 {name=p38 lab=START}
C {lab_pin.sym} -30 -210 0 0 {name=p39 lab=0}
C {lab_pin.sym} -30 -290 0 0 {name=p40 lab=INPUT}
C {lab_pin.sym} 50 -210 0 0 {name=p1 lab=0}
C {lab_pin.sym} 50 -290 0 0 {name=p41 lab=VCC}
C {res.sym} 840 -130 0 0 {name=R2
value=2
footprint=1206
device=resistor
m=1}
C {lab_pin.sym} 840 -190 0 0 {name=p42 lab=D5}
C {res.sym} 920 -130 0 0 {name=R3
value=4
footprint=1206
device=resistor
m=1}
C {lab_pin.sym} 920 -190 0 0 {name=p43 lab=D4}
C {res.sym} 1000 -130 0 0 {name=R4
value=8
footprint=1206
device=resistor
m=1}
C {lab_pin.sym} 1000 -190 0 0 {name=p44 lab=D3}
C {res.sym} 1080 -130 0 0 {name=R5
value=16
footprint=1206
device=resistor
m=1}
C {lab_pin.sym} 1080 -190 0 0 {name=p45 lab=D2}
C {res.sym} 1160 -130 0 0 {name=R6
value=32
footprint=1206
device=resistor
m=1}
C {lab_pin.sym} 1160 -190 0 0 {name=p46 lab=D1}
C {res.sym} 1240 -130 0 0 {name=R7
value=64
footprint=1206
device=resistor
m=1}
C {lab_pin.sym} 1240 -190 0 0 {name=p47 lab=D0}
C {lab_pin.sym} 1330 -100 0 1 {name=p48 lab=SUM}
C {lab_pin.sym} 300 -410 0 0 {name=p7 lab=INPUT}
C {lab_pin.sym} 300 -390 0 0 {name=p8 lab=VCC}
C {lab_pin.sym} 300 -370 0 0 {name=p9 lab=START}
C {lab_pin.sym} 600 -410 0 1 {name=p10 lab=VALID}
C {lab_pin.sym} 600 -390 0 1 {name=p11 lab=D[5:0]}
C {lab_pin.sym} 300 -350 0 0 {name=p12 lab=CLK}
C {title.sym} 160 -10 0 0 {name=l1 author="Stefan Schippers"}
C {dac_bridge.sym} 330 -160 0 0 {name=A2 dac_bridge_model= dac_buff

device_model=".model dac_buff dac_bridge input_load=1e-15 t_rise=10n t_fall=10n
+ out_low=0 out_high=3.3"
}
C {lab_pin.sym} 300 -160 0 0 {name=p4 lab=VALID}
C {lab_pin.sym} 360 -160 0 1 {name=p5 lab=VALID_A}
C {lab_pin.sym} 40 -60 0 0 {name=p6 lab=0}
C {lab_pin.sym} 40 -140 0 0 {name=p13 lab=CLK}
C {launcher.sym} 150 -490 0 0 {name=h3
descr="load waves" 
tclcommand="xschem raw_read $netlist_dir/cosim_tran.raw tran"
}
C {launcher.sym} 1590 -310 0 0 {name=h0
descr="0. Build cosim bridge (once)"
tclcommand="execute 1 sh -c \\"cd $netlist_dir && make -C .. VACASK_PREFIX=/usr/local/vacask-dev\\""}
C {launcher.sym} 1590 -280 0 0 {name=h1
descr="1. Netlist"
tclcommand="xschem netlist"
}
C {launcher.sym} 1590 -250 0 0 {name=h2
descr="2. Generate cosim_top.v"
tclcommand="execute 1 sh -c \\"cd $netlist_dir && python3 ../gen_cosim_top.py --netlist tb_sar_adc.spectre --modules 'adc:sar_adc_vlog' -o cosim_top.v\\""
}
C {launcher.sym} 1590 -220 0 0 {name=h4
descr="3. Compile (Icarus)"
tclcommand="execute 1 sh -c \\"cd $netlist_dir && iverilog -o t.vvp cosim_top.v adc.v\\""
}
C {launcher.sym} 1590 -190 0 0 {name=h5
descr="4. Run cosim (VACASK)"
tclcommand="execute 1 sh -c \\"export LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu/ && export VACASK_INCLUDE_PATH=/var/home/alberto/Scripts/pdk_validation/pdks/ihp-sg13g2/models_ihp-sg13g2/vacask/models && export VACASK_MODULE_PATH=/var/home/alberto/Scripts/pdk_validation/pdks/ihp-sg13g2/models_ihp-sg13g2/vacask/osdi && cd $netlist_dir && vvp -M../ -mcosim t.vvp\\""
}
C {launcher.sym} 1590 -160 0 0 {name=h6
descr="load waves" 
tclcommand="xschem raw_read $netlist_dir/cosim_tran.raw tran"
}
C {launcher.sym} 1590 -130 0 0 {name=h7
descr="View digital waves (gtkwave)"
tclcommand="execute 1 sh -c \\"cd $netlist_dir && gtkwave cosim.vcd &\\""
}
C {code_shown.sym} 800 -470 0 0 {name=COMMANDS only_toplevel=false value="
ground 0

load \\"resistor.osdi\\"
load \\"capacitor.osdi\\"

parameters VCC=3.3

control
  tran tran1 tstop=250u tstep=10n
endc
"}
C {vsource.sym} 50 -240 0 0 {name=V3 value="dc=VCC" savecurrent=false}
C {vsource.sym} -30 -240 0 1 {name=V2 value="type=\\"pulse\\" val0=0 val1=3 delay=0 rise=200u fall=200u width=1u period=402u" savecurrent=false}
C {vsource.sym} 280 -240 0 0 {name=V1 value="type=\\"pulse\\" val0=0 val1=VCC delay=0.2u rise=10n fall=10n width=1.3u period=10u" savecurrent=false}
C {vsource.sym} 40 -90 0 0 {name=V4 value="type=\\"pulse\\" val0=0 val1='VCC' delay=500n rise=10n fall=10n width=490n period=1u" savecurrent=false}
C {simulator_commands_shown.sym} 1160 -450 0 0 {
name=Libs_VACASK
simulator=vacask
only_toplevel=false
value="
include \\"sg13g2_vacask_common.lib\\"
include \\"cornerMOSlv.lib\\" section=mos_tt
include \\"cornerMOShv.lib\\" section=mos_tt
include \\"cornerHBT.lib\\" section=hbt_typ
include \\"cornerRES.lib\\" section=res_typ
include \\"cornerCAP.lib\\" section=cap_typ
"
      }
C {/home/alberto/Scaricati/ngspice_verilog_cosim/vacask_cosim_hier/final_folder/sar_adc.sym} 450 -380 0 0 {name=x1}
