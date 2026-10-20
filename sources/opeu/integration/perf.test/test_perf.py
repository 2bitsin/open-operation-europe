#!/usr/bin/env python3
"""The perf gate of the functional suite: its runs may not get slower than the baseline by more than the budget
(perf.py says how a run is measured and judged; ../fixtures/perf/baseline.json holds the numbers). The benchmarks
have their own gates, one per module (sources/opeu/<module>/<module>.bench/benches.test.py), each its own ctest entry.

  test_run_cost[<run>]     the run's CPU time (median of its plays) is within budget of its baseline, scaled by the
                           session's slowdown on the calibration processes played beside it
  test_suite_cost          the total of all baselined runs is within budget of the baseline total
  test_baseline_covers_every_run  the baseline names exactly the runs the gate times, so a new run cannot go
                           unmeasured
  test_run_memory[<run>]   every run with a memory budget: its peak RSS (median of the session's plays, no extra
                           play) is within the budget (its measured peak x 1.5, at least 150 MB); the smokes have their
                           own gate with re-attempts, test_smoke_memory.py

Without the game's data the tests that play skip (tools/testing/game_data.py); a clone without the golden saved games
skips the runs that load them.

A Debug build times perf.DEBUG_RUNS (the runs short enough there) against the Debug baseline.

OPEU_PERF_BUDGET=<percent> sets the budget (default 10 on a machine the baseline lists, 50 elsewhere, where the
closest listed machine's numbers apply). Collection reads only the baseline file and `opeu` on PATH, so it never fails
on an unlisted machine; a missing baseline fails the tests that need it. A failure names the run, its time, the limit
and the baseline; docs/building.md says how to profile it and how to re-baseline after a deliberate change.
"""
import statistics

import pytest

import perf

BUILD_TYPE = perf.tested_build_type()
GATED_RUNS = perf.gated_runs(BUILD_TYPE)


@pytest.fixture(scope='session')
def build():
    perf.play.game_data.require_data()  # without the game's data, every test that plays skips here
    return perf.tested_build()


def playable(name):
    """The run's name; the test skips when this clone lacks the golden saved game the run loads."""
    if not perf.play.playable(name):
        saved = perf.play.saved_game_path(perf.play.RUNS[name].saves_in)
        perf.play.game_data.require_file(saved, f'the golden saved game {saved.name}')
    return name


@pytest.fixture(scope='session')
def standard(build):
    return perf.Standard.here(build.build_type)


@pytest.fixture(scope='session')
def session(build, standard):
    """RUN_SAMPLES plays of every gated run with their calibration processes; the runs over their CPU limit are played RUN_RETRY_SAMPLES more times (with calibrations again) and judged on all their plays."""
    found = perf.sample_runs(build.opeu, [name for name in GATED_RUNS if perf.play.playable(name)], perf.RUN_SAMPLES)
    slowdown = standard.run_slowdown(found)
    over = [name for name, samples in found.runs.items()
            if name in standard.numbers['runs'] and median_cpu(samples) > standard.run_limit(name, slowdown)]
    if over:
        found.extend(perf.sample_runs(build.opeu, over, perf.RUN_RETRY_SAMPLES))
    return found


def median_cpu(samples):
    return statistics.median(s.cpu for s in samples)


def judged(standard, session):
    """The slowdown the limits use and its description for the failure messages."""
    slowdown = standard.run_slowdown(session)
    return slowdown, (f'{standard.budget:.0%} over {standard.source}, the machine {slowdown:.2f} times as slow on '
                      f'{len(session.calibration)} calibrations')


def test_baseline_covers_every_run(standard):
    assert sorted(standard.numbers['runs']) == sorted(GATED_RUNS), \
        f'the {BUILD_TYPE} baseline does not name exactly the runs the gate times; re-baseline (docs/building.md)'


@pytest.mark.parametrize('name', GATED_RUNS)
def test_run_cost(name, session, standard):
    playable(name)
    assert name in standard.numbers['runs'], f'{name} has no baseline yet; re-baseline (docs/building.md)'
    slowdown, why = judged(standard, session)
    cost, limit = median_cpu(session.runs[name]), standard.run_limit(name, slowdown)
    assert cost <= limit, f'{name} got slower: {cost:.3f} s of CPU, limit {limit:.3f} s ({why}, plus {perf.RUN_SLACK_S} s)'


def test_suite_cost(session, standard):
    cost = sum(median_cpu(samples) for name, samples in session.runs.items() if name in standard.numbers['runs'])
    slowdown, why = judged(standard, session)
    limit = standard.suite_limit(slowdown)
    assert cost <= limit, f'the functional suite got slower: {cost:.3f} s of CPU in all, limit {limit:.3f} s ({why})'



@pytest.mark.parametrize('name', [name for name in perf.memory_budgeted_names() if name in GATED_RUNS])
def test_run_memory(name, session, standard):
    playable(name)
    peak, limit = perf.median_rss(session.runs[name]), standard.memory_limit(name)
    assert peak <= limit, (f'{name} uses too much memory: peak RSS {peak:.1f} MB (median of {len(session.runs[name])} '
                           f'plays), budget {limit:.1f} MB (measured {standard.memory[name]["peak_mb"]:.1f} MB)')
