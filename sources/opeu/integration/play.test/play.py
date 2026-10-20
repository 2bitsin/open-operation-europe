#!/usr/bin/env python3
"""Scripted headless play of MAIN and its golden outputs (the functional suite).

Every run is `opeu --headless` with a generated `--input` script. Headless
time is virtual (1 ms per input poll), so a script gives the same frames,
sound and saved games on every run and every machine. The goldens in
`../fixtures/play/` record the game's behaviour (the three smokes' frames,
WAV and OPL log equal the v1.0 C build's); a deliberate behaviour change
regenerates them (`regen` below) in the commit that makes it.

The runs (RUNS below):

  demo-<scenario>   each of the six scenarios in Demo Mode (the computer
                    commands both sides), battles not viewed, for an hour of
                    virtual time: set-up, briefing, then about four game days
                    of turn processing (orders, moves, combat, supply,
                    reinforcements, air and special forces, events, the AI of
                    both sides). France reaches its end within the hour
                    (de Gaulle defeated) and the program exits by itself.
  demo-france-battles  Demo Mode with every battle viewed: the tactical
                    battle screen and its turns.
  soak-kursk-6      Kursk in Demo Mode with battles viewed, six minutes: the
                    turn report naming Belenikhino at 330 s (the formatted
                    report line, whole and in the side's colour).
  smoke-input, smoke-turn, smoke-intro  the three headless smokes (menus by
                    mouse; Enter through set-up into turn processing; the
                    opening, OPEN.EXE), with their WAV and OPL register log.
  save-new          One Player as the Allies in France: staff chosen, seven
                    one-hour turns passed (nine Pass cycles), then Game > Save / Quit > Save
                    Game, which creates SAVEDATA (save-new.SAVEDATA.gz).
  load-play-save    Load Game of that file, three more one-hour turns, then
                    save into slot 2 (load-play-save.SAVEDATA.gz).
  load-save         Load Game and save at once into the same slot: the file
                    must come out byte-identical to the one loaded.
  save-africa       One Player as the Allies in North Africa: two turns, the
                    first save (slot 1), a turn, slot 2, a turn
                    (save-africa.SAVEDATA.gz).
  load-africa       Load Game of save-africa's slot 2, two turns, save into
                    slot 3 (load-africa.SAVEDATA.gz).
  orders, officers-advice, organize-request, air-special-forces,
  settings-quit     One Player games in France driven through the command
                    bar's dialogs (Tact, City, Tsfr; Ofcr, Info; Orgz, Rqst;
                    Air and SpFc as the Axis; the Game menu's settings and
                    Quit), each ending with turns that carry the orders out.
                    Chosen with the coverage build (tools/coverage/,
                    docs/coverage.md) for the code they reach.
  campaign-bulge-allies, campaign-kursk-axis  One Player outside France, one
                    run per side: three turns, Tact (a division's move, a
                    regiment's Occupy order on the enemy army HQ), City, four
                    turns.
  menus-mouse       save-new's game loaded by mouse; every command clicked
                    open and right-clicked shut, every Game menu entry too.
  end-france        One Player as the Allies in France, battles not viewed,
                    a day passed per turn until the scenario ends: the
                    defeat screen, the KOEI logo, and the program exits by
                    itself after 47.5 minutes.

Each run records the exit code, the SHA-1 of every `--shots` frame and of
the final `--png` frame, of the WAV and OPL log where recorded, and of every
file the game left in its save directory. `play_measured` also returns the
run's peak resident memory, which the memory gates judge: the `opeu`
process's own `ru_maxrss` from `os.wait4` on macOS, its VmHWM polled from
/proc on Linux (where a child's ru_maxrss inherits the forking parent's).

The suites play the runs `default_workers()` at a time: min(cores, 4), or
OPEU_TEST_WORKERS when it is set. A game that leaks takes the machine down
with it once every core plays one; four bound the damage.

    python3 play.py regen <opeu> <dir>   rewrite ../fixtures/play/ from <opeu>, outputs in <dir>
    python3 play.py run <opeu> <name> <dir>   one run, outputs kept in <dir>
"""
from __future__ import annotations

import gzip
import hashlib
import json
import os
import shutil
import subprocess
import sys
import threading
from collections.abc import Callable
from dataclasses import dataclass, field
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parents[3] / 'tools' / 'testing'))
import game_data  # noqa: E402

