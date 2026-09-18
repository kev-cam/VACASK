#!/usr/bin/env python3
"""Run a netlist test whose postprocessing may ask to be skipped.

usage: run_skippable.py <simulator> <netlist>

The simulator is run as `vacask -dp <netlist>` with its output passed through.
runtest.skipUnlessOsdi() prints a line starting with "SKIP:" and exits with
runtest.SKIP_EXIT_CODE when the compiled model cannot provide what the test
needs. The simulator reports that as a failed postprocessing step with its own
exit status, so the code has to be recovered from the output here. ctest maps
the exit code to "skipped" through SKIP_RETURN_CODE in CMakeLists.txt.
"""
import subprocess
import sys

SKIP_EXIT_CODE = 77

proc = subprocess.run([sys.argv[1], '-dp', sys.argv[2]], capture_output=True, text=True)
sys.stdout.write(proc.stdout)
sys.stderr.write(proc.stderr)
if proc.returncode != 0 and any(line.startswith('SKIP:') for line in proc.stdout.splitlines()):
    sys.exit(SKIP_EXIT_CODE)
sys.exit(proc.returncode)
