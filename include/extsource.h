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
// Every such source also sees the accepted timepoints of transient analysis
// (and the initial point at t=0) together with the voltage across its
// terminals. A zero-current isource therefore acts as an analog probe
// (analog to digital boundary) while a vsource acts as a driver
// (digital to analog boundary).

extern "C" {

#define VACASK_EXTSRC_ABI 1

typedef struct VacaskExtSource {
    // Set by VACASK before calling init.
    int abi;

    // Library-owned context passed to all callbacks.
    void* ctx;

    // Source value (V for vsource, A for isource) at time t.
    // Called at every NR evaluation (the point may later be rejected).
    // *nextBreak receives the next breakpoint (>t), 0 for none.
    double (*value)(void* ctx, double t, double* nextBreak);

    // Called at every accepted timepoint with v = V(p)-V(n). May be null.
    // Return nonzero to request that the analysis pauses at this point so that
    // a co-simulation master can react (honoured only when a TranSync hook is
    // installed).
    int (*accepted)(void* ctx, double t, double v);

    // Called when the source is destroyed. May be null.
    void (*destroy)(void* ctx);
} VacaskExtSource;

// Returns nonzero on success.
typedef int (*VacaskExtSourceInit)(const char* args, int isVoltageSource, VacaskExtSource* src);

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

    // Notify all live external sources of an accepted timepoint,
    // returns true if any of them requested a pause
    static bool notifyAccepted(double t, const double* solution);

    // Earliest breakpoint after t reported by live external sources (0 for none)
    static double nextBreakpoint(double t);

    // Number of live external sources
    static size_t count() { return registry().size(); };

private:
    ExtSource() {};
    static std::vector<ExtSource*>& registry();

    VacaskExtSource src {};
    void* lib {nullptr};
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
