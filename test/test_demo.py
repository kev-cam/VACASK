import os
import sys

from pyvacask import *
from pyvacask import simulator as sim

import numpy as np
import matplotlib.pyplot as plt

from pyvacask.lib.python.rawfile import rawread

def test_demo1(): 
    sim.setup()
    s   = Status()
    tab = ParserTables("RC transient")
    p   = Parser(tab)

    tab = tab.add(PTLoad("resistor.osdi"))
    tab = tab.add(PTLoad("capacitor.osdi"))
    tab = tab.defaultGround()
    sub = PTSubcircuitDefinition()
    par = PTParameters()
    par = par.add(PV("c0", 1e-6))
    par = par.add(PV("v0", 5))
    sub = sub.add(par)
    sub = sub.add(PTModel("res", "resistor"))
    sub = sub.add(PTModel("cap", "capacitor"))
    sub = sub.add(PTModel("vsrc", "vsource"))
    sub = sub.add(PTInstance("r1", "res", ["1", "2"]).add(PV("r", 1000)))
    sub = sub.add(PTInstance("c1", "cap", ["2", "0"]).add(PE("c", p.parseExpression("2*c0"))))
    sub = sub.add(PTInstance("v1", "vsrc", ["1", "0"]).add(
        p.parseParameters("type=\"pulse\" val0=0 val1=v0 delay=1m rise=1u fall=1u width=4m"))
    )
    tab = tab.setDefaultSubDef(sub)

    assert tab.verify(s)
    assert tab.writeEmbedded(1, s)

    comp = OpenvafCompiler()
    cir  = Circuit(tab, comp, s)
    assert cir.isValid()

    cir.setOption("reltol", Value(1e-4))
    assert cir.elaborate([], "__topdef__", "__topinst__", status=s)
    cir.dumpHierarchy(0)

    tran_desc = PTAnalysis("tran1", "tran")
    tran_desc.add(PV("step", 1e-6))
    tran_desc.add(PV("stop", 10e-3))

    tran = Analysis.create(tran_desc, cir, s)
    assert tran

    tran.add(PTSave("default"))
    tran.add(PTSave("p", "r1", "i"))

    (ok, _) = tran.run(s)
    assert ok

    tran1 = rawread('tran1.raw').get()
    print("Vectors:", tran1.names)
    fig1, _ = plt.subplots(1, 1, figsize=(6,4), dpi=100, constrained_layout=True)
    fig1.axes[0].plot(tran1["time"], tran1["2"], "r", marker=".")
    fig1.axes[0].plot(tran1["time"], tran1["1"], "b")
    fig1.axes[0].plot(tran1["time"], tran1["r1.i"]*1000, "--")
    fig1.savefig("d1tran1.jpg")

