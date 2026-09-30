#include "vacaskcinterface.h"
#include "parser.h"
#include "status.h"
#include "openvafcomp.h"
#include "circuit.h"
#include "simulator.h"
#include "extsource.h"
#include "platform.h"
#include "cmd.h"
#include "config.h"
#include "common.h"
#include <filesystem>
#include <fstream>
#include <memory>
#include <cmath>
#include <cstdlib>
#include <unordered_set>

using namespace NAMESPACE;

namespace {

// One co-simulation session. It is also the TranSync hook that pauses
// the transient analysis at the target time requested by simulateUntil().
class Session : public TranSync {
public:
    ParserTables tab;
    std::unique_ptr<OpenvafCompiler> comp;
    std::unique_ptr<Circuit> circuit;
    std::unique_ptr<CommandInterpreter> interp;

    double target {0.0};   // Pause when an accepted point reaches this time
    double time {0.0};     // Last accepted transient time
    bool complete {false};
    bool failed {false};

    virtual double nextBreakpoint(double t) override {
        return target>t ? target : 0.0;
    }

    virtual bool accepted(double t) override {
        time = t;
        return t>=target-1e-12*std::abs(target);
    }

    // Run the control block until it pauses in a transient or ends
    bool run() {
        Status s;
        auto st = interp->run(0, s);
        if (st==InterpreterExitStatus::Paused) {
            return true;
        } else if (st==InterpreterExitStatus::EndReached) {
            complete = true;
            return true;
        }
        Simulator::err() << s.message() << "\n";
        failed = true;
        return false;
    }
};

Session* session(void** ptr) {
    return ptr ? static_cast<Session*>(*ptr) : nullptr;
}

}

extern "C" {

void vacask_open(void** ptr) {
    if (ptr) {
        *ptr = new Session();
    }
}

int vacask_initialize(void** ptr, int argc, char** argv) {
    auto S = session(ptr);
    if (!S || argc<1 || !argv[argc-1]) {
        return 0;
    }
    const char* fileArg = argv[argc-1];
    Status status;

    Platform::setup();
    if (auto e = std::getenv("SIM_OPENVAF")) {
        Platform::setOpenVaf(e);
    }
    auto modCstring = std::getenv("SIM_MODULE_PATH");
    auto incCstring = std::getenv("SIM_INCLUDE_PATH");
    if (!Simulator::setup(
        modCstring ? modCstring : "", incCstring ? incCstring : "",
        Simulator::defaultNcpu, Simulator::defaultBlasNcpu, status
    )) {
        Simulator::err() << status.message() << "\n";
        return 0;
    }

    auto stackPosition = S->tab.fileStack().addFile(fileArg);
    if (stackPosition==FileStack::badFileId) {
        Simulator::err() << std::string("File '")+fileArg+"' not found.\n";
        return 0;
    }
    auto inputFileDir = std::filesystem::path(S->tab.fileStack().canonicalName(stackPosition)).parent_path();

    // Configuration files, as in the vacask executable
    std::vector<std::string> configFiles = {
        Platform::systemConfig(),
        Platform::userConfig(),
        Platform::localConfig(),
        (inputFileDir / ".vacaskrc.toml").string(),
    };
    std::unordered_set<std::string> processed;
    for(auto& cfg : configFiles) {
        if (processed.contains(cfg)) {
            continue;
        }
        processed.insert(cfg);
        std::ifstream f(cfg);
        if (f && !readConfig(f, cfg, status)) {
            Simulator::err() << status.message() << "\n";
            return 0;
        }
    }

    Parser parser(S->tab);
    if (!parser.parseNetlistFile(stackPosition, status) ||
        !S->tab.verify(status) ||
        !S->tab.processBehaviorals(Simulator::fileDebug(), status)) {
        Simulator::err() << status.message() << "\n";
        return 0;
    }

    S->comp = std::make_unique<OpenvafCompiler>(Platform::openVaf(), Platform::openVafArgs());
    S->circuit = std::make_unique<Circuit>(S->tab, S->comp.get(), status);
    if (!S->circuit->isValid()) {
        Simulator::err() << status.message() << "\n";
        return 0;
    }

    S->interp = std::make_unique<CommandInterpreter>(S->tab, S->tab.control(), *S->circuit);
    S->interp->setPrintProgress(false);
    S->interp->setPauseOnStop(true);

    // Run up to and including the initial point of the first transient
    TranSync::install(S);
    S->target = 0.0;
    return S->run() ? 1 : 0;
}

int vacask_simulateUntil(void** ptr, double t, double* actualTime) {
    auto S = session(ptr);
    if (!S || !S->interp) {
        return 0;
    }
    if (S->complete || S->failed) {
        if (actualTime) {
            *actualTime = S->time;
        }
        return 0;
    }
    if (t>S->time) {
        S->target = t;
        TranSync::install(S);
        S->run();
    }
    if (actualTime) {
        *actualTime = S->time;
    }
    return S->failed ? 0 : 1;
}

int vacask_simulationComplete(void** ptr) {
    auto S = session(ptr);
    return S && S->complete ? 1 : 0;
}

double vacask_getTime(void** ptr) {
    auto S = session(ptr);
    return S ? S->time : 0.0;
}

void vacask_close(void** ptr) {
    auto S = session(ptr);
    if (!S) {
        return;
    }
    if (S->interp) {
        // Flush outputs of a paused analysis
        S->interp->abandonPaused();
    }
    if (TranSync::installed()==S) {
        TranSync::install(nullptr);
    }
    S->interp.reset();
    S->circuit.reset();
    delete S;
    *ptr = nullptr;
}

}
