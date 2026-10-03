#ifndef __EXTSOURCE_DEFINED
#define __EXTSOURCE_DEFINED

#include <string>
#include <memory>
#include <vector>
#include "common.h"

// Externally driven independent sources (mixed-signal co-simulation).
//
// A vsource/isource with type="pwl" and a URI in the file parameter
//   v1 (a 0) vsource type="pwl" file="code:<library>:<initfn>:<args>"
// takes its value from a function in a dynamically loaded library instead
// of from a waveform (this mirrors Xyce's PWL FILE "code:..." sources).
// The library is opened, <initfn> is looked up and called as
//   initfn(args, isVoltageSource, &src)
// and must fill in the VacaskExtSource structure.
//
// The transient analysis is the master schedule of whatever the library
// talks to (e.g. a digital simulator). Every converged step tk -> t is
// offered to the library before it is accepted: each source gets
// candidate() with the voltage across its terminals at both ends (a
// zero-current isource is thus an analog probe, an analog to digital
// boundary; a vsource is a driver, a digital to analog boundary), then the
// library's optional vacask_extsource_step(t, &tEvt) runs. If it returns
// VACASK_EXTSRC_VETO (1) with tEvt >= 0, an input of the circuit (a driver)
// changed at tEvt, tk <= tEvt < t, and the step is not accepted but redone
// so that it ends at tEvt. If it returns VACASK_EXTSRC_FINISH (2), the
// library's world has ended (e.g. the digital simulator executed $finish):
// the point is accepted and the transient analysis then finishes, as it
// does for a Verilog-A $finish. The initial point (t=0, after the operating
// point) is offered too; a veto there cannot be honoured and is ignored, a
// finish ends the analysis right after it.

extern "C" {

#define VACASK_EXTSRC_ABI 2

typedef struct VacaskExtSource {
    // Set by VACASK before calling init.
    int abi;

    // Library-owned context passed to all callbacks.
    void* ctx;

    // Source value (V for vsource, A for isource) at time t.
    // Called at every NR evaluation (the point may later be rejected).
    // *nextBreak receives the next breakpoint (>t), 0 for none.
    double (*value)(void* ctx, double t, double* nextBreak);

    // A step tPrev -> t has converged and is about to be accepted: the voltage
    // across the terminals goes from vPrev to v. Also called for the initial
    // point (tPrev == t). May be null.
    void (*candidate)(void* ctx, double tPrev, double vPrev, double t, double v);

    // Called when the source is destroyed. May be null.
    void (*destroy)(void* ctx);
} VacaskExtSource;

// Returns nonzero on success.
typedef int (*VacaskExtSourceInit)(const char* args, int isVoltageSource, VacaskExtSource* src);

// Optional, looked up by this name in the library: all sources have been
// given their candidate() for time t. Returns VACASK_EXTSRC_VETO if the step
// must be redone to end at *tEvt (see above; *tEvt == tPrev redoes the same
// step), VACASK_EXTSRC_FINISH to accept the point and finish the analysis,
// VACASK_EXTSRC_ACCEPT (0) otherwise. Any other nonzero value with
// *tEvt >= 0 is a veto (the meaning of nonzero before the finish protocol).
#define VACASK_EXTSRC_STEP "vacask_extsource_step"
typedef int (*VacaskExtSourceStep)(double t, double* tEvt);

#define VACASK_EXTSRC_ACCEPT 0
#define VACASK_EXTSRC_VETO   1
#define VACASK_EXTSRC_FINISH 2

// Version of the co-simulation protocol, exported by the C interface as
// vacask_cosim_abi() (see cinterface/vacaskcinterface.h): 2 = candidate
// steps with veto and finish.
#define VACASK_COSIM_ABI 2

}

namespace NAMESPACE {

class ExtSource {
public:
    ~ExtSource();

    ExtSource(const ExtSource&) = delete;
    ExtSource& operator=(const ExtSource&) = delete;

    // Is the string a code: URI
    static bool isUri(const std::string& uri);

    // Bind a source to a code: URI, returns nullptr and sets err on failure
    static std::shared_ptr<ExtSource> create(const std::string& uri, bool isVoltageSource, std::string& err);

    double value(double t, double& nextBreak) {
        nextBreak = 0;
        return src.value ? src.value(src.ctx, t, &nextBreak) : 0.0;
    };

    void setUnknowns(UnknownIndex p, UnknownIndex n) { uP = p; uN = n; };

    // Offer a converged step tPrev -> t (solutions prev and cur) to all live
    // external sources' libraries before accepting it. Returns true if the
    // step must be redone to end at tEvt (tPrev <= tEvt < t); a veto wins
    // over a finish. Otherwise finish is set if a library asked to accept
    // the point and then finish the analysis (it is left alone if not).
    static bool preAccept(double tPrev, const double* prev, double t, const double* cur, double& tEvt, bool& finish);

    // Earliest breakpoint after t reported by live external sources (0 for none)
    static double nextBreakpoint(double t);

    // Number of live external sources
    static size_t count() { return registry().size(); };

private:
    ExtSource() {};
    static std::vector<ExtSource*>& registry();

    VacaskExtSource src {};
    void* lib {nullptr};
    VacaskExtSourceStep step {nullptr};
    UnknownIndex uP {0};
    UnknownIndex uN {0};
};


// Hook that lets a co-simulation master drive a transient analysis
// (see TranCore). Only one can be installed at a time.
class TranSync {
public:
    virtual ~TranSync() = default;

    // Extra breakpoint after t (0 for none)
    virtual double nextBreakpoint(double t) = 0;

    // Called at every accepted timepoint (including t=0),
    // return true to stop the analysis there (it can be resumed).
    virtual bool accepted(double t) = 0;

    static TranSync* installed() { return hook; };
    static void install(TranSync* h) { hook = h; };

private:
    static TranSync* hook;
};

}

#endif
