import os
import sys

from pyvacask.simulator import Simulator
from pyvacask.status import Status
from pyvacask.parser_output import ParserTables
from pyvacask.parser_output import PTLoad
from pyvacask.parser_output import PTSubcircuitDefinition
from pyvacask.parser_output import PTModel
from pyvacask.parser_output import PTParameter
from pyvacask.parser_output import PTParameterValue
from pyvacask.parser_output import PTParameterExpression
from pyvacask.parser_output import PTAnalysis
from pyvacask.parser_output import PTSave
from pyvacask.compiler import OpenvafCompiler
from pyvacask.circuit import Circuit
from pyvacask.analysis import Analysis


_THIS_FILE  = os.path.abspath(__file__)
_TEST_DIR   = os.path.dirname(_THIS_FILE)
ROOT_DIR    = os.path.normpath(os.path.join(_TEST_DIR, ".."))
SITE_PATH   = os.path.join(ROOT_DIR, "venv", "lib", "python3.12", "site-packages")
MODULE_PATH = os.path.join(SITE_PATH, "lib", "vacask", "mod")
PY_LIB_PATH = os.path.join(SITE_PATH, "lib", "vacask", "python")

sys.path.append(PY_LIB_PATH)
from rawfile import rawread


def PV(name, value):
    return PTParameterValue(name, value)

def PE(ident, expr):
    return PTParameterExpression(ident, expr)

def test_demo1(): 
    sim = Simulator()
    sim.setup()
    sim.prependModulePath(MODULE_PATH)

    s   = Status()
    tab = ParserTables("RC transient")
    p   = Parser(tab)

    tab = tab.add(PTLoad("resistor.osdi"))
    tab = tab.add(PTLoad("capacitor.osdi"))
    tab = tab.defaultGround()
    sub = PTSubcircuitDefinition()
    sub = sub.add(PTParameters().add(PV("c0", 1e-6).add(PV("v0", 5))))
    sub = sub.add(PTModel("res", "resistor"))
    sub = sub.add(PTModel("cap", "capacitor"))
    sub = sub.add(PTModel("vsrc", "vsource"))
    sub = sub.add(PTInstance("r1", "res", ["1", "2"]).add(PV("r", 1000)))
    sub = sub.add(PTInstance("c1", "cap", ["2", "0"]).add(PE("c", p.parseExpression("2*c0"))))
    sub = sub.add(PTInstance("v1", "vsrc", {"1", "0"}).add(
        p.parseParameters("type=\"pulse\" val0=0 val1=v0 delay=1m rise=1u fall=1u width=4m"))
    )
    tab = tab.setDefaultSubDef(sub)

    assert tab.verify(s)
    assert tab.writeEmbedded(1, s)

    comp = OpenvafCompiler()
    cir  = Circuit(tab, comp, s)
    assert cir.isValid()

    cir.setOption("reltol", 1e-4)
    assert cir.elaborate([], "__topdef__", "__topinst__", None, s)
    cir.dumpHierarchy(0);

    tran_desc = PTAnalysis("tran1", "tran")
    tran_desc.add(PV("step", 1e-6))
    tran_desc.add(PV("stop", 10e-3))

    tran = Analysis().create(tran_desc, cir, s)
    assert tran

    tran.add(PTSave("default"))
    tran.add(PTSave("p", "r1", "i"))

    (ok, can_resume) = tran.run(s)
    assert ok