FIXTURES = HERE.parent / 'fixtures'
GOLDEN_DIR = FIXTURES / 'play'
GOLDENS = GOLDEN_DIR / 'goldens.json'

SCENARIOS = ['france', 'africa', 'kursk', 'normandy', 'bulge', 'berlin']
MINUTE = 60000
SCRIPT_TAIL_MS = 5000  # virtual time a run lasts after its script's last event, where the length is the script's
RUN_TIMEOUT_S = 600
MAX_WORKERS = 4  # the suites' fan-out unless WORKERS_VARIABLE says otherwise
WORKERS_VARIABLE = 'OPEU_TEST_WORKERS'


def default_workers() -> int:
  """How many runs the suites (play.test, perf.test) play at once: min(cores, MAX_WORKERS), or OPEU_TEST_WORKERS."""
  configured = os.environ.get(WORKERS_VARIABLE, '').strip()
  if configured:
    return max(1, int(configured))
  return max(1, min(os.cpu_count() or 1, MAX_WORKERS))


class Script:
  """An --input log built from (ms, event) pairs."""

  def __init__(self) -> None:
    self.events: dict[tuple[int, int], str] = {}

  def key(self, ms: int, name: str) -> Script:
    self.events[(ms, len(self.events))] = f'key {name}'
    return self

  def text(self, ms: int, chars: str) -> Script:
    self.events[(ms, len(self.events))] = f'text {chars}'
    return self

  def click(self, ms: int, x: int, y: int) -> Script:
    """The pointer moved onto (x, y) 400 ms before a left click there."""
    return self._pointer(ms, f'{x} {y}', f'click {x} {y}')

  def right_click(self, ms: int, x: int, y: int) -> Script:
    return self._pointer(ms, f'{x} {y}', f'click {x} {y} right')

  def enters(self, start: int, stop: int, step: int = 1500) -> Script:
    for ms in range(start, stop, step):
      self.key(ms, 'enter')
    return self

  def last_ms(self) -> int:
    return max(ms for ms, _ in self.events)

  def render(self) -> str:
    return ''.join(f'{ms} {ev}\n' for (ms, _), ev in sorted(self.events.items()))

  def _pointer(self, ms: int, place: str, event: str) -> Script:
    self.events[(ms - 400, len(self.events))] = f'move {place}'
    self.events[(ms, len(self.events))] = event
    return self


# Keyboard control throughout: Enter at the input prompt picks the keyboard,
# and every menu then starts on its first entry; `down` moves the highlight.
def demo(scenario: int, view_battles: bool, ms: int) -> Script:
  """New Game, the scenario, Demo Mode, the Allies' view, the field map,
  battles viewed or not, then Enter every 1.5 s through every message."""
  s = Script().key(1000, 'enter').key(2500, 'enter')  # keyboard; New Game
  for i in range(scenario):
    s.key(3500 + 50 * i, 'down')
  s.key(4000, 'enter')  # the scenario
  s.key(5500, 'enter')  # its history page
  s.key(7000, 'enter')  # Demo Mode
  s.key(8500, 'enter')  # view the Allies
  s.key(10000, 'enter')  # field map
  if not view_battles:
    s.key(11000, 'down')  # Don't View
  s.key(11500, 'enter').key(13000, 'enter').key(14500, 'enter')  # settings OK; ready
  return s.enters(16000, ms)


@dataclass(frozen=True)
class Side:
  """A One Player game's start: the scenario, the side commanded, whether battles are viewed, and when the first
  turn's command bar is up after the staff and the briefing (the Enters stop there)."""
  scenario: str
  axis: bool = False
  view_battles: bool = True
  ready_ms: int = 45000


# The first moment the command bar is up, found with 120 s runs that press Enter every 1.5 s.
FRANCE_ALLIES = Side('france')
AFRICA_ALLIES = Side('africa', ready_ms=46500)
BULGE_ALLIES = Side('bulge', ready_ms=46500)
KURSK_AXIS = Side('kursk', axis=True, ready_ms=48000)

# The enemy army HQ's cell for a side, read off the Tact target cursor's title line ("50-20 : ...  Battle ...").
ENEMY_HQ = {
  BULGE_ALLIES: (72, 14),
  KURSK_AXIS: (38, 50),
}


