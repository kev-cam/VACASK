#ifndef __SOLBASKERITF_DEFINED
#define __SOLBASKERITF_DEFINED

#include "common.h"

// Small clash-free interface between the rest of VACASK and the isolated
// Trilinos ShyLU-Basker backend TUs (lib/solbasker_{real,complex}.cpp).
// Nothing here pulls in Kokkos or Basker headers.

namespace NAMESPACE {
namespace basker_wrapper {

// Number of worker threads handed to Basker::SetThreads(). Basker requires a
// power of two, so this is the largest power of two <= Simulator::nCpu().
// Circuit Jacobians are usually small enough that 1 is fastest.
int threadCount();

// Process-wide, idempotent Kokkos bring-up. The first call runs
// Kokkos::initialize() and registers a std::atexit() hook for Kokkos::finalize()
// (Kokkos cannot be re-initialized once finalized, so we never finalize early).
// Safe to call from any backend entry point before touching a Basker object.
void ensureKokkosInitialized();

} // namespace basker_wrapper
} // namespace NAMESPACE

#endif
