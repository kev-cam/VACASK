from rpn2valib.parser import BinOp, Call, Ident, Num, Ternary, UnaryOp, parse_text


def test_precedence_times_before_plus():
    # a + b*c -> +(a, *(b,c))
    ast = parse_text("a + b*c")
    assert ast == BinOp("+", Ident("a"), BinOp("*", Ident("b"), Ident("c")))


def test_left_associativity():
    # a - b - c -> -(-(a,b),c)
    ast = parse_text("a - b - c")
    assert ast == BinOp("-", BinOp("-", Ident("a"), Ident("b")), Ident("c"))


def test_explicit_parens_override_grouping():
    ast = parse_text("a - (b - c)")
    assert ast == BinOp("-", Ident("a"), BinOp("-", Ident("b"), Ident("c")))


def test_ternary():
    ast = parse_text("v(sig) > thresh ? 5 : 0")
    assert ast == Ternary(
        BinOp(">", Call("v", [Ident("sig")]), Ident("thresh")),
        Num(5.0), Num(0.0))


def test_ternary_right_associative_chaining():
    # a?b:c?d:e -> a ? b : (c ? d : e)
    ast = parse_text("a?b:c?d:e")
    assert ast == Ternary(
        Ident("a"), Ident("b"),
        Ternary(Ident("c"), Ident("d"), Ident("e")))


def test_and_or():
    ast = parse_text("a && b || c")
    # || binds looser than && (lib/dflparser.y:193-194)
    assert ast == BinOp("||", BinOp("&&", Ident("a"), Ident("b")), Ident("c"))


def test_unary_binds_tighter_than_power():
    # -a**b parses as (-a)**b, not -(a**b) - see parser.py's precedence
    # table comment for why (lib/dflparser.y: NEG > POWER).
    ast = parse_text("-a**b")
    assert ast == BinOp("**", UnaryOp("-", Ident("a")), Ident("b"))


def test_function_call():
    ast = parse_text("v(in1)*v(in2) - offset")
    assert ast == BinOp(
        "-",
        BinOp("*", Call("v", [Ident("in1")]), Call("v", [Ident("in2")])),
        Ident("offset"))


def test_call_no_args():
    ast = parse_text("foo()")
    assert ast == Call("foo", [])


def run():
    test_precedence_times_before_plus()
    test_left_associativity()
    test_explicit_parens_override_grouping()
    test_ternary()
    test_ternary_right_associative_chaining()
    test_and_or()
    test_unary_binds_tighter_than_power()
    test_function_call()
    test_call_no_args()


if __name__ == "__main__":
    run()
    print("test_parser: OK")