def one_player(s: Script, side: Side) -> Script:
  """New Game, the scenario, One Player, the side, field map, battles viewed
  or not, Very Easy, settings OK, then the staff: each post takes the next
  officer down the list (Army commander, Operations, Intelligence, Divisions
  1-4), confirmed, and Enter through the briefing until the command bar is
  up (side.ready_ms)."""
  s.key(1000, 'enter').key(2500, 'enter')
  for i in range(SCENARIOS.index(side.scenario)):
    s.key(3500 + 50 * i, 'down')
  s.key(4000, 'enter').key(5500, 'enter')
  s.key(6800, 'down').key(7000, 'enter')  # One Player
  if side.axis:
    s.key(8400, 'down')
  if not side.view_battles:
    s.key(11000, 'down')  # Don't View
  s.enters(8500, 16600)  # side, field map, view, Very Easy, OK, ready
  ms = 18000
  for _ in range(9):
    s.key(ms, 'down').key(ms + 300, 'enter')
    ms += 1500
  return s.enters(ms, side.ready_ms)


def start(side: Side) -> tuple[Script, int]:
  """A new One Player game of `side` and the time its first command may start."""
  s = Script()
  one_player(s, side)
  return s, side.ready_ms + 2000


def one_player_france(s: Script) -> Script:
  """One Player as the Allies in France; the first turn's command bar is up by 45 s."""
  return one_player(s, FRANCE_ALLIES)


def one_player_france_axis(s: Script) -> Script:
  """As one_player_france, commanding the Axis (the second side)."""
  return one_player(s, Side('france', axis=True))


def pass_turns(s: Script, ms: int, count: int) -> int:
  """Pass (the command bar's first entry), the hours prompt, confirm: each
  cycle lets one hour of the war run. Returns the time after the last."""
  for _ in range(count):
    s.key(ms, 'enter').text(ms + 700, '12').key(ms + 1000, 'enter').key(ms + 2000, 'y')
    ms += 5000
  return ms


def pass_settled(s: Script, ms: int, count: int, hours: str = '1') -> int:
  """Pass `hours` per turn, then four (Esc, Enter, Esc) triples 1.8 s
  apart: Esc lets a unit that reports itself ready and waiting wait on,
  Enter dismisses a turn message, and the second Esc closes whatever that
  Enter opened at the idle bar (the Pass prompt) or at a waiting unit (its
  order menu). Returns the time after the last."""
  typing = 200 * len(hours)
  for _ in range(count):
    s.key(ms, 'enter')
    for i, char in enumerate(hours):
      s.text(ms + 700 + 200 * i, char)
    s.key(ms + 1000 + typing, 'enter').key(ms + 2000 + typing, 'y')
    ms = _settle(s, ms + 3500 + typing)
  return ms


def _settle(s: Script, ms: int) -> int:
  for k in range(4):
    s.key(ms + 1800 * k, 'esc').key(ms + 1800 * k + 600, 'enter').key(ms + 1800 * k + 1200, 'esc')
  return ms + 1800 * 4 + 500


def open_save_menu(s: Script, ms: int) -> int:
  """Game (the last of the eleven entries), Save / Quit (the sixth), Save
  Game: ends at the slot list, or with the first save at "Game Saved"."""
  for i in range(10):
    s.key(ms + 100 * i, 'right')
  s.key(ms + 2000, 'enter')
  for i in range(5):
    s.key(ms + 3000 + 100 * i, 'down')
  s.key(ms + 4000, 'enter')
  s.key(ms + 6000, 'enter')
  return ms + 8000


def save_to_slot(s: Script, ms: int, slot: int) -> int:
  """With SAVEDATA present: open the menu, pick the slot, confirm OK."""
  ms = open_save_menu(s, ms)
  for i in range(slot):
    s.key(ms + 100 * i, 'down')
  s.key(ms + 300 * (slot > 0), 'enter')
  s.key(ms + 1500, 'y')
  return ms + 4000


def load_slot(s: Script, slot: int) -> int:
  """Keyboard, Load Game, the slot; the command bar is up by 7 s."""
  s.key(1000, 'enter').key(2400, 'down').key(2500, 'enter')
  for i in range(slot):
    s.key(3500 + 100 * i, 'down')
  s.key(4000, 'enter').key(5500, 'enter')
  return 10000


def save_new() -> Script:
  s = Script()
  one_player_france(s)
  ms = pass_turns(s, 50000, 9)
  open_save_menu(s, ms + 3000)
  return s


def load_play_save() -> Script:
  s = Script()
  ms = load_slot(s, 0)
  ms = pass_turns(s, ms, 3)
  save_to_slot(s, ms + 3000, 1)
  return s


