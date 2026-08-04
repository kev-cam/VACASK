// VACASK side of the cosim bridge.  Knows nothing about VPI, DPI or Verilator.
//
// Analog node names are fully-qualified hierarchical paths using ':' as the
// level separator (docs/cir-hier.md), e.g. "xcore:xana:CLK".  This works for
// D2A because a bridge instance added to defaultSubDef() is a child of
// __topinst__, whose own parent_ is null, so Instance::translate()
// (lib/devbase.cpp:112) returns terminal names unprefixed and getNode()
// resolves the literal path against the flat nodeMap.

#include "cosim_core.h"

#include <ucontext.h>
#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>

#include "libplatform.h"
#include "simulator.h"
#include "parser.h"
#include "filestack.h"
#include "openvafcomp.h"
#include "circuit.h"
#include "an.h"
#include "antran.h"

using namespace sim;
namespace cosim {

// Max breakpoint refinements per threshold approach; stops an asymptotic
// trajectory that never crosses from injecting forever.
static constexpr int MAX_BP_RETRIES = 4;

namespace {

struct A2DPort {
    std::string node;
    double vth_lo = 0.0, vth_hi = 0.0;
    int    sol   = -1;
    Bit    state = BitX;
};
struct D2APort {
    std::string node, isrc;
    double vdd = 0.0, r = 0.0, c = 0.0;
    int    last = -2;
};

std::vector<A2DPort> a2d;
std::vector<D2APort> d2a;

ucontext_t main_ctx, vacask_ctx;
constexpr size_t STACK_SIZE = 8 * 1024 * 1024;
// makecontext() requires a suitably aligned stack.  No guard page: a deep
// solver recursion overruns into adjacent BSS rather than faulting.
alignas(16) char vacask_stack[STACK_SIZE];

struct Shared {
    double              time_s = 0.0;
    std::vector<double> v;      // A2D voltages to publish at time_s
    bool                done   = false;
    long                steps  = 0;
} shared;

struct VacaskState {
    ParserTables* tab      = nullptr;
    Parser*       parser   = nullptr;
    Circuit*      cir      = nullptr;
    Analysis*     tran     = nullptr;
    Tran*         tranCast = nullptr;
    PTAnalysis    anDescriptor {"cosim_tran", "tran"};
    Status        status;
    std::ofstream trace;
    bool          traceHeaderWritten = false;
} vs;

struct Crossing {
    std::vector<double> prev_v;
    double              prev_time = 0.0;
    bool                ready     = false;
    std::vector<bool>   bp_pending;
    std::vector<double> bp_time;
    std::vector<int>    bp_retries;
    long                count = 0;

