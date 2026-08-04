// Verilator adapter: implements DigitalSim over Verilator's native C++ API.
//
// Verilator's VPI does now support cbAfterDelay and cbReadWriteSynch, but
// only "via vlt_main.cpp" -- the main loop still has to dispatch them.  Since
// the loop is ours either way, this adapter skips VPI entirely and drives the
// model directly, which is faster and has no VPI dependency at all.
//
// wakeAt / wakeAfterSettle are callback-shaped but Verilator wants a loop, so
// the adapter records the request and the loop below services it.  That
// inversion is the entire difference from the Icarus adapter.
//
// See README.md in this directory for the full build command and this
// adapter's verification status (it has not been built against a real
// Verilator install).

#include "../cosim_core.h"
#include "Vcosim_top.h"
#include "verilated.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace cosim;

namespace {

class VerilatorSim : public DigitalSim {
public:
    VerilatedContext* ctx = nullptr;
    Vcosim_top*       top = nullptr;

    // Port wiring: the top-level signals the bridge drives / samples.
    // Verilator flattens ports to members, so this is a table of pointers.
    std::vector<CData*> a2d_sig, d2a_sig;

    enum Pending { None, AtTime, Settle };
    Pending  pending      = None;
    uint64_t pendingTicks = 0;
    bool     done         = false;

    double   ticksPerSecond() override {
        // 10^(-timeprecision); Verilator reports the precision exponent.
        double p = 1.0;
        for (int i = 0; i < -ctx->timeprecision(); i++) p *= 10.0;
        return p;
    }
    uint64_t nowTicks() override { return ctx->time(); }

    void wakeAt(uint64_t t) override { pending = AtTime; pendingTicks = t; }
    void wakeAfterSettle() override  { pending = Settle; }

    void writeBit(int port, Bit v) override {
        *a2d_sig[(size_t)port] = (v == Bit1) ? 1 : 0;   // Verilator is 2-state
    }
    Bit readBit(int port) override {
        return *d2a_sig[(size_t)port] ? Bit1 : Bit0;
    }

    void finish(int code) override { done = true; exitCode = code; }
    void message(const char* text) override { std::fputs(text, stdout); }

    int exitCode = 0;
};

VerilatorSim sim;

} // namespace

int main(int argc, char** argv) {
    VerilatedContext ctx;
    ctx.commandArgs(argc, argv);
    Vcosim_top top{&ctx};

    sim.ctx = &ctx;
    sim.top = &top;

    auto& core = Core::instance();
    core.attach(&sim);

    // Port map.  In the Icarus flow these come from $cosim_a2d/$cosim_d2a
    // inside cosim_top.v; Verilator has no system tasks, so the same
    // information is supplied here.  Everything after this point -- the
    // circuit, the thresholds, the crossing interpolation, the breakpoint
    // prediction -- is the identical core.
    int p;
    p = core.bindA2D("xcore:xana:CLK", 0.6, 1.2);
    sim.a2d_sig.resize((size_t)p + 1); sim.a2d_sig[(size_t)p] = &top.clk_v1;

    p = core.bindD2A("xcore:xana:Q", 1.8, 1e3, 1e-12);
    sim.d2a_sig.resize((size_t)p + 1); sim.d2a_sig[(size_t)p] = &top.q_v1;

    if (!core.start("analog.scs", 2001e-6, 50e-9)) return 1;

    // Service the core's wake requests.  start() already issued the first.
    while (!sim.done) {
        switch (sim.pending) {
        case VerilatorSim::AtTime:
            ctx.time(sim.pendingTicks);
            sim.pending = VerilatorSim::None;
            core.onAnalogTime();
            break;
        case VerilatorSim::Settle:
            // eval() to a fixed point == "all events at this timestamp
            // processed, including NBAs", with no advance of time.
            top.eval();
            sim.pending = VerilatorSim::None;
            core.onSettled();
            break;
        case VerilatorSim::None:
            sim.done = true;   // core asked for nothing: nothing left to do
            break;
        }
    }
    top.final();
    return sim.exitCode;
}
