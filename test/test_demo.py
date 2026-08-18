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
    assert cir.is_valid()

    cir.setOption(Id("reltol"), Value(1e-4))
    assert cir.elaborate([], "__topdef__", "__topinst__", status=s)
    cir.dumpHierarchy(0);

    tran_desc = PTAnalysis(Id("tran1"), Id("tran"))
    tran_desc.add(PV("step", 1e-6))
    tran_desc.add(PV("stop", 10e-3))

    tran = Analysis.create(tran_desc, cir, s)
    assert tran

    tran.add(PTSave(Id("default")))
    tran.add(PTSave(Id("p"), Id("r1"), Id("i")))

    (ok, can_resume) = tran.run(s)
    assert ok

    tran1 = rawread('tran1.raw').get()
    print("Vectors:", tran1.names)
    fig1, ax1 = plt.subplots(1, 1, figsize=(6,4), dpi=100, constrained_layout=True)
    fig1.axes[0].plot(tran1["time"], tran1["2"], "r", marker=".")
    fig1.axes[0].plot(tran1["time"], tran1["1"], "b")
    fig1.axes[0].plot(tran1["time"], tran1["r1.i"]*1000, "--")
    fig1.savefig("tran1.jpg")

