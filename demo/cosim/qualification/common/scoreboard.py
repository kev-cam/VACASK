"""Two-tier scoreboard for VACASK cosim test suite.

Tier 1 — DUT functional correctness (regulation, fault handling, etc.)
Tier 2 — Bridge fidelity (timing error, event attribution, drift)
"""

import json
import sys


class Scoreboard:
    TIER2_ABS_TOL_NS = 1.0
    TIER2_DRIFT_TOL_NS_PER_1K = 0.01

    def __init__(self, test_name):
        self.test_name = test_name
        self.results = []

    def add(self, tier, name, passed, measured=None, expected=None,
            tolerance=None, unit='ns', detail=''):
        self.results.append({
            'tier': tier,
            'name': name,
            'passed': bool(passed),
            'measured': measured,
            'expected': expected,
            'tolerance': tolerance,
            'unit': unit,
            'detail': detail,
        })

    def check_timing(self, name, measured_ns, expected_ns,
                     tolerance_ns=None):
        if tolerance_ns is None:
            tolerance_ns = self.TIER2_ABS_TOL_NS
        error = abs(measured_ns - expected_ns)
        passed = error <= tolerance_ns
        self.add(
            tier=2,
            name=name,
            passed=passed,
            measured=measured_ns,
            expected=expected_ns,
            tolerance=tolerance_ns,
            detail=f'error={error:.4f} ns',
        )
        return passed

    def check_event_count(self, name, measured_count, expected_count):
        passed = measured_count == expected_count
        self.add(
            tier=1,
            name=name,
            passed=passed,
            measured=measured_count,
            expected=expected_count,
            unit='count',
            detail='',
        )
        return passed

    def check_drift(self, name, max_drift_ns, num_cycles):
        tol = self.TIER2_DRIFT_TOL_NS_PER_1K * (num_cycles / 1000.0)
        passed = max_drift_ns <= tol
        self.add(
            tier=2,
            name=name,
            passed=passed,
            measured=max_drift_ns,
            expected=0.0,
            tolerance=tol,
            detail=f'{num_cycles} cycles, max_drift={max_drift_ns:.4f} ns',
        )
        return passed

    def check_bool(self, tier, name, condition, detail=''):
        self.add(tier=tier, name=name, passed=condition, detail=detail)
        return condition

    def to_dict(self):
        tier1 = [r for r in self.results if r['tier'] == 1]
        tier2 = [r for r in self.results if r['tier'] == 2]
        return {
            'test': self.test_name,
            'tier1_pass': all(r['passed'] for r in tier1) if tier1 else True,
            'tier2_pass': all(r['passed'] for r in tier2) if tier2 else True,
            'results': self.results,
        }

    def to_json(self):
        return json.dumps(self.to_dict(), indent=2)

    def report_and_exit(self):
        d = self.to_dict()
        print(self.to_json())

        n_pass = sum(1 for r in self.results if r['passed'])
        n_fail = sum(1 for r in self.results if not r['passed'])
        label = 'PASS' if d['tier1_pass'] and d['tier2_pass'] else 'FAIL'

        print(f'\n=== {self.test_name}: {label} '
              f'({n_pass} passed, {n_fail} failed) ===',
              file=sys.stderr)

        for r in self.results:
            tag = 'OK' if r['passed'] else 'FAIL'
            line = f"  [{tag}] T{r['tier']} {r['name']}"
            if r.get('measured') is not None and r.get('expected') is not None:
                line += (f"  measured={r['measured']:.4f} "
                         f"expected={r['expected']:.4f} "
                         f"{r['unit']}")
            if r.get('detail'):
                line += f"  ({r['detail']})"
            print(line, file=sys.stderr)

        sys.exit(0 if label == 'PASS' else 1)
