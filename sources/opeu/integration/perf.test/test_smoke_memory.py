#!/usr/bin/env python3
"""The memory gate of `./buildutil test`: the three smokes (perf.SMOKES: smoke-input, smoke-turn, smoke-intro; 15, 120
and 70 s of game time, seconds of wall time each) may not peak above the memory budget the baseline records for them
(perf.py: the measured peak x 1.5, at least 150 MB). Each smoke is played once; one over its budget is played
RUN_RETRY_SAMPLES more times and judged on the median of all its plays. `perf.py memory <build dir> --measure` records
the budgets (docs/testing.md, "The memory gate").

  test_smoke_memory[<smoke>]          the smoke's peak RSS is within its budget
  test_every_smoke_has_a_budget       the baseline budgets every smoke
"""
import statistics

import pytest

import perf


@pytest.fixture(scope='session')
def standard():
    return perf.Standard.here(perf.tested_build_type())


@pytest.fixture(scope='session')
def peaks(standard):
    """{smoke: peak RSS of each play}: one play each, RUN_RETRY_SAMPLES more for a smoke over its budget."""
    opeu = perf.tested_build().opeu
    found = perf.play_smokes(opeu, 1)
    over = [name for name, p in found.items() if name in standard.memory and p[0] > standard.memory_limit(name)]
    for _ in range(perf.RUN_RETRY_SAMPLES if over else 0):
        for name, more in perf.play_smokes(opeu, 1).items():
            if name in over:
                found[name] += more
    return found


def test_every_smoke_has_a_budget(standard):
    missing = [name for name in perf.SMOKES if name not in standard.memory]
    assert not missing, f'no memory budget for {", ".join(missing)}; record them with `perf.py memory <build dir> --measure`'


@pytest.mark.parametrize('name', perf.SMOKES)
def test_smoke_memory(name, peaks, standard):
    assert name in standard.memory, f'{name} has no memory budget; record it with `perf.py memory <build dir> --measure`'
    peak, limit = statistics.median(peaks[name]), standard.memory_limit(name)
    assert peak <= limit, (f'{name} uses too much memory: peak RSS {peak:.1f} MB (median of {len(peaks[name])} plays), '
                           f'budget {limit:.1f} MB (measured {standard.memory[name]["peak_mb"]:.1f} MB)')
