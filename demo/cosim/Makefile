VACASK_PREFIX ?= $(abspath ../install)
VPI_INC    := $(VACASK_PREFIX)/include/iverilog

VACASK_INC := $(VACASK_PREFIX)/include
VACASK_LIB := $(VACASK_PREFIX)/lib64/libsimlib.a
SS_LIB     := $(VACASK_PREFIX)/lib64
BOOST_LIB  := $(VACASK_PREFIX)/lib

MOD_PATH   := $(VACASK_PREFIX)/lib/vacask/mod

CXX        := g++
CXXFLAGS   := -std=c++20 -O0 -g -fPIC -DNAMESPACE=sim -DVACASK_MOD_PATH='"$(MOD_PATH)"'

VACASK_LIBS := $(VACASK_LIB) \
               -L$(SS_LIB) -lklu -lbtf -lamd -lcolamd -lcamd -lccolamd -lcholmod -lsuitesparseconfig \
               -L$(BOOST_LIB) -lboost_filesystem -lboost_system -lboost_process \
               -ldl -lpthread -lm

RPATH_FLAGS := -Wl,-rpath,$(SS_LIB) -Wl,-rpath,$(BOOST_LIB)

.PHONY: all clean

# The one build step that actually needs a Makefile: the VPI shared
# library, linking VACASK's libsimlib.a plus SuiteSparse and Boost.
# Everything downstream of this (running any example or test) is just
# `iverilog -o t.vvp *.v` then `vvp -M<this dir> -mcosim t.vvp` -- see
# README.md.
all: cosim.vpi

cosim.vpi: cosim_core.cpp cosim_core.h cosim_vpi_adapter.cpp
	$(CXX) $(CXXFLAGS) -shared \
		-I$(VACASK_INC) -I$(VPI_INC) -I. \
		-o $@ cosim_core.cpp cosim_vpi_adapter.cpp \
		$(VACASK_LIBS) $(RPATH_FLAGS)

clean:
	rm -f cosim.vpi
