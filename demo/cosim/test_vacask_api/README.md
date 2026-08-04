# Standalone VACASK C++ API test

`test_vacask.cpp` builds the same circuit as `example/` (a pulse
source into an R/C), but runs it directly against VACASK's own
coroutine API (`Analysis::start()`/`resume()`) instead of through the
cosim bridge -- no VPI, no Verilog, no Icarus involved at all.

It exists to isolate VACASK's C++ API from the bridge: if something
here breaks, the problem is in how VACASK's API is used, not in the
bridge or the digital side. It's also a minimal, runnable companion
to `docs/cpp-api.md` -- a second, working example of the same
build-circuit/create-analysis/resume-loop sequence documented there.

Single file, no Makefile -- one compile command:

```sh
g++ -std=c++20 -DNAMESPACE=sim \
    -DVACASK_MOD_PATH='"'$VACASK_PREFIX'/lib/vacask/mod"' \
    -I$VACASK_PREFIX/include \
    -o test_vacask test_vacask.cpp \
    $VACASK_PREFIX/lib64/libsimlib.a \
    -L$VACASK_PREFIX/lib64 -lklu -lbtf -lamd -lcolamd -lcamd -lccolamd -lcholmod -lsuitesparseconfig \
    -L$VACASK_PREFIX/lib -lboost_filesystem -lboost_system -lboost_process \
    -ldl -lpthread -lm \
    -Wl,-rpath,$VACASK_PREFIX/lib64 -Wl,-rpath,$VACASK_PREFIX/lib

./test_vacask
```

Expected output ends with `Finished at step 1` / `Done. 1 steps.` --
verified against a real build.
