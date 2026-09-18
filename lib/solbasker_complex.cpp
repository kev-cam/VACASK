// Complex (std::complex<double>) backend for the Trilinos ShyLU-Basker solver.
//
// Isolated translation unit: the Kokkos + Basker headers are large and slow and
// nothing else in VACASK should see them. Basker templates cleanly on the entry
// type, so this shares solbasker_common.h with the real backend and only differs
// in the instantiation below.

#include <iostream>

// Enables Basker's internal Kokkos::Timer instrumentation (BASKER_TIMER /
// BASKER_TIMER_FINE in shylubasker_types.hpp), which prints per-phase timing
// (order, sfactor, factor_notoken, domain/separator factor, ...) to stdout.
// Diagnostic only - noisy, remove once the ND scaling question is settled.
// #define BASKER_TIME

// That BASKER_TIMER debug code (shylubasker_tree.hpp) uses unqualified cout/ios
// instead of std::cout/std::ios - a latent bug that only surfaces once this
// normally-dead code path is compiled. Bring the names into scope rather than
// patch vendored Trilinos headers.
// using std::cout;
// using std::ios;

#include <Kokkos_Core.hpp>
#include <shylubasker_decl.hpp>
#include <shylubasker_def.hpp>

#include "solbasker_common.h"

namespace NAMESPACE {
namespace basker_wrapper {

VACASK_BASKER_INSTANTIATE(Complex);

} // namespace basker_wrapper
} // namespace NAMESPACE
