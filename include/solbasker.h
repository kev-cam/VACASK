#ifndef __SOLBASKER_DEFINED
#define __SOLBASKER_DEFINED

#include <cstdint>
#include <cstddef>
#include <tuple>
#include <stdexcept>
#include <type_traits>

#include "simulator.h"
#include "solver.h"
#include "solbaskeritf.h"
#include "common.h"

// Trilinos ShyLU-Basker-backed sparse direct solver (multithreaded, BTF-aware).
//
// Basker is a shared-memory sparse LU factorization built for the block
// triangular form typical of circuit Jacobians - the same niche KLU occupies,
// but threaded. It is used here through its *native* C++ API (BaskerNS::Basker),
// not through Amesos2: the native API takes raw CSC arrays (zero-copy, like KLU
// and SuperLU_MT), splits cleanly into Symbolic / Factor / Solve that map
// straight onto rebuild() / factor() / solve(), and needs only Kokkos + Basker
// headers rather than the whole Tpetra/Teuchos/Amesos2 stack. (Basker::Factor_Inc
// - true incremental refactor reusing the previous numeric pivoting - is declared
// in Trilinos but never defined anywhere in the library, so refactor() just reruns
// Factor() on the existing symbolic structure; see lib/solbasker_common.h.)
//
// Basker's templated headers do not redeclare the Fortran BLAS symbols, so there
// is no blaslapack.h clash (unlike SuperLU_MT). The backend is still confined to
// lib/solbasker_{real,complex}.cpp (shared body in lib/solbasker_common.h)
// purely because the Kokkos + Basker headers are enormous and slow to compile
// and nothing else in VACASK should have to see them. This header sees only the
// opaque handle and entry points below.

namespace NAMESPACE {

namespace basker_wrapper {

// Opaque per-value-type solver handle.
template<typename Value> class SolverImpl;

// Allocate / free a solver instance.
template<typename Value> SolverImpl<Value>* create();
template<typename Value> void               destroy(SolverImpl<Value>*);

// Install / replace the CSC sparsity pattern. colptr / rowind / nzval are the
// caller's CSCMatrix buffers and must stay valid until the next setPattern() or
// clear(); their values may change freely between factor() calls, the pattern
// must not. minBlockSize is the block-size hint from setBlockSize() (0 = none).
template<typename Value>
void setPattern(
    SolverImpl<Value>*, int n, int nnz,
    const MatrixEntryIndex* colptr, const MatrixEntryIndex* rowind,
    const Value* nzval, int minBlockSize
);

// Numeric factorization of the values currently in the buffer handed to
// setPattern(). fact: 0 = first factorization (runs Symbolic first),
// 1 or 2 = reuse the column ordering / symbolic structure (both just call
// Factor() again - see the Factor_Inc note above). Return codes:
//   0  success
//   1  singular / rank deficient
//   2  NaN encountered
//   3  out of memory (allocation / realloc failure)
//   4  other internal Basker error
//  -1  no pattern installed
template<typename Value> int factor(SolverImpl<Value>*, int fact);

// Triangular solves against the stored factorization. B is column-major with
// leading dimension n and is overwritten with the solution. trans: 0 = A,
// 1 = A^T (Basker::Solve(..., transpose=true)). Returns 0 on success.
template<typename Value> int solve(SolverImpl<Value>*, Value* B, int nrhs, int trans);

// LAPACK-style 1-norm reciprocal condition estimate (Hager/Higham iteration
// driven through solve()/tsolve(), since Basker exposes no native estimate).
// Returns 0 on success, -1 if not factored.
template<typename Value> int rcond(SolverImpl<Value>*, double* rc);

// Drop the factorization but keep the pattern and the column ordering.
template<typename Value> void freeFactor(SolverImpl<Value>*);

// Drop the factorization and the pattern.
template<typename Value> void clear(SolverImpl<Value>*);

} // namespace basker_wrapper


SIMPLE_ERRORCLASS(BaskerEnvError, "Failed to initialize the Trilinos Basker solver.");

SIMPLE_ERRORCLASS(BaskerSolveError, "Failed to solve factorized system.");

SIMPLE_ERRORCLASS(BaskerNumericError, "NaN encountered during Basker factorization.");

SIMPLE_ERRORCLASS(BaskerMemoryError, "Basker ran out of memory during factorization.");

SIMPLE_ERRORCLASS(BaskerInternalError, "Basker factorization failed (internal error).");

// Basker's native API does not report which column/pivot went singular, so
// (unlike the KLU / SuperLU_MT variants) column and node are best-effort only.
ERRORCLASS(BaskerFactorizationError)
    MatrixEntryIndex size;
    BaskerFactorizationError(MatrixEntryIndex size) : size(size) {}
    std::string format() const {
        return "Factorization failed, size=" + std::to_string(size) +
               ", matrix is singular or rank deficient.";
    }
END_ERRORCLASS(BaskerFactorizationError);

ERRORCLASS(BaskerRefactorizationError)
    MatrixEntryIndex size;
    BaskerRefactorizationError(MatrixEntryIndex size) : size(size) {}
    std::string format() const {
        return "Refactorization failed, size=" + std::to_string(size) +
               ", matrix is singular or rank deficient.";
    }
END_ERRORCLASS(BaskerRefactorizationError);


//   rebuild()  -> CSC pattern install (Symbolic is deferred to the first factor)
//   factor()   -> Basker::Symbolic + Basker::Factor
//   refactor() -> Basker::Factor reusing the existing ordering / symbolic structure
//   solve()    -> Basker::Solve (transpose flag for tsolve)
//   rcond()    -> 1-norm reciprocal condition estimate via repeated solves
template<typename IndexType, typename ValueType>
class BaskerLinearSparseSolver : public LinearSparseSolver<IndexType, ValueType> {
public:
    using Base   = LinearSparseSolver<IndexType, ValueType>;
    using Matrix = typename Base::Matrix;

