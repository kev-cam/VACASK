#!/usr/bin/env python3
"""
Generate a cosim_top.v from a VACASK/spectre netlist + one or more
Verilog DUT modules.

Algorithm (see docs/cosim-architecture.md and the GENESIS_AA case study
in tb_top_cosim/COSIM_NOTES.md for how this was derived and verified):

VACASK collapses a hierarchical node down to the SHALLOWEST scope at
which it is not itself threaded further outward as a subckt port --
full stop, regardless of whether any device is locally attached at an
intermediate scope. So for each DUT port: start at the net connected
at the DUT's own (innermost) instantiation; while that net is ALSO a
declared port of the enclosing subckt, climb to the parent
instantiation and follow it; stop at the first scope where it isn't
(or at the true top level). That climb is a purely static property of
the netlist text -- no live elaboration needed to compute it, though
this script is meant to be paired with a live check (e.g. the
nodedump tool) before trusting its output on anything unfamiliar.

Where each DUT lives in the hierarchy is discovered automatically by
searching the netlist for any instantiation whose subckt type matches
the module name -- you don't tell it the path, it finds it (and finds
every occurrence, if the same module is instantiated more than once).

Usage:
    gen_cosim_top.py --netlist NETLIST.spectre --modules top_dig \\
        [--modules top_dig,example_dig,counter_dig] \\
        [--verilog-dir DIR] [--vth-lo V] [--vth-hi V] [--d2a-vdd V] \\
        [--d2a-r OHMS] [--d2a-c FARADS] [--tstop SECONDS] \\
        [--tstep SECONDS] [--netlist-file NAME_FOR_COSIM_RUN] \\
        [-o cosim_top.v]

--modules is a single comma-separated list (spaces around commas are
fine). Each name is expected to have a matching "<module>.v" file
(--verilog-dir, default: the netlist's own directory) -- same
convention already used throughout this project (top_dig.v contains
module top_dig, etc). If the netlist's subckt/symbol name differs from
the Verilog module name (e.g. an xschem symbol named
"dut_counter_block" wrapping "module dut_counter" in dut_counter.v --
a real, deliberate naming split, not unusual), write module:subckt,
e.g. --modules dut_counter:dut_counter_block. Any subckt in the
netlist NOT named in --modules is left completely untouched: still
whatever real analog content it already is, simulated normally by
VACASK.

--tstop/--tstep default to whatever the netlist's own `tran tstop=...
tstep=...` or `analysis <name> tran stop=... step=...` statement
specifies (both real syntax forms), so editing that in the schematic
and regenerating carries through automatically -- pass either flag
explicitly to override. Note this only covers $cosim_run's own
timing: any other netlist-level `options reltol=... abstol=...`
(VACASK's real syntax for these) needs the bridge itself
(cosim_core.cpp) to forward them, not this script -- see the bridge
changelog for that fix.
"""
import argparse
import ast
import difflib
import operator
import os
import re
import sys


# ---------------------------------------------------------------------------
# Netlist parsing
#
# Bus connections in a VACASK netlist must appear as a single-quoted
# token, e.g. "x2 ( ... 'data[3]' ... ) sar_adc" -- this isn't just a
# convention of this tool, it's a real VACASK parser requirement
# (unquoted "data[3]" is a hard netlist syntax error: "unexpected [,
# expecting identifier or integer"). _tokenize()'s quote-aware split
# below matches that convention when walking a netlist's own connection
# lists, and the '<name>[<n>]' addresses this script emits into
# $cosim_a2d/$cosim_d2a follow the same quoted form so they match what
# VACASK's own elaborated node names actually look like.
# ---------------------------------------------------------------------------

_TOKEN_RE = re.compile(r"'[^']*'|\S+")


def _tokenize(s):
    return [t[1:-1] if t.startswith("'") and t.endswith("'") else t
            for t in _TOKEN_RE.findall(s)]


# ---------------------------------------------------------------------------
# Include resolution and line continuation pre-passes
# ---------------------------------------------------------------------------

_INCLUDE_RE = re.compile(r'^\s*include\s+["\']([^"\']+)["\']\s*$')


def _resolve_includes(path, _depth=0, _visited=None):
    """Recursively resolve `include "filename"` directives, returning a
    flat list of all lines.  Tracks visited files to detect cycles."""
    if _depth > 10:
        raise ValueError(f"include depth exceeded at {path}")
    if _visited is None:
        _visited = set()
    real = os.path.realpath(path)
    if real in _visited:
        raise ValueError(f"circular include: {path}")
    _visited.add(real)
    lines = []
    base = os.path.dirname(real)
    with open(path) as f:
        for line in f:
            m = _INCLUDE_RE.match(line)
            if m:
                inc_path = os.path.join(base, m.group(1))
                lines.extend(_resolve_includes(inc_path, _depth + 1, _visited))
            else:
                lines.append(line.rstrip())
    return lines


