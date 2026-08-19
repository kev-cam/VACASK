#!/usr/bin/env python3
"""Smoke tests for gen_cosim_top.py.

Pure-Python static-parser/generator, no VACASK/Icarus build needed --
unlike qualification/run_all.py's end-to-end tests, these just exercise
the tool against small synthetic fixtures written to a tempdir. Run
directly:

    python3 test_gen_cosim_top.py

Written after an external review (see CHANGELOG history around commit
ae86665) found several real bugs that this project's own byte-for-byte
validation against a production netlist hadn't exercised: a suffix
table that rejected legal VACASK values, ANSI parameterized bus widths
silently broken, an eval() "sandbox" that wasn't one, and a couple of
minor error-reporting gaps. Each of those gets a dedicated regression
case here so they can't come back unnoticed.
"""
import os
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import gen_cosim_top as g


def write(tmpdir, name, content):
    path = os.path.join(tmpdir, name)
    with open(path, "w") as f:
        f.write(content)
    return path


class SuffixParsing(unittest.TestCase):
    """Mirrors lib/dfllexer.l:580-626 exactly, not generic SPICE
    convention -- VACASK's suffix matching is case-sensitive and
    asymmetric (bare 'M' is always mega; bare 'm' is milli unless
    followed by exact lowercase 'eg'/'il')."""

    def test_matches_vacask_lexer(self):
        cases = {
            "1ms": 1e-3, "600ns": 6e-7, "20us": 2e-5, "10us": 1e-5,
            "600M": 6e8, "1meg": 1e6, "1Meg": 1e6, "1MEG": 1e6,
            "1mEG": 1e-3, "1x": 1e6, "1mil": 25.4e-6, "0.001": 0.001,
            "400e-9": 4e-7, "20ms": 0.02, "5k": 5e3, "5K": 5e3,
        }
        for s, expect in cases.items():
            self.assertAlmostEqual(g._to_seconds(s), expect, delta=abs(expect) * 1e-9 or 1e-30,
                                    msg=f"_to_seconds({s!r})")


class SafeEval(unittest.TestCase):
    def test_arithmetic_still_works(self):
        self.assertEqual(g._eval_int_expr("Bits - 1", {"Bits": 6}), 5)
        self.assertEqual(g._eval_int_expr("2*Bits+1", {"Bits": 3}), 7)
        self.assertEqual(g._eval_int_expr("7", {}), 7)

    def test_object_graph_escape_blocked(self):
        with self.assertRaises(ValueError):
            g._eval_int_expr("().__class__.__bases__[0].__subclasses__()", {})

    def test_unknown_identifier_blocked(self):
        with self.assertRaises(ValueError):
            g._eval_int_expr("os.system('true')", {})


class VerilogPortParsing(unittest.TestCase):
    def test_ansi_parameterized_width(self):
        with tempfile.TemporaryDirectory() as d:
            path = write(d, "foo.v",
                         "module foo #(parameter Bits = 4) "
                         "(input wire [Bits-1:0] data, output wire y);\n"
                         "endmodule\n")
            bits = g.parse_verilog_ports(path, "foo")
            self.assertEqual(bits, [
                ("data[3]", "input"), ("data[2]", "input"),
                ("data[1]", "input"), ("data[0]", "input"),
                ("y", "output"),
            ])

    def test_old_style_parameterized_width(self):
        with tempfile.TemporaryDirectory() as d:
            path = write(d, "adc.v",
                         "module adc(Clk, Result);\n"
                         "parameter Bits=6;\n"
                         "input wire Clk;\n"
                         "output reg [Bits-1:0] Result;\n"
                         "endmodule\n")
            bits = g.parse_verilog_ports(path, "adc")
            names = [b for b, _ in bits]
            self.assertEqual(names, ["Clk"] + [f"Result[{i}]" for i in range(5, -1, -1)])

    def test_portless_module_gives_clean_error(self):
        with tempfile.TemporaryDirectory() as d:
            path = write(d, "portless.v", "module portless;\nendmodule\n")
            with self.assertRaises(ValueError):
                g.parse_verilog_ports(path, "portless")


class NetlistAndCli(unittest.TestCase):
    def test_fuzzy_match_ignores_uninstantiated_subckts(self):
        with tempfile.TemporaryDirectory() as d:
            net = write(d, "t.spectre", "subckt top_dig ( a b )\nends\n")
            write(d, "top_dig.v", "module top_dig(a, b);\ninput a;\noutput b;\nendmodule\n")
            out = os.path.join(d, "out.v")
            with self.assertRaises(SystemExit):
                self._run_main(["--netlist", net, "--modules", "top_dig", "-o", out])

    def test_inout_port_warns_and_binds_d2a(self):
        with tempfile.TemporaryDirectory() as d:
            net = write(d, "t.spectre", "subckt dig ( a b )\nends\nx1 ( n1 n2 ) dig\n")
            write(d, "dig.v", "module dig(a, b);\ninput a;\ninout b;\nendmodule\n")
            out = os.path.join(d, "out.v")
            self._run_main(["--netlist", net, "--modules", "dig", "-o", out])
            with open(out) as f:
                text = f.read()
            self.assertIn('$cosim_d2a("n2"', text)

    def test_cyclic_hierarchy_raises_clean_error(self):
        net_text = (
            "subckt a ( p )\nxb ( p ) b\nends\n"
            "subckt b ( p )\nxa ( p ) a\nends\n"
            "xtop ( n1 ) a\n"
        )
        with tempfile.TemporaryDirectory() as d:
            net = write(d, "t.spectre", net_text)
            write(d, "a.v", "module a(p);\ninput p;\nendmodule\n")
            out = os.path.join(d, "out.v")
            with self.assertRaises(SystemExit):
                self._run_main(["--netlist", net, "--modules", "a", "-o", out])

    def _run_main(self, argv):
        old_argv = sys.argv
        sys.argv = ["gen_cosim_top.py"] + argv
        try:
            g.main()
        finally:
            sys.argv = old_argv


if __name__ == "__main__":
    unittest.main()
