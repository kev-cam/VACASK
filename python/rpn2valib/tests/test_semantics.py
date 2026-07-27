from rpn2valib import semantics
from rpn2valib.exc import RpnToVaError
from rpn2valib.parser import parse_text


def test_ports_and_params_running_example():
    ast = parse_text("v(in1)*v(in2) - offset")
    result = semantics.analyze(ast, {"offset": 0.5})
    assert result.ports == ["in1", "in2"]
    assert result.params == {"offset": 0.5}


def test_ports_first_seen_order():
    ast = parse_text("v(b) + v(a) + v(b)")
    result = semantics.analyze(ast, {})
    assert result.ports == ["b", "a"]


def test_missing_param_lists_all():
    ast = parse_text("gain*v(in1) + offset")
    try:
        semantics.analyze(ast, {})
        assert False, "expected RpnToVaError"
    except RpnToVaError as e:
        assert "gain" in str(e) and "offset" in str(e)


def test_constants_are_not_parameters():
    ast = parse_text("v(in1) * M_PI")
    result = semantics.analyze(ast, {})
    assert result.params == {}


def test_time_is_not_a_parameter():
    ast = parse_text("sin(time)")
    result = semantics.analyze(ast, {})
    assert result.params == {}
    assert result.ports == []


def test_unsupported_function_rejected():
    ast = parse_text("vector(1,2,3)")
    try:
        semantics.analyze(ast, {})
        assert False, "expected RpnToVaError"
    except RpnToVaError as e:
        assert "vector" in str(e)


def test_v_wrong_arity_rejected():
    ast = parse_text("v(a,b,c)")
    try:
        semantics.analyze(ast, {})
        assert False, "expected RpnToVaError"
    except RpnToVaError:
        pass


def test_i_with_two_args_rejected():
    ast = parse_text("i(a,b)")
    try:
        semantics.analyze(ast, {})
        assert False, "expected RpnToVaError"
    except RpnToVaError:
        pass


def test_node_param_name_collision_rejected():
    ast = parse_text("v(x) + x")
    try:
        semantics.analyze(ast, {"x": 1.0})
        assert False, "expected RpnToVaError"
    except RpnToVaError as e:
        assert "x" in str(e)


def test_dollar_identifier_rejected():
    ast = parse_text("$temp")
    try:
        semantics.analyze(ast, {})
        assert False, "expected RpnToVaError"
    except RpnToVaError as e:
        assert "$temp" in str(e)


def run():
    test_ports_and_params_running_example()
    test_ports_first_seen_order()
    test_missing_param_lists_all()
    test_constants_are_not_parameters()
    test_time_is_not_a_parameter()
    test_unsupported_function_rejected()
    test_v_wrong_arity_rejected()
    test_i_with_two_args_rejected()
    test_node_param_name_collision_rejected()
    test_dollar_identifier_rejected()


if __name__ == "__main__":
    run()
    print("test_semantics: OK")