def _join_continuations(lines):
    """Join backslash-continued and plus-continued lines."""
    joined = []
    buf = ""
    for line in lines:
        stripped = line.rstrip()
        if stripped.endswith("\\"):
            buf += stripped[:-1] + " "
            continue
        if buf:
            buf += stripped
            joined.append(buf)
            buf = ""
        elif stripped.lstrip().startswith("+"):
            # SPICE-style continuation: append to previous line
            if joined:
                joined[-1] += " " + stripped.lstrip()[1:].lstrip()
            else:
                joined.append(stripped)
        else:
            joined.append(stripped)
    if buf:
        joined.append(buf)
    return joined


def _split_instantiation(line):
    """Split a SPICE-like instantiation/device line into
    (name, connections_tokens, type). Returns None if the line doesn't
    have the "name ( ... ) type ..." shape at all (comments,
    directives, subckt/ends keywords, etc.)."""
    line = line.strip()
    if not line or line.startswith("//") or line.startswith("*"):
        return None
    paren_start = line.find("(")
    if paren_start == -1:
        return None
    name = line[:paren_start].strip()
    if not name or " " in name:
        return None
    # Find the matching close paren (connection lists have no nested
    # parens in VACASK netlists -- first ")" after the first "(" is it).
    paren_end = line.find(")", paren_start)
    if paren_end == -1:
        return None
    conns = _tokenize(line[paren_start + 1:paren_end])
    rest = line[paren_end + 1:].strip()
    if not rest:
        return None
    inst_type = rest.split(None, 1)[0]
    return name, conns, inst_type


# Mirrors lib/dfllexer.l:580-626 exactly -- NOT generic SPICE convention,
# which VACASK deliberately departs from. Matching is case-sensitive and
# asymmetric: bare "M" is always mega, regardless of what follows it;
# bare "m" is milli UNLESS followed by literal lowercase "eg" (mega) or
# "il" (mil, 25.4 micron) -- "1mEG" (mixed case) is still milli, only
# exact lowercase "eg" triggers mega for the "m" branch. Confirmed
# against the lexer source, not assumed from SPICE norms (an earlier
# version of this table used generic SPICE suffix matching and both
# mis-set "600M" to milli instead of mega, and outright rejected legal
# VACASK values like "1ms"/"600ns"/"20us" -- their trailing unit letter
# was never accounted for at all).
_NUM_SUFFIX_CHARS = set("munpfakKMGTxX")
_MANTISSA_RE = re.compile(r"^(?:\.[0-9]+|(?:0|[1-9][0-9]*)(?:\.[0-9]*)?)")


def _to_seconds(s):
    """Parse a VACASK-native engineering-suffixed number ('600n', '1u',
    '400e-9', '1Meg', '20ms', plain floats, ...) into a plain float.
    Verilog has no such suffix notation -- a bare 'tstop' pulled from
    the netlist's own 'tran tstop=600n' and dropped into
    $cosim_run(...) as-is would be a syntax error, not just wrong.

    Only the single character immediately after the numeric mantissa is
    ever inspected (plus, for 'm', the next two literal characters) --
    anything else trailing (e.g. the "s" in "1ms"/"600ns"/"20us") is
    unit text and is discarded, exactly like the lexer it mirrors."""
    s = s.strip()
    m = _MANTISSA_RE.match(s)
    if m and m.end() < len(s) and s[m.end()] in _NUM_SUFFIX_CHARS:
        mantissa = float(m.group(0))
        c = s[m.end()]
        if c == "M":
            mult = 1e6
        elif c == "m":
            tail = s[m.end() + 1:m.end() + 3]
            mult = 1e6 if tail == "eg" else 25.4e-6 if tail == "il" else 1e-3
        elif c in ("k", "K"):
            mult = 1e3
        elif c in ("x", "X"):
            mult = 1e6
        elif c == "u":
            mult = 1e-6
        elif c == "n":
            mult = 1e-9
        elif c == "p":
            mult = 1e-12
        elif c == "f":
            mult = 1e-15
        elif c == "a":
            mult = 1e-18
        elif c == "G":
            mult = 1e9
        else:  # c == "T", the only character left in _NUM_SUFFIX_CHARS
            mult = 1e12
        return mantissa * mult
    return float(s)


_TRAN_LINE_RE = re.compile(
    r"^\s*(?:analysis\s+\S+\s+)?tran\b(?P<params>.*)$")
