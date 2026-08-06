// Icarus Verilog adapter: implements DigitalSim over VPI and exposes the
// three binding tasks to Verilog.
//
//   $cosim_a2d("<analog node>", <verilog reg>, vth_lo, vth_hi)
//   $cosim_d2a("<analog node>", <verilog net>, vdd, r, c)
//   $cosim_run("<netlist>", tstop, tstep)

#include "cosim_core.h"

#include <vpi_user.h>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

using namespace cosim;

namespace {

class VpiSim : public DigitalSim {
public:
    std::vector<vpiHandle> a2d_h, d2a_h;

    double ticksPerSecond() override {
        return std::pow(10.0, -(double)vpi_get(vpiTimePrecision, nullptr));
    }
    uint64_t nowTicks() override {
        s_vpi_time t; t.type = vpiSimTime;
        vpi_get_time(nullptr, &t);
        return ((uint64_t)t.high << 32) | (unsigned)t.low;
    }
    void wakeAt(uint64_t t) override {
        uint64_t now = nowTicks();
        uint64_t d   = (t > now) ? (t - now) : 0ULL;
        s_vpi_time tm; tm.type = vpiSimTime;
        tm.high = (PLI_UINT32)(d >> 32);
        tm.low  = (PLI_UINT32)(d & 0xffffffffULL);
        s_cb_data cb; std::memset(&cb, 0, sizeof cb);
        cb.reason = cbAfterDelay; cb.cb_rtn = cb_analog_time; cb.time = &tm;
        vpi_free_object(vpi_register_cb(&cb));
    }
    void wakeAfterSettle() override {
        // cbReadWriteSynch fires after every event at the current timestamp
        // including nonblocking updates, with no advance of simulation time.
        s_vpi_time z; z.type = vpiSimTime; z.low = 0; z.high = 0;
        s_cb_data cb; std::memset(&cb, 0, sizeof cb);
        cb.reason = cbReadWriteSynch; cb.cb_rtn = cb_settled; cb.time = &z;
        vpi_free_object(vpi_register_cb(&cb));
    }
    void writeBit(int port, Bit v) override {
        s_vpi_value val; val.format = vpiScalarVal;
        val.value.scalar = (v == Bit1) ? vpi1 : (v == Bit0) ? vpi0 : vpiX;
        vpi_put_value(a2d_h[(size_t)port], &val, nullptr, vpiNoDelay);
    }
    Bit readBit(int port) override {
        s_vpi_value val; val.format = vpiScalarVal;
        vpi_get_value(d2a_h[(size_t)port], &val);
        return (val.value.scalar == vpi1) ? Bit1
             : (val.value.scalar == vpi0) ? Bit0 : BitX;
    }
    void finish(int code) override { vpi_control(vpiFinish, code); }
    void message(const char* text) override { vpi_printf("%s", text); }

private:
    static PLI_INT32 cb_analog_time(p_cb_data) { Core::instance().onAnalogTime(); return 0; }
    static PLI_INT32 cb_settled(p_cb_data)     { Core::instance().onSettled();    return 0; }
};

VpiSim g_vpi;

// ------------------------------------------------------------ arg helpers
bool scan_args(int want, vpiHandle* out) {
    vpiHandle it = vpi_iterate(vpiArgument, vpi_handle(vpiSysTfCall, nullptr));
    int n = 0;
    if (it) { vpiHandle h; while ((h = vpi_scan(it))) { if (n < want) out[n] = h; n++; } }
    return n == want;
}
double arg_real(vpiHandle h) {
    s_vpi_value v; v.format = vpiRealVal; vpi_get_value(h, &v); return v.value.real;
}
std::string arg_str(vpiHandle h) {
    s_vpi_value v; v.format = vpiStringVal; vpi_get_value(h, &v);
    return v.value.str ? std::string(v.value.str) : std::string();
}

// ------------------------------------------------------------ system tasks
PLI_INT32 a2d_compiletf(PLI_BYTE8*) {
    vpiHandle a[4];
    if (!scan_args(4, a)) {
        vpi_printf("COSIM ERROR: $cosim_a2d(\"node\", reg, vth_lo, vth_hi)\n");
        vpi_control(vpiFinish, 1); return 1;
    }
    // The target must be a variable: vpi_put_value on a net is not portable.
    PLI_INT32 t = vpi_get(vpiType, a[1]);
    // A bit-select of a vector reg (e.g. `reg [7:0] x; ...; x[3]`) shows up
    // as vpiPartSelect, not vpiRegBit -- vpiRegBit is only for bits of a
    // reg *memory* array (`reg mem[0:7]`). Accept a part-select too, as
    // long as it's a slice of an actual reg/integer (writable procedural
    // storage) and not of a net (which vpi_put_value can't safely drive).
    bool okPartSelect = false;
    if (t == vpiPartSelect) {
        vpiHandle parent = vpi_handle(vpiParent, a[1]);
        PLI_INT32 pt = parent ? vpi_get(vpiType, parent) : -1;
        okPartSelect = (pt == vpiReg || pt == vpiIntegerVar);
    }
    if (t != vpiReg && t != vpiRegBit && t != vpiIntegerVar && !okPartSelect) {
        vpi_printf("COSIM ERROR: $cosim_a2d target '%s' must be a reg, not a net\n",
                   vpi_get_str(vpiName, a[1]));
        vpi_control(vpiFinish, 1); return 1;
    }
    return 0;
}
PLI_INT32 a2d_calltf(PLI_BYTE8*) {
    vpiHandle a[4]; scan_args(4, a);
    Core::instance().attach(&g_vpi);
    int p = Core::instance().bindA2D(arg_str(a[0]), arg_real(a[2]), arg_real(a[3]));
    if (p < 0) { vpi_control(vpiFinish, 1); return 0; }
    g_vpi.a2d_h.resize((size_t)p + 1);
    g_vpi.a2d_h[(size_t)p] = a[1];
    return 0;
}

PLI_INT32 d2a_compiletf(PLI_BYTE8*) {
    vpiHandle a[5];
    if (!scan_args(5, a)) {
        vpi_printf("COSIM ERROR: $cosim_d2a(\"node\", net, vdd, r, c)\n");
        vpi_control(vpiFinish, 1); return 1;
    }
    return 0;
}
PLI_INT32 d2a_calltf(PLI_BYTE8*) {
    vpiHandle a[5]; scan_args(5, a);
    Core::instance().attach(&g_vpi);
    int p = Core::instance().bindD2A(arg_str(a[0]), arg_real(a[2]),
                                     arg_real(a[3]), arg_real(a[4]));
    if (p < 0) { vpi_control(vpiFinish, 1); return 0; }
    g_vpi.d2a_h.resize((size_t)p + 1);
    g_vpi.d2a_h[(size_t)p] = a[1];
    return 0;
}

PLI_INT32 run_compiletf(PLI_BYTE8*) {
    vpiHandle a[3];
    if (!scan_args(3, a)) {
        vpi_printf("COSIM ERROR: $cosim_run(\"netlist\", tstop, tstep)\n");
        vpi_control(vpiFinish, 1); return 1;
    }
    return 0;
}
PLI_INT32 run_calltf(PLI_BYTE8*) {
    vpiHandle a[3]; scan_args(3, a);
    Core::instance().attach(&g_vpi);
    Core::instance().start(arg_str(a[0]), arg_real(a[1]), arg_real(a[2]));
    return 0;
}

void reg_tf() {
    struct { const char* n; PLI_INT32 (*c)(PLI_BYTE8*); PLI_INT32 (*p)(PLI_BYTE8*); } t[] = {
        {"$cosim_a2d", a2d_calltf, a2d_compiletf},
        {"$cosim_d2a", d2a_calltf, d2a_compiletf},
        {"$cosim_run", run_calltf, run_compiletf},
    };
    s_vpi_systf_data d;
    for (auto& e : t) {
        std::memset(&d, 0, sizeof d);
        d.type = vpiSysTask;
        d.tfname = e.n; d.calltf = e.c; d.compiletf = e.p;
        vpi_register_systf(&d);
    }
}

} // namespace

void (*vlog_startup_routines[])() = { reg_tf, nullptr };
