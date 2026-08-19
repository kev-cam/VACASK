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
    sub = sub.add(PTModel(Id("res"), Id("resistor")))
    sub = sub.add(PTModel(Id("cap"), Id("capacitor")))
    sub = sub.add(PTModel(Id("vsrc"), Id("vsource")))
    sub = sub.add(PTInstance(Id("r1"), Id("res"), PTIds(["1", "2"])).add(PV("r", 1000)))
    sub = sub.add(PTInstance(Id("c1"), Id("cap"), PTIds(["2", "0"])).add(PE("c", p.parseExpression("2*c0"))))
    sub = sub.add(PTInstance(Id("v1"), Id("vsrc"), PTIds(["1", "0"])).add(
        p.parseParameters("type=\"pulse\" val0=0 val1=v0 delay=1m rise=1u fall=1u width=4m"))
    )
    tab = tab.setDefaultSubDef(sub)

    assert tab.verify(s)
    assert tab.writeEmbedded(1, s)

    comp = OpenvafCompiler()
    cir  = Circuit(tab, comp, s)
    assert cir.isValid()

    cir.setOption(Id("reltol"), Value(1e-4))
    assert cir.elaborate([], "__topdef__", "__topinst__", status=s)
    cir.dumpHierarchy(0)

    tran_desc = PTAnalysis(Id("tran1"), Id("tran"))
    tran_desc.add(PV("step", 1e-6))
    tran_desc.add(PV("stop", 10e-3))

    tran = Analysis.create(tran_desc, cir, s)
    assert tran

    tran.add(PTSave(Id("default")))
    tran.add(PTSave(Id("p"), Id("r1"), Id("i")))

    (ok, _) = tran.run(s)
    assert ok

    tran1 = rawread('tran1.raw').get()
    print("Vectors:", tran1.names)
    fig1, _ = plt.subplots(1, 1, figsize=(6,4), dpi=100, constrained_layout=True)
    fig1.axes[0].plot(tran1["time"], tran1["2"], "r", marker=".")
    fig1.axes[0].plot(tran1["time"], tran1["1"], "b")
    fig1.axes[0].plot(tran1["time"], tran1["r1.i"]*1000, "--")
    fig1.savefig("tran1.jpg")

def test_demo2():
    sim.setup()
    s = Status()
    tab = ParserTables("Conditional blocks")
    p = Parser(tab)

    tab = tab.add(PTLoad("resistor.osdi"))
    tab = tab.defaultGround()
    sub = PTSubcircuitDefinition()
    sub = sub.add(PTModel(Id("res"), Id("resistor")))
    sub = sub.add(PTModel(Id("vsrc"), Id("vsource")))
    sub2 = PTSubcircuitDefinition(Id("mysub"), PTIds(["p", "out", "n"]))
    sub2 = sub2.add(PV("r", 1000))
    sub2 = sub2.add(PV("fact", 0.5))
    sub2 = sub2.add(PV("mode", 0))
    r1i = PTInstance(Id("r1"), Id("res"), PTIds(["p", "out"]))
    r1i = r1i.add(PE("r", p.parseExpression("r*(1-fact)")))
    r2i = PTInstance(Id("r2"), Id("res"), PTIds(["out", "n"]))
    r2i = r2i.add(PE("r", p.parseExpression("r*fact")))
    r1e = PTInstance(Id("r1"), Id("res"), PTIds(["p", "out"]))
    r1e = r1e.add(PE("r", p.parseExpression("r*fact")))
    r2e = PTInstance(Id("r2"), Id("res"), PTIds(["out", "n"]))
    r2e = r2e.add(PE("r", p.parseExpression("r*(1-fact)")))
    sub2 = sub2.add(PTBlockSequence().add(p.parseExpression("mode==0"), PTBlock().add(r1i).add(r2i)).add(
        Rpn(), PTBlock().add(r1e).add(r2e)
    ))
    sub = sub.add(sub2)
    sub = sub.add(PTInstance(Id("x1"), Id("mysub"), PTIds(["1", "2", "0"])).add(PV("r", 1000)).add(PV("fact", 0.2)))
    sub = sub.add(PTInstance(Id("v1"), Id("vsrc"), PTIds(["1", "0"])).add(PV("dc", 10)))
    tab = tab.setDefaultSubDef(sub)
    
    assert tab.verify(s)
    assert tab.writeEmbedded(1, s)

    comp = OpenvafCompiler()
    cir  = Circuit(tab, comp, s)
    assert cir.isValid()

    cir.setOption(Id("reltol"), Value(1e-4))
    cir.setVariable(Id("var1"), Value(60))

    pp = p.parseParameters("temp=var1")
    cir.setOptions(pp)

    assert cir.elaborate([], "__topdef__", "__topinst__", status=s)
    print(f"temp={cir.simulatorOptions().temp}")
    cir.dumpHierarchy(0)

    dcDesc = PTAnalysis(Id("dc1"), Id("op"))
    dcDesc = dcDesc.add(PTSweep(Id("mode"))
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
    fig1.savefig("dc1.jpg")