_TSTOP_RE = re.compile(r"\b(?:tstop|stop)\s*=\s*([^\s]+)")
_TSTEP_RE = re.compile(r"\b(?:tstep|step)\s*=\s*([^\s]+)")


def find_tran_settings(netlist_path):
    """Best-effort scan of the netlist's own control block for a `tran
    tstop=... tstep=...` or `analysis <name> tran stop=... step=...`
    statement (both real, both seen in this project) and pull out
    whatever tstop/tstep it specifies. Returns (tstop, tstep), either
    element None if not found -- used only as a fallback default so a
    netlist's own settings carry through to the generated cosim_top.v
    without the caller having to re-type them and risk drift between
    the two. Explicit --tstop/--tstep always take priority over this."""
    tstop = tstep = None
    raw_lines = _resolve_includes(netlist_path)
    for raw in _join_continuations(raw_lines):
        line = raw.split("//")[0]
        m = _TRAN_LINE_RE.match(line)
        if not m:
            continue
        params = m.group("params")
        sm = _TSTOP_RE.search(params)
        tm = _TSTEP_RE.search(params)
        if sm: tstop = sm.group(1)
        if tm: tstep = tm.group(1)
        break  # first (active, uncommented) tran statement wins
    return tstop, tstep


class Netlist:
    """subckt_ports: {subckt_name: [port_names...]}
    instances: {(enclosing_scope, instance_name): (connections, inst_type)}
    enclosing_scope is "ROOT" for anything outside any subckt...ends
    block, or the subckt's own name otherwise. VACASK netlists do not
    nest "subckt ... ends" blocks textually, so a single current-scope
    flag (not a stack) is enough while scanning."""

    def __init__(self, path):
        self.subckt_ports = {}
        self.instances = {}
        self._parse(path)
        # Reverse index: subckt_type -> [(enclosing_scope, instance_name), ...]
        self.by_type = {}
        for (scope, name), (_, inst_type) in self.instances.items():
            self.by_type.setdefault(inst_type, []).append((scope, name))

    def _parse(self, path):
        scope = "ROOT"
        subckt_re = re.compile(r"^subckt\s+(\S+)\s*\((.*)\)\s*$")
        raw_lines = _resolve_includes(path)
        for raw in _join_continuations(raw_lines):
            line = raw.strip()
            m = subckt_re.match(line)
            if m:
                scope = m.group(1)
                self.subckt_ports[scope] = _tokenize(m.group(2))
                continue
            if re.match(r"^ends(\s|$)", line):
                scope = "ROOT"
                continue
            parsed = _split_instantiation(line)
            if parsed is None:
                continue
            name, conns, inst_type = parsed
            self.instances[(scope, name)] = (conns, inst_type)

    def find_instance_paths(self, module_name):
        """Every instance path (outermost-first list of instance names)
        at which `module_name` is instantiated anywhere in the design.
        A subckt reachable via more than one parent placement yields one
        path per placement -- each is a genuinely distinct physical
        instance, not an ambiguity to resolve."""
        placements = self.by_type.get(module_name, [])
        if not placements:
            return []

        def paths_to_scope(scope_type, _visiting=frozenset()):
            if scope_type == "ROOT":
                yield []
                return
            if scope_type in _visiting:
                raise ValueError(
                    f"cycle detected in subckt hierarchy while resolving "
                    f"'{scope_type}'")
            for parent_scope, inst_name in self.by_type.get(scope_type, []):
                for prefix in paths_to_scope(parent_scope, _visiting | {scope_type}):
                    yield prefix + [inst_name]

        paths = []
        for enclosing_scope, inst_name in placements:
            for prefix in paths_to_scope(enclosing_scope):
                paths.append(prefix + [inst_name])
        return paths

    def resolve_chain(self, path):
        """Walk `path` (list of instance names, outermost first) from
        ROOT down, returning the list of per-level dicts needed by
        resolve_address(). Raises ValueError with a clear message on
        any lookup failure."""
        chain = []
        scope = "ROOT"
        for inst_name in path:
            key = (scope, inst_name)
            if key not in self.instances:
                raise ValueError(
                    f"no instance named '{inst_name}' found inside scope "
                    f"'{scope}' (looking for path {':'.join(path)})")
            conns, inst_type = self.instances[key]
            if inst_type not in self.subckt_ports:
                raise ValueError(
                    f"instance '{inst_name}' in scope '{scope}' has type "
                    f"'{inst_type}', which has no 'subckt {inst_type} "
                    f"( ... )' definition in the netlist -- not a subckt?")
            chain.append({
                "scope": scope,
                "connections": conns,
                "subckt_type": inst_type,
                "port_list": self.subckt_ports[inst_type],
            })
            scope = inst_type
        return chain

    @staticmethod
    def resolve_address(chain, path, port_name):
        """Apply the collapse-climbing rule for one DUT port name.
        `chain`/`path` as returned by / passed to resolve_chain."""
        level = len(chain) - 1
        port_list = chain[level]["port_list"]
        if port_name not in port_list:
            raise ValueError(
                f"port '{port_name}' not found in '{chain[level]['subckt_type']}' "
                f"netlist port list: {port_list}")
        net = chain[level]["connections"][port_list.index(port_name)]
        while level > 0 and net in chain[level - 1]["port_list"]:
            idx = chain[level - 1]["port_list"].index(net)
            net = chain[level - 1]["connections"][idx]
            level -= 1
        if level == 0:
            return net
        return ":".join(path[:level]) + ":" + net


