from rpn2valib.codegen import generate
from rpn2valib.parser import parse_text


def gen(expr):
    return generate(parse_text(expr))[0]


def test_running_example():
    assert gen("v(in1)*v(in2) - offset") == "V(in1) * V(in2) - offset"


def test_v_two_args():
    assert gen("v(out, in1)") == "V(out, in1)"


def test_i_one_arg():
    assert gen("i(rsense)") == "I(rsense)"


def test_power_maps_to_pow():
    assert gen("a ** b") == "pow(a, b)"


def test_log_is_natural_log():
    # VACASK's log() is natural log (lib/context.cpp:52-53, both "log" and
    # "ln" registered to FwLn) - must become Verilog-A's ln(), not log().
    text = gen("log(a)")
    assert text == "ln(a)"


def test_log10_is_base10_log():
    assert gen("log10(a)") == "log(a)"


def test_sgn_one_arg_expansion():
    assert gen("sgn(a)") == "((a)>=0 ? 1 : -1)"


def test_sign_two_arg_copysign_expansion():
    # sign(x1,x2) is a *different* function from sgn(x): copysign-style,
    # 2 args (lib/rpnfunctor.h FwSign).
    assert gen("sign(a, b)") == "((b)>=0 ? abs(a) : -abs(a))"


def test_round_expansion():
    assert gen("round(a)") == "((a)>=0 ? floor((a)+0.5) : ceil((a)-0.5))"


def test_atan2_matches_vacasks_own_definition():
    # VACASK's atan2(x1,x2) is literally atan(x1/x2) (lib/rpnfunctor.h
    # FwAtan2), not a quadrant-correct atan2 - replicated exactly.
    text, notes = generate(parse_text("atan2(a, b)"))
    assert text == "(atan((a)/(b)))"
    assert any("atan2" in n for n in notes)


def test_int_maps_to_rtoi():
    assert gen("int(a)") == "$rtoi(a)"
    assert gen("integer(a)") == "$rtoi(a)"


def test_real_is_a_noop():
    assert gen("real(a+b)") == "(a + b)"


def test_m_degperrad_inlined_with_note():
    text, notes = generate(parse_text("a * M_DEGPERRAD"))
    assert text.startswith("a * 57.29577951308232")
    assert any("M_DEGPERRAD" in n for n in notes)


def test_renamed_constants():
    assert gen("P_Q") == "`P_Q_OLD"
    assert gen("P_K") == "`P_K_OLD"
    assert gen("P_H") == "`P_H_OLD"
    assert gen("P_EPS0") == "`P_EPS0_OLD"


def test_direct_constant():
    assert gen("M_PI") == "`M_PI"


def test_time_maps_to_abstime():
    text, notes = generate(parse_text("time"))
    assert text == "$abstime"
    assert any("time" in n for n in notes)


def test_unary_minus_needs_parens_around_binop():
    assert gen("-(a+b)") == "-(a + b)"


def test_unary_binds_tighter_than_power_in_output():
    assert gen("-a**b") == "pow(-a, b)"


def test_explicit_parens_reproduced_on_right_child():
    assert gen("a-(b-c)") == "a - (b - c)"


def test_ternary_parenthesized_inside_binop():
    assert gen("2*(a>b?1:0)") == "2.0 * (a > b ? 1.0 : 0.0)"


def run():
    test_running_example()
    test_v_two_args()
    test_i_one_arg()
    test_power_maps_to_pow()
    test_log_is_natural_log()
    test_log10_is_base10_log()
    test_sgn_one_arg_expansion()
    test_sign_two_arg_copysign_expansion()
    test_round_expansion()
    test_atan2_matches_vacasks_own_definition()
    test_int_maps_to_rtoi()
    test_real_is_a_noop()
    test_m_degperrad_inlined_with_note()
    test_renamed_constants()
    test_direct_constant()
    test_time_maps_to_abstime()
    test_unary_minus_needs_parens_around_binop()
    test_unary_binds_tighter_than_power_in_output()
    test_explicit_parens_reproduced_on_right_child()
    test_ternary_parenthesized_inside_binop()


if __name__ == "__main__":
    run()
    print("test_codegen: OK")
