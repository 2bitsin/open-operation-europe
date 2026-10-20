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
  test_save_touches_one_slot  saving into a slot of a loaded game's file
                           changes that slot's record and game copy and
                           nothing else (France into slot 2, North Africa
                           into slot 3).
  test_deterministic       a second play of the same script gives the same
                           everything (a demo, a save, a command-dialog run,
                           a scenario played to its end).

Without the game's data (a clone without it, unless OPEU_DATA_DIR names the
26 files, which `opeu` then finds itself) the suite skips; a clone without
the golden saved games skips the runs and checks that use them.

The runs are played play.default_workers() at a time (min(cores, 4), or
OPEU_TEST_WORKERS).

On a failure the run's outputs (input.txt, shots/, final.png, saves/, the
WAV and OPL log) stay in pytest's tmp directory, named in the message, and
the expected saved game is written beside the actual one.
"""
from __future__ import annotations

import shutil
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

import pytest

import play

SAVEDATA_HEADER = 240  # 20 slot records of 12 bytes, then the slots' game copies
SLOT = 0x277d  # one slot's game copy
SLOTS = 20
# Played a second time for test_deterministic: a long demo, a save, a run through the command dialogs and a scenario
# played to its end.
TWICE = ['demo-africa', 'save-new', 'air-special-forces', 'end-france']
# The runs that load a golden saved game and save into one slot (numbered from 0): the run, the slot.
ONE_SLOT_SAVES = {'load-play-save': 1, 'load-africa': 2}


def opeu() -> str:
  path = shutil.which('opeu')
  assert path, 'opeu is not on PATH (buildutil puts <build>/bin there)'
  return str(Path(path).resolve())


@pytest.fixture(scope='session')
def played(tmp_path_factory: pytest.TempPathFactory) -> dict[tuple[str, int], tuple[dict, Path, float]]:
  """{(name, pass): (result, output dir, peak RSS in MB)} for every run, played in parallel, the longest first."""
  play.game_data.require_data()  # without the game's data, every test of the suite skips here
  root = tmp_path_factory.mktemp('play')
  jobs = sorted([(name, 1) for name in play.RUNS if play.playable(name)] + [(name, 2) for name in TWICE if play.playable(name)],
                key=lambda job: -play.RUNS[job[0]].ms)
  binary = opeu()

  def one(job: tuple[str, int]) -> tuple[tuple[str, int], tuple[dict, Path, float]]:
    name, round_ = job
    out = root / (name if round_ == 1 else f'{name}-again')
    measured = play.play_measured(binary, name, out)
    return job, (measured.result, out, measured.peak_rss_mb)

  with ThreadPoolExecutor(max_workers=play.default_workers()) as pool:
    return dict(pool.map(one, jobs))


def test_goldens_cover_every_run() -> None:
  assert sorted(play.load_goldens()) == sorted(play.RUNS)


def golden_saved_game(name: str) -> bytes:
  """A golden saved game, or the test skips in a clone without it."""
  play.game_data.require_file(play.saved_game_path(name), f'the golden saved game {play.saved_game_path(name).name}')
  return play.saved_game(name)


def result(played: dict, name: str, round_: int = 1) -> tuple[dict, Path, float]:
  """A run's result; the test skips when the clone lacks the saved game the run loads."""
  if not play.playable(name):
    golden_saved_game(play.RUNS[name].saves_in)
  return played[(name, round_)]


def saved_game_problem(name: str, out: Path) -> list[str]:
  """The first byte where a run's SAVEDATA differs from its golden saved game, with the expected copy written beside it."""
  want = golden_saved_game(name)
  path = out / 'saves' / 'SAVEDATA'
  got = path.read_bytes() if path.exists() else b''
  if got == want:
    return []
  (out / 'expected-SAVEDATA').write_bytes(want)
  first = next((i for i, (a, b) in enumerate(zip(got, want)) if a != b), min(len(got), len(want)))
  slot = (first - SAVEDATA_HEADER) // SLOT + 1 if first >= SAVEDATA_HEADER else 'header'
  return [f'SAVEDATA differs from the golden saved game from byte {first:#x} (slot {slot}); expected copy in '
          f'{out / "expected-SAVEDATA"}']


@pytest.mark.parametrize('name', list(play.RUNS))
def test_run(name: str, played: dict) -> None:
  actual, out, _ = result(played, name)
  problems = play.differences(play.load_goldens()[name], actual)
  assert not problems, f'{name}: behaviour changed; outputs in {out}\n  ' + '\n  '.join(problems)
  if name in play.SAVED_GAMES:
    problems += saved_game_problem(name, out)
  assert not problems, f'{name}: behaviour changed; outputs in {out}\n  ' + '\n  '.join(problems)


def test_load_save_round_trip(played: dict) -> None:
  _, out, _ = result(played, 'load-save')
  loaded = golden_saved_game('save-new')
  saved = (out / 'saves' / 'SAVEDATA').read_bytes()
  assert saved == loaded, f'loading and saving at once changed the saved game; outputs in {out}'


def changed_slots(before: bytes, after: bytes) -> list[int]:
  """The slots (from 0) whose record or game copy differ between two SAVEDATA files."""
  def parts(data: bytes, i: int) -> tuple[bytes, bytes]:
    copy = SAVEDATA_HEADER + i * SLOT
    return data[i * 12:i * 12 + 12], data[copy:copy + SLOT]
  return [i for i in range(SLOTS) if parts(before, i) != parts(after, i)]


@pytest.mark.parametrize('name', list(ONE_SLOT_SAVES))
def test_save_touches_one_slot(name: str, played: dict) -> None:
  _, out, _ = result(played, name)
  before = golden_saved_game(play.RUNS[name].saves_in)
  after = (out / 'saves' / 'SAVEDATA').read_bytes()
  assert len(after) == len(before) == SAVEDATA_HEADER + SLOTS * SLOT
  changed = changed_slots(before, after)
  assert changed == [ONE_SLOT_SAVES[name]], f'slots changed: {[i + 1 for i in changed]}; outputs in {out}'


def test_slot_record_names_the_game(played: dict) -> None:
  """Slot 2's record after load-play-save: scenario, delivery, year, month, day, hour, ...: France, 5/10 09:00."""
  _, out, _ = result(played, 'load-play-save')
  after = (out / 'saves' / 'SAVEDATA').read_bytes()
  assert after[12] == 0 and after[15:18] == bytes([5, 10, 9]), after[12:24].hex()


@pytest.mark.parametrize('name', TWICE)
def test_deterministic(name: str, played: dict) -> None:
  (first, out1, _), (second, out2, _) = result(played, name), result(played, name, 2)
  problems = play.differences(first, second)
  assert not problems, f'{name}: two plays of one script differ; outputs in {out1} and {out2}\n  ' + '\n  '.join(
    problems)

