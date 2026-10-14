#!/usr/bin/env python3
"""How the memory gates judge a run and how many runs the suites play at once, on scripted numbers: no game runs.

  test_budget_is_half_again_the_peak   a run's budget is its measured peak x MEMORY_FACTOR
  test_budget_has_a_floor              a small run's budget is MEMORY_FLOOR_MB
  test_memory_over_names_the_runs_over_budget  the median peak of a run's plays decides, not one play
  test_workers_are_capped              the suites play min(cores, MAX_WORKERS) runs at once
  test_workers_follow_the_variable     OPEU_TEST_WORKERS overrides the cap
  test_rss_units                       ru_maxrss is bytes on macOS, kilobytes on Linux
  test_children_report_their_own_peaks two children of different sizes, measured one after the other by a parent
                                       holding more memory than either, report their own peaks (on Linux a child's
                                       ru_maxrss inherits the parent's at fork; play.waited must not)
"""
import subprocess
import sys

import pytest

import perf

play = perf.play


def standard(memory: dict) -> perf.Standard:
    return perf.Standard({}, 0.1, 'a scripted baseline', False, memory)


def plays(*peaks: float) -> list[perf.Sample]:
    return [perf.Sample(1.0, 1.0, peak) for peak in peaks]


def test_budget_is_half_again_the_peak():
    assert perf.memory_budget(200.0) == 300.0


def test_budget_has_a_floor():
    assert perf.memory_budget(40.0) == perf.MEMORY_FLOOR_MB


def test_memory_over_names_the_runs_over_budget():
    budgets = {'leaks': {'peak_mb': 100, 'budget_mb': 150}, 'spikes': {'peak_mb': 100, 'budget_mb': 150},
               'steady': {'peak_mb': 100, 'budget_mb': 150}}
    session = perf.Session({'leaks': plays(400, 410, 120), 'spikes': plays(120, 900, 110),
                            'steady': plays(100, 101, 99), 'unbudgeted': plays(5000)})
    assert standard(budgets).memory_over(session) == ['leaks']


def test_workers_are_capped(monkeypatch):
    monkeypatch.delenv(play.WORKERS_VARIABLE, raising=False)
    monkeypatch.setattr(play.os, 'cpu_count', lambda: 16)
    assert play.default_workers() == play.MAX_WORKERS
    monkeypatch.setattr(play.os, 'cpu_count', lambda: 2)
    assert play.default_workers() == 2


def test_workers_follow_the_variable(monkeypatch):
    monkeypatch.setenv(play.WORKERS_VARIABLE, '9')
    assert play.default_workers() == 9


@pytest.mark.parametrize('platform, maxrss, mb', [('darwin', 3 * 2**20, 3.0), ('linux', 3 * 2**10, 3.0)])
def test_rss_units(monkeypatch, platform, maxrss, mb):
    monkeypatch.setattr(sys, 'platform', platform)
    assert play.rss_mb(maxrss) == mb


BALLAST_MB = 300
CHILD = 'import time; block = b"x" * ({mb} * 2**20); time.sleep(0.3)'


def child_peak(mb: int) -> float:
    return play.waited(subprocess.Popen([sys.executable, '-c', CHILD.format(mb=mb)]))[1]


def test_children_report_their_own_peaks():
    ballast = b'x' * (BALLAST_MB * 2**20)
    small, large = child_peak(0), child_peak(150)
    assert len(ballast) and small < BALLAST_MB / 3, f'a bare Python child peaked at {small:.1f} MB: the parent leaked in'
    assert large - small > 120, f'a child holding 150 MB more peaked at {large:.1f} MB against {small:.1f} MB'
