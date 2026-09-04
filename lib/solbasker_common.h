#ifndef __SOLBASKER_COMMON_DEFINED
#define __SOLBASKER_COMMON_DEFINED

// Shared body of the Trilinos ShyLU-Basker backend, included by
// lib/solbasker_real.cpp and lib/solbasker_complex.cpp *after* their
// <Kokkos_Core.hpp> and <shylubasker.hpp>.
//
// Value is double or std::complex<double>; both are handled by the same
// BaskerNS::Basker<int, Value, HostSpace> instantiation, so - unlike SuperLU_MT -
// there is no per-value backend policy struct, only two TUs for compile-time
// parallelism. Each TU calls VACASK_BASKER_INSTANTIATE(Value) once.
//
// The forward CSC pattern and value arrays are NOT copied: Basker::Symbolic /
// Factor read the caller's CSCMatrix buffers in place. CSCMatrix reallocates
// those buffers only in its own rebuild(), which is always paired with a solver
// rebuild() -> setPattern().

#if !defined(SHYLUBASKER_HPP) && !defined(BASKER_HPP) && !defined(SHYLUBASKER_DECL_HPP)
#error "solbasker_common.h must be included after <shylubasker.hpp>"
#endif

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <cstddef>
#include <vector>

#include "solbaskeritf.h"
#include "common.h"

namespace NAMESPACE {
namespace basker_wrapper {

// Basker's Int is plain int; VACASK hands it MatrixEntryIndex arrays directly.
static_assert(sizeof(int) == sizeof(MatrixEntryIndex) && std::is_signed<MatrixEntryIndex>::value,
              "Basker Int (int) width/signedness differs from MatrixEntryIndex; "
              "VACASK aliases CSCMatrix index arrays directly.");

using HostSpace = Kokkos::DefaultHostExecutionSpace;

// Wrapper status codes (see include/solbasker.h for the contract).
enum : int { OK = 0, SINGULAR = 1, NANFOUND = 2, NOMEM = 3, OTHER = 4, NOPATTERN = -1 };

template<typename Value>
class SolverImpl {
    using BaskerT = BaskerNS::Basker<int, Value, HostSpace>;
    using Magnitude = double;

    BaskerT basker_;

    int n_   { 0 };
    int nnz_ { 0 };
    int minBlockSize_ { 0 };

    // Aliased forward CSC (caller-owned; see file header).
    int*   cp_ { nullptr };
    int*   ri_ { nullptr };
    Value* nz_ { nullptr };

    std::vector<Value> xscratch_;   // Basker::Solve wants distinct b / x

    bool havePattern_  { false };
    bool symbolicDone_ { false };
    bool factored_     { false };

    void applyOptions() {
        // Circuit Jacobians: structurally unsymmetric, block triangular, and we
        // must be able to *detect* a singular matrix (gmin stepping depends on
        // it) - so no pivot replacement.
        basker_.Options.symmetric          = BASKER_FALSE;
        basker_.Options.transpose          = BASKER_FALSE;
        basker_.Options.matching           = BASKER_TRUE;   // pre-order for a zero-free diagonal
        basker_.Options.btf                = BASKER_TRUE;
        basker_.Options.no_pivot           = BASKER_FALSE;  // threshold partial pivoting, like KLU
        basker_.Options.replace_zero_pivot = BASKER_FALSE;
        basker_.Options.replace_tiny_pivot = BASKER_FALSE;
        basker_.Options.verbose            = BASKER_FALSE;
        basker_.Options.realloc            = BASKER_TRUE;
        if (minBlockSize_ > 0) {
            basker_.Options.min_block_size = minBlockSize_;
        }
        basker_.SetThreads(threadCount());
    }

    // Map a Basker return / Info() code onto a wrapper status code.
    int classify(int rc) {
        if (rc == BASKER_SUCCESS) {
            return OK;
        }
        switch (basker_.Info()) {
            case BASKER_ERROR_SINGULAR: return SINGULAR;
            case BASKER_ERROR_NAN:      return NANFOUND;
            case BASKER_ERROR_REMALLOC:
            case BASKER_ERROR_NOMALLOC: return NOMEM;
            default:                    return OTHER;
        }
    }