# ---------------------------------------------------------------------------
# Verilog port-list parsing -- ANSI-style ("module foo(input wire [7:0]
# x, ...)") and old-style/Verilog-1995 ("module foo(x, y); input [7:0]
# x; output y;", direction declared separately in the body) both
# handled, since real files (this project's own top_dig.v vs. e.g. a
# classic testbench-provided adc.v) use either one.
# ---------------------------------------------------------------------------

_PORT_DECL_RE = re.compile(
    r"^\s*(input|output|inout)\s+(?:wire|reg)?\s*"
    r"(?:\[\s*(\S+?)\s*:\s*(\S+?)\s*\])?\s*"
    r"([A-Za-z_][A-Za-z0-9_]*)\s*$")

# Old-style: one declaration statement can cover several comma-separated
# names at once ("input wire Clk, Comp, Start;"), and the bit range (if
# any) can be a parameterized expression ("[Bits - 1 : 0]"), not just a
# numeric literal.
_OLD_STYLE_DECL_RE = re.compile(
    r"\b(input|output|inout)\b\s*(?:wire|reg|logic)?\s*"
    r"(?:\[\s*([^\]:]+?)\s*:\s*([^\]]+?)\s*\])?\s*"
    r"([A-Za-z_]\w*(?:\s*,\s*[A-Za-z_]\w*)*)\s*;")

_PARAMETER_RE = re.compile(
    r"\bparameter\b\s*(?:integer\s+|real\s+)?([A-Za-z_]\w*)\s*=\s*([^,;]+)")

# ANSI parameter port list ("module foo #(parameter Bits = 4, Other = 2)
# (...)"): "parameter"/"localparam" is only mandatory before the first
# entry in a comma-separated run, later ones may omit it and just
# continue the list -- unlike the old-style _PARAMETER_RE case, where
# each is its own ";"-terminated statement.
_ANSI_PARAM_KEYWORD_RE = re.compile(r"\b(?:parameter|localparam)\b\s*(?:integer\s+|real\s+)?")
_ANSI_PARAM_ASSIGN_RE = re.compile(r"^([A-Za-z_]\w*)\s*=\s*(.+)$")


def _parse_ansi_params(param_text):
    """Parse a "#( parameter A = 1, B = A+1, parameter C = 2 )" block's
    inner text into an ordered {name: value} dict, evaluating each
    value against the params already defined earlier in the same list
    (a later default referencing an earlier parameter is normal)."""
    text = _ANSI_PARAM_KEYWORD_RE.sub("", param_text)
    params = {}
    for entry in text.split(","):
        m = _ANSI_PARAM_ASSIGN_RE.match(entry.strip())
        if not m:
            continue  # not a "name = value" entry -- ignore rather than fail
        name, expr = m.groups()
        try:
            params[name] = _eval_int_expr(expr, params)
        except ValueError:
            pass  # non-numeric default (e.g. a string) -- fine, not used for widths
    return params


_SAFE_BINOPS = {
    ast.Add: operator.add, ast.Sub: operator.sub, ast.Mult: operator.mul,
    ast.FloorDiv: operator.floordiv, ast.Div: operator.truediv,
    ast.Mod: operator.mod,
}
_SAFE_UNARYOPS = {ast.UAdd: operator.pos, ast.USub: operator.neg}


