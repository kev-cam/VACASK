#ifndef __VACASKCINTERFACE_DEFINED
#define __VACASKCINTERFACE_DEFINED

// C interface for driving VACASK from another program (e.g. a digital
// simulator acting as co-simulation master). Mirrors Xyce's
// libxycecinterface (xyce_open, xyce_initialize, xyce_simulateUntil, ...).
//
// The netlist is interpreted as by the vacask executable, except that
// a transient analysis pauses at the times requested by simulateUntil().
// External sources (vsource/isource with file="code:...", see extsource.h)
// are the co-simulation boundary.
//
// Module (.osdi) and include search paths are taken from SIM_MODULE_PATH and
// SIM_INCLUDE_PATH, the OpenVAF-reloaded compiler from SIM_OPENVAF.
//
// All functions return 1 on success and 0 on failure unless stated otherwise.

#ifdef __cplusplus
extern "C" {
#endif

void vacask_open(void** ptr);

// argv[argc-1] is the netlist file. Other arguments are ignored.
// Parses and elaborates the circuit and runs the control block up to and
// including the initial point (t=0) of the first transient analysis.
int vacask_initialize(void** ptr, int argc, char** argv);

// Advance the transient analysis to time t (it lands on t exactly unless
// the analysis ends earlier). *actualTime receives the time reached.
// Returns 0 if the analysis failed or has already completed.
int vacask_simulateUntil(void** ptr, double t, double* actualTime);

// 1 if the transient (and the rest of the control block) has completed
int vacask_simulationComplete(void** ptr);

// Time of the last accepted transient point
double vacask_getTime(void** ptr);

// Finish (flush outputs) and free
void vacask_close(void** ptr);

// Version of the co-simulation protocol (VACASK_COSIM_ABI, extsource.h):
// 2 = every converged step is offered to the external sources, which may
// veto it or finish the transient (vacask_extsource_step returning 2: the
// point is accepted and the analysis ends, as for a Verilog-A $finish; at
// the t=0 point too). A co-simulation master checks it before it starts:
// an older library reads a finish as an accepted step and never ends.
int vacask_cosim_abi(void);

#ifdef __cplusplus
}
#endif

#endif
