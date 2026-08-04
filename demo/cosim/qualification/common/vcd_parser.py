"""VCD parser for VACASK cosim test suite.

Extracts signal transition times from Value Change Dump files produced
by Icarus Verilog.  Returns transitions as (time_ns, value) tuples.

The $timescale section is parsed to determine the VCD time unit; the
parser does not assume a particular timescale.
"""

import re


_UNIT_TO_NS = {
    'fs': 1e-6,
    'ps': 1e-3,
    'ns': 1.0,
    'us': 1e3,
    'ms': 1e6,
    's':  1e9,
}

_TIMESCALE_RE = re.compile(r'(\d+)\s*(fs|ps|ns|us|ms|s)')


def _parse_timescale(lines):
    """Extract the VCD timescale as a multiplier to nanoseconds.

    Handles two forms Icarus emits:
      $timescale 1ps $end
      $timescale
        1ps
      $end
    """
    buf = ' '.join(lines)
    m = _TIMESCALE_RE.search(buf)
    if not m:
        raise ValueError(
            f'VCD file has no parseable $timescale (saw: {buf!r})')
    magnitude = int(m.group(1))
    unit = m.group(2)
    return magnitude * _UNIT_TO_NS[unit]


def parse_vcd(path, wanted_signals):
    """Parse a VCD file and return transition events for requested signals.

    Args:
        path: filesystem path to the .vcd file
        wanted_signals: list of signal name strings (leaf names, case-sensitive)

    Returns:
        dict mapping signal_name -> list of (time_ns: float, value: str)
        Only signals that appear in both wanted_signals and the VCD are returned.
    """
    var_re = re.compile(
        r'^\$var\s+\w+\s+\d+\s+(\S+)\s+(\S+?)(?:\s+\[\d+:\d+\])?\s+\$end'
    )

    id_to_name = {}
    name_to_id = {}
    transitions = {}
    timescale_to_ns = None
    timescale_lines = []
    in_timescale = False

    with open(path) as f:
        in_defs = True
        current_time_ticks = 0

        for line in f:
            line = line.rstrip()

            if in_defs:
                if in_timescale:
                    timescale_lines.append(line)
                    if '$end' in line:
                        in_timescale = False
                        timescale_to_ns = _parse_timescale(timescale_lines)
                    continue

                if line.startswith('$timescale'):
                    timescale_lines.append(line)
                    if '$end' in line:
                        timescale_to_ns = _parse_timescale(timescale_lines)
                    else:
                        in_timescale = True
                    continue

                m = var_re.match(line)
                if m:
                    vid, vname = m.group(1), m.group(2)
                    if vname in wanted_signals:
                        id_to_name[vid] = vname
                        name_to_id[vname] = vid
                        transitions[vname] = []
                if line.startswith('$enddefinitions'):
                    in_defs = False
                    if timescale_to_ns is None:
                        raise ValueError(
                            'VCD file has no $timescale section')
                continue

            if line.startswith('#'):
                current_time_ticks = int(line[1:])
                continue

            if not line:
                continue

            if line[0] in ('0', '1', 'x', 'X', 'z', 'Z'):
                val = line[0]
                vid = line[1:]
                if vid in id_to_name:
                    name = id_to_name[vid]
                    transitions[name].append(
                        (current_time_ticks * timescale_to_ns, val))
            elif line[0] in ('b', 'B'):
                parts = line.split()
                if len(parts) == 2:
                    val = parts[0][1:]
                    vid = parts[1]
                    if vid in id_to_name:
                        name = id_to_name[vid]
                        transitions[name].append(
                            (current_time_ticks * timescale_to_ns, val))

    return transitions


def get_posedges(transitions, signal):
    """Return list of times (ns) where signal transitions from 0 to 1."""
    events = transitions.get(signal, [])
    edges = []
    prev = None
    for t_ns, val in events:
        if val == '1' and prev == '0':
            edges.append(t_ns)
        prev = val
    return edges


def get_negedges(transitions, signal):
    """Return list of times (ns) where signal transitions from 1 to 0."""
    events = transitions.get(signal, [])
    edges = []
    prev = None
    for t_ns, val in events:
        if val == '0' and prev == '1':
            edges.append(t_ns)
        prev = val
    return edges


def get_first_value(transitions, signal, target_val):
    """Return the first time (ns) signal reaches target_val, or None."""
    for t_ns, val in transitions.get(signal, []):
        if val == target_val:
            return t_ns
    return None