def load_save() -> Script:
  s = Script()
  save_to_slot(s, load_slot(s, 0), 0)
  return s


def save_africa() -> Script:
  """North Africa as the Allies: two turns, Game > Save / Quit > Save Game
  (creates SAVEDATA, slot 1), a turn, save into slot 2, a turn."""
  s, ms = start(AFRICA_ALLIES)
  ms = pass_settled(s, ms, 2)
  ms = open_save_menu(s, ms)
  ms = pass_settled(s, ms + 1000, 1)
  ms = save_to_slot(s, ms, 1)
  pass_settled(s, ms + 1000, 1)
  return s


def load_africa() -> Script:
  """Load slot 2 of save-africa's SAVEDATA, two turns, save into slot 3."""
  s = Script()
  ms = load_slot(s, 1)
  ms = pass_settled(s, ms, 2)
  save_to_slot(s, ms, 2)
  return s


# The command bar of One Player, left to right; `command` opens an entry by
# moving right from Pass, where the highlight rests after every command.
BAR = ['pass', 'tact', 'city', 'tsfr', 'ofcr', 'info', 'orgz', 'rqst', 'air', 'spfc', 'game']


def keys(s: Script, ms: int, *tokens: str, step: int = 1500) -> int:
  """The tokens one `step` apart: a key name, `name*N` (the key N times,
  100 ms apart) or `=digits` (typed 200 ms apart); a long token takes the
  time it needs. Returns the time after the last."""
  for token in tokens:
    if token.startswith('='):
      for i, char in enumerate(token[1:]):
        s.text(ms + 200 * i, char)
      ms += max(step, 200 * len(token) + 500)
    else:
      name, _, count = token.partition('*')
      for i in range(int(count or 1)):
        s.key(ms + 100 * i, name)
      ms += max(step, 100 * int(count or 1) + 500)
  return ms


def at(x: int, y: int) -> list[str]:
  """The tokens that put the map cursor on cell x-y (even numbers): into
  the north-west corner, where it stops, then east and south."""
  return ['left*80', 'up*80', f'right*{x // 2}', f'down*{y // 2}']


def command(s: Script, ms: int, entry: str, *tokens: str) -> int:
  """The command bar's `entry` opened, then the tokens."""
  moves = [f'right*{BAR.index(entry)}'] if BAR.index(entry) else []
  return keys(s, ms, *moves, 'enter', *tokens)


# The One Player command runs below each start a game of their own and send
# a fixed key sequence through the command bar's dialogs. Map cursor moves
# are two cells a press; Villers (44-6) holds the Allied army HQ, Battle 11.
# Where a dialog takes another path than the comment says, the goldens
# record what the game does; the sequences were chosen with the coverage
# build (docs/coverage.md) for the code they reach.
def orders() -> Script:
  """Tact, City and Tsfr as the Allies, then three turns to carry them out."""
  s = Script()
  one_player_france(s)
  # Division 1 as a whole: Move, the target four presses east of its HQ.
  ms = command(s, 47000, 'tact', 'enter', 'enter', 'enter', 'right*4', 'enter', 'enter', 'esc')
  # Division 2 regiment by regiment: the first regiment's second order, a target to the north-east.
  ms = command(s, ms, 'tact', 'down', 'enter', 'n', 'enter', 'down', 'enter', 'right*13', 'up*2', 'enter', 'enter',
               'enter', 'esc', 'esc', 'esc', 'esc')
  # The first city's stockpile: fuel, ammo and food levels typed in.
  ms = command(s, ms, 'city', 'enter', '=30000', 'enter', '=25000', 'enter', '=40000', 'enter', 'enter', 'esc')
  # A regiment of Division 1 handed to Division 2 (the transfer time confirmed).
  ms = command(s, ms, 'tsfr', 'down', 'enter', 'enter', 'down', 'enter', 'enter', 'enter', 'esc', 'esc', 'esc')
  pass_turns(s, ms, 3)
  return s


