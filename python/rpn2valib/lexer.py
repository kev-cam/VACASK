import re

from .exc import RpnToVaError

# Multi-character operators are listed before any single-character operator
# that is one of their prefixes, so the scanner tries the longer match first.
_OPERATORS = [
    ("**", "POWER"),
    ("==", "EQUAL"),
    ("!=", "NOTEQUAL"),
    ("<=", "LESSEQ"),
    (">=", "GREATEREQ"),
    ("&&", "AND"),
    ("||", "OR"),
    ("<<", "BITSHIFTL"),
    (">>", "BITSHIFTR"),
    ("+", "PLUS"),
    ("-", "MINUS"),
    ("*", "TIMES"),
    ("/", "DIVIDE"),
    ("(", "LPAREN"),
    (")", "RPAREN"),
    (",", "COMMA"),
    ("<", "LESS"),
    (">", "GREATER"),
    ("&", "BITAND"),
    ("|", "BITOR"),
    ("^", "BITEXOR"),
    ("~", "BITNOT"),
    ("!", "NOT"),
    ("?", "QUESTION"),
    (":", "COLON"),
]

# SI-prefix suffixes recognized on numeric literals, taken from VACASK's own
# tokenizer (lib/dfllexer.l:519-558). Three-letter suffixes are listed before
# the single letter they start with, so "1meg"/"1mil" are not mistaken for
# "1" milli followed by garbage. Case-sensitive: lowercase "m" is milli,
# uppercase "M" is mega; "a" (atto, 1e-18) has no uppercase form in VACASK.
_SI_SUFFIXES = [
    ("meg", 1e6),
    ("mil", 25.4e-6),
    ("f", 1e-15),
    ("p", 1e-12),
    ("n", 1e-9),
    ("u", 1e-6),
    ("m", 1e-3),
    ("k", 1e3),
    ("K", 1e3),
    ("M", 1e6),
    ("G", 1e9),
    ("T", 1e12),
    ("x", 1e6),
    ("X", 1e6),
    ("a", 1e-18),
]

_MANTISSA_RE = re.compile(r"\d+\.\d*|\.\d+|\d+")
_EXPONENT_RE = re.compile(r"[eE][+-]?\d+")
_UNIT_TAIL_RE = re.compile(r"[a-zA-Z_]*")
# Identifier charset matches VACASK's own lib/dfllexer.l:731 ([a-zA-Z_$][0-9a-zA-Z_$]*).
_IDENT_RE = re.compile(r"[A-Za-z_$][A-Za-z_$0-9]*")
_HEX_RE = re.compile(r"0[xX]")
_WS_RE = re.compile(r"\s+")


class Token:
    __slots__ = ("kind", "value", "pos")

    def __init__(self, kind, value, pos):
        self.kind = kind
        self.value = value
        self.pos = pos

    def __repr__(self):
        return "Token(%s, %r, %d)" % (self.kind, self.value, self.pos)


def tokenize(text):
    """
    Turns *text* into a flat list of Token objects, mirroring the token set
    of VACASK's own expression tokenizer (lib/dfllexer.l), restricted to
    just what a standalone expression needs (no whole-netlist-file syntax).
    """
    tokens = []
    i = 0
    n = len(text)
    while i < n:
        ws = _WS_RE.match(text, i)
        if ws:
            i = ws.end()
            continue
        if i >= n:
            break

        start = i
        c = text[i]

        if _HEX_RE.match(text, i):
            raise RpnToVaError(
                "hexadecimal literals are not supported here", pos=i)

        if c.isdigit() or (c == "." and i+1 < n and text[i+1].isdigit()):
            mantissa = _MANTISSA_RE.match(text, i)
            mantissa_end = mantissa.end()

            exponent = _EXPONENT_RE.match(text, mantissa_end)
            if exponent:
                i = exponent.end()
                value = float(text[start:i])
            else:
                suffix_info = None
                for suf, mult in _SI_SUFFIXES:
                    if text.startswith(suf, mantissa_end):
                        suffix_info = (suf, mult)
                        break

                if suffix_info is not None:
                    suf, mult = suffix_info
                    value = float(text[start:mantissa_end]) * mult
                    tail = _UNIT_TAIL_RE.match(text, mantissa_end+len(suf))
                    i = tail.end()
                else:
                    i = mantissa_end
                    value = float(text[start:i])

            tokens.append(Token("NUMBER", value, start))
            continue

        ident = _IDENT_RE.match(text, i)
        if ident:
            tokens.append(Token("IDENT", ident.group(0), start))
            i = ident.end()
            continue

        matched_op = False
        for op, kind in _OPERATORS:
            if text.startswith(op, i):
                tokens.append(Token(kind, op, start))
                i += len(op)
                matched_op = True
                break
        if matched_op:
            continue

        raise RpnToVaError("unexpected character '"+c+"'", pos=i)

    return tokens


def parse_number(text):
    """
    Parses a single standalone numeric literal, with an optional VACASK-style
    SI suffix (e.g. "2.5u"), and returns its resolved float value. Used to
    resolve --param defaults given on the command line, so suffix handling
    only lives in one place.
    """
    tokens = tokenize(text)
    if len(tokens) != 1 or tokens[0].kind != "NUMBER":
        raise RpnToVaError("'"+text+"' is not a valid number")
    return tokens[0].value
