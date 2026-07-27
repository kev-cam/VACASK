from dataclasses import dataclass, field

from .exc import RpnToVaError
from .lexer import tokenize


# AST node types. Plain dataclasses, one per kind of thing a formula can do.
@dataclass
class Num:
    value: float


@dataclass
class Ident:
    name: str


@dataclass
class Call:
    name: str
    args: list = field(default_factory=list)


@dataclass
class BinOp:
    op: str
    left: object
    right: object


@dataclass
class UnaryOp:
    op: str
    operand: object


@dataclass
class Ternary:
    cond: object
    if_true: object
    if_false: object


# Operator precedence and associativity, transcribed from VACASK's own
# grammar (lib/dflparser.y:192-205). Precedence increases with the number
# (tighter binding). Note that NEG/NOT/BITNOT (unary prefix operators, 13)
# bind *tighter* than POWER (12) in VACASK's grammar - so "-a**b" parses as
# "(-a)**b", not "-(a**b)" as in many other languages. This is intentional
# and mirrored here exactly, not a bug.
#   (token kind, operator symbol, precedence, right-associative)
_PRECEDENCE_TABLE = [
    ("OR", "||", 2, False),
    ("AND", "&&", 3, False),
    ("BITOR", "|", 4, False),
    ("BITEXOR", "^", 5, False),
    ("BITAND", "&", 6, False),
    ("EQUAL", "==", 7, False),
    ("NOTEQUAL", "!=", 7, False),
    ("LESS", "<", 8, False),
    ("GREATER", ">", 8, False),
    ("GREATEREQ", ">=", 8, False),
    ("LESSEQ", "<=", 8, False),
    ("BITSHIFTL", "<<", 9, False),
    ("BITSHIFTR", ">>", 9, False),
    ("PLUS", "+", 10, False),
    ("MINUS", "-", 10, False),
    ("TIMES", "*", 11, False),
    ("DIVIDE", "/", 11, False),
    ("POWER", "**", 12, True),
]

QUESTION_PRECEDENCE = 1
UNARY_PRECEDENCE = 13

_KIND_PRECEDENCE = {kind: (prec, right) for kind, sym, prec, right in _PRECEDENCE_TABLE}

# Re-exported for codegen.py, so parenthesization decisions there use the
# exact same table instead of a second, possibly-drifting copy of it.
OPERATOR_PRECEDENCE = {sym: prec for kind, sym, prec, right in _PRECEDENCE_TABLE}
RIGHT_ASSOC_OPERATORS = {sym for kind, sym, prec, right in _PRECEDENCE_TABLE if right}

_UNARY_KINDS = {"MINUS", "PLUS", "NOT", "BITNOT"}


class _Parser:
    def __init__(self, tokens):
        self.tokens = tokens
        self.i = 0

    def peek(self):
        if self.i < len(self.tokens):
            return self.tokens[self.i]
        return None

    def advance(self):
        tok = self.tokens[self.i]
        self.i += 1
        return tok

    def expect(self, kind, what):
        tok = self.peek()
        if tok is None or tok.kind != kind:
            pos = tok.pos if tok is not None else None
            raise RpnToVaError("expected "+what, pos=pos)
        return self.advance()

    def parse_expr(self, min_prec):
        left = self._parse_operand()

        while True:
            tok = self.peek()
            if tok is None:
                break

            if tok.kind == "QUESTION":
                if QUESTION_PRECEDENCE < min_prec:
                    break
                self.advance()
                if_true = self.parse_expr(0)
                self.expect("COLON", "':' in ternary expression")
                if_false = self.parse_expr(QUESTION_PRECEDENCE)
                left = Ternary(left, if_true, if_false)
                continue

            info = _KIND_PRECEDENCE.get(tok.kind)
            if info is None:
                break
            prec, right_assoc = info
            if prec < min_prec:
                break

            self.advance()
            next_min = prec if right_assoc else prec+1
            right = self.parse_expr(next_min)
            left = BinOp(tok.value, left, right)

        return left

    def _parse_operand(self):
        tok = self.peek()
        if tok is not None and tok.kind in _UNARY_KINDS:
            self.advance()
            operand = self.parse_expr(UNARY_PRECEDENCE)
            if tok.kind == "PLUS":
                # Unary plus is a no-op, matching dflparser.y's
                # "PLUS expr %prec NEG" rule (passes the operand through).
                return operand
            return UnaryOp(tok.value, operand)
        return self._parse_primary()

    def _parse_primary(self):
        tok = self.peek()
        if tok is None:
            raise RpnToVaError("unexpected end of expression")

        if tok.kind == "NUMBER":
            self.advance()
            return Num(tok.value)

        if tok.kind == "IDENT":
            self.advance()
            if self.peek() is not None and self.peek().kind == "LPAREN":
                self.advance()
                args = self._parse_args()
                self.expect("RPAREN", "')' to close call to '"+tok.value+"'")
                return Call(tok.value, args)
            return Ident(tok.value)

        if tok.kind == "LPAREN":
            self.advance()
            inner = self.parse_expr(0)
            self.expect("RPAREN", "')' to close '('")
            return inner

        raise RpnToVaError(
            "unexpected token '"+str(tok.value)+"'", pos=tok.pos)

    def _parse_args(self):
        args = []
        if self.peek() is not None and self.peek().kind == "RPAREN":
            return args
        args.append(self.parse_expr(0))
        while self.peek() is not None and self.peek().kind == "COMMA":
            self.advance()
            args.append(self.parse_expr(0))
        return args


def parse(tokens):
    """
    Parses a flat token list (as produced by lexer.tokenize) into an AST,
    respecting the same operator precedence and associativity as VACASK's
    own grammar (lib/dflparser.y).
    """
    parser = _Parser(tokens)
    if parser.peek() is None:
        raise RpnToVaError("empty expression")
    node = parser.parse_expr(0)
    trailing = parser.peek()
    if trailing is not None:
        raise RpnToVaError(
            "unexpected token '"+str(trailing.value)+"' after expression",
            pos=trailing.pos)
    return node


def parse_text(text):
    """Convenience wrapper: tokenize and parse *text* in one call."""
    return parse(tokenize(text))
