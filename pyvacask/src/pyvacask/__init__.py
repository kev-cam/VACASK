from . import _pyvacask

def _conf_pyvacask():
    import os
    this_file = os.path.abspath(__file__)
    this_dir  = os.path.dirname(this_file)
    lib_dir   = os.path.normpath(os.path.join(this_dir, "lib", "mod"))
    _pyvacask.simulator.prependModulePath([lib_dir])

_conf_pyvacask()

status = _pyvacask.status
from ._pyvacask.status import *

parser = _pyvacask.parser
from ._pyvacask.parser import *

parser_output = _pyvacask.parser_output
from ._pyvacask.parser_output import *

compiler = _pyvacask.compiler
from ._pyvacask.compiler import *

circuit = _pyvacask.circuit
from ._pyvacask.circuit import *

analysis = _pyvacask.analysis
from ._pyvacask.analysis import *

id = _pyvacask.id
from ._pyvacask.id import *

value = _pyvacask.value
from ._pyvacask.value import *

simulator = _pyvacask.simulator
from ._pyvacask.simulator import *

options = _pyvacask.options
from ._pyvacask.options import *

from .helpers import *

# Monkeypath the setup function because it always overwrites the "default module path" set above.
_original_setup = simulator.setup
def _setup(*args, **kwargs):
    result = _original_setup(*args, **kwargs)
    _conf_pyvacask()
    return result
simulator.setup = _setup