def test_demo2():
    sim.setup()
    s = Status()
    tab = ParserTables("Conditional blocks")
    p = Parser(tab)

    tab = tab.add(PTLoad("resistor.osdi"))
    tab = tab.defaultGround()
    sub = PTSubcircuitDefinition()
    sub = sub.add(PTModel("res", "resistor"))
    sub = sub.add(PTModel("vsrc", "vsource"))
    sub2 = PTSubcircuitDefinition("mysub", ["p", "out", "n"])
    sub2 = sub2.add(PV("r", 1000))
    sub2 = sub2.add(PV("fact", 0.5))
    sub2 = sub2.add(PV("mode", 0))
    r1i = PTInstance("r1", "res", ["p", "out"])
    r1i = r1i.add(PE("r", p.parseExpression("r*(1-fact)")))
    r2i = PTInstance("r2", "res", ["out", "n"])
    r2i = r2i.add(PE("r", p.parseExpression("r*fact")))
    r1e = PTInstance("r1", "res", ["p", "out"])
    r1e = r1e.add(PE("r", p.parseExpression("r*fact")))
    r2e = PTInstance("r2", "res", ["out", "n"])
    r2e = r2e.add(PE("r", p.parseExpression("r*(1-fact)")))
    sub2 = sub2.add(PTBlockSequence().add(p.parseExpression("mode==0"), PTBlock().add(r1i).add(r2i)).add(
        Rpn(), PTBlock().add(r1e).add(r2e)
    ))
    sub = sub.add(sub2)
    sub = sub.add(PTInstance("x1", "mysub", ["1", "2", "0"]).add(PV("r", 1000)).add(PV("fact", 0.2)))
    sub = sub.add(PTInstance("v1", "vsrc",  ["1", "0"]).add(PV("dc", 10)))
    tab = tab.setDefaultSubDef(sub)
    
    assert tab.verify(s)
    assert tab.writeEmbedded(1, s)

    comp = OpenvafCompiler()
    cir  = Circuit(tab, comp, s)
    assert cir.isValid()

    cir.setOption("reltol", Value(1e-4))
    cir.setVariable("var1", Value(60))

    pp = p.parseParameters("temp=var1")
    cir.setOptions(pp)

    assert cir.elaborate([], "__topdef__", "__topinst__", status=s)
    print(f"temp={cir.simulatorOptions().temp}")
    cir.dumpHierarchy(0)

    dcDesc = PTAnalysis("dc1", "op")
    dcDesc = dcDesc.add(PTSweep("mode")
        .add(PV("instance", "x1"))
        .add(PV("parameter", "mode"))
        .add(PV("values", [0, 1]))
    )
    dc = Analysis.create(dcDesc, cir, s)
    assert dc

    (ok, canResume) = dc.run(s)
    assert ok

    print(f"Analysis OK. Can resume {'true' if canResume else 'false'}.")

    dc1 = rawread("dc1.raw").get()
    print("Vectors:", dc1.names)
    fig1, _ = plt.subplots(1, 1, figsize=(6,4), dpi=100, constrained_layout=True)
    print("Mode:", dc1["mode"])
    print("Output:", dc1["2"])
    print("Expected: [2, 8]")
    fig1.savefig("d2dc1.jpg")

def test_demo3():
    sim.setup()
    s = Status()
    tab = ParserTables("Variables and parametrized options sweep")
    p = Parser(tab)

    tab = tab.add(PTLoad("resistor.osdi"))
    tab = tab.add(PTLoad("diode.osdi"))
    tab = tab.defaultGround()
    sub = PTSubcircuitDefinition()
    sub = sub.add(PTModel("res", "resistor"))
    sub = sub.add(PTModel("dio", "diode")
        .add(p.parseParameters("is=1e-12 n=2 rs=1 eg=1.2 xti=2"))
    )
    sub = sub.add(PTModel("vsrc", "vsource"))
    sub = sub.add(PTInstance("r1", "res", ["1", "2"])
        .add(PV("r", 100))
    )
    sub = sub.add(PTInstance("d1", "dio", ["2", "0"]))
    sub = sub.add(PTInstance("v1", "vsrc", ["1", "0"])
        .add(PV("dc", 2))
    )
    tab = tab.setDefaultSubDef(sub)

    assert tab.verify(s)

    # Dump tables for debugging
    tab.dump(0)

    assert tab.writeEmbedded(1, s)

    comp = OpenvafCompiler()
    cir  = Circuit(tab, comp, s)
    assert cir.isValid()

    cir.setVariable("myvar", Value(0))
    cir.setOption("reltol", Value(1e-4))

    assert cir.elaborate([], "__topdef__", "__topinst__", status=s)
    cir.dumpHierarchy(0)

    dc1Desc = PTAnalysis("dc1", "op")
    dc1Desc.add(PTSweep("temp").add(
        PV("variable", "myvar")).add(
        PV("from", -50)).add(
        PV("to", 100)).add(
        PV("step", 2))
    )
    anPar = p.parseParameters("temp=myvar")
    dc1 = Analysis.create(dc1Desc, cir, s)
    dc1.add(anPar)
    assert dc1
    ok, canResume = dc1.run(s)
    print(f"DC1 analysis OK. Can resume: {'true' if canResume else 'false'}\n" if ok else "DC1 analysis failed")

    dc2Desc = PTAnalysis("dc2", "op")
    dc2Desc.add(PTSweep("temp").add(
        PV("option", "temp")).add(
        PV("from", -50)).add(
        PV("to", 100)).add(
        PV("step", 2))
    )
    dc2 = Analysis.create(dc2Desc, cir, s)
    assert dc2
    ok, canResume = dc2.run(s)
    print(f"DC2 analysis OK. Can resume: {'true' if canResume else 'false'}\n" if ok else "DC2 analysis failed")

    dc1 = rawread("dc1.raw").get()
    dc2 = rawread("dc2.raw").get()
    print("Vectors:", dc1.names)
    fig1, _ = plt.subplots(1, 1, figsize=(6,4), dpi=100, constrained_layout=True)
    fig1.axes[0].plot(dc2["temp"], dc2["2"], "g")
    fig1.axes[0].plot(dc1["temp"], dc1["2"], "r", marker=".", linestyle="none")
    fig1.savefig("d3dc1.jpg")

