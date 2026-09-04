// Clash-free part of the Trilinos ShyLU-Basker backend.
//
// Kept separate from the backend TUs (solbasker_{real,complex}.cpp) so it can
// pull in simulator.h without dragging Kokkos / Basker headers into a TU that
// needs the rest of VACASK.

#include "simulator.h"
#include "solbaskeritf.h"
#include "common.h"

namespace NAMESPACE {
namespace basker_wrapper {

// Basker::SetThreads() requires a power of two and rejects anything else, so
// clamp Simulator::nCpu() down to the nearest power of two (>= 1).
int threadCount() {
    int n = Simulator::nCpu();
    if (n < 1) {
        return 1;
    }
    int p = 1;
    while ((p << 1) <= n) {
        p <<= 1;
    }
    return p;
}

} // namespace basker_wrapper
} // namespace NAMESPACE
