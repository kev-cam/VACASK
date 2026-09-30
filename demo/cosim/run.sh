#!/bin/bash
# Analog-on-top nvc<->VACASK cosim demo (VACASK analog master via
# libvacaskcinterface, nvc digital slave). Same designs as the Xyce demo in
# xyce/utils/test_simetrix_cosim (run.sh min|a2d).
#
# Prereqs:
#   - nvc with cosim (--vacask-netlist / --cosim-config)
#   - libcosim_bridge.so with vacask_bridge_init:
#       c++ -O2 -shared -fPIC -o $NVCB/lib/libcosim_bridge.so $NVCSRC/src/cosim_bridge.cpp
#   - VACASK built with -DVACASK_CINTERFACE=ON (default on Linux)
set -e
cd "$(dirname "$0")"
NVCB=${NVCB:-/usr/local/src/nvc-build}
NVC=$NVCB/bin/nvc; LIBS=$NVCB/lib
VCB=${VCB:-/opt/build.VACASK/cosim}
export LD_LIBRARY_PATH=$NVCB/lib:$VCB/cinterface${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
export SIM_MODULE_PATH=${SIM_MODULE_PATH:-$VCB/devices}

TEST=${1:-min}   # 'min' (D2A square wave) or 'a2d' (A2D round trip)
case $TEST in
  min) VHD=cosim_min.vhd; TOP=cosim_min; SIM=min.sim; BND=min.boundary;;
  a2d) VHD=cosim_a2d.vhd; TOP=cosim_a2d; SIM=a2d.sim; BND=a2d.boundary;;
  *) echo "usage: $0 [min|a2d]"; exit 1;;
esac
W=work_$TEST; rm -rf $W
$NVC --std=2040 --work=$W:$W -L $LIBS -a $VHD
$NVC --std=2040 --work=$W:$W -L $LIBS -e $TOP
$NVC --std=2040 --work=$W:$W -L $LIBS -r --stop-time=${STOP:-200ns} \
     --vacask-netlist=$SIM --cosim-config=$BND $TOP
