#!/usr/bin/python3

# RPN expression to Verilog-A converter.
# Use as module, i.e. python3 -m rpn2va ...
# or edit hashbang line and make it executable.

import sys

from rpn2valib import codegen, emit, semantics
from rpn2valib.exc import RpnToVaError
from rpn2valib.lexer import parse_number, tokenize
from rpn2valib.parser import parse

if __name__ == "__main__":
    help = """RPN expression to Verilog-A converter.
Usage: python3 rpn2va.py [<args>] -o <output file>

Writes a Verilog-A module implementing a SPICE-style behavioral source: an
instance that forces a voltage or current equal to a formula written in
VACASK's own expression syntax, with references to live node voltages and
branch currents via v(...)/i(...).

Arguments:
  -h --help           print help
  --name <name>       name of the generated Verilog-A module
  --type <V|I>        does the source force a voltage or a current
  --port <p,n>        the instance's own two terminals, comma separated
  --expr <expr>       the formula, in VACASK's expression syntax
  --param <name=val>  default value for a parameter used in <expr>.
                      May be repeated for multiple parameters.
  -o --output <file>  where to write the generated .va file. If omitted,
                      the file is printed to standard output.
"""
    ndx = 1
    name = None
    kind = None
    ports = None
    expr = None
    params = {}
    outfile = None

    while ndx < len(sys.argv):
        arg = sys.argv[ndx]
        if arg == "-h" or arg == "--help":
            print(help)
            sys.exit(0)
        elif arg == "--name":
            ndx += 1
            if ndx >= len(sys.argv):
                print("Too few arguments.")
                sys.exit(1)
            name = sys.argv[ndx]
        elif arg == "--type":
            ndx += 1
            if ndx >= len(sys.argv):
                print("Too few arguments.")
                sys.exit(1)
            kind = sys.argv[ndx]
        elif arg == "--port":
            ndx += 1
            if ndx >= len(sys.argv):
                print("Too few arguments.")
                sys.exit(1)
            ports = [p.strip() for p in sys.argv[ndx].split(",")]
        elif arg == "--expr":
            ndx += 1
            if ndx >= len(sys.argv):
                print("Too few arguments.")
                sys.exit(1)
            expr = sys.argv[ndx]
        elif arg == "--param":
            ndx += 1
            if ndx >= len(sys.argv):
                print("Too few arguments.")
                sys.exit(1)
            spec = sys.argv[ndx]
            if "=" not in spec:
                print("Malformed --param (expected name=value): "+spec)
                sys.exit(1)
            pname, pvalue = spec.split("=", 1)
            try:
                params[pname] = parse_number(pvalue)
            except RpnToVaError as e:
                print("error: --param "+spec+": "+str(e))
                sys.exit(1)
        elif arg == "-o" or arg == "--output":
            ndx += 1
            if ndx >= len(sys.argv):
                print("Too few arguments.")
                sys.exit(1)
            outfile = sys.argv[ndx]
        else:
            print("Unknown argument:", arg)
            print(help)
            sys.exit(1)
        ndx += 1

    if name is None or kind is None or ports is None or expr is None:
        print("Missing required argument(s): --name, --type, --port and --expr are all required.")
        print(help)
        sys.exit(1)

    if kind not in ("V", "I"):
        print("error: --type must be 'V' or 'I', got: "+kind)
        sys.exit(1)

    if len(ports) != 2 or ports[0] == "" or ports[1] == "":
        print("error: --port must name exactly two terminals, comma separated, e.g. p,n")
        sys.exit(1)

    try:
        tokens = tokenize(expr)
        ast = parse(tokens)
        result = semantics.analyze(ast, params)
        expr_text, notes = codegen.generate(ast)
        va_text = emit.emit_module(
            name, kind, ports, result.ports, result.params, expr_text, notes)
    except RpnToVaError as e:
        print("error: "+str(e))
        sys.exit(1)

    if outfile is None:
        print(va_text)
    else:
        with open(outfile, "w") as f:
            f.write(va_text)
