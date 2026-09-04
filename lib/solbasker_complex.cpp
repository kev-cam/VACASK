// Complex (std::complex<double>) backend for the Trilinos ShyLU-Basker solver.
//
// Isolated translation unit: the Kokkos + Basker headers are large and slow and
// nothing else in VACASK should see them. Basker templates cleanly on the entry
// type, so this shares solbasker_common.h with the real backend and only differs
// in the instantiation below.

#include <Kokkos_Core.hpp>
#include <shylubasker.hpp>

#include "solbasker_common.h"

namespace NAMESPACE {
namespace basker_wrapper {

VACASK_BASKER_INSTANTIATE(Complex);

} // namespace basker_wrapper
} // namespace NAMESPACE