    static constexpr bool complexValue = std::is_same<ValueType, Complex>::value;

    static_assert(
        std::is_same<ValueType, double>::value || std::is_same<ValueType, Complex>::value,
        "BaskerLinearSparseSolver value type is neither double nor std::complex<double>."
    );
    static_assert(
        std::is_same<IndexType, std::int32_t>::value,
        "BaskerLinearSparseSolver index type must be int32_t (Basker Int)."
    );

    // Name this solver is registered under.
    static inline const Id solverId = Id::createStatic("basker");

    explicit BaskerLinearSparseSolver(Matrix& matrix) : Base(matrix) {}

    ~BaskerLinearSparseSolver() override {
        if (impl_) {
            basker_wrapper::destroy(impl_);
        }
    }

    static Base* create(Matrix& matrix) {
        return new BaskerLinearSparseSolver(matrix);
    }

    bool isBuilt() const override { return built_; }
    bool isFactored() const override { return factored_; }

    void clear() override {
        if (impl_) {
            basker_wrapper::clear(impl_);
        }
        built_        = false;
        factored_     = false;
        factExecuted_ = false;
    }

    void setBlockSize(UnknownIndex blockSize) override { minBlockSize_ = static_cast<int>(blockSize); }

    bool rebuild(ErrorConsumer& ec) override {
        if (!impl_) {
            impl_ = basker_wrapper::create<ValueType>();
        }
        if (!impl_) {
            ec.push(BaskerEnvError{});
            return false;
        }
        clear();

        auto& m = this->matrix();
        basker_wrapper::setPattern(
            impl_,
            static_cast<int>(m.nRow()), static_cast<int>(m.nnz()),
            m.apData(), m.aiData(), m.axData(), minBlockSize_
        );
        built_ = true;
        return true;
    }

    bool factor(ErrorConsumer& ec) override {
        if (!built_ && !rebuild(ec)) {
            return false;
        }

        auto* acct = this->matrix().accounting();
        auto t0 = Accounting::wclk();
        if (acct) {
            if constexpr (complexValue) { acct->acctNew.cxfactor++; }
            else { acct->acctNew.factor++; }
        }

        int info = basker_wrapper::factor(impl_, factExecuted_ ? 1 : 0);
        bool ok = checkInfo(ec, info, false);

        if (acct) {
            if constexpr (complexValue) { acct->acctNew.tcxfactor += Accounting::wclkDelta(t0); }
            else { acct->acctNew.tfactor += Accounting::wclkDelta(t0); }
        }
        return ok;
    }

