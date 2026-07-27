import math

from .exc import RpnToVaError
from .parser import BinOp, Call, Ident, Num, Ternary, UnaryOp
from .parser import OPERATOR_PRECEDENCE, QUESTION_PRECEDENCE, UNARY_PRECEDENCE


def _straight(name, min_args, max_args):
    """One-to-one function: VACASK name == Verilog-A name (lib/context.cpp)."""
    def gen(args):
        return name+"("+", ".join(args)+")"
    return (min_args, max_args, gen)


def _expand(min_args, max_args, template):
    """A function with no Verilog-A equivalent, expanded to an inline expression."""
    def gen(args):
        return "("+template(args)+")"
    return (min_args, max_args, gen)


# Straight across, one-to-one (lib/context.cpp mathFuncComp1<...> entries).
FUNCTIONS = {}
for _name in ["sin", "cos", "tan", "asin", "acos", "atan", "sinh", "cosh",
              "tanh", "asinh", "acosh", "atanh", "exp", "sqrt", "abs",
              "ceil", "floor", "ln"]:
    FUNCTIONS[_name] = _straight(_name, 1, 1)

FUNCTIONS["hypot"] = _straight("hypot", 2, 2)
# VACASK allows min/max with 1 arg (vector aggregate) or 2 (component-wise);
# only the 2-arg form makes sense inside a scalar Verilog-A contribution.
FUNCTIONS["min"] = _straight("min", 2, 2)
FUNCTIONS["max"] = _straight("max", 2, 2)

# Needing translation - see docs/behavioral-source-rpn-to-verilog-a-guide.md section 9.2.
# These two are still plain, atomic function calls under a different name
# (unlike round/sgn/sign/atan2/real below), so no extra wrapping is needed.
FUNCTIONS["log"] = _straight("ln", 1, 1)
FUNCTIONS["log10"] = _straight("log", 1, 1)
FUNCTIONS["round"] = _expand(1, 1, lambda a: (
    "("+a[0]+")>=0 ? floor(("+a[0]+")+0.5) : ceil(("+a[0]+")-0.5)"))
# sgn(x) (1 arg, lib/rpnfunctor.h FwSgn) and sign(x1,x2) (2-arg copysign-style,
# FwSign) are two different functions, not the same one under two names.
FUNCTIONS["sgn"] = _expand(1, 1, lambda a: "("+a[0]+")>=0 ? 1 : -1")
FUNCTIONS["sign"] = _expand(2, 2, lambda a: (
    "("+a[1]+")>=0 ? abs("+a[0]+") : -abs("+a[0]+")"))
FUNCTIONS["int"] = _straight("$rtoi", 1, 1)
FUNCTIONS["integer"] = FUNCTIONS["int"]
FUNCTIONS["real"] = _expand(1, 1, lambda a: a[0])
# VACASK's atan2(x1,x2) is literally atan(x1/x2) (lib/rpnfunctor.h FwAtan2),
# not a quadrant-correct atan2. Replicated exactly rather than switching to
# Verilog-A's real (and different) atan2 builtin.
FUNCTIONS["atan2"] = _expand(2, 2, lambda a: "atan(("+a[0]+")/("+a[1]+"))")

_FUNCTION_NOTES = {
    "atan2": ("atan2(a,b) is translated as atan(a/b), matching VACASK's own "
              "atan2 definition (include/rpnfunctor.h), not Verilog-A's "
              "quadrant-correct atan2 builtin."),
}

# Direct-match constants (lib/context.cpp), same name, backtick-prefixed.
_DIRECT_CONSTANTS = [
    "M_E", "M_LOG2E", "M_LOG10E", "M_LN2", "M_LN10", "M_PI", "M_TWO_PI",
    "M_PI_2", "M_PI_4", "M_1_PI", "M_2_PI", "M_2_SQRTPI", "M_SQRT2",
    "M_SQRT1_2", "P_C", "P_U0", "P_CELSIUS0",
]
CONSTANTS = {name: "`"+name for name in _DIRECT_CONSTANTS}
# constants.vams has no bare P_Q/P_K/P_H/P_EPS0 - only *_OLD/_SPICE/_NIST...
# variants. VACASK's own numeric values match the _OLD variant exactly.
CONSTANTS.update({
    "P_Q": "`P_Q_OLD",
    "P_K": "`P_K_OLD",
    "P_H": "`P_H_OLD",
    "P_EPS0": "`P_EPS0_OLD",
})

_M_DEGPERRAD_NOTE = (
    "M_DEGPERRAD has no equivalent in constants.vams; inlined as its "
    "literal decimal value (180/pi).")

# VACASK's own RPN expression language (lib/context.cpp) has no live
# simulation-time value - it only evaluates static, pre-simulation formulas.
# Verilog-A's analog blocks do have one, the standard $abstime system
# function, which a time-varying behavioral source (a sine/pulse source,
# for instance) needs. "time" is recognized as a reserved reference to it,
# the same way "v"/"i" are reserved for node/branch probes.
_TIME_NOTE = (
    "'time' is translated to Verilog-A's $abstime (the current analog "
    "simulation time); VACASK's own RPN expression language has no "
    "equivalent, since it only evaluates static formulas.")


