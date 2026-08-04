// Digital-simulator adapter interface for the VACASK cosim bridge.
//
// Everything that knows about VACASK -- circuit build, coroutine, threshold
// state machine, crossing interpolation, breakpoint prediction -- lives in
// cosim_core.cpp and never mentions VPI, DPI or Verilator.
//
// A digital simulator is supported by implementing DigitalSim and calling
// Core::bindA2D / bindD2A / start.  Two adapters ship:
//   cosim_vpi_adapter.cpp        Icarus (VPI system tasks)
//   cosim_verilator_adapter.cpp  Verilator (C++-owned loop)

#pragma once
#include <cstdint>
#include <string>

namespace cosim {

// Logic values on the digital side.
enum Bit { Bit0 = 0, Bit1 = 1, BitX = -1 };

// ---------------------------------------------------------------------------
// Implemented by the adapter; called by the core.
// ---------------------------------------------------------------------------
class DigitalSim {
public:
    virtual ~DigitalSim() = default;

    // --- clock ---------------------------------------------------------
    // Ticks per second of the digital time grid (1e12 for a 1 ps precision).
    virtual double   ticksPerSecond() = 0;
    // Current digital time, in those ticks.
    virtual uint64_t nowTicks() = 0;

    // --- scheduling ----------------------------------------------------
    // Arrange for Core::onAnalogTime() to run when digital time reaches
    // absolute tick `t`.  If `t` is already past, run at the earliest
    // opportunity -- never rewind.
    virtual void wakeAt(uint64_t t) = 0;
    // Arrange for Core::onSettled() to run once every event at the current
    // timestamp has been processed, including nonblocking updates, WITHOUT
    // advancing time.  (VPI: cbReadWriteSynch.  Verilator: after eval().)
    virtual void wakeAfterSettle() = 0;

    // --- signals -------------------------------------------------------
    // `port` is the index returned by Core::bindA2D / bindD2A.
    virtual void writeBit(int port, Bit v) = 0;
    virtual Bit  readBit(int port) = 0;

    // --- lifetime ------------------------------------------------------
    virtual void finish(int code) = 0;
    // Diagnostics; routed to the simulator's own output stream.
    virtual void message(const char* text) = 0;
};

// ---------------------------------------------------------------------------
// The bridge core.  Single instance; not thread safe.
// ---------------------------------------------------------------------------
class Core {
public:
    static Core& instance();

    void attach(DigitalSim* sim) { sim_ = sim; }

    // Bind before start().  `node` is a fully-qualified VACASK node path
    // using ':' as the hierarchy separator, e.g. "xcore:xana:CLK".
    // Returns the port index the adapter must use in writeBit / readBit,
    // or -1 on error.
    int  bindA2D(const std::string& node, double vth_lo, double vth_hi);
    int  bindD2A(const std::string& node, double vdd, double r, double c);

    // Build the circuit, start the transient, schedule the first wake.
    bool start(const std::string& netlist, double tstop, double tstep);

    // Called by the adapter in response to wakeAt / wakeAfterSettle.
    void onAnalogTime();
    void onSettled();

    int a2dCount() const;
    int d2aCount() const;

private:
    Core() = default;
    DigitalSim* sim_ = nullptr;
    void pump();
};

} // namespace cosim