def _safe_eval_node(node, params):
    if isinstance(node, ast.Expression):
        return _safe_eval_node(node.body, params)
    if isinstance(node, ast.Constant) and isinstance(node.value, (int, float)):
        return node.value
    if isinstance(node, ast.Name):
        if node.id in params:
            return params[node.id]
        raise ValueError(f"unknown identifier {node.id!r}")
    if isinstance(node, ast.BinOp) and type(node.op) in _SAFE_BINOPS:
        return _SAFE_BINOPS[type(node.op)](
            _safe_eval_node(node.left, params), _safe_eval_node(node.right, params))
    if isinstance(node, ast.UnaryOp) and type(node.op) in _SAFE_UNARYOPS:
        return _SAFE_UNARYOPS[type(node.op)](_safe_eval_node(node.operand, params))
    raise ValueError(f"disallowed expression element: {ast.dump(node)}")


def _eval_int_expr(expr, params):
    """A bit-range bound: a plain int, or a small arithmetic expression
    over already-known parameters (e.g. "Bits - 1"). Evaluated via a
    whitelisted AST walk (only +, -, *, /, //, %, unary +/-, numeric
    constants, and known parameter names) -- NOT Python's eval(), even
    sandboxed with an empty __builtins__: that does not actually block
    the classic object-graph escape
    (`().__class__.__bases__[0].__subclasses__()`, confirmed reachable
    with zero builtins in scope), so it provided no real restriction to
    "just arithmetic" despite an earlier version of this function
    claiming exactly that."""
    expr = expr.strip()
    try:
        return int(expr)
    except ValueError:
        pass
    try:
        tree = ast.parse(expr, mode="eval")
        value = _safe_eval_node(tree, dict(params))
    except Exception as e:
        raise ValueError(f"cannot evaluate bit-range expression {expr!r}: {e}")
    return int(value)


def _expand_bits(name, direction, msb_expr, lsb_expr, params):
    if msb_expr is None:
        return [(name, direction)]
    msb = _eval_int_expr(msb_expr, params)
    lsb = _eval_int_expr(lsb_expr, params)
    step = -1 if msb >= lsb else 1
    return [(f"{name}[{b}]", direction) for b in range(msb, lsb + step, step)]


