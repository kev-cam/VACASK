#ifndef __ANPAC_DEFINED
#define __ANPAC_DEFINED

#include "anpsssmsig.h"
#include "corepss.h"
#include "corepac.h"
#include "parameterized.h"
#include "common.h"


namespace NAMESPACE {

// PAC analysis data
class PacData {
public:
    static inline const Id analysisId = Id::createStatic("pac");

protected:
    CSCBlockSparseComplexMatrix jacSpec;
    CSCBlockSparseComplexMatrix smsigMatrix;
    Vector<Complex> smsigSolution;
};

// Constructor specialization
template<> PssSmallSignal<PACCore, PacData>::PssSmallSignal(const std::string& name, Circuit& circuit, PTAnalysis& ptAnalysis);

// Resolve save specialization
template<> bool PssSmallSignal<PACCore, PacData>::resolveSave(const PTSave& save, bool verify, ErrorConsumer& errors);

// Dump specialization
template<> void PssSmallSignal<PACCore, PacData>::dump(std::ostream& os) const;

// Typedef PAC
typedef PssSmallSignal<PACCore, PacData> PAC;

}

#endif