    int runSymbolic() {
        if (symbolicDone_) {
            basker_.Finalize();
            symbolicDone_ = false;
        }
        factored_ = false;
        applyOptions();
        int rc = basker_.Symbolic(n_, n_, nnz_, cp_, ri_, nz_, /*transpose_needed=*/false);
        if (rc != BASKER_SUCCESS) {
            return classify(rc);
        }
        symbolicDone_ = true;
        return OK;
    }

public:
    ~SolverImpl() { clearAll(); }

    void clearAll() {
        if (symbolicDone_) {
            basker_.Finalize();
            symbolicDone_ = false;
        }
        factored_    = false;
        havePattern_ = false;
        cp_ = ri_ = nullptr;
        nz_ = nullptr;
        xscratch_.clear();
        n_ = nnz_ = 0;
    }

    // Basker has no "drop numeric, keep symbolic" call; the next factor(0) will
    // re-run Symbolic anyway, so just tear the whole thing down.
    void freeFactor() {
        if (symbolicDone_) {
            basker_.Finalize();
            symbolicDone_ = false;
        }
        factored_ = false;
    }

    void setPattern(int n, int nnz, const MatrixEntryIndex* cp,
                    const MatrixEntryIndex* ri, const Value* nz, int minBlockSize) {
        clearAll();
        ensureKokkosInitialized();

        n_   = n;
        nnz_ = nnz;
        minBlockSize_ = minBlockSize;
        cp_  = const_cast<int*>(reinterpret_cast<const int*>(cp));
        ri_  = const_cast<int*>(reinterpret_cast<const int*>(ri));
        nz_  = const_cast<Value*>(nz);
        xscratch_.assign(static_cast<std::size_t>(n), Value{});
        havePattern_ = true;
    }

    int factor(int fact) {
        if (!havePattern_) {
            return NOPATTERN;
        }

        if (fact == 0 || !symbolicDone_) {
            int st = runSymbolic();
            if (st != OK) {
                return st;
            }
        }

        int rc;
        if (fact >= 2 && factored_) {
            // Incremental refactor: reuse ordering + symbolic, new values.
            basker_.Options.same_pattern = BASKER_TRUE;
            rc = basker_.Factor_Inc(0);
            basker_.Options.same_pattern = BASKER_FALSE;
            if (rc != BASKER_SUCCESS) {
                // Fall back to a full numeric factorization on the same symbolic.
                rc = basker_.Factor(n_, n_, nnz_, cp_, ri_, nz_);
            }
        } else {
            rc = basker_.Factor(n_, n_, nnz_, cp_, ri_, nz_);
        }

        if (rc != BASKER_SUCCESS) {
            factored_ = false;
            return classify(rc);
        }
        factored_ = true;
        return OK;
    }

    int solve(void* rhsv, int nrhs, int trans) {
        if (!factored_) {
            return -1;
        }
        auto* rhs = static_cast<Value*>(rhsv);
        const bool transpose = (trans != 0);

        if (static_cast<int>(xscratch_.size()) < n_) {
            xscratch_.assign(static_cast<std::size_t>(n_), Value{});
        }

        for (int j = 0; j < nrhs; ++j) {
            Value* b = rhs + static_cast<std::size_t>(j) * n_;
            int rc = basker_.Solve(b, xscratch_.data(), transpose);
            if (rc != BASKER_SUCCESS) {
                return rc ? rc : -1;
            }
            std::copy(xscratch_.begin(), xscratch_.begin() + n_, b);
        }
        return 0;
    }

