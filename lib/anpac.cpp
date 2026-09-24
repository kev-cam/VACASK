#include "anpac.h"
#include "simulator.h"
#include "common.h"


namespace NAMESPACE {

template<> PssSmallSignal<PACCore, PacData>::PssSmallSignal(const std::string& name, Circuit& circuit, PTAnalysis& ptAnalysis)
    : Analysis(name, circuit, ptAnalysis),
      opCore_(*this, params.core().pssParams.opParams, circuit, commons, jac_, opSolution_, states_, delayLines_, delayBindings_),
      stabilTran_(*this, params.core().pssParams.stabilParams, opCore_, circuit, commons, jac_, opSolution_, solution_, states_, delayLines_, delayBindings_),
      pssTran_(*this, params.core().pssParams.shootParams, opCore_, circuit, commons, jac_, opSolution_, solution_, states_, delayLines_, delayBindings_),
      pssCore_(*this, params.core().pssParams, circuit, commons, jac_, solution_, states_, opCore_, stabilTran_, pssTran_),
      smsigCore(*this, params.core(), pssCore_, circuit, commons, jacSpec, smsigMatrix, smsigSolution) {
}

template<> bool PssSmallSignal<PACCore, PacData>::resolveSave(const PTSave& save, bool verify, ErrorConsumer& errors) {
    static const auto idDefault  = Id("default");
    static const auto idFull     = Id("full");
    static const auto idDv       = Id("dv");
    static const auto idDi       = Id("di");

    // When verification is not required, save-resolution errors are not fatal.
    ErrorConsumer sink;
    ErrorConsumer& e1 = verify ? errors : sink;

    bool st = true;
    bool handled = true;
    if (save.typeName() == idDefault) {
        st = smsigCore.addAllUnknowns(save, e1);
    } else if (save.typeName() == idFull) {
        st = smsigCore.addAllNodes(save, e1);
    } else if (save.typeName() == idDv) {
        st = smsigCore.addNode(save, e1);
    } else if (save.typeName() == idDi) {
        st = smsigCore.addFlow(save, e1);
    } else {
        std::tie(st, handled) = resolvePssSave(save, verify, e1);
        if (!verify) {
            st = true;
        }
    }

    if (verify && !st) {
        errors.push(AnSaveDirectiveLocation{save.location()});
        return false;
    }
    return true;
}

template<> void PssSmallSignal<PACCore, PacData>::dump(std::ostream& os) const {
    Analysis::dump(os);
    os << "Analysis type: PAC periodic small-signal\n";
    os << "PSS analysis core:\n";
    pssCore_.dump(os);
    os << "\n";
    os << "PAC small-signal analysis core:\n";
    smsigCore.dump(os);
}

}
