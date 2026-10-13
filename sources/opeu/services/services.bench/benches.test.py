#!/usr/bin/env python3
"""The services benchmarks' perf gate: each benchmark of opeu-services-benches may not get slower than its baseline by more than
the budget. The measuring and judging is sources/opeu/integration/perf.test/perf.py's (the fastest repetition,
re-measured while over); the numbers are in sources/opeu/integration/fixtures/perf/baseline.json.

  test_bench_cost[<bench>]  the benchmark's fastest repetition is within budget of the baseline's fastest (scaled by
                            the machine's momentary slowdown on the Calibration benchmark)
  test_baseline_covers_every_bench  the baseline names exactly the benchmarks opeu-services-benches runs
"""
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'integration' / 'perf.test'))
import perf  # noqa: E402

TARGET = 'opeu-services-benches'


GATED = [name for name in perf.baselined_names(perf.tested_build_type(), 'benches')
         if name.startswith(TARGET + '/') and not name.endswith('/' + perf.CALIBRATION)]


@pytest.fixture(scope='session')
def standard():
    return perf.Standard.here(perf.tested_build().build_type)


@pytest.fixture(scope='session')
def fastest(standard):
    executable = perf.tested_build().benches([TARGET])[0]
    return perf.fastest_benches(executable, standard)


def test_baseline_covers_every_bench(standard, fastest):
    assert standard.baselined_benches(TARGET) == sorted(fastest), \
        f'the baseline does not name exactly the benchmarks of {TARGET}; re-baseline (docs/building.md)'


@pytest.mark.parametrize('name', GATED)
def test_bench_cost(name, standard, fastest):
    assert name in standard.numbers['benches'], f'{name} has no baseline on {standard.source}; re-baseline'
    assert name in fastest, f'{name} is in the baseline but no benchmark of that name ran'
    cost, limit = fastest[name], standard.bench_limit(name, fastest)
    slowdown = standard.machine_slowdown(name, fastest)
    assert cost <= limit, (f'{name} got slower: {cost:.0f} ns per iteration at best, limit {limit:.0f} ns '
                           f'({standard.budget:.0%} over {standard.source}, the machine {slowdown:.2f} times slower)')