def parse_verilog_ports(path, module_name):
    """Returns a list of (bit_name, direction) tuples, one per bit,
    buses expanded MSB-first to match the netlist's own '<name>[<n>]'
    bracket-literal convention."""
    with open(path) as f:
        text = f.read()

    def _skip_balanced(s, open_pos):
        """open_pos indexes the opening '(' (or '['); returns index of
        its matching close. Ignores parens inside "// ..." comments --
        a port-list comment describing bit meanings (e.g. "(active
        high: 0=input, 1=output)") would otherwise close the scan early."""
        opener, closer = s[open_pos], {"(": ")", "[": "]"}[s[open_pos]]
        depth = 0
        i = open_pos
        n = len(s)
        while i < n:
            if s[i:i + 2] == "//":
                i = s.find("\n", i)
                if i == -1:
                    break
                continue
            if s[i] == opener:
                depth += 1
            elif s[i] == closer:
                depth -= 1
                if depth == 0:
                    return i
            i += 1
        raise ValueError(f"unbalanced '{opener}' starting at offset {open_pos}")

    mod_re = re.compile(r"\bmodule\s+" + re.escape(module_name) + r"\b")
    m = mod_re.search(text)
    if not m:
        raise ValueError(f"module '{module_name}' not found in {path}")
    pos = m.end()
    # Optional #( parameter ... ) block before the real port list -- its
    # parameters are in scope for ANSI-style [msb:lsb] bit-range
    # expressions in the port list that follows (e.g. "#(parameter
    # Bits = 4) (input wire [Bits-1:0] data, ...)").
    ansi_params = {}
    ws_and_hash = re.match(r"\s*#\s*", text[pos:])
    if ws_and_hash:
        candidate = pos + ws_and_hash.end()
        if text[candidate] == "(":
            param_end = _skip_balanced(text, candidate)
            ansi_params = _parse_ansi_params(text[candidate + 1:param_end])
            pos = param_end + 1
    try:
        paren_pos = text.index("(", pos)
    except ValueError:
        raise ValueError(
            f"module '{module_name}' has no port list -- no '(' found "
            f"after its name (and optional #(...) parameter block) in "
            f"{path}; a portless module has nothing for this tool to bind")
    end = _skip_balanced(text, paren_pos)
    port_section = text[paren_pos + 1:end]
    # Strip "// ..." comments from the WHOLE section before splitting on
    # commas -- a comment can contain its own commas (e.g. "(active
    # high: 0=input, 1=output)"), which would otherwise be mistaken for
    # port-list separators.
    port_section = "\n".join(ln.split("//")[0] for ln in port_section.splitlines())

    raw_entries = [" ".join(e.split()).strip() for e in port_section.split(",")]
    raw_entries = [e for e in raw_entries if e]
    is_ansi = bool(raw_entries) and re.match(r"^(input|output|inout)\b", raw_entries[0])

    if is_ansi:
        bits = []
        for decl in raw_entries:
            pm = _PORT_DECL_RE.match(decl)
            if not pm:
                raise ValueError(
                    f"cannot parse port declaration: {decl!r} (only simple "
                    f"ANSI-style 'input/output [wire|reg] [msb:lsb] name' "
                    f"declarations are supported)")
            direction, msb, lsb, name = pm.groups()
            bits.extend(_expand_bits(name, direction, msb, lsb, ansi_params))
        return bits

    # Old-style (Verilog-1995): the header lists bare port names only;
    # input/output/inout direction (and any bit range) is declared
    # separately in the module body, one statement per port or group of
    # ports (e.g. "input wire Clk, Comp, Start;").
    header_names = raw_entries
    body_end_match = re.search(r"\bendmodule\b", text[end + 1:])
    if not body_end_match:
        raise ValueError(f"no 'endmodule' found for module '{module_name}' in {path}")
    body = text[end + 1: end + 1 + body_end_match.start()]
    body = "\n".join(ln.split("//")[0] for ln in body.splitlines())

    params = {}
    for pm in _PARAMETER_RE.finditer(body):
        pname, pexpr = pm.groups()
        try:
            params[pname] = _eval_int_expr(pexpr, params)
        except ValueError:
            pass  # non-numeric parameter default (e.g. a string) -- fine, not used for widths

    declared = {}  # name -> (direction, msb_expr, lsb_expr)
    for dm in _OLD_STYLE_DECL_RE.finditer(body):
        direction, msb, lsb, names = dm.groups()
        for nm in names.split(","):
            nm = nm.strip()
            if nm in declared:
                raise ValueError(
                    f"port '{nm}' of module {module_name} is declared more "
                    f"than once (once as {declared[nm][0]}, again as {direction})")
            declared[nm] = (direction, msb, lsb)

    missing = [n for n in header_names if n not in declared]
    if missing:
        raise ValueError(
            f"module {module_name}'s port list names {missing}, but no "
            f"input/output/inout declaration for {'them' if len(missing) > 1 else 'it'} "
            f"was found in the module body -- old-style port declarations "
            f"need a separate 'input/output [wire|reg] [msb:lsb] name;' "
            f"statement per port")
    extra = [n for n in declared if n not in header_names]
    if extra:
        raise ValueError(
            f"module {module_name} declares {extra} as input/output/inout "
            f"in its body, but {'they are' if len(extra) > 1 else 'it is'} "
            f"not in the module's own port list in the header -- typo, or "
            f"a genuinely internal signal accidentally given a direction "
            f"keyword?")

    bits = []
    for name in header_names:  # preserve the header's original port order
        direction, msb, lsb = declared[name]
        bits.extend(_expand_bits(name, direction, msb, lsb, params))
    return bits


# ---------------------------------------------------------------------------
# cosim_top.v generation
# ---------------------------------------------------------------------------

def _sanitize(path):
    """Instance path -> a valid, unique Verilog identifier fragment."""
    return re.sub(r"[^A-Za-z0-9_]", "_", ":".join(path))


def _plan_one_dut(netlist, module_name, verilog_dir, path):
    """Resolve one (module, instance path) occurrence into everything
    the emitter needs: bits, per-bit addresses, and a unique naming
    namespace so multiple DUT instances (same or different modules)
    never collide in the generated file."""
    verilog_path = os.path.join(verilog_dir, module_name + ".v")
    if not os.path.isfile(verilog_path):
        raise ValueError(
            f"module '{module_name}' matched instance path "
            f"{':'.join(path)}, but no Verilog file found at "
            f"{verilog_path} (expected '<module>.v' by convention -- "
            f"pass --verilog-dir if your files live elsewhere)")
    bits = parse_verilog_ports(verilog_path, module_name)
    chain = netlist.resolve_chain(path)

    a2d, d2a = [], []
    for bit_name, direction in bits:
        try:
            addr = Netlist.resolve_address(chain, path, bit_name)
        except ValueError as e:
            raise ValueError(f"resolving '{bit_name}' of {module_name} "
                              f"at {':'.join(path)}: {e}")
        if direction == "inout":
            print(f"WARNING: {module_name}'s port '{bit_name}' is declared "
                  f"inout; the bridge can't do real bidirectional "
                  f"signaling, so it's bound as d2a (digital drives "
                  f"analog) only. If the analog side needs to drive this "
                  f"net back, bind it manually instead.", file=sys.stderr)
        (a2d if direction == "input" else d2a).append((bit_name, addr))

    def base_name(bit):
        return bit.split("[")[0]

    ports_in_order, seen, width, dir_of = [], set(), {}, {}
    for bit_name, direction in bits:
        b = base_name(bit_name)
        if b not in seen:
            seen.add(b)
            ports_in_order.append(b)
        width.setdefault(b, []).append(bit_name)
        dir_of[b] = direction

    return {
        "module": module_name,
        "path": path,
        "namespace": f"{module_name}__{_sanitize(path)}",
        "ports_in_order": ports_in_order,
        "width": width,
        "dir_of": dir_of,
        "a2d": a2d,
        "d2a": d2a,
    }


