// Real (double) backend for the Trilinos ShyLU-Basker solver.
//
// Isolated translation unit: the Kokkos + Basker headers are large and slow and
// nothing else in VACASK should see them. This TU also carries the process-wide
// Kokkos bring-up (ensureKokkosInitialized), since it is always compiled when
// the Basker backend is enabled.

#include <cstdlib>

#include <Kokkos_Core.hpp>
#include <shylubasker.hpp>

#include "solbasker_common.h"

namespace NAMESPACE {
namespace basker_wrapper {

void ensureKokkosInitialized() {
    static const bool done = [] {
        if (!Kokkos::is_initialized() && !Kokkos::is_finalized()) {
            Kokkos::initialize();
            std::atexit([] {
                if (Kokkos::is_initialized() && !Kokkos::is_finalized()) {
                    Kokkos::finalize();
                }
            });
        }
        return true;
    }();
    (void)done;
}

VACASK_BASKER_INSTANTIATE(double);

} // namespace basker_wrapper
} // namespace NAMESPACE
