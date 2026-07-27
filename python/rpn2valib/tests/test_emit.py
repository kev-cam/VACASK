from rpn2valib import codegen, emit, semantics
from rpn2valib.parser import parse_text


def test_worked_example():
    # docs/behavioral-source-rpn-to-verilog-a-guide.md section 8.
    ast = parse_text("v(in1)*v(in2) - offset")
    result = semantics.analyze(ast, {"offset": 0.5})
    expr_text, notes = codegen.generate(ast)
    va = emit.emit_module(
        "bsrc1", "V", ["p", "n"], result.ports, result.params, expr_text, notes)

    assert '`include "constants.vams"' in va
    assert '`include "disciplines.vams"' in va
    assert "module bsrc1(p, n, in1, in2);" in va
    assert "electrical p, n, in1, in2;" in va
    assert 'parameter real offset = 0.5;' in va
    assert "V(p, n) <+ V(in1) * V(in2) - offset;" in va
    assert va.rstrip().endswith("endmodule")


def test_feedback_port_not_duplicated():
    # Referencing the instance's own output port inside the formula
    # (e.g. v(p) for feedback) must not declare a duplicate port.
    ast = parse_text("v(p) + gain")
    result = semantics.analyze(ast, {"gain": 1.0})
    expr_text, notes = codegen.generate(ast)
    va = emit.emit_module(
        "fb1", "V", ["p", "n"], result.ports, result.params, expr_text, notes)
    assert "module fb1(p, n);" in va
    assert va.count("parameter real gain") == 1


def test_current_type_uses_I_contribution():
    ast = parse_text("1m")
    result = semantics.analyze(ast, {})
    expr_text, notes = codegen.generate(ast)
    va = emit.emit_module(
        "isrc1", "I", ["p", "n"], result.ports, result.params, expr_text, notes)
    assert "I(p, n) <+ 0.001;" in va


def test_notes_rendered_as_comments_and_deduped():
    ast = parse_text("atan2(v(a), v(b)) + atan2(v(a), v(b))")
    result = semantics.analyze(ast, {})
    expr_text, notes = codegen.generate(ast)
    va = emit.emit_module(
        "t1", "V", ["p", "n"], result.ports, result.params, expr_text, notes)
    assert va.count("// atan2(a,b) is translated") == 1


def run():
    test_worked_example()
    test_feedback_port_not_duplicated()
    test_current_type_uses_I_contribution()
    test_notes_rendered_as_comments_and_deduped()


if __name__ == "__main__":
    run()
    print("test_emit: OK")
