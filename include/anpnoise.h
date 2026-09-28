#ifndef __ANPNOISE_DEFINED
#define __ANPNOISE_DEFINED

#include "anpsssmsig.h"
#include "corepss.h"
#include "corepnoise.h"
#include "parameterized.h"
#include "common.h"


namespace NAMESPACE {

// PNoise analysis data
class PNoiseData {
public:
    static inline const Id analysisId = Id::createStatic("pnoise");

protected:
    // Field names (jacSpec, smsigMatrix, smsigSolution) are fixed by the
    // PssSmallSignal template (anpsssmsig.h), which references smsigMatrix
    // by name; see PacData for the same constraint.
    CSCBlockSparseComplexMatrix jacSpec;
    CSCBlockSparseComplexMatrix smsigMatrix;
    Vector<Complex> smsigSolution;

    std::unordered_map<std::pair<Id, Id>, size_t> contributionOffset;
    Vector<double> results;
    double powerGain;
    double outputNoise;
};

// Constructor specialization
template<> PssSmallSignal<PNoiseCore, PNoiseData>::PssSmallSignal(const std::string& name, Circuit& circuit, PTAnalysis& ptAnalysis);

// Resolve save specialization
template<> bool PssSmallSignal<PNoiseCore, PNoiseData>::resolveSave(const PTSave& save, bool verify, ErrorConsumer& errors);

// Dump specialization
template<> void PssSmallSignal<PNoiseCore, PNoiseData>::dump(std::ostream& os) const;

// Typedef PNoise
typedef PssSmallSignal<PNoiseCore, PNoiseData> PNoise;

}

#endif
