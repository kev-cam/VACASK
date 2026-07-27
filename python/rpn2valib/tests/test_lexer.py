from rpn2valib.exc import RpnToVaError
from rpn2valib.lexer import parse_number, tokenize


def test_running_example():
    tokens = tokenize("v(in1)*v(in2) - offset")
    kinds = [t.kind for t in tokens]
    assert kinds == [
        "IDENT", "LPAREN", "IDENT", "RPAREN", "TIMES",
        "IDENT", "LPAREN", "IDENT", "RPAREN",
        "MINUS", "IDENT",
    ]
    assert [t.value for t in tokens if t.kind == "IDENT"] == [
        "v", "in1", "v", "in2", "offset"]


def test_si_suffixes():
    # lib/dfllexer.l:519-558, including the "a" (atto) suffix the doc
    # originally missed.
    cases = {
        "1f": 1e-15, "1p": 1e-12, "1n": 1e-9, "1u": 1e-6, "1m": 1e-3,
        "1k": 1e3, "1K": 1e3, "1M": 1e6, "1G": 1e9, "1T": 1e12,
        "1x": 1e6, "1X": 1e6, "1meg": 1e6, "1mil": 25.4e-6, "2.5a": 2.5e-18,
    }
    for text, expected in cases.items():
        got = parse_number(text)
        assert abs(got-expected) <= abs(expected)*1e-12, (text, got, expected)


def test_m_and_M_are_different():
    assert parse_number("1m") == 1e-3
    assert parse_number("1M") == 1e6


def test_meg_and_mil_checked_before_bare_m():
    assert parse_number("1meg") == 1e6
    assert parse_number("1mil") == 25.4e-6


def test_exponent_form():
    assert parse_number("1e3") == 1e3
    assert parse_number("1.5e-2") == 1.5e-2


def test_plain_float_forms():
    assert parse_number("12.34") == 12.34
    assert parse_number(".5") == 0.5
    assert parse_number("12.") == 12.0


def test_rejects_hex():
    try:
        tokenize("0x2A")
        assert False, "expected RpnToVaError"
    except RpnToVaError:
        pass


def test_rejects_bad_character():
    try:
        tokenize("a @ b")
        assert False, "expected RpnToVaError"
    except RpnToVaError:
        pass


def run():
    test_running_example()
    test_si_suffixes()
    test_m_and_M_are_different()
    test_meg_and_mil_checked_before_bare_m()
    test_exponent_form()
    test_plain_float_forms()
    test_rejects_hex()
    test_rejects_bad_character()


if __name__ == "__main__":
    run()
    print("test_lexer: OK")