def officers_and_advice() -> Script:
  """Ofcr (appoint, rest three days, reassign) and Info (the Operations
  chief's five reports, the intelligence views), then a turn."""
  s = Script()
  one_player_france(s)
  ms = command(s, 47000, 'ofcr', 'down', 'enter', 'enter', 'enter', 'enter', 'esc', 'esc', 'esc')
  ms = command(s, ms, 'ofcr', 'down*2', 'enter', 'enter', '=3', 'enter', 'enter', 'esc', 'esc')
  ms = command(s, ms, 'ofcr', 'enter', 'down', 'enter', 'enter', 'enter', 'esc', 'esc')
  ms = command(s, ms, 'info', 'enter', 'enter', 'enter', 'down', 'enter', 'enter', 'down*2', 'enter', 'enter',
               'down*3', 'enter', 'enter', 'down*4', 'enter', 'enter', 'esc', 'esc')
  ms = command(s, ms, 'info', 'down', 'enter', 'enter', 'enter', 'esc', 'down', 'enter', 'enter', 'esc',
               'down*2', 'enter', 'enter', 'esc', 'esc', 'esc')
  pass_turns(s, ms, 1)
  return s


def organize_and_request() -> Script:
  """Orgz (ten Mechanized dispatched in Battle 11's reorganisation, then
  cancelled) and Rqst (supplies for Villers, a battalion, a regiment),
  then two turns."""
  s = Script()
  one_player_france(s)
  ms = command(s, 47000, 'orgz', 'enter', 'enter', 'enter', 'down', 'enter', '=10', 'enter', 'esc', 'n', 'esc', 'y',
               'esc')
  ms = command(s, ms, 'rqst', 'enter', 'right*6', 'down*2', 'enter', 'y', 'enter', 'enter', 'esc', 'esc')
  ms = command(s, ms, 'rqst', 'down', 'enter', 'enter', 'enter', 'enter', 'enter', 'esc', 'esc')
  ms = command(s, ms, 'rqst', 'down*2', 'enter', 'enter', 'enter', 'enter', 'enter', 'esc')
  pass_turns(s, ms, 2)
  return s


def air_and_special_forces() -> Script:
  """The Axis in France: a carpet bombing of Villers (15 planes in two
  hours), paratroopers and a cargo drop, then the special forces' recon,
  cut lines, sabotage, raid and assassination against Villers and its
  garrison, then four turns for the missions to run."""
  s = Script()
  one_player_france_axis(s)
  ms = command(s, 47000, 'air', 'enter', *at(44, 6), 'enter', 'enter', '=15', 'enter', '=2', 'enter', 'enter',
               'enter', 'esc')
  ms = command(s, ms, 'air', 'down', 'enter', 'enter', 'enter', 'enter', 'enter', 'enter', 'enter', 'esc')
  ms = command(s, ms, 'air', 'down*2', 'enter', 'enter', 'enter', 'enter', 'enter', 'enter', 'enter', 'esc')
  for entry in range(5):
    ms = command(s, ms, 'spfc', f'down*{entry}', 'enter', *at(44, 6), 'enter', 'y', 'y', 'enter', 'esc')
  pass_turns(s, ms, 4)
  return s


def settings_and_quit() -> Script:
  """The Game menu's settings (battles, message delay, the aerial map,
  music, event pictures), two turns under them, then Save / Quit > Quit,
  which goes back to the title and into a new game of France."""
  s = Script()
  one_player_france(s)
  ms = command(s, 47000, 'game', 'enter', 'down', 'enter', 'down', 'enter', '=2', 'enter', 'down*2', 'enter', 'down',
               'enter', 'down*3', 'enter', 'enter', 'down*4', 'enter', 'enter', 'esc')
  ms = pass_turns(s, ms, 2)
  command(s, ms, 'game', 'down*5', 'enter', 'down', 'enter', 'y', 'enter', 'enter')
  return s


def campaign(side: Side) -> Script:
  """Three turns; Division 1 moved four presses east as a whole; Division
  2's first regiment given the order menu's second entry (Occupy) on the
  enemy army HQ's cell (refused unless that cell is an enemy city); the
  first city's levels typed in; four turns to carry it out."""
  s, ms = start(side)
  ms = pass_settled(s, ms, 3) + 1000
  ms = command(s, ms, 'tact', 'enter', 'enter', 'enter', 'right*4', 'enter', 'enter', 'esc')
  ms = command(s, ms, 'tact', 'down', 'enter', 'n', 'enter', 'down', 'enter', *at(*ENEMY_HQ[side]), 'enter', 'enter',
               'enter', 'esc', 'esc', 'esc', 'esc')
  ms = command(s, ms, 'city', 'enter', '=30000', 'enter', '=25000', 'enter', '=40000', 'enter', 'enter', 'esc')
  pass_settled(s, ms, 4)
  return s


def to_the_end(side: Side, ms: int, hours: str) -> Script:
  """`hours` passed per turn until `ms`; the scenario ends on its own before."""
  s, at_ms = start(side)
  while at_ms < ms:
    at_ms = pass_settled(s, at_ms, 1, hours)
  return s


