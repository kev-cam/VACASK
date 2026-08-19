#!/bin/sh
# Sets the IHP-SG13G2 PDK paths this netlist's own "include ..."/"load
# ..." statements need. The cosim bridge is deliberately PDK-agnostic
# (see VACASK_cosim/demo/cosim/CHANGELOG.md) -- it never assumes any
# particular PDK, so whoever runs a PDK-based netlist through it has to
# supply these paths, same as they would for a plain (non-cosim) VACASK
# run via the PDK's own .vacaskrc.toml.
export PDK_ROOT=/home/ciel
export PDK=ihp-sg13g2
export VACASK_INCLUDE_PATH="${PDK_ROOT}/${PDK}/libs.tech/vacask/models:${PDK_ROOT}/${PDK}/libs.ref/sg13g2_stdcell/vacask:${PDK_ROOT}/${PDK}/libs.ref/sg13g2_io/vacask"
export VACASK_MODULE_PATH="${PDK_ROOT}/${PDK}/libs.tech/vacask/osdi"

exec vvp -M/home/alberto/Scripts/VACASK/VACASK_cosim/demo/cosim -mcosim t.vvp