def test_demo4():
    sim.setup()
    s = Status()
    tab = ParserTables("Variables and parametrized options sweep")
    p = Parser(tab)

    tab = tab.add(PTLoad("resistor.osdi"))
    tab = tab.add(PTLoad("diode.osdi"))
    tab = tab.defaultGround()
    sub = PTSubcircuitDefinition()
    sub = sub.add(PTModel("res", "resistor"))
    sub = sub.add(PTModel("dio", "diode")
        .add(p.parseParameters("is=1e-12 n=2 rs=1 eg=1.2 xti=2"))
    )
    sub = sub.add(PTModel("vsrc", "vsource"))
    sub = sub.add(PTSubcircuitDefinition("sub1").add(
        PTInstance("r1", "res", ["1", "2"]).add(PV("r", 10))).add(
        PTInstance("d1", "dio", ["2", "0"]))
    )
    sub = sub.add(PTSubcircuitDefinition("sub2").add(
        PTInstance("d1", "dio", ["1", "2"])).add(
        PTInstance("r1", "res", ["2", "0"]).add(PV("r", 10)))
    )
    sub = sub.add(PTInstance("v1", "vsrc", ["1", "0"]).add(PV("dc", 10)))
    tab = tab.setDefaultSubDef(sub)

    assert tab.verify(s)

    # Dump tables for debugging
    tab.dump(0)

    assert tab.writeEmbedded(1, s)

    comp = OpenvafCompiler()
    cir  = Circuit(tab, comp, s)
    assert cir.isValid()

    cir.setVariable("myvar", Value(0))
    cir.setOption("reltol", Value(1e-4))

    assert cir.elaborate([Id("sub1")], "__topdef__", "__topinst__", status=s)
    cir.dumpHierarchy(0)
    
    dc1Desc = PTAnalysis("dc1", "op")
    dc1 = Analysis.create(dc1Desc, cir, s)
    assert dc1
    ok, canResume = dc1.run(s)
    print(f"DC1 analysis OK. Can resume: {'true' if canResume else 'false'}\n" if ok else "DC1 analysis failed")
    
    assert cir.elaborate([Id("sub2")], "__topdef__", "__topinst__", status=s)

    dc2Desc = PTAnalysis("dc2", "op")
    dc2 = Analysis.create(dc2Desc, cir, s)
    assert dc2
    ok, canResume = dc2.run(s)
    print(f"DC2 analysis OK. Can resume: {'true' if canResume else 'false'}\n" if ok else "DC2 analysis failed")

    dc1 = rawread('dc1.raw').get()                 
    dc2 = rawread('dc2.raw').get()
    print("dc1 v(2)=", dc1["2"])
    print("dc2 v(2)=", dc2["2"])
    print("Sum (should be 10):", dc1["2"]+dc2["2"])