# The pointer's targets on the 640x480 screen.
BAR_X = {'pass': 40, 'tact': 97, 'city': 152, 'tsfr': 209, 'ofcr': 266, 'info': 321, 'orgz': 377, 'rqst': 432,
         'air': 488, 'spfc': 544, 'game': 600}
BAR_Y = 51
GAME_MENU_X = 560
GAME_MENU_Y = [76, 99, 123, 147, 171, 195]  # Battle Screen, Message Delay, Change Maps, Set Music, Event Pictures, Save / Quit
NEUTRAL = (600, 460)  # black margin under the advisor panel: a click there only dismisses a message


def mouse_load(s: Script, slot: int) -> int:
  """The mouse chosen by a click, Load Game, the slot clicked, the two
  messages clicked away: the bar is up by 9 s. In mouse mode the keyboard
  is ignored."""
  s.click(1000, 320, 240).click(2500, 90, 235).click(4000, 125, 90 + 17 * slot)
  s.click(5500, *NEUTRAL).click(7000, *NEUTRAL)
  return 9000


def menus_by_mouse() -> Script:
  """save-new's game loaded by mouse; every command clicked open and
  right-clicked shut; every Game menu entry clicked (Change Maps switches to
  the aerial map) and right-clicked shut."""
  s = Script()
  ms = mouse_load(s, 0)
  for entry in BAR[:-1]:
    s.click(ms, BAR_X[entry], BAR_Y)
    _right_clicks(s, ms, BAR_X[entry], BAR_Y)
    ms += 5000
  for y in GAME_MENU_Y:
    s.click(ms, BAR_X['game'], BAR_Y).click(ms + 1500, GAME_MENU_X, y)
    _right_clicks(s, ms + 1000, GAME_MENU_X, y)
    ms += 6000
  return s


def _right_clicks(s: Script, ms: int, x: int, y: int) -> None:
  """Three right clicks a second apart from 2 s after `ms`: each closes a dialog level."""
  for k in (2000, 3000, 4000):
    s.right_click(ms + k, x, y)


def fixture_text(name: str) -> str:
  return (FIXTURES / name).read_text()


def saved_game_path(name: str) -> Path:
  """A golden saved game: made with the game's data, so the private repository's only."""
  return GOLDEN_DIR / f'{name}.SAVEDATA.gz'


def saved_game(name: str) -> bytes:
  return gzip.decompress(saved_game_path(name).read_bytes())


def playable(name: str) -> bool:
  """Whether this clone holds what the run starts from (the golden saved game it loads, if any)."""
  return not RUNS[name].saves_in or saved_game_path(RUNS[name].saves_in).exists()


@dataclass(frozen=True)
class Run:
  """A run: its input script, how long it plays and how often a frame is
  shot (0: only the final frame)."""
  script: Callable[[], str]
  ms: int
  shot_ms: int
  saves_in: str = ''  # the golden saved game copied into the save directory first
  audio: bool = False  # record --wav and --opl-log
  args: list[str] = field(default_factory=list)  # extra options


def scripted(build: Callable[[], Script], shot_ms: int, saves_in: str = '') -> Run:
  """A run as long as its script and SCRIPT_TAIL_MS more."""
  return Run(lambda: build().render(), build().last_ms() + SCRIPT_TAIL_MS, shot_ms, saves_in)


# end-france's shots: about two minutes apart, at an interval that puts one on the turn report naming San Michel
# (2718.6 s) and one on the defeat screen (2836.8 s), both lines formatted text in a side's colour.
END_FRANCE_SHOT_MS = 118200


