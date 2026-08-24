#include "libplatform.h"
#include "simulator.h"
#include "parser.h"
#include "openvafcomp.h"
#include "circuit.h"

using namespace sim;

int main() {
    Status s;

    // ------------------------------------------------------------------------
    // Simulator setup
    // ------------------------------------------------------------------------

    Simulator::setup();

    // Adjust these to your installation.
    std::string modulePath = "../../lib/vacask/mod";
    Simulator::prependModulePath({modulePath});

    ParserTables tab("Self-heating lightbulb");
    Parser p(tab);

    // ------------------------------------------------------------------------
    // Circuit description
    // ------------------------------------------------------------------------

    tab
        .add(PTLoad("vsource.osdi"))
        .defaultGround()

        .setDefaultSubDef(
            PTSubcircuitDefinition()

            // ------------------------------------------------------------
            // Voltage source
            // ------------------------------------------------------------

            .add(PTModel("vsrc", "vsource"))

            .add(
                PTInstance("v1", "vsrc", {"a", "0"})
                    .add(
                        p.parseParameters(
                            "dc=0 "
                            "type=\"sine\" "
                            "sinedc=0 "
                            "ampl=325 "
                            "freq=50"
                        )
                    )
            )

            // ------------------------------------------------------------
            // Electrical behavioral current source
            //
            // i(a,0) =
            //   V(a,0) /
            //   (r0*(p0+p1*T+p2*T^2))
            // ------------------------------------------------------------

            .add(
                PTBehavioral(
                    "bi",
                    {"a", "0"},
                    p.parseExpression(
                        "v(a,0)/(r0*(p0+p1*v(t)+p2*v(t)**2))"
                    ),
                    true
                )
            )

            // ------------------------------------------------------------
            // Thermal capacitance
            //
            // flow = ddt(cth*T)
            // ------------------------------------------------------------

            .add(
                PTBehavioral(
                    "ct",
                    {"t", "0"},
                    p.parseExpression(
                        "ddt(cth*v(t))"
                    ),
                    false,
                    "thermal",
                    "Temp",
                    "Pwr"
                )
            )

            // ------------------------------------------------------------
            // Thermal resistance
            //
            // flow = (T-Ta)/Rth
            // ------------------------------------------------------------

            .add(
                PTBehavioral(
                    "rt",
                    {"t", "0"},
                    p.parseExpression(
                        "(v(t)-ta)/rth"
                    ),
                    false,
                    "thermal",
                    "Temp",
                    "Pwr"
                )
            )

            // ------------------------------------------------------------
            // Electrical power -> thermal power
            //
            // flow =
            //   -V(a,0)^2 / R(T)
            // ------------------------------------------------------------

            .add(
                PTBehavioral(
                    "pwr",
                    {"t", "0"},
                    p.parseExpression(
                        "-(v(a,0)**2/(r0*(p0+p1*v(t)+p2*v(t)**2)))"
                    ),
                    false,
                    "thermal",
                    "Temp",
                    "Pwr"
                )
            )
        );

    // ------------------------------------------------------------------------
    // Verify parser tables
    // ------------------------------------------------------------------------

    if (!tab.verify(s)) {
        Simulator::err() << s.message() << "\n";
        return 1;
    }

    tab.dump(0, Simulator::out());

    // ------------------------------------------------------------------------
    // Create compiler and circuit
    // ------------------------------------------------------------------------

    OpenvafCompiler comp;

    Circuit cir(tab, &comp, s);

    if (!cir.isValid()) {
        Simulator::err() << s.message() << "\n";
        return 1;
    }

    cir.setVariable("rth", (3000.0 - 25.0) / 60.0);
    cir.setVariable("cth", 10e-3);
    cir.setVariable("ta", 25.0);
    cir.setVariable("r0", 8.612);
    cir.setVariable("p0", 8.612);
    cir.setVariable("p1", 0.0269);
    cir.setVariable("p2", 1.914e-6);


    // ------------------------------------------------------------------------
    // Elaborate
    // ------------------------------------------------------------------------

    if (!cir.elaborate(
            {},
            "__topdef__",
            "__topinst__",
            nullptr,
            s)) {

        Simulator::err() << "Elaboration failed.\n";
        Simulator::err() << s.message() << "\n";
        return 1;
    }

    // ------------------------------------------------------------------------
    // DC sweep
    //
    // Original:
    //
    // alter instance("v1") type="dc"
    // sweep v1 instance="v1" parameter="dc"
    //       from=0 to=230 mode=lin points=50
    //     analysis dc1 op
    //
    // ------------------------------------------------------------------------

    cir.setInstanceParameter("v1", "type", "dc");

    auto dc1Desc = PTAnalysis("dc1", "op");

    dc1Desc
        .add(
            PTSweep("v1")
                .add(PV("instance", "v1"))
                .add(PV("parameter", "dc"))
                .add(PV("from", 0))
                .add(PV("to", 230))
                .add(PV("mode", "lin"))
                .add(PV("points", 50))
        );

    auto dc1 = Analysis::create(dc1Desc, cir, s);

    if (!dc1) {
        Simulator::err() << "Failed to create DC analysis.\n";
        Simulator::err() << s.message() << "\n";
        return 1;
    }

    if (auto [ok, canResume] = dc1->run(s); !ok) {
        Simulator::err() << "DC analysis failed.\n";
        Simulator::err() << s.message() << "\n";
        delete dc1;
        return 1;
    }

    Simulator::out() << "DC analysis OK.\n";

    delete dc1;

    // ------------------------------------------------------------------------
    // Transient analysis
    //
    // Original:
    //
    // alter instance("v1") type="sine"
    // analysis tran1 tran stop=2 step=0.01m maxstep=0.05m
    //
    // ------------------------------------------------------------------------

    cir.setInstanceParameter("v1", "type", "sine");

    auto tran1Desc = PTAnalysis("tran1", "tran");

    tran1Desc
        .add(PV("stop", 2.0))
        .add(PV("step", 0.01e-3))
        .add(PV("maxstep", 0.05e-3));

    auto tran1 = Analysis::create(tran1Desc, cir, s);

    if (!tran1) {
        Simulator::err() << "Failed to create transient analysis.\n";
        Simulator::err() << s.message() << "\n";
        return 1;
    }

    if (auto [ok, canResume] = tran1->run(s); !ok) {
        Simulator::err() << "Transient analysis failed.\n";
        Simulator::err() << s.message() << "\n";
        delete tran1;
        return 1;
    }

    Simulator::out() << "Transient analysis OK.\n";

    delete tran1;

    // ------------------------------------------------------------------------
    // Harmonic balance
    //
    // Original:
    //
    // analysis hb1 hb freq=[50] nharm=10
    //
    // ------------------------------------------------------------------------

    auto hb1Desc = PTAnalysis("hb1", "hb");

    hb1Desc
        .add(PV("freq", std::vector<double>{50.0}))
        .add(PV("nharm", 10));

    auto hb1 = Analysis::create(hb1Desc, cir, s);

    if (!hb1) {
        Simulator::err() << "Failed to create HB analysis.\n";
        Simulator::err() << s.message() << "\n";
        return 1;
    }

    if (auto [ok, canResume] = hb1->run(s); !ok) {
        Simulator::err() << "HB analysis failed.\n";
        Simulator::err() << s.message() << "\n";
        delete hb1;
        return 1;
    }

    Simulator::out() << "HB analysis OK.\n";

    delete hb1;

    return 0;
}