    // LAPACK dlacon-style 1-norm reciprocal condition estimate: ||A||_1 from the
    // CSC column sums, ||A^{-1}||_1 from a few solve / tsolve iterations.
    int rcond(double* rc) {
        *rc = 0.0;
        if (!factored_) {
            return -1;
        }

        // ||A||_1 = max column sum of |A|.
        double anorm = 0.0;
        for (int j = 0; j < n_; ++j) {
            double s = 0.0;
            for (int p = cp_[j]; p < cp_[j + 1]; ++p) {
                s += std::abs(nz_[p]);
            }
            anorm = std::max(anorm, s);
        }
        if (anorm == 0.0) {
            return 0;   // *rc stays 0
        }

        const std::size_t n = static_cast<std::size_t>(n_);
        std::vector<Value> x(n, Value{ 1.0 / n_ });
        std::vector<Value> y(n), z(n);
        double invNorm = 0.0;
        int jmax = 0;

        for (int iter = 0; iter < 5; ++iter) {
            y = x;
            if (basker_.Solve(x.data(), y.data(), /*transpose=*/false) != BASKER_SUCCESS) {
                return -1;
            }
            double gamma = 0.0;
            for (std::size_t i = 0; i < n; ++i) {
                gamma += std::abs(y[i]);
            }

            std::vector<Value> xi(n);
            for (std::size_t i = 0; i < n; ++i) {
                double m = std::abs(y[i]);
                xi[i] = (m > 0.0) ? y[i] / Value(m) : Value(1.0);
            }

            z = xi;
            if (basker_.Solve(xi.data(), z.data(), /*transpose=*/true) != BASKER_SUCCESS) {
                return -1;
            }

            jmax = 0;
            double zmax = 0.0;
            for (std::size_t i = 0; i < n; ++i) {
                double m = std::abs(z[i]);
                if (m > zmax) { zmax = m; jmax = static_cast<int>(i); }
            }

            invNorm = gamma;

            // <z, x> convergence test.
            double zx = 0.0;
            for (std::size_t i = 0; i < n; ++i) {
                zx += std::abs(z[i]) * std::abs(x[i]);
            }
            if (zmax <= zx) {
                break;
            }
            std::fill(x.begin(), x.end(), Value{});
            x[static_cast<std::size_t>(jmax)] = Value(1.0);
        }

        if (invNorm <= 0.0) {
            return 0;
        }
        *rc = 1.0 / (anorm * invNorm);
        return 0;
    }
};

// Opaque-handle boundary. Declared (without definitions) in include/solbasker.h;
// defined here and explicitly instantiated per value type by each backend TU via
// VACASK_BASKER_INSTANTIATE.
template<typename Value> SolverImpl<Value>* create()  { return new SolverImpl<Value>(); }
template<typename Value> void               destroy(SolverImpl<Value>* s) { delete s; }

template<typename Value>
void setPattern(
    SolverImpl<Value>* s, int n, int nnz,
    const MatrixEntryIndex* colptr, const MatrixEntryIndex* rowind,
    const Value* nzval, int minBlockSize
) {
    s->setPattern(n, nnz, colptr, rowind, nzval, minBlockSize);
}

template<typename Value> int  factor(SolverImpl<Value>* s, int fact)            { return s->factor(fact); }
template<typename Value> int  solve(SolverImpl<Value>* s, Value* B, int nrhs, int trans) { return s->solve(B, nrhs, trans); }
template<typename Value> int  rcond(SolverImpl<Value>* s, double* rc)           { return s->rcond(rc); }
template<typename Value> void freeFactor(SolverImpl<Value>* s)                  { s->freeFactor(); }
template<typename Value> void clear(SolverImpl<Value>* s)                       { s->clearAll(); }

// Instantiates the class and the eight entry points.
#define VACASK_BASKER_INSTANTIATE(Value)                                        \
    template class SolverImpl<Value>;                                           \
    template SolverImpl<Value>* create<Value>();                                \
    template void destroy<Value>(SolverImpl<Value>*);                           \
    template void setPattern<Value>(                                            \
        SolverImpl<Value>*, int, int,                                           \
        const MatrixEntryIndex*, const MatrixEntryIndex*, const Value*, int     \
    );                                                                          \
    template int  factor<Value>(SolverImpl<Value>*, int);                       \
    template int  solve<Value>(SolverImpl<Value>*, Value*, int, int);           \
    template int  rcond<Value>(SolverImpl<Value>*, double*);                    \
    template void freeFactor<Value>(SolverImpl<Value>*);                        \
    template void clear<Value>(SolverImpl<Value>*)

} // namespace basker_wrapper
} // namespace NAMESPACE

#endif