    bool refactor(ErrorConsumer& ec) override {
        if (!factored_) {
            return factor(ec);
        }

        auto* acct = this->matrix().accounting();
        auto t0 = Accounting::wclk();
        if (acct) {
            if constexpr (complexValue) { acct->acctNew.cxrefactor++; }
            else { acct->acctNew.refactor++; }
        }

        int info = basker_wrapper::factor(impl_, 2);
        bool ok = checkInfo(ec, info, true);

        if (acct) {
            if constexpr (complexValue) { acct->acctNew.tcxrefactor += Accounting::wclkDelta(t0); }
            else { acct->acctNew.trefactor += Accounting::wclkDelta(t0); }
        }
        return ok;
    }

    std::tuple<bool, double> rcond(ErrorConsumer& ec) override {
        if (!factored_) {
            return { false, 0.0 };
        }
        double rc = 0.0;
        if (basker_wrapper::rcond(impl_, &rc) != 0) {
            return { false, 0.0 };
        }
        return { true, rc };
    }

    bool solve(ValueType* rhs, ErrorConsumer& ec) override { return runSolve<false>(rhs, 1, ec); }
    bool solve(ValueType* B, IndexType nrhs, ErrorConsumer& ec) override { return runSolve<false>(B, nrhs, ec); }
    bool tsolve(ValueType* rhs, ErrorConsumer& ec) override { return runSolve<true>(rhs, 1, ec); }
    bool tsolve(ValueType* B, IndexType nrhs, ErrorConsumer& ec) override { return runSolve<true>(B, nrhs, ec); }

protected:
    basker_wrapper::SolverImpl<ValueType>* impl_ { nullptr };

    int  minBlockSize_ { 0 };
    bool built_        { false };
    bool factored_     { false };
    bool factExecuted_ { false };   // a factorization has populated L / U

    bool checkInfo(ErrorConsumer& ec, int info, bool isRefactor) {
        auto n = static_cast<MatrixEntryIndex>(this->matrix().nRow());
        switch (info) {
            case 0:
                factExecuted_ = true;
                factored_     = true;
                return true;
            case 1:
                if (isRefactor) { ec.push(BaskerRefactorizationError{ n }); }
                else            { ec.push(BaskerFactorizationError{ n }); }
                break;
            case 2:  ec.push(BaskerNumericError{});  break;
            case 3:  ec.push(BaskerMemoryError{});   break;
            default: ec.push(BaskerInternalError{}); break;
        }
        basker_wrapper::freeFactor(impl_);
        factExecuted_ = false;
        factored_     = false;
        return false;
    }

    template<bool Transpose>
    bool runSolve(ValueType* B, IndexType nrhs, ErrorConsumer& ec) {
        if (!factored_) {
            throw std::logic_error(
                std::string("BaskerLinearSparseSolver::") + (Transpose ? "tsolve" : "solve") +
                ": matrix is not factored."
            );
        }

        auto* acct = this->matrix().accounting();
        auto t0 = Accounting::wclk();
        if (acct) {
            if constexpr (complexValue) { acct->acctNew.cxsolve += nrhs; }
            else { acct->acctNew.solve += nrhs; }
        }

        int info = basker_wrapper::solve(impl_, B, static_cast<int>(nrhs), Transpose ? 1 : 0);

        if (acct) {
            if constexpr (complexValue) { acct->acctNew.tcxsolve += Accounting::wclkDelta(t0); }
            else { acct->acctNew.tsolve += Accounting::wclkDelta(t0); }
        }

        if (info != 0) {
            ec.push(BaskerSolveError{});
            return false;
        }
        return true;
    }
};


typedef BaskerLinearSparseSolver<MatrixEntryIndex, double>  BaskerRealSparseSolver;
typedef BaskerLinearSparseSolver<MatrixEntryIndex, Complex> BaskerComplexSparseSolver;

}

#endif