def runs() -> dict[str, Run]:
  r = {f'demo-{name}': Run(lambda i=i: demo(i, False, 60 * MINUTE).render(), 60 * MINUTE, 5 * MINUTE)
       for i, name in enumerate(SCENARIOS)}
  r['demo-france-battles'] = Run(lambda: demo(0, True, 30 * MINUTE).render(), 30 * MINUTE, 2 * MINUTE)
  r['soak-kursk-6'] = Run(lambda: demo(2, True, 6 * MINUTE).render(), 6 * MINUTE, 30000)
  r['smoke-input'] = Run(lambda: fixture_text('smoke-input.txt'), 15000, 0, audio=True, args=['--mute'])
  r['smoke-turn'] = Run(lambda: fixture_text('smoke-turn.txt'), 120000, 0, audio=True, args=['--mute'])
  r['smoke-intro'] = Run(lambda: fixture_text('smoke-input.txt'), 70000, 0, audio=True, args=['--mute', '--intro'])
  r['save-new'] = Run(lambda: save_new().render(), 110000, 10000)
  r['load-play-save'] = Run(lambda: load_play_save().render(), 60000, 10000, 'save-new')
  r['load-save'] = Run(lambda: load_save().render(), 30000, 10000, 'save-new')
  r['save-africa'] = scripted(save_africa, 10000)
  r['load-africa'] = scripted(load_africa, 10000, 'save-africa')
  r['orders'] = Run(lambda: orders().render(), 150000, 10000)
  r['officers-advice'] = Run(lambda: officers_and_advice().render(), 160000, 10000)
  r['organize-request'] = Run(lambda: organize_and_request().render(), 140000, 10000)
  r['air-special-forces'] = Run(lambda: air_and_special_forces().render(), 330000, 30000)
  r['settings-quit'] = Run(lambda: settings_and_quit().render(), 120000, 10000)
  r['campaign-bulge-allies'] = scripted(lambda: campaign(BULGE_ALLIES), 10000)
  r['campaign-kursk-axis'] = scripted(lambda: campaign(KURSK_AXIS), 10000)
  r['menus-mouse'] = scripted(menus_by_mouse, 10000, 'save-new')
  r['end-france'] = Run(lambda: to_the_end(Side('france', view_battles=False), 50 * MINUTE, '24').render(),
                        50 * MINUTE, END_FRANCE_SHOT_MS)
  return r


RUNS = runs()

# The runs whose saved game becomes a golden file (and a later run's input), in the order regen plays them.
SAVED_GAMES = ['save-new', 'load-play-save', 'save-africa', 'load-africa']


def sha1(path: Path) -> str:
  return hashlib.sha1(Path(path).read_bytes()).hexdigest()


def command_line(opeu: Path, spec: Run, out: Path) -> list[str]:
  cmd = [str(opeu), '--headless', '--saves', str(out / 'saves'), '--ms', str(spec.ms), '--input',
         str(out / 'input.txt'), '--png', str(out / 'final.png'), *spec.args]
  if spec.shot_ms:
    cmd += ['--shots', str(out / 'shots'), str(spec.shot_ms)]
  if spec.audio:
    cmd += ['--wav', str(out / 'audio.wav'), '--opl-log', str(out / 'opl.log')]
  return cmd


def prepare(spec: Run, out: Path) -> None:
  """A fresh output directory with the save directory (and its saved game), the shots directory and the script."""
  if out.exists():
    shutil.rmtree(out)
  (out / 'saves').mkdir(parents=True)
  if spec.saves_in:
    (out / 'saves' / 'SAVEDATA').write_bytes(saved_game(spec.saves_in))
  if spec.shot_ms:
    (out / 'shots').mkdir()
  (out / 'input.txt').write_text(spec.script())


def recorded(spec: Run, out: Path, exit_code: int) -> dict:
  """The result record of a finished run."""
  result: dict = {'exit': exit_code, 'frames': {}}
  if spec.shot_ms:
    result['frames'] = {png.stem: sha1(png) for png in sorted((out / 'shots').iterdir())}
  if (out / 'final.png').exists():
    result['frames']['final'] = sha1(out / 'final.png')
  if spec.audio:
    result['wav'] = sha1(out / 'audio.wav')
    result['opl'] = sha1(out / 'opl.log')
  result['saves'] = {f.name: sha1(f) for f in sorted((out / 'saves').iterdir())}
  return result


@dataclass(frozen=True)
class Played:
  """A finished run: its result record and the peak resident memory of its `opeu` process."""
  result: dict
  peak_rss_mb: float


def run(opeu: str | Path, name: str, out: str | Path) -> dict:
  """One run into the directory `out`; returns its result record."""
  return play_measured(opeu, name, out).result


def play_measured(opeu: str | Path, name: str, out: str | Path) -> Played:
  """One run into the directory `out`, with its peak resident memory."""
  spec, out = RUNS[name], Path(out)
  prepare(spec, out)
  # cwd and OPEU_LOG inside `out`: nothing lands in the caller's directory.
  env = dict(os.environ, OPEU_LOG=str(out / 'port.log'))
  env.pop('OPEU_TRACE', None)
  exit_code, peak = waited(subprocess.Popen(command_line(Path(opeu), spec, out), cwd=out, env=env,
                                            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL))
  return Played(recorded(spec, out, exit_code), peak)