def test_demo5():
    sim.setup()
    s = Status()
    tab = ParserTables("Variables and parametrized options sweep")
    p = Parser(tab)

    tab = tab.add(PTLoad("resistor.osdi"))
    tab = tab.add(PTLoad("diode.osdi"))
    tab = tab.defaultGround()
    sub = PTSubcircuitDefinition()
    sub = sub.add(PTModel("res", "resistor"))
    sub = sub.add(PTModel("dio", "diode")
        .add(p.parseParameters("is=1e-12 n=2 rs=1 eg=1.2 xti=2"))
    )
    sub = sub.add(PTModel("vsrc", "vsource"))
    sub = sub.add(PTBlockSequence().add(
        p.parseExpression("fixed!=0"), PTBlock().add(
            PTInstance("r1", "res", ["1", "2"]).add(PV("r", 100)))
        ).add(Rpn(), PTBlock().add(
            PTInstance("r1", "res", ["1", "2"]).add(
                p.parseParameters("r=10*(1+($temp-tnominal)/100)")
            ))
        )
    )
    sub = sub.add(PTInstance("d1", "dio", ["2", "0"]))
    sub = sub.add(PTInstance("v1", "vsrc", ["1", "0"]).add(PV("dc", 10)))
    tab = tab.setDefaultSubDef(sub)
    assert tab.verify(s)

    # Dump tables for debugging
    tab.dump(0)

    assert tab.writeEmbedded(1, s)

    comp = OpenvafCompiler()
    cir  = Circuit(tab, comp, s)
    assert cir.isValid()
    
    def state():
        print(f"reltol={cir.simulatorOptions().reltol}")
        var1 = cir.getVariable("tnominal")
        print(f"tnominal={var1 if var1 else 'not found'}")
        var2 = cir.getVariable("fixed")
        print(f"fixed={var2 if var2 else 'notfound'}")
        ok1, val1 = cir.instanceParameter("v1", "dc")
        print(f"v1 dc={str(val1) if ok1 else 'not found'}")
        ok2, val2 = cir.instanceParameter("r1", "r")
        print(f"r1 r={str(val2) if ok2 else 'not found'}")

    def partialElaborate():
        print("\nPartial elaboration")
        s = Status()
        ok, topologyChange, bindingNeeded = cir.elaborateChanges(s)
        assert ok
        if topologyChange:
            print("Topology changed.")
        if bindingNeeded:
            print("Analysis rebinding needed.")

    cir.setVariable("tnominal", Value(27))
    cir.setVariable("fixed", Value(1))

    print("\nBefore elaboration")
    state()
    assert cir.elaborate([], "__topdef__", "__topinst__", status=s)
    print("\nAfter elaboration")
    state()
    
    cir.setOption("reltol", Value(1e-6))
    cir.setVariable("tnominal", Value(40))
    cir.setInstanceParameter("v1", "dc", Value(20))

    partialElaborate()

    print("After first partial elaboration")
    state()

    cir.setVariable("fixed", Value(0))
    partialElaborate()

    print("After second partial elaboration")
    state()

    cir.setVariable("tnominal", Value(27))
    partialElaborate()

    print("After third partial elaboration")
    state()

    dc1Desc = PTAnalysis("dc1", "op")
    dc1Desc.add(
        PTSweep("tnom").add(
            PV("variable", "tnominal")).add(
            PV("from", 0)).add(
            PV("to", 100)).add(
            PV("step", 2))
    )
    dc1 = Analysis.create(dc1Desc, cir, s)
    assert dc1
    ok, canResume = dc1.run(s)
    assert ok
    print(f"DC1 analysis OK. Can resume: {'true' if canResume else 'false'}\n" if ok else "DC1 analysis failed")
    dc1 = rawread('dc1.raw').get()
    fig1, ax1 = plt.subplots(1, 1, figsize=(6,4), dpi=100, constrained_layout=True)
    fig1.axes[0].plot(dc1["tnom"], dc1["2"], "r", marker=".")
    fig1.savefig("d5dc1.jpg")