def generate(args):
    netlist = Netlist(args.netlist)
    verilog_dir = args.verilog_dir or os.path.dirname(os.path.abspath(args.netlist))

    modules = list(dict.fromkeys(args.modules))  # de-dupe, preserve order
    if len(modules) != len(args.modules):
        print(f"NOTE: duplicate entries in --modules were collapsed to "
              f"{modules}", file=sys.stderr)

    duts = []
    for module_name, subckt_name in modules:
        try:
            paths = netlist.find_instance_paths(subckt_name)
        except ValueError as e:
            print(f"ERROR: {e}", file=sys.stderr)
            sys.exit(1)
        if not paths:
            msg = (f"ERROR: no subckt named '{subckt_name}' is instantiated "
                   f"anywhere in {args.netlist}")
            if subckt_name != module_name:
                msg += f" (looking for Verilog module '{module_name}')"
            # Suggest from what's actually INSTANTIATED (by_type), not
            # from every subckt DEFINITION -- a defined-but-never-used
            # subckt would otherwise get suggested right back as the
            # "fix" for its own "not instantiated anywhere" error.
            close = difflib.get_close_matches(
                subckt_name, netlist.by_type.keys(), n=3, cutoff=0.6)
            if close:
                msg += (f"\n  Did you mean one of: {', '.join(close)}? If the "
                        f"netlist's subckt/symbol name genuinely differs from "
                        f"the Verilog module name, use "
                        f"--modules {module_name}:<real_subckt_name>.")
            else:
                msg += (f"\n  No similarly-named subckt found either -- check "
                        f"'{subckt_name}' is really instantiated in this "
                        f"netlist, or that --netlist points at the right file.")
            print(msg, file=sys.stderr)
            sys.exit(1)
        for path in paths:
            try:
                duts.append(_plan_one_dut(netlist, module_name, verilog_dir, path))
            except ValueError as e:
                print(f"ERROR: {e}", file=sys.stderr)
                sys.exit(1)
        label = module_name if subckt_name == module_name else f"{module_name} (subckt {subckt_name})"
        print(f"{label}: found {len(paths)} instance(s) at "
              f"{', '.join(':'.join(p) for p in paths)}", file=sys.stderr)

    lines = []
    lines.append("`timescale 1ns / 1ps")
    lines.append("module cosim_top;")
    lines.append(f"    // Auto-generated by gen_cosim_top.py from {args.netlist}.")
    lines.append(f"    // Instance paths were discovered automatically by searching "
                 f"the netlist for each --modules name.")
    lines.append(f"    // Defaults below are placeholders -- check against the real "
                 f"supply domain before trusting them.")
    lines.append(f"    localparam VTH_LO = {args.vth_lo};")
    lines.append(f"    localparam VTH_HI = {args.vth_hi};")
    lines.append(f"    localparam D2A_VDD = {args.d2a_vdd};")
    lines.append(f"    localparam D2A_R   = {args.d2a_r};")
    lines.append(f"    localparam D2A_C   = {args.d2a_c};")
    lines.append("")

    for d in duts:
        lines.append(f"    // {d['module']} at {':'.join(d['path'])}")
        for b in d["ports_in_order"]:
            n = len(d["width"][b])
            decl = "reg" if d["dir_of"][b] == "input" else "wire"
            rng = f" [{n - 1}:0]" if n > 1 else ""
            lines.append(f"    {decl}{rng} {d['namespace']}__{b};")
    lines.append("")

    for d in duts:
        lines.append(f"    {d['module']} u_{d['namespace']} (")
        conn_lines = [f"        .{b}({d['namespace']}__{b})" for b in d["ports_in_order"]]
        lines.append(",\n".join(conn_lines))
        lines.append("    );")
        lines.append("")

    lines.append("    initial begin")
    lines.append('        $dumpfile("cosim.vcd"); $dumpvars(0, cosim_top);')
    total_a2d = total_d2a = 0
    for d in duts:
        lines.append(f"\n        // {d['module']} at {':'.join(d['path'])}")
        for bit_name, addr in d["a2d"]:
            lines.append(f'        $cosim_a2d("{addr}", {d["namespace"]}__{bit_name}, VTH_LO, VTH_HI);')
        for bit_name, addr in d["d2a"]:
            lines.append(f'        $cosim_d2a("{addr}", {d["namespace"]}__{bit_name}, D2A_VDD, D2A_R, D2A_C);')
        total_a2d += len(d["a2d"])
        total_d2a += len(d["d2a"])
    lines.append("")
    lines.append(f'        $cosim_run("{args.netlist_file}", {args.tstop}, {args.tstep});')
    lines.append("    end")
    lines.append("endmodule")

    return "\n".join(lines) + "\n", total_a2d, total_d2a


