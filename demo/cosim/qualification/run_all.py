#!/usr/bin/env python3
"""Run all qualification tests and generate a qualification report.

Usage:
    python3 run_all.py           # build + check all
    python3 run_all.py --check   # check only (assumes cosim already ran)

Generates report.json and prints a human-readable summary.
"""
import json
import os
import subprocess
import sys

TESTS = [
    'A1_ramp_crossing',
    'A2_d2a_propagation',
    'A3_rapid_crossings',
    'A4_multi_rail',
    'A5_drift',
    'A6_breakpoint_prediction',
    'A7_hierarchical',
    'B_simo',
]

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
COSIM_DIR = os.path.abspath(os.path.join(BASE_DIR, '..'))
VACASK_PREFIX = os.environ.get(
    'VACASK_PREFIX', os.path.abspath(os.path.join(COSIM_DIR, '..', 'install')))
IVERILOG = os.path.join(VACASK_PREFIX, 'bin', 'iverilog')
VVP = os.path.join(VACASK_PREFIX, 'bin', 'vvp')


def build_bridge():
    """Build the shared cosim.vpi once, before any test runs."""
    result = subprocess.run(
        ['make', 'cosim.vpi'], cwd=COSIM_DIR,
        capture_output=True, text=True, timeout=600)
    if result.returncode != 0:
        print('  Building cosim.vpi failed:', file=sys.stderr)
        print(result.stderr[-2000:], file=sys.stderr)
        return False
    return True


def run_test(name, check_only=False):
    test_dir = os.path.join(BASE_DIR, name)

    if not check_only:
        print(f'\n{"="*60}', file=sys.stderr)
        print(f'  Building and running {name}', file=sys.stderr)
        print(f'{"="*60}', file=sys.stderr)

        compile_result = subprocess.run(
            [IVERILOG, '-o', 't.vvp', 'cosim_top.v', 'dut.v'],
            cwd=test_dir, capture_output=True, text=True)
        if compile_result.returncode != 0:
            print(f'  iverilog failed for {name}:', file=sys.stderr)
            print(compile_result.stderr, file=sys.stderr)
            return {'test': name, 'error': 'iverilog failed',
                    'tier1_pass': False, 'tier2_pass': False, 'results': []}

        cosim_result = subprocess.run(
            [VVP, f'-M{COSIM_DIR}', '-mcosim', 't.vvp'],
            cwd=test_dir, capture_output=True, text=True, timeout=600)
        if cosim_result.returncode != 0:
            print(f'  cosim failed for {name}:', file=sys.stderr)
            print(cosim_result.stderr[-2000:], file=sys.stderr)
            return {'test': name, 'error': 'cosim failed',
                    'tier1_pass': False, 'tier2_pass': False, 'results': []}

    print(f'\n  Checking {name}...', file=sys.stderr)
    check_result = subprocess.run(
        ['python3', 'check.py'], cwd=test_dir,
        capture_output=True, text=True)

    try:
        result = json.loads(check_result.stdout)
    except json.JSONDecodeError:
        print(f'  check.py output not valid JSON for {name}:', file=sys.stderr)
        print(check_result.stdout[:500], file=sys.stderr)
        print(check_result.stderr[:500], file=sys.stderr)
        return {'test': name, 'error': 'check.py parse error',
                'tier1_pass': False, 'tier2_pass': False, 'results': []}

    return result


def generate_report(results):
    tier1_all = all(r.get('tier1_pass', False) for r in results)
    tier2_all = all(r.get('tier2_pass', False) for r in results)

    report = {
        'suite': 'qualification',
        'overall_tier1': tier1_all,
        'overall_tier2': tier2_all,
        'tests': results,
    }

    report_path = os.path.join(BASE_DIR, 'report.json')
    with open(report_path, 'w') as f:
        json.dump(report, f, indent=2)

    print(f'\n{"="*60}', file=sys.stderr)
    print(f'  QUALIFICATION REPORT', file=sys.stderr)
    print(f'{"="*60}', file=sys.stderr)
    print(f'  Tier 1 (functional):     '
          f'{"PASS" if tier1_all else "FAIL"}', file=sys.stderr)
    print(f'  Tier 2 (bridge fidelity): '
          f'{"PASS" if tier2_all else "FAIL"}', file=sys.stderr)
    print(f'', file=sys.stderr)

    for r in results:
        tag = 'PASS' if r.get('tier1_pass') and r.get('tier2_pass') else 'FAIL'
        if r.get('error'):
            tag = 'ERROR'
        print(f'  [{tag}] {r["test"]}', file=sys.stderr)
        if r.get('error'):
            print(f'         {r["error"]}', file=sys.stderr)
        for check in r.get('results', []):
            ctag = 'OK' if check['passed'] else 'FAIL'
            line = f'         [{ctag}] T{check["tier"]} {check["name"]}'
            if check.get('detail'):
                line += f'  ({check["detail"]})'
            print(line, file=sys.stderr)

    print(f'\n  Report written to {report_path}', file=sys.stderr)
    return report


def main():
    check_only = '--check' in sys.argv

    if not check_only and not build_bridge():
        sys.exit(1)

    results = []
    for name in TESTS:
        result = run_test(name, check_only=check_only)
        results.append(result)

    report = generate_report(results)

    print(json.dumps(report, indent=2))

    if not report['overall_tier1'] or not report['overall_tier2']:
        sys.exit(1)


if __name__ == '__main__':
    main()
