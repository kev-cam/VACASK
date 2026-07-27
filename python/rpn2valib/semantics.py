from collections import namedtuple

from . import codegen
from .exc import RpnToVaError
from .parser import BinOp, Call, Ident, Num, Ternary, UnaryOp

AnalysisResult = namedtuple("AnalysisResult", ["ports", "params"])


def analyze(node, given_params):
    """
    Walks *node* (the parsed AST) once and:
    - finds every v()/i() call, validates its argument count, and records
      the node names it references (in first-seen order) - these become the
      module's extra ports
    - finds every other function call and validates its name and argument
      count against codegen.FUNCTIONS
    - finds every remaining bare identifier and treats it as a parameter
      that must have a default in *given_params*
    - rejects an identifier used both as a v()/i() node name and as a plain
      parameter, since Verilog-A can't declare a port and a parameter with
      the same name

    Returns an AnalysisResult(ports, params). Raises RpnToVaError on
    anything unsupported, naming exactly what's wrong.
    """
    ports = []
    port_set = set()
    param_names = []
    param_set = set()

    def add_port(name):
        if name not in port_set:
            port_set.add(name)
            ports.append(name)

    def add_param(name):
        if name not in param_set:
            param_set.add(name)
            param_names.append(name)

    def check_probe(call):
        if call.name == "v":
            if not (1 <= len(call.args) <= 2):
                raise RpnToVaError(
                    "'v' takes 1 or 2 arguments (a node, or two nodes for "
                    "a voltage difference), got "+str(len(call.args)))
        else:
            if len(call.args) != 1:
                raise RpnToVaError(
                    "'i' takes exactly 1 argument (a node/branch), got "
                    +str(len(call.args)))
        for arg in call.args:
            if not isinstance(arg, Ident):
                raise RpnToVaError(
                    "arguments to '"+call.name+"(...)' must be plain node "
                    "names, not expressions")

    def walk(n):
        if isinstance(n, Num):
            return
        if isinstance(n, Ident):
            if n.name.startswith("$"):
                raise RpnToVaError(
                    "'"+n.name+"': VACASK's special variables (like $temp) "
                    "are not supported by this converter yet")
            if codegen.is_reserved_ident(n.name):
                # A known constant (M_PI, P_Q, ...) or the "time" pseudo-
                # reference - resolved directly by codegen, not a parameter.
                return
            add_param(n.name)
            return
        if isinstance(n, Call):
            if n.name in ("v", "i"):
                check_probe(n)
                for arg in n.args:
                    add_port(arg.name)
                return
            arity = codegen.get_arity(n.name)
            if arity is None:
                raise RpnToVaError("unsupported function '"+n.name+"'")
            min_args, max_args = arity
            if not (min_args <= len(n.args) <= max_args):
                raise RpnToVaError(
                    "'"+n.name+"' takes "+_arity_text(min_args, max_args)
                    +", got "+str(len(n.args)))
            for arg in n.args:
                walk(arg)
            return
        if isinstance(n, UnaryOp):
            walk(n.operand)
            return
        if isinstance(n, BinOp):
            walk(n.left)
            walk(n.right)
            return
        if isinstance(n, Ternary):
            walk(n.cond)
            walk(n.if_true)
            walk(n.if_false)
            return
        raise RpnToVaError("internal error: unknown AST node "+repr(n))

    walk(node)

    missing = [name for name in param_names if name not in given_params]
    if missing:
        raise RpnToVaError(
            "missing default value for parameter(s): "+", ".join(missing)
            +" (pass --param <name>=<value>)")

    collisions = sorted(port_set & param_set)
    if collisions:
        raise RpnToVaError(
            "'"+collisions[0]+"' is used both as a node (via v()/i()) and "
            "as a plain identifier (parameter) - Verilog-A can't declare a "
            "port and a parameter with the same name")

    params = {name: given_params[name] for name in param_names}
    return AnalysisResult(ports, params)


def _arity_text(min_args, max_args):
    if min_args == max_args:
        return "exactly "+str(min_args)+(" argument" if min_args == 1 else " arguments")
    return "between "+str(min_args)+" and "+str(max_args)+" arguments"
