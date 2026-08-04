// Standalone test: same circuit as cosim_vpi, but run with coroutine API directly
#include "libplatform.h"
#include "simulator.h"
#include "parser.h"
#include "openvafcomp.h"
#include "circuit.h"
#include "an.h"
#include <cstdio>

using namespace sim;

int main() {
    std::string modulePath = VACASK_MOD_PATH;
    Status s;

    printf("Setting up VACASK...\n");
    Simulator::setup();
    Simulator::prependModulePath({modulePath});

    ParserTables tab("cosim_test");
    Parser p(tab);

    tab.add(PTLoad("resistor.osdi"))
       .add(PTLoad("capacitor.osdi"))
       .defaultGround()
       .setDefaultSubDef(
           PTSubcircuitDefinition()
           .add(PTModel("res", "resistor"))
           .add(PTModel("cap", "capacitor"))
           .add(PTModel("vsrc", "vsource"))
           .add(PTInstance("v1", "vsrc", {"1", "0"})
               .add(p.parseParameters(
                   "type=\"pulse\" val0=0 val1=1.8 delay=0 rise=100p fall=100p width=500n period=1u"))
           )
           .add(PTInstance("r1", "res", {"1", "2"})
               .add(PV{"r", 50.0})
           )
           .add(PTInstance("c1", "cap", {"2", "0"})
               .add(PV{"c", 1e-12})
           )
       );

    printf("Verifying tables...\n");
    if (!tab.verify(s)) {
        fprintf(stderr, "Table verify failed: %s\n", s.message().c_str());
        return 1;
    }

    printf("Creating circuit...\n");
    OpenvafCompiler comp;
    Circuit cir(tab, &comp, s);
    if (!cir.isValid()) {
        fprintf(stderr, "Circuit creation failed: %s\n", s.message().c_str());
        return 1;
    }

    cir.setOption("reltol", 1e-4);

    printf("Elaborating...\n");
    if (!cir.elaborate({}, "__topdef__", "__topinst__", nullptr, s)) {
        fprintf(stderr, "Elaboration failed: %s\n", s.message().c_str());
        return 1;
    }

    printf("Creating analysis...\n");
    auto tranDesc = PTAnalysis("cosim_tran", "tran");
    tranDesc
        .add(PV{"step", 1e-9})
        .add(PV{"stop", 10e-6});

    auto tran = Analysis::create(tranDesc, cir, s);
    if (!tran) {
        fprintf(stderr, "Analysis creation failed: %s\n", s.message().c_str());
        return 1;
    }

    tran->add(PTSave("default"));

    printf("Starting coroutine...\n");
    if (!tran->start(s)) {
        fprintf(stderr, "Start failed: %s\n", s.message().c_str());
        return 1;
    }

    printf("First resume...\n");
    int step = 0;
    while (tran->isRunning()) {
        auto state = tran->resume();
        step++;
        if (state == AnalysisState::SweepPoint) {
            printf("  SweepPoint at step %d\n", step);
        } else if (state == AnalysisState::Finished) {
            printf("  Finished at step %d\n", step);
            break;
        } else if (state == AnalysisState::Aborted) {
            fprintf(stderr, "  Aborted at step %d: %s\n", step, s.message().c_str());
            break;
        } else if (state == AnalysisState::Stopped) {
            printf("  Stopped at step %d\n", step);
            break;
        }
    }

    tran->finish(s);
    delete tran;
    printf("Done. %d steps.\n", step);
    return 0;
}
