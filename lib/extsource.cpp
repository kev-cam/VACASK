#include <algorithm>
#include "extsource.h"
#include "dynload.h"
#include "common.h"

namespace NAMESPACE {

TranSync* TranSync::hook = nullptr;

std::vector<ExtSource*>& ExtSource::registry() {
    static std::vector<ExtSource*> reg;
    return reg;
}

bool ExtSource::isUri(const std::string& uri) {
    return uri.rfind("code:", 0)==0;
}

std::shared_ptr<ExtSource> ExtSource::create(const std::string& uri, bool isVoltageSource, std::string& err) {
    // code:<library>:<initfn>[:<args>]
    if (!isUri(uri)) {
        err = "Not a code: URI '"+uri+"'.";
        return nullptr;
    }
    auto rest = uri.substr(5);
    auto p1 = rest.find(':');
    if (p1==std::string::npos || p1==0) {
        err = "Malformed URI '"+uri+"', expected code:<library>:<function>:<args>.";
        return nullptr;
    }
    auto libName = rest.substr(0, p1);
    auto p2 = rest.find(':', p1+1);
    auto fnName = p2==std::string::npos ? rest.substr(p1+1) : rest.substr(p1+1, p2-p1-1);
    auto args = p2==std::string::npos ? std::string() : rest.substr(p2+1);
    if (fnName.empty()) {
        err = "Malformed URI '"+uri+"', missing function name.";
        return nullptr;
    }

    auto handle = openDynamicLibrary(libName.c_str());
    if (!handle) {
        err = "Failed to load '"+libName+"': "+dynamicLibraryError();
        return nullptr;
    }
    auto init = (VacaskExtSourceInit)dynamicLibrarySymbol(handle, fnName.c_str());
    if (!init) {
        err = "Function '"+fnName+"' not found in '"+libName+"'.";
        closeDynamicLibrary(handle);
        return nullptr;
    }

    std::shared_ptr<ExtSource> ext(new ExtSource());
    ext->lib = handle;
    ext->src.abi = VACASK_EXTSRC_ABI;
    if (!init(args.c_str(), isVoltageSource ? 1 : 0, &ext->src)) {
        err = "Initialization '"+fnName+"("+args+")' in '"+libName+"' failed.";
        // Do not call destroy, init failed
        ext->src = VacaskExtSource {};
        return nullptr;
    }
    if (!ext->src.value) {
        err = "Initialization '"+fnName+"("+args+")' in '"+libName+"' did not provide a value function.";
        return nullptr;
    }

    registry().push_back(ext.get());
    return ext;
}

ExtSource::~ExtSource() {
    auto& reg = registry();
    reg.erase(std::remove(reg.begin(), reg.end(), this), reg.end());
    if (src.destroy) {
        src.destroy(src.ctx);
    }
    if (lib) {
        closeDynamicLibrary(lib);
    }
}

double ExtSource::nextBreakpoint(double t) {
    double tBr = 0;
    for(auto ext : registry()) {
        double nb;
        ext->value(t, nb);
        if (nb>t && (tBr==0 || nb<tBr)) {
            tBr = nb;
        }
    }
    return tBr;
}

bool ExtSource::notifyAccepted(double t, const double* solution) {
    bool pause = false;
    for(auto ext : registry()) {
        if (ext->src.accepted) {
            // Unknown 0 is the ground node, its value is always 0
            if (ext->src.accepted(ext->src.ctx, t, solution[ext->uP]-solution[ext->uN])) {
                pause = true;
            }
        }
    }
    return pause;
}

}
