#!/usr/bin/env python3
"""The functional suite: scripted headless play compared with goldens.

play.py holds the scripts, the runner and the golden I/O (its docstring
lists the runs). Every run is played once per session, all of them at once
on a thread pool (each is its own `opeu` process), and the tests read the
results: the module's python is one ctest entry under `./buildutil test`'s
60 s budget, and a run takes well under a second in Release and a few
seconds in Debug. Here:

  test_run[<name>]         every run against ../fixtures/play/goldens.json
                           (exit code, frames, WAV, OPL log, saved files);
                           the runs that save also compare SAVEDATA byte for
                           byte with the golden saved game.
  test_load_save_round_trip  load a saved game and save it into its slot at
                           once: the file is unchanged.
  test_save_touches_one_slot  saving into slot 2 changes slot 2's record and
                           game copy and nothing else.
  test_deterministic       a second play of the same script gives the same
                           everything.

On a failure the run's outputs (input.txt, shots/, final.png, saves/, the
WAV and OPL log) stay in pytest's tmp directory, named in the message, and
the expected saved game is written beside the actual one.
"""
import os
import shutil
from concurrent.futures import ThreadPoolExecutor

import pytest

import play

SLOT = 0x277d
# Played a second time for test_deterministic: a long demo and a save.
TWICE = ['demo-africa', 'save-new']


def opeu():
    path = shutil.which('opeu')
    assert path, 'opeu is not on PATH (buildutil puts <build>/bin there)'
    return path


@pytest.fixture(scope='session')
def played(tmp_path_factory):
    """{(name, pass): (result, output dir)} for every run, played in parallel."""
    root = tmp_path_factory.mktemp('play')
    jobs = [(name, 1) for name in play.RUNS] + [(name, 2) for name in TWICE]
    binary = opeu()

    def one(job):
        name, round_ = job
        out = root / (name if round_ == 1 else f'{name}-again')
        return job, (play.run(binary, name, out), out)

    with ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as pool:
        return dict(pool.map(one, jobs))


def test_goldens_cover_every_run():
    assert sorted(play.load_goldens()) == sorted(play.RUNS)


@pytest.mark.parametrize('name', list(play.RUNS))
def test_run(name, played):
    actual, out = played[(name, 1)]
    expected = play.load_goldens()[name]
    problems = play.differences(expected, actual)
    if name in play.SAVED_GAMES:
        want = play.saved_game(play.SAVED_GAMES[name])
        got = (out / 'saves' / 'SAVEDATA').read_bytes() if (out / 'saves' / 'SAVEDATA').exists() else b''
        if got != want:
            (out / 'expected-SAVEDATA').write_bytes(want)
            first = next((i for i, (a, b) in enumerate(zip(got, want)) if a != b), min(len(got), len(want)))
            problems.append(f'SAVEDATA differs from the golden saved game from byte {first:#x} '
                            f'(slot {(first - 240) // SLOT + 1 if first >= 240 else "header"}); '
                            f'expected copy in {out / "expected-SAVEDATA"}')
    assert not problems, f'{name}: behaviour changed; outputs in {out}\n  ' + '\n  '.join(problems)


def test_load_save_round_trip(played):
    _, out = played[('load-save', 1)]
    loaded = play.saved_game('save-new')
    saved = (out / 'saves' / 'SAVEDATA').read_bytes()
    assert saved == loaded, f'loading and saving at once changed the saved game; outputs in {out}'


def test_save_touches_one_slot(played):
    _, out = played[('load-play-save', 1)]
    before = play.saved_game('save-new')
    after = (out / 'saves' / 'SAVEDATA').read_bytes()
    assert len(after) == len(before) == 240 + 20 * SLOT
    changed = [i for i in range(20)
               if before[i * 12:i * 12 + 12] != after[i * 12:i * 12 + 12]
               or before[240 + i * SLOT:240 + (i + 1) * SLOT] != after[240 + i * SLOT:240 + (i + 1) * SLOT]]
    assert changed == [1], f'slots changed: {[i + 1 for i in changed]}; outputs in {out}'
    # slot record: scenario, delivery, year, month, day, hour, ...: France, 5/10 09:00
    assert after[12] == 0 and after[15:18] == bytes([5, 10, 9]), after[12:24].hex()


@pytest.mark.parametrize('name', TWICE)
def test_deterministic(name, played):
    (first, out1), (second, out2) = played[(name, 1)], played[(name, 2)]
    problems = play.differences(first, second)
    assert not problems, f'{name}: two plays of one script differ; outputs in {out1} and {out2}\n  ' + '\n  '.join(
      problems)