def waited(proc: subprocess.Popen) -> tuple[int, float]:
  """(exit code, peak RSS in MB) of a process, its own whatever else runs in parallel; killed after RUN_TIMEOUT_S."""
  timer = threading.Timer(RUN_TIMEOUT_S, proc.kill)
  high_water = HighWater(proc.pid) if HighWater.AVAILABLE else None
  timer.start()
  try:
    _, status, usage = os.wait4(proc.pid, 0)
  finally:
    timer.cancel()
  proc.returncode = os.waitstatus_to_exitcode(status)
  return proc.returncode, high_water.stop() if high_water else rss_mb(usage.ru_maxrss)


class HighWater:
  """The peak RSS of a running Linux process, polled from VmHWM in /proc/<pid>/status. Its ru_maxrss will not do: at
  execve the kernel folds the forking parent's high-water mark into the child's, so every game a large pytest process
  starts reports at least that process's RSS. VmHWM belongs to the game's own address space and only rises, so polling
  it misses at most the growth of the last POLL_S before exit."""

  AVAILABLE = Path('/proc/self/status').exists()
  POLL_S = 0.02

  def __init__(self, pid: int) -> None:
    self._status = Path(f'/proc/{pid}/status')
    self._peak_kb = 0
    self._done = threading.Event()
    self._thread = threading.Thread(target=self._poll, daemon=True)
    self._thread.start()

  def stop(self) -> float:
    """The peak in MB, once the process has exited."""
    self._done.set()
    self._thread.join()
    return self._peak_kb / 2**10

  def _poll(self) -> None:
    while not self._done.is_set():
      self._peak_kb = max(self._peak_kb, self._read_kb())
      self._done.wait(self.POLL_S)

  def _read_kb(self) -> int:
    try:
      lines = self._status.read_text().splitlines()
    except OSError:
      return 0  # exited (or a zombie being reaped)
    return next((int(line.split()[1]) for line in lines if line.startswith('VmHWM:')), 0)


def rss_mb(maxrss: int) -> float:
  """ru_maxrss in MB: bytes on macOS, kilobytes on Linux."""
  return maxrss / 2**20 if sys.platform == 'darwin' else maxrss / 2**10


def differences(expected: dict, actual: dict) -> list[str]:
  """Readable list of what differs between two result records."""
  out = []
  for key in sorted(set(expected) | set(actual)):
    e, a = expected.get(key), actual.get(key)
    if isinstance(e, dict) and isinstance(a, dict):
      out += [f'{key}/{sub}: expected {e.get(sub)}, got {a.get(sub)}' for sub in sorted(set(e) | set(a))
              if e.get(sub) != a.get(sub)]
    elif e != a:
      out.append(f'{key}: expected {e}, got {a}')
  return out


def load_goldens() -> dict:
  return json.loads(GOLDENS.read_text())


def regen(opeu: Path, scratch: Path) -> None:
  """Run everything with `opeu` and rewrite the goldens. The saved games
  are written first, in SAVED_GAMES order, because later runs load them."""
  GOLDEN_DIR.mkdir(exist_ok=True)
  goldens = {}
  for name in SAVED_GAMES + [n for n in RUNS if n not in SAVED_GAMES]:
    out = scratch / name
    goldens[name] = run(opeu, name, out)
    if name in SAVED_GAMES:
      data = (out / 'saves' / 'SAVEDATA').read_bytes()
      (GOLDEN_DIR / f'{name}.SAVEDATA.gz').write_bytes(gzip.compress(data, 9, mtime=0))
    print(name, goldens[name]['exit'], len(goldens[name]['frames']), 'frames')
  GOLDENS.write_text(json.dumps({n: goldens[n] for n in RUNS}, indent=1, sort_keys=True) + '\n')


if __name__ == '__main__':
  if len(sys.argv) == 4 and sys.argv[1] == 'regen':
    regen(Path(sys.argv[2]).resolve(), Path(sys.argv[3]).resolve())
  elif len(sys.argv) == 5 and sys.argv[1] == 'run':
    print(json.dumps(run(Path(sys.argv[2]).resolve(), sys.argv[3], Path(sys.argv[4]).resolve()), indent=1))
  else:
    raise SystemExit('play.py regen <opeu> <scratch dir> | play.py run <opeu> <name> <dir>')
