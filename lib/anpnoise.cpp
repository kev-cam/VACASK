#include "anpnoise.h"
#include "common.h"


namespace NAMESPACE {

template<> PssSmallSignal<PNoiseCore, PNoiseData>::PssSmallSignal(const std::string& name, Circuit& circuit, PTAnalysis& ptAnalysis)
    : Analysis(name, circuit, ptAnalysis),
      opCore_(*this, params.core().pssParams.opParams, circuit, commons, jac_, opSolution_, states_, delayLines_, delayBindings_),
      stabilTran_(*this, params.core().pssParams.stabilParams, opCore_, circuit, commons, jac_, opSolution_, solution_, states_, delayLines_, delayBindings_),
      pssTran_(*this, params.core().pssParams.shootParams, opCore_, circuit, commons, jac_, opSolution_, solution_, states_, delayLines_, delayBindings_),
      pssCore_(*this, params.core().pssParams, circuit, commons, jac_, solution_, states_, opCore_, stabilTran_, pssTran_),
      smsigCore(
        *this, params.core(), pssCore_, contributionOffset, circuit, commons,
        jacSpec, smsigMatrix, smsigSolution, results, powerGain, outputNoise
      ) {
}

template<> bool PssSmallSignal<PNoiseCore, PNoiseData>::resolveSave(const PTSave& save, bool verify, ErrorConsumer& errors) {
    // Noise saves
    static const auto idDefault = Id("default");
    static const auto idFull = Id("full");
    static const auto idN  = Id("n");
    static const auto idNc = Id("nc");

    // When verification is not required, save-resolution errors are not fatal.
    ErrorConsumer sink;
    ErrorConsumer& e1 = verify ? errors : sink;

    bool st = true;
    bool handled = true;
    bool addLoc = true;
    if (save.typeName() == idDefault) {
        st = smsigCore.addAllNoiseContribInst(save, false, e1);
    } else if (save.typeName() == idFull) {
        st = smsigCore.addAllNoiseContribInst(save, true, e1);
    } else if (save.typeName() == idN) {
        st = smsigCore.addNoiseContribInst(save, false, e1);
    } else if (save.typeName() == idNc) {
        st = smsigCore.addNoiseContribInst(save, true, e1);
    } else {
        // Handle PSS saves
        std::tie(st, handled) = resolvePssSave(save, verify, e1);
        // resolvePssSave() adds location to error
        addLoc = false;
        // Not handled error was formatted by resolvePssSave()
        // Also all PSS errors were formatted
        if (!verify) {
            // No checking, assume status is OK
            st = true;
        }
    }

    // Handled save via smsigCore, check error if verification required
    if (verify && !st) {
        // Format error
        if (addLoc) {
            errors.push(AnSaveDirectiveLocation{save.location()});
        }
        return false;
    }

    // No error
    return true;
}

template<> void PssSmallSignal<PNoiseCore, PNoiseData>::dump(std::ostream& os) const {
    Analysis::dump(os);
    os << "Analysis type: PNOISE periodic small-signal noise\n";
    os << "PSS analysis core:\n";
    pssCore_.dump(os);
    os << "\n";
    os << "PNOISE small-signal analysis core:\n";
    smsigCore.dump(os);
}

}