def is_reserved_ident(name):
    """
    True if a bare identifier is resolved directly by codegen (a known
    constant, M_DEGPERRAD, or the "time" pseudo-reference) rather than
    being an ordinary user-supplied parameter.
    """
    return name == "time" or name == "M_DEGPERRAD" or name in CONSTANTS


def get_arity(name):
    """Returns (min_args, max_args) for a supported function, else None."""
    entry = FUNCTIONS.get(name)
    if entry is None:
        return None
    return (entry[0], entry[1])


def generate(node):
    """
    Turns a parsed AST into Verilog-A expression text, applying the
    operator/function/constant mapping tables. Returns (text, notes) where
    notes is a list of human-readable strings about any non-obvious
    substitution made (e.g. atan2, M_DEGPERRAD), meant to be surfaced as
    comments in the generated .va file.
    """
    return _gen(node)


def _gen(node):
    if isinstance(node, Num):
        return repr(node.value), []
    if isinstance(node, Ident):
        return _gen_ident(node.name)
    if isinstance(node, Call):
        return _gen_call(node)
    if isinstance(node, UnaryOp):
        return _gen_unary(node)
    if isinstance(node, BinOp):
        return _gen_binop(node)
    if isinstance(node, Ternary):
        return _gen_ternary(node)
    raise RpnToVaError("internal error: unknown AST node "+repr(node))


def _gen_ident(name):
    if name == "time":
        return "$abstime", [_TIME_NOTE]
    if name == "M_DEGPERRAD":
        return repr(180.0/math.pi), [_M_DEGPERRAD_NOTE]
    mapped = CONSTANTS.get(name)
    if mapped is not None:
        return mapped, []
    return name, []


def _gen_call(node):
    if node.name in ("v", "i"):
        probe = "V" if node.name == "v" else "I"
        args = [a.name for a in node.args]
        return probe+"("+", ".join(args)+")", []

    entry = FUNCTIONS.get(node.name)
    if entry is None:
        # semantics.py rejects unsupported functions before codegen runs.
        raise RpnToVaError(
            "internal error: unsupported function '"+node.name+"' reached codegen")
    _min_args, _max_args, gen = entry

    arg_texts = []
    notes = []
    for arg in node.args:
        text, arg_notes = _gen(arg)
        arg_texts.append(text)
        notes.extend(arg_notes)

    text = gen(arg_texts)
    if node.name in _FUNCTION_NOTES:
        notes.append(_FUNCTION_NOTES[node.name])
    return text, notes


def _child_precedence(node):
    """
    Precedence of *node*'s own top-level operator, for parenthesization
    decisions - or None if *node* is atomic (never needs wrapping): a
    literal, identifier, function call, unary operator result, or a "**"
    BinOp (rendered as an atomic pow(...) call).
    """
    if isinstance(node, Ternary):
        return QUESTION_PRECEDENCE
    if isinstance(node, BinOp) and node.op != "**":
        return OPERATOR_PRECEDENCE[node.op]
    return None


def _wrap_child(node, min_prec, force_on_tie):
    text, notes = _gen(node)
    prec = _child_precedence(node)
    if prec is not None and (prec < min_prec or (prec == min_prec and force_on_tie)):
        text = "("+text+")"
    return text, notes


def _gen_unary(node):
    # NEG/NOT/BITNOT bind tighter than every binary operator (precedence 13,
    # see parser.py), so any BinOp/Ternary operand needs explicit parens -
    # e.g. "-(a+b)" must not come out as "-a + b".
    text, notes = _wrap_child(node.operand, UNARY_PRECEDENCE, True)
    return node.op+text, notes


def _gen_binop(node):
    if node.op == "**":
        left_text, left_notes = _gen(node.left)
        right_text, right_notes = _gen(node.right)
        return "pow("+left_text+", "+right_text+")", left_notes+right_notes

    parent_prec = OPERATOR_PRECEDENCE[node.op]
    # All binary operators reaching here are left-associative (lib/dflparser.y),
    # so an equal-precedence right child only appears when the source had
    # explicit parens overriding left-to-right grouping - reproduce them.
    left_text, left_notes = _wrap_child(node.left, parent_prec, False)
    right_text, right_notes = _wrap_child(node.right, parent_prec, True)
    return left_text+" "+node.op+" "+right_text, left_notes+right_notes


def _gen_ternary(node):
    cond_text, cond_notes = _gen(node.cond)
    # A bare ternary as its own condition only ever appears via explicit
    # source parens (the grammar can't otherwise put one there); reproduce them.
    if isinstance(node.cond, Ternary):
        cond_text = "("+cond_text+")"
    true_text, true_notes = _gen(node.if_true)
    false_text, false_notes = _gen(node.if_false)
    text = cond_text+" ? "+true_text+" : "+false_text
    return text, cond_notes+true_notes+false_notes