def main():
    p = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--netlist", required=True,
                    help="the exported VACASK/spectre netlist (.spectre)")
    def _parse_modules(s):
        out = []
        for m in s.split(","):
            m = m.strip()
            if not m:
                continue
            # "module:subckt" when the netlist's subckt/symbol name
            # differs from the Verilog module name (e.g. an xschem
            # symbol named "dut_counter_block" wrapping "module
            # dut_counter" in dut_counter.v) -- defaults to matching
            # names, which covers the common case with no extra syntax.
            if ":" in m:
                module_name, subckt_name = m.split(":", 1)
            else:
                module_name = subckt_name = m
            out.append((module_name.strip(), subckt_name.strip()))
        return out

    p.add_argument("--modules", required=True, type=_parse_modules,
                    help="comma-separated Verilog module name(s) to bind, e.g. "
                         "--modules top_dig or "
                         "--modules top_dig,example_dig,counter_dig "
                         "(spaces around commas are fine). If the netlist's "
                         "subckt/symbol name differs from the Verilog module "
                         "name, write module:subckt, e.g. "
                         "--modules dut_counter:dut_counter_block")
    p.add_argument("--verilog-dir", default=None,
                    help="directory to find '<module>.v' files in "
                         "(default: the netlist's own directory)")
    p.add_argument("--vth-lo", default="0.6")
    p.add_argument("--vth-hi", default="1.2")
    p.add_argument("--d2a-vdd", default="1.8")
    p.add_argument("--d2a-r", default="1e3")
    p.add_argument("--d2a-c", default="1e-12")
    p.add_argument("--tstop", default=None,
                    help="stop time for $cosim_run, e.g. 400e-9 (default: read "
                         "from the netlist's own 'tran'/'analysis ... tran' "
                         "statement; falls back to 400e-9 if neither is given)")
    p.add_argument("--tstep", default=None,
                    help="fixed timestep for $cosim_run (default: read from "
                         "the netlist the same way as --tstop; falls back to "
                         "1e-9 if neither is given)")
    p.add_argument("--netlist-file", default=None,
                    help="netlist filename to pass to $cosim_run (defaults to --netlist's basename)")
    p.add_argument("-o", "--output", default="cosim_top.v")
    args = p.parse_args()
    if args.netlist_file is None:
        args.netlist_file = os.path.basename(args.netlist)

    if args.tstop is None or args.tstep is None:
        net_tstop, net_tstep = find_tran_settings(args.netlist)
        if args.tstop is None:
            args.tstop = net_tstop or "400e-9"
            print(f"NOTE: --tstop not given, using "
                  f"{'netlist tran statement' if net_tstop else 'built-in default'}: "
                  f"{args.tstop}", file=sys.stderr)
        if args.tstep is None:
            args.tstep = net_tstep or "1e-9"
            print(f"NOTE: --tstep not given, using "
                  f"{'netlist tran statement' if net_tstep else 'built-in default'}: "
                  f"{args.tstep}", file=sys.stderr)

    # Normalize to plain Verilog-safe numeric literals regardless of
    # source -- a netlist-derived "600n"/"1u" is SPICE engineering
    # notation, not valid Verilog, and needs converting either way.
    try:
        args.tstop = "%.10g" % _to_seconds(str(args.tstop))
        args.tstep = "%.10g" % _to_seconds(str(args.tstep))
    except ValueError as e:
        print(f"ERROR: could not parse tstop/tstep as a number "
              f"(tstop={args.tstop!r} tstep={args.tstep!r}): {e}",
              file=sys.stderr)
        sys.exit(1)

    text, n_a2d, n_d2a = generate(args)
    with open(args.output, "w") as f:
        f.write(text)
    print(f"wrote {args.output}: {n_a2d} A2D, {n_d2a} D2A bindings", file=sys.stderr)


if __name__ == "__main__":
    main()