    void init(size_t n) {
        prev_v.assign(n, 0.0);
        bp_pending.assign(n, false);
        bp_time.assign(n, -1.0);
        bp_retries.assign(n, 0);
        prev_time = 0.0;
        ready = false;
        count = 0;
    }
} cx;

DigitalSim* g_sim = nullptr;
double g_ticks_per_s = 1e12;

void say(const char* fmt, ...) {
    char buf[512];
    va_list ap; va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (g_sim) g_sim->message(buf); else std::fputs(buf, stderr);
}

void yield_event(double t_s) {
    shared.time_s = t_s;
    shared.steps++;
    swapcontext(&vacask_ctx, &main_ctx);
}

bool timestep_cb(double tSolve, double hk) {
    auto& core = vs.tranCast->core();
    const int n = (int)a2d.size();

    std::vector<double> curr(n);
    for (int i = 0; i < n; i++) curr[i] = core.solutionValue(a2d[i].sol);

    // Interpolated threshold crossings inside this step, in time order.
    if (cx.ready) {
        struct Cross { int port; double t; double thr; };
        std::vector<Cross> cr;
        double dt = tSolve - cx.prev_time;
        if (dt > 0) {
            for (int i = 0; i < n; i++) {
                double vp = cx.prev_v[i], vc = curr[i];
                double hi = a2d[i].vth_hi, lo = a2d[i].vth_lo;
                if (vp < hi && vc >= hi)
                    cr.push_back({i, cx.prev_time + (hi - vp) / (vc - vp) * dt, hi});
                else if (vp > lo && vc <= lo)
                    cr.push_back({i, cx.prev_time + (lo - vp) / (vc - vp) * dt, lo});
            }
        }
        std::sort(cr.begin(), cr.end(),
                  [](const Cross& a, const Cross& b) { return a.t < b.t; });

        for (auto& e : cr) {
            double frac = (e.t - cx.prev_time) / dt;
            for (int i = 0; i < n; i++)
                shared.v[i] = cx.prev_v[i] + frac * (curr[i] - cx.prev_v[i]);
            // The crossing port gets its threshold assigned exactly, not
            // re-derived by interpolation.  Re-deriving lands within 1 ulp
            // either side, so a >= test fails on roughly half of otherwise
            // identical cycles.  Assigning the exact value removes the need
            // for any epsilon: the crossing time was solved for it.
            shared.v[e.port] = e.thr;
            cx.bp_pending[e.port] = false;
            cx.bp_retries[e.port] = 0;
            cx.count++;
            yield_event(e.t);
        }
    }

    // Forward-predict the next crossing and inject a breakpoint.
    if (cx.ready && hk > 0) {
        double earliest = -1.0;
        for (int i = 0; i < n; i++) {
            // Re-arm: breakpoint reached/passed with no crossing means the
            // prediction was wrong (linear extrapolation undershooting a
            // curve).  Refine from the better local slope, bounded.
            if (cx.bp_pending[i] && cx.bp_time[i] <= tSolve &&
                cx.bp_retries[i] < MAX_BP_RETRIES) {
                cx.bp_pending[i] = false;
                cx.bp_retries[i]++;
            }

            double v  = curr[i];
            double dv = (v - cx.prev_v[i]) / hk;
            double hi = a2d[i].vth_hi, lo = a2d[i].vth_lo;
            bool approaching = false;
            double bp = -1.0;
            if (dv > 0 && v < hi)      { bp = tSolve + (hi - v) / dv; approaching = true; }
            else if (dv < 0 && v > lo) { bp = tSolve + (lo - v) / dv; approaching = true; }

            if (!approaching || (v >= lo && v <= hi && dv == 0)) {
                cx.bp_pending[i] = false;
                cx.bp_retries[i] = 0;
            }
            // Minimum gap: a breakpoint a hair above tSolve yields a
            // near-zero step and the solver has no min-step guard here.
            if (approaching && !cx.bp_pending[i] && bp > tSolve + hk * 1e-3) {
                if (earliest < 0 || bp < earliest) earliest = bp;
                cx.bp_pending[i] = true;
                cx.bp_time[i]    = bp;
            }
        }
        if (earliest > tSolve) core.setExternalBreakPoint(earliest);
    }

    for (int i = 0; i < n; i++) cx.prev_v[i] = curr[i];
    cx.prev_time = tSolve;
    cx.ready = true;

    if (vs.trace.is_open()) {
        if (!vs.traceHeaderWritten) {
            vs.trace << "t,hk";
            for (size_t i = 0; i < core.solutionLength(); i++)
                vs.trace << ",v" << i;
            vs.trace << "\n";
            vs.traceHeaderWritten = true;
        }
        vs.trace << tSolve << "," << hk;
        for (size_t i = 0; i < core.solutionLength(); i++)
            vs.trace << "," << core.solutionValue(i);
        vs.trace << "\n";
    }

    for (int i = 0; i < n; i++) shared.v[i] = curr[i];
    yield_event(tSolve);
    return true;
}

void vacask_entry() {
    auto [ok, canResume] = vs.tran->run(vs.status);
    (void)canResume;
    if (!ok) say("COSIM ERROR: run: %s\n", vs.status.message().c_str());
    shared.done = true;
    swapcontext(&vacask_ctx, &main_ctx);
}

// Resume the analog side; false when the transient has finished.
bool analog_resume() {
    if (shared.done) return false;
    swapcontext(&main_ctx, &vacask_ctx);
    return !shared.done;
}

} // namespace

// ---------------------------------------------------------------------------
Core& Core::instance() { static Core c; return c; }
int   Core::a2dCount() const { return (int)a2d.size(); }
int   Core::d2aCount() const { return (int)d2a.size(); }

int Core::bindA2D(const std::string& node, double vth_lo, double vth_hi) {
    g_sim = sim_;
    if (vth_hi <= vth_lo) {
        say("COSIM ERROR: A2D %s: vth_hi (%g) must exceed vth_lo (%g)\n",
            node.c_str(), vth_hi, vth_lo);
        return -1;
    }
    A2DPort p; p.node = node; p.vth_lo = vth_lo; p.vth_hi = vth_hi;
    a2d.push_back(p);
    say("COSIM: A2D  %-28s port %d  [%g, %g]\n",
        node.c_str(), (int)a2d.size() - 1, vth_lo, vth_hi);
    return (int)a2d.size() - 1;
}

int Core::bindD2A(const std::string& node, double vdd, double r, double c) {
    g_sim = sim_;
    if (r <= 0 || c < 0) {
        say("COSIM ERROR: D2A %s: need r>0 and c>=0 (got r=%g c=%g)\n",
            node.c_str(), r, c);
        return -1;
    }
    D2APort p; p.node = node; p.vdd = vdd; p.r = r; p.c = c;
    p.isrc = "__cosim_i" + std::to_string(d2a.size());
    d2a.push_back(p);
    say("COSIM: D2A  %-28s port %d  vdd=%g r=%g c=%g\n",
        node.c_str(), (int)d2a.size() - 1, vdd, r, c);
    return (int)d2a.size() - 1;
}

bool Core::start(const std::string& netlist, double tstop, double tstep) {
    g_sim = sim_;
    if (!sim_) { say("COSIM ERROR: no DigitalSim attached\n"); return false; }
    if (a2d.empty() && d2a.empty()) {
        say("COSIM ERROR: no ports bound before start()\n");
        sim_->finish(1); return false;
    }
    g_ticks_per_s = sim_->ticksPerSecond();

    Simulator::setup();
    Simulator::prependModulePath({std::string(VACASK_MOD_PATH)});

    vs.tab    = new ParserTables();
    vs.parser = new Parser(*vs.tab);

    auto stackPos = vs.tab->fileStack().addFile(netlist.c_str());
    if (stackPos == FileStack::badFileId) {
        say("COSIM ERROR: cannot open netlist \"%s\"\n", netlist.c_str());
        sim_->finish(1); return false;
    }
    if (!vs.parser->parseNetlistFile(stackPos, vs.status)) {
        say("COSIM ERROR: parse: %s\n", vs.status.message().c_str());
        sim_->finish(1); return false;
    }
    vs.tab->defaultGround();

    // Synthesize the Norton for each D2A port.  Toplevel instances, so their
    // terminal names are not prefixed and may be hierarchical paths.
    if (!d2a.empty()) {
        auto& subdef = vs.tab->defaultSubDef();
        // These reference device types the netlist must already have loaded
        // ("resistor" and "capacitor" need the corresponding .osdi loads);
        // the bridge does not load them itself, because a duplicate load is
        // deleted by Circuit::add() and leaves the caller holding a freed
        // pointer.
        subdef.add(PTModel("__cosim_res",  "resistor"))
              .add(PTModel("__cosim_cap",  "capacitor"))
              .add(PTModel("__cosim_isrc", "isource"));
        for (size_t i = 0; i < d2a.size(); i++) {
            const char* node = d2a[i].node.c_str();
            std::string rn = "__cosim_r" + std::to_string(i);
            std::string cn = "__cosim_c" + std::to_string(i);
            subdef.add(PTInstance(d2a[i].isrc.c_str(), "__cosim_isrc", {"0", node})
                       .add(vs.parser->parseParameters("type=\"dc\" dc=0")))
                  .add(PTInstance(rn.c_str(), "__cosim_res", {node, "0"})
                       .add(PV{"r", d2a[i].r}));
            if (d2a[i].c > 0)
                subdef.add(PTInstance(cn.c_str(), "__cosim_cap", {node, "0"})
                           .add(PV{"c", d2a[i].c}));
        }
    }

    if (!vs.tab->verify(vs.status)) {
        say("COSIM ERROR: verify: %s\n", vs.status.message().c_str());
        sim_->finish(1); return false;
    }

    static OpenvafCompiler comp;
    vs.cir = new Circuit(*vs.tab, &comp, vs.status);
    if (!vs.cir->isValid()) {
        say("COSIM ERROR: circuit: %s\n", vs.status.message().c_str());
        sim_->finish(1); return false;
    }
    vs.cir->setOption("gshunt", 1e-12);

    if (!vs.cir->elaborate({}, "__topdef__", "__topinst__", nullptr, vs.status)) {
        say("COSIM ERROR: elaborate: %s\n", vs.status.message().c_str());
        sim_->finish(1); return false;
    }

    vs.anDescriptor = PTAnalysis("cosim_tran", "tran");
    vs.anDescriptor.add(PV{"stop", tstop});
    if (tstep > 0) {
        vs.anDescriptor.add(PV{"step",    tstep});
        vs.anDescriptor.add(PV{"maxstep", tstep});
    } else {
        vs.anDescriptor.add(PV{"step", tstop / 1000.0});
    }

    vs.tran = Analysis::create(vs.anDescriptor, *vs.cir, vs.status);
    if (!vs.tran) {
        say("COSIM ERROR: analysis: %s\n", vs.status.message().c_str());
        sim_->finish(1); return false;
    }
    vs.tran->add(PTSave("default"));
    vs.tranCast = dynamic_cast<Tran*>(vs.tran);
    if (!vs.tranCast) {
        say("COSIM ERROR: not a Tran analysis\n");
        sim_->finish(1); return false;
    }

    for (auto& p : a2d) {
        auto* node = vs.cir->findNode(Id(p.node.c_str()));
        if (!node) {
            say("COSIM ERROR: A2D node \"%s\" not found "
                "(hierarchical paths use ':')\n", p.node.c_str());
            sim_->finish(1); return false;
        }
        p.sol = node->unknownIndex();
    }
    for (auto& p : d2a) {
        if (!vs.cir->findNode(Id(p.node.c_str()))) {
            say("COSIM ERROR: D2A node \"%s\" not found\n", p.node.c_str());
            sim_->finish(1); return false;
        }
    }

    shared.v.assign(a2d.size(), 0.0);
    cx.init(a2d.size());
    vs.tranCast->core().setTimestepCallback(timestep_cb);

    // Header is written from timestep_cb() on the first accepted step, not
    // here: solutionLength() is not meaningful until the analysis has
    // actually solved once (it reads 0 at this point, before the solution
    // vector is allocated).
    vs.trace.open("cosim_trace.csv");
    vs.trace << std::setprecision(17);

    getcontext(&vacask_ctx);
    vacask_ctx.uc_stack.ss_sp   = vacask_stack;
    vacask_ctx.uc_stack.ss_size = STACK_SIZE;
    vacask_ctx.uc_link          = &main_ctx;
    makecontext(&vacask_ctx, vacask_entry, 0);

    say("COSIM: %zu A2D, %zu D2A, tstop=%g tstep=%g, %g ticks/s\n",
        a2d.size(), d2a.size(), tstop, tstep, g_ticks_per_s);
    pump();
    return true;
}

void Core::pump() {
    if (!analog_resume()) {
        say("COSIM: transient complete, %ld yields, %ld crossings\n",
            shared.steps, cx.count);
        if (vs.trace.is_open()) vs.trace.close();
        sim_->finish(0);
        return;
    }
    uint64_t want = (uint64_t)llround(shared.time_s * g_ticks_per_s);
    uint64_t now  = sim_->nowTicks();
    sim_->wakeAt(want > now ? want : now);
}

void Core::onAnalogTime() {
    for (size_t i = 0; i < a2d.size(); i++) {
        Bit ns = a2d[i].state;
        if      (shared.v[i] >= a2d[i].vth_hi) ns = Bit1;
        else if (shared.v[i] <= a2d[i].vth_lo) ns = Bit0;
        if (ns != a2d[i].state) {
            a2d[i].state = ns;
            sim_->writeBit((int)i, ns);
        }
    }
    sim_->wakeAfterSettle();
}

void Core::onSettled() {
    for (size_t i = 0; i < d2a.size(); i++) {
        int bit = (sim_->readBit((int)i) == Bit1) ? 1 : 0;
        if (bit != d2a[i].last) {
            d2a[i].last = bit;
            // Norton: I = V/R into the R||C the bridge instantiated.
            double i_src = bit ? (d2a[i].vdd / d2a[i].r) : 0.0;
            if (!vs.cir->setInstanceParameter(Id(d2a[i].isrc.c_str()), Id("dc"),
                                              Value(i_src), vs.status)) {
                say("COSIM ERROR: set %s.dc: %s\n",
                    d2a[i].isrc.c_str(), vs.status.message().c_str());
                sim_->finish(1);
                return;
            }
        }
    }
    pump();
}

} // namespace cosim
