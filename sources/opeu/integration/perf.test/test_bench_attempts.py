#!/usr/bin/env python3
"""How the bench gates judge a benchmark over its limit (perf.fastest_benches), on scripted measurements: no
benchmark executable runs and no time passes.

  test_burst_passes          a benchmark slowed with its calibration in an attempt passes on that attempt
  test_regression_fails      a benchmark slower in every attempt, its calibration not, stays over after BENCH_ATTEMPTS
  test_quiet_bench_untouched a benchmark within its limit is measured once
"""
from pathlib import Path

import pytest

import perf

EXECUTABLE = Path('opeu-services-benches')
BENCH = 'opeu-services-benches/CellBlockCopy'
CALIBRATION = 'opeu-services-benches/' + perf.CALIBRATION


def standard(foreign):
    numbers = {'benches': {BENCH: {'cpu_min_ns': 20.0}, CALIBRATION: {'cpu_min_ns': 1000.0}}}
    return perf.Standard(numbers, 1.0, 'the scripted baseline', foreign)


def scripted(monkeypatch, measurements):
    """sample_benches answers the scripted measurements in turn ({bench: ns}, each one repetition); returns the list
    of the names each call asked for."""
    asked = []
    pending = list(measurements)

    def sample(executable, repetitions, names=None):
        asked.append(sorted(names) if names else None)
        answer = pending.pop(0)
        return {name: [ns] for name, ns in answer.items() if names is None or name in names}
    monkeypatch.setattr(perf, 'sample_benches', sample)
    monkeypatch.setattr(perf, 'BENCH_RETRY_SECONDS', 0)
    monkeypatch.setattr(perf.time, 'sleep', lambda seconds: None)
    return asked


@pytest.mark.parametrize('foreign', [False, True])
def test_burst_passes(monkeypatch, foreign):
    # The session: calibration at the baseline, the bench 3 times its baseline (limit 40). The first attempt runs
    # both twice as slow: the bench is within the limit its calibration gives then (80).
    scripted(monkeypatch, [{BENCH: 60.0, CALIBRATION: 1000.0}, {BENCH: 70.0, CALIBRATION: 2000.0}])
    s = standard(foreign)
    fastest = perf.fastest_benches(EXECUTABLE, s)
    assert fastest[BENCH] <= s.bench_limit(BENCH, fastest)
    assert fastest[CALIBRATION] == 1000.0


@pytest.mark.parametrize('foreign', [False, True])
def test_regression_fails(monkeypatch, foreign):
    rounds = [{BENCH: 60.0, CALIBRATION: 1000.0}] + [{BENCH: 60.0, CALIBRATION: 1000.0}] * perf.BENCH_ATTEMPTS
    asked = scripted(monkeypatch, rounds)
    s = standard(foreign)
    fastest = perf.fastest_benches(EXECUTABLE, s)
    assert fastest[BENCH] > s.bench_limit(BENCH, fastest)
    assert asked[1:] == [sorted([BENCH, CALIBRATION])] * perf.BENCH_ATTEMPTS


def test_quiet_bench_untouched(monkeypatch):
    asked = scripted(monkeypatch, [{BENCH: 30.0, CALIBRATION: 1000.0}])
    s = standard(False)
    fastest = perf.fastest_benches(EXECUTABLE, s)
    assert fastest[BENCH] == 30.0
    assert asked == [None]
