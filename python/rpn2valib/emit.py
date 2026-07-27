def emit_module(name, kind, main_ports, extra_ports, params, expr_text, notes=None):
    """
    Wraps *expr_text* (Verilog-A expression text from codegen.generate) in a
    full .va module: the standard includes, port/discipline declarations,
    one parameter per entry in *params*, and a single contribution
    statement - matching the style of every existing file under devices/
    (e.g. devices/resistor.va, devices/opamp.va).

    *kind* is "V" or "I". *main_ports* are the instance's own terminals
    (e.g. ["p", "n"]); *extra_ports* are the node/branch names discovered
    via v()/i() in the formula - appended after *main_ports*, skipping any
    already present (so referencing your own output port in the formula,
    e.g. for feedback, doesn't declare a duplicate port). *params* is a
    dict of {name: default_value}. *notes* are human-readable strings about
    any non-obvious substitution codegen made (e.g. atan2); emitted as
    comments above the module so they're visible to whoever reads the file.
    """
    all_ports = list(main_ports)
    for port in extra_ports:
        if port not in all_ports:
            all_ports.append(port)

    ports_text = ", ".join(all_ports)

    lines = []
    lines.append('`include "constants.vams"')
    lines.append('`include "disciplines.vams"')
    lines.append("")

    if notes:
        seen = set()
        for note in notes:
            if note in seen:
                continue
            seen.add(note)
            lines.append("// "+note)
        lines.append("")

    lines.append("module "+name+"("+ports_text+");")
    lines.append("\tinout "+ports_text+";")
    lines.append("\telectrical "+ports_text+";")

    if params:
        lines.append("")
        for pname, pvalue in params.items():
            lines.append(
                '\t(*desc="Generated parameter", type="instance", units="1"*)'
                ' parameter real '+pname+' = '+repr(pvalue)+';')

    lines.append("")
    lines.append("\tanalog begin")
    main_text = ", ".join(main_ports)
    lines.append("\t\t"+kind+"("+main_text+") <+ "+expr_text+";")
    lines.append("\tend")
    lines.append("endmodule")
    lines.append("")

    return "\n".join(lines)