def test_demo6(): 
    sim.setup()
    s   = Status()
    tab = ParserTables("Self-heating lightbulb")
    p   = Parser(tab)

    tab = tab.defaultGround()
    sub = PTSubcircuitDefinition()
    sub = sub.add(PTModel("vsrc", "vsource"))
    # ----------------------------------------------------------------
    # v1 (a 0) vsource dc=0 type="sine"
    #      sinedc=0 ampl=325 freq=50
    # ----------------------------------------------------------------
    sub = sub.add(PTInstance("v1", "vsrc", ["a", "0"]).add(
        p.parseParameters(
            "dc=0 type=\"sine\" "
            "sinedc=0 ampl=325 freq=50"
        ))
    )
    # ----------------------------------------------------------------
    # bi (a 0)
    #
    # i(a,0) =
    #     V(a,0) /
    #     (r0*(p0+p1*T+p2*T^2))
    # ----------------------------------------------------------------
    sub = sub.add(PTBehavioral("bi", 
                               ["a", "0"],
                               p.parseExpression(
                                   "v(a,0)/(r0*(p0+p1*v(t)+p2*v(t)**2))"
                               ),
                               True
        )
    )
    # ----------------------------------------------------------------
    # ct (t 0)
    #
    # flow = ddt(cth * Temp(t))
    # discipline = thermal
    # ----------------------------------------------------------------
    sub = sub.add(PTBehavioral("ct", 
                               ["t", "0"],
                               p.parseExpression(
                                   "ddt(cth*v(t))"
                               ),
                               False,
                               "thermal",
                               "Temp",
                               "Pwr"
        )
    )
    # ----------------------------------------------------------------
    # rt (t 0)
    #
    # flow = (Temp(t)-ta)/rth
    # ----------------------------------------------------------------
    sub = sub.add(PTBehavioral("rt", 
                               ["t", "0"],
                               p.parseExpression(
                                   "(v(t)-ta)/rth"
                               ),
                               False,
                               "thermal",
                               "Temp",
                               "Pwr"
        )
    )
    # ----------------------------------------------------------------
    # pwr (t 0)
    #
    # flow =
    #   -V(a,0)^2 /
    #    (r0*(p0+p1*T+p2*T^2))
    # ----------------------------------------------------------------
    sub = sub.add(PTBehavioral("pwr", 
                               ["t", "0"],
                               p.parseExpression(
                                   "-(v(a,0)**2/(r0*(p0+p1*v(t)+p2*v(t)**2)))"
                               ),
                               False,
                               "thermal",
                               "Temp",
                               "Pwr"    
        )
    )
    tab = tab.setDefaultSubDef(sub)
    assert tab.verify(s)
    tab.dump(0)
    tab.processBehaviorals(0, s)
    assert tab.writeEmbedded(1, s)

    comp = OpenvafCompilerBuiltin()
    cir  = Circuit(tab, comp, s)
    assert cir.isValid()

    cir.setVariable("rth", Value((3000.0 - 25.0) / 60.0))
    cir.setVariable("cth", Value(10e-3))
    cir.setVariable("ta", Value(25.0))
    cir.setVariable("r0", Value(8.612))
    cir.setVariable("p0", Value(8.612))
    cir.setVariable("p1", Value(0.0269))
    cir.setVariable("p2", Value(1.914e-6))

    assert cir.elaborate([], "__topdef__", "__topinst__", status=s)

    # ------------------------------------------------------------------------
    # DC sweep
    #
    # Original:
    #
    # alter instance("v1") type="dc"
    # sweep v1 instance="v1" parameter="dc"
    #       from=0 to=230 mode=lin points=50
    #     analysis dc1 op
    #
    # ------------------------------------------------------------------------

    cir.setInstanceParameter("v1", "type", Value("dc"))
    dc1Desc = PTAnalysis("dc1", "op")
    dc1Desc.add(PTSweep("v1").add(
        PV("instance", "v1")).add(
        PV("parameter", "dc")).add(
        PV("from", 0)).add(
        PV("to", 230)).add(
        PV("mode", "lin")).add(
        PV("points", 50))
    )

    dc1 = Analysis.create(dc1Desc, cir, s)
    assert dc1
    ok, _ = dc1.run(s)
    assert ok
    print("DC analysis OK")

    # ------------------------------------------------------------------------
    # Transient analysis
    #
    # Original:
    #
    # alter instance("v1") type="sine"
    # analysis tran1 tran stop=2 step=0.01m maxstep=0.05m
    #
    # ------------------------------------------------------------------------
    cir.setInstanceParameter("v1", "type", Value("sine"))
    tran1Desc = PTAnalysis("tran1", "tran")
    tran1Desc.add(PV("stop", 2.0)
            ).add(PV("step", 0.01e-3)
            ).add(PV("maxstep", 0.05e-3))
    tran1 = Analysis.create(tran1Desc, cir, s)
    assert tran1
    ok, _ = tran1.run(s)
    assert ok
    print("Transient analysis OK")

    # ------------------------------------------------------------------------
    # Harmonic balance
    #
    # Original:
    #
    # analysis hb1 hb freq=[50] nharm=10
    #
    # ------------------------------------------------------------------------
    hb1Desc = PTAnalysis("hb1", "hb")
    hb1Desc.add(PV("freq", [50])
            ).add(PV("nharm", 10))
    hb1 = Analysis.create(hb1Desc, cir, s)
    assert hb1
    ok, _ = hb1.run(s)
    assert ok
    print("HB analysis OK")

    dc1 = rawread('dc1.raw').get()
    v = dc1['a'] 
    T = dc1['t']

    tran1 = rawread('tran1.raw').get()
    time = tran1['time']
    Tt = tran1['t']

    hb1 = rawread('hb1.raw').get()
    f = np.real(hb1['frequency'])
    Ts = hb1['t']
    Tmag = np.abs(Ts)

    t = np.linspace(0, 1.0/50, 1000)
    y = np.zeros(t.size)
    for f1, Ts1 in zip(f, Ts):
        y += np.real(Ts1)*np.cos(2*np.pi*f1*t)
        y += -np.imag(Ts1)*np.sin(2*np.pi*f1*t)

    ytran = np.interp(time[-1]-1.0/50+t, time, Tt)

    fig1, ax1 = plt.subplots(4, 1, figsize=(5,8), dpi=100, constrained_layout=True)
    fig1.suptitle('HB analysis of self-heating lightbulb (behavioral sources)')
    fig1.axes[0].set_title('DC response')
    fig1.axes[0].set_ylabel('T [deg C]')
    fig1.axes[0].set_xlabel('V [V]')
    fig1.axes[0].plot(v, T)

    fig1.axes[1].set_title('Transient response')
    fig1.axes[1].set_ylabel('T [deg C]')
    fig1.axes[1].set_xlabel('time [s]')
    fig1.axes[1].plot(time, Tt)

    fig1.axes[2].set_title('Steady-state magnitude spectrum')
    fig1.axes[2].set_ylabel('T magnitude [deg C]')
    fig1.axes[2].set_xlabel('frequency [Hz]')
    fig1.axes[2].set_yscale("log")
    fig1.axes[2].stem(f, Tmag, markerfmt='.')

    fig1.axes[3].set_title('Steady-state response')
    fig1.axes[3].set_ylabel('T [deg C]')
    fig1.axes[3].set_xlabel('time [ms]')
    fig1.axes[3].plot(t*1e3, y, color="blue", label="HB")
    fig1.axes[3].plot(t*1e3, ytran, color="red", label="tran")
    fig1.axes[3].legend(loc="upper left")

