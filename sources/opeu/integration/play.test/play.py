#!/usr/bin/env python3
"""Scripted headless play of MAIN and its golden outputs (the functional suite).

Every run is `opeu --headless` with a generated `--input` script. Headless
time is virtual (1 ms per input poll), so a script gives the same frames,
sound and saved games on every run and every machine. The goldens in
`../fixtures/play/` record the behaviour of v1.0 (the three smokes' frames,
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
  orders, officers-advice, organize-request, air-special-forces,
  settings-quit     One Player games in France driven through the command
                    bar's dialogs (Tact, City, Tsfr; Ofcr, Info; Orgz, Rqst;
                    Air and SpFc as the Axis; the Game menu's settings and
                    Quit), each ending with turns that carry the orders out.
                    Chosen with the coverage build (tools/coverage/,
                    docs/coverage.md) for the code they reach.

Each run records the exit code, the SHA-1 of every `--shots` frame and of
the final `--png` frame, of the WAV and OPL log where recorded, and of every
file the game left in its save directory.

    python3 play.py regen <opeu> <dir>   rewrite ../fixtures/play/ from <opeu>, outputs in <dir>
    python3 play.py run <opeu> <name> <dir>   one run, outputs kept in <dir>
"""
import gzip
import hashlib
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
FIXTURES = HERE.parent / 'fixtures'
GOLDEN_DIR = FIXTURES / 'play'
GOLDENS = GOLDEN_DIR / 'goldens.json'

SCENARIOS = ['france', 'africa', 'kursk', 'normandy', 'bulge', 'berlin']
MINUTE = 60000


class Script:
    """An --input log built from (ms, event) pairs."""

    def __init__(self):
        self.events = {}

    def key(self, ms, name):
        self.events[(ms, len(self.events))] = f'key {name}'
        return self

    def text(self, ms, chars):
        self.events[(ms, len(self.events))] = f'text {chars}'
        return self

    def enters(self, start, stop, step=1500):
        for ms in range(start, stop, step):
            self.key(ms, 'enter')
        return self

    def render(self):
        return ''.join(f'{ms} {ev}\n' for (ms, _), ev in sorted(self.events.items()))


# Keyboard control throughout: Enter at the input prompt picks the keyboard,
# and every menu then starts on its first entry; `down` moves the highlight.
def demo(scenario, view_battles, ms):
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


def one_player_france(s):
    """New Game, France, One Player, the Allies, field map, view battles, Very
    Easy, settings OK, then the staff: each post takes the next officer down
    the list (Army commander, Operations, Intelligence, Divisions 1-4),
    confirmed. The first turn's command bar is up by 45 s."""
    s.key(1000, 'enter').key(2500, 'enter').key(4000, 'enter').key(5500, 'enter')
    s.key(6800, 'down').key(7000, 'enter')  # One Player
    s.enters(8500, 16600)  # Allies, field map, view, Very Easy, OK, ready
    ms = 18000
    for _ in range(9):
        s.key(ms, 'down').key(ms + 300, 'enter')
        ms += 1500
    return s.enters(ms, 45000)


def one_player_france_axis(s):
    """As one_player_france, commanding the Axis (the second side)."""
    s.key(8400, 'down')
    return one_player_france(s)


def pass_turns(s, ms, count):
    """Pass (the command bar's first entry), the hours prompt, confirm: each
    cycle lets one hour of the war run. Returns the time after the last."""
    for _ in range(count):
        s.key(ms, 'enter').text(ms + 700, '12').key(ms + 1000, 'enter').key(ms + 2000, 'y')
        ms += 5000
    return ms


def open_save_menu(s, ms):
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


def save_to_slot(s, ms, slot):
    """With SAVEDATA present: open the menu, pick the slot, confirm OK."""
    ms = open_save_menu(s, ms)
    for i in range(slot):
        s.key(ms + 100 * i, 'down')
    s.key(ms + 300 * (slot > 0), 'enter')
    s.key(ms + 1500, 'y')
    return ms + 4000


def load_slot(s, slot):
    """Keyboard, Load Game, the slot; the command bar is up by 7 s."""
    s.key(1000, 'enter').key(2400, 'down').key(2500, 'enter')
    for i in range(slot):
        s.key(3500 + 100 * i, 'down')
    s.key(4000, 'enter').key(5500, 'enter')
    return 10000


def save_new():
    s = Script()
    one_player_france(s)
    ms = pass_turns(s, 50000, 9)
    open_save_menu(s, ms + 3000)
    return s


def load_play_save():
    s = Script()
    ms = load_slot(s, 0)
    ms = pass_turns(s, ms, 3)
    save_to_slot(s, ms + 3000, 1)
    return s


def load_save():
    s = Script()
    save_to_slot(s, load_slot(s, 0), 0)
    return s


# The command bar of One Player, left to right; `command` opens an entry by
# moving right from Pass, where the highlight rests after every command.
BAR = ['pass', 'tact', 'city', 'tsfr', 'ofcr', 'info', 'orgz', 'rqst', 'air', 'spfc', 'game']


def keys(s, ms, *tokens, step=1500):
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


def at(x, y):
    """The tokens that put the map cursor on cell x-y (even numbers): into
    the north-west corner, where it stops, then east and south."""
    return ['left*80', 'up*80', f'right*{x // 2}', f'down*{y // 2}']


def command(s, ms, entry, *tokens):
    """The command bar's `entry` opened, then the tokens."""
    moves = [f'right*{BAR.index(entry)}'] if BAR.index(entry) else []
    return keys(s, ms, *moves, 'enter', *tokens)


# The One Player command runs below each start a game of their own and send
# a fixed key sequence through the command bar's dialogs. Map cursor moves
# are two cells a press; Villers (44-6) holds the Allied army HQ, Battle 11.
# Where a dialog takes another path than the comment says, the goldens
# record what the game does; the sequences were chosen with the coverage
# build (docs/coverage.md) for the code they reach.
def orders():
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


def officers_and_advice():
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


def organize_and_request():
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


def air_and_special_forces():
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


def settings_and_quit():
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


def fixture_text(name):
    return (FIXTURES / name).read_text()


def saved_game(name):
    return gzip.decompress((GOLDEN_DIR / f'{name}.SAVEDATA.gz').read_bytes())


# name: (script, virtual ms, shot interval ms, options)
#   saves_in: golden saved game copied into the save directory first
#   audio:    record --wav and --opl-log
#   args:     extra options
def runs():
    r = {}
    for i, name in enumerate(SCENARIOS):
        r[f'demo-{name}'] = (lambda i=i: demo(i, False, 60 * MINUTE).render(), 60 * MINUTE, 5 * MINUTE, {})
    r['demo-france-battles'] = (lambda: demo(0, True, 30 * MINUTE).render(), 30 * MINUTE, 2 * MINUTE, {})
    smoke = {'args': ['--mute'], 'audio': True}
    r['smoke-input'] = (lambda: fixture_text('smoke-input.txt'), 15000, 0, smoke)
    r['smoke-turn'] = (lambda: fixture_text('smoke-turn.txt'), 120000, 0, smoke)
    r['smoke-intro'] = (lambda: fixture_text('smoke-input.txt'), 70000, 0, dict(smoke, args=['--mute', '--intro']))
    r['save-new'] = (lambda: save_new().render(), 110000, 10000, {})
    r['load-play-save'] = (lambda: load_play_save().render(), 60000, 10000, {'saves_in': 'save-new'})
    r['load-save'] = (lambda: load_save().render(), 30000, 10000, {'saves_in': 'save-new'})
    r['orders'] = (lambda: orders().render(), 150000, 10000, {})
    r['officers-advice'] = (lambda: officers_and_advice().render(), 160000, 10000, {})
    r['organize-request'] = (lambda: organize_and_request().render(), 140000, 10000, {})
    r['air-special-forces'] = (lambda: air_and_special_forces().render(), 330000, 30000, {})
    r['settings-quit'] = (lambda: settings_and_quit().render(), 120000, 10000, {})
    return r


RUNS = runs()

# The saved games a run leaves that become golden files (and later inputs).
SAVED_GAMES = {'save-new': 'save-new', 'load-play-save': 'load-play-save'}


def sha1(path):
    return hashlib.sha1(Path(path).read_bytes()).hexdigest()


def run(opeu, name, out):
    """One run into the directory `out`; returns its result record."""
    make_script, ms, shot_ms, opts = RUNS[name]
    out = Path(out)
    if out.exists():
        shutil.rmtree(out)
    saves, shots = out / 'saves', out / 'shots'
    saves.mkdir(parents=True)
    if opts.get('saves_in'):
        (saves / 'SAVEDATA').write_bytes(saved_game(opts['saves_in']))
    script = out / 'input.txt'
    script.write_text(make_script())
    cmd = [str(opeu), '--headless', '--saves', str(saves), '--ms', str(ms), '--input', str(script),
           '--png', str(out / 'final.png')] + opts.get('args', [])
    if shot_ms:
        shots.mkdir()
        cmd += ['--shots', str(shots), str(shot_ms)]
    if opts.get('audio'):
        cmd += ['--wav', str(out / 'audio.wav'), '--opl-log', str(out / 'opl.log')]
    # cwd and OPEU_LOG inside `out`: nothing lands in the caller's directory.
    env = dict(os.environ, OPEU_LOG=str(out / 'port.log'))
    env.pop('OPEU_TRACE', None)
    proc = subprocess.run(cmd, cwd=out, env=env, capture_output=True, timeout=600)
    result = {'exit': proc.returncode, 'frames': {}}
    if shot_ms:
        for png in sorted(shots.iterdir()):
            result['frames'][png.stem] = sha1(png)
    if (out / 'final.png').exists():
        result['frames']['final'] = sha1(out / 'final.png')
    if opts.get('audio'):
        result['wav'] = sha1(out / 'audio.wav')
        result['opl'] = sha1(out / 'opl.log')
    result['saves'] = {f.name: sha1(f) for f in sorted(saves.iterdir())}
    return result


def differences(expected, actual):
    """Readable list of what differs between two result records."""
    out = []
    for key in sorted(set(expected) | set(actual)):
        e, a = expected.get(key), actual.get(key)
        if isinstance(e, dict) and isinstance(a, dict):
            for sub in sorted(set(e) | set(a)):
                if e.get(sub) != a.get(sub):
                    out.append(f'{key}/{sub}: expected {e.get(sub)}, got {a.get(sub)}')
        elif e != a:
            out.append(f'{key}: expected {e}, got {a}')
    return out


def load_goldens():
    return json.loads(GOLDENS.read_text())


def regen(opeu, scratch):
    """Run everything with `opeu` and rewrite the goldens. The saved games
    are written first, because later runs load them."""
    GOLDEN_DIR.mkdir(exist_ok=True)
    goldens = {}
    order = [n for n in RUNS if n in SAVED_GAMES] + [n for n in RUNS if n not in SAVED_GAMES]
    for name in order:
        out = Path(scratch) / name
        goldens[name] = run(opeu, name, out)
        if name in SAVED_GAMES:
            data = (out / 'saves' / 'SAVEDATA').read_bytes()
            (GOLDEN_DIR / f'{SAVED_GAMES[name]}.SAVEDATA.gz').write_bytes(gzip.compress(data, 9, mtime=0))
        print(name, goldens[name]['exit'], len(goldens[name]['frames']), 'frames')
    GOLDENS.write_text(json.dumps({n: goldens[n] for n in RUNS}, indent=1, sort_keys=True) + '\n')


if __name__ == '__main__':
    if len(sys.argv) == 4 and sys.argv[1] == 'regen':
        regen(Path(sys.argv[2]).resolve(), Path(sys.argv[3]).resolve())
    elif len(sys.argv) == 5 and sys.argv[1] == 'run':
        print(json.dumps(run(Path(sys.argv[2]).resolve(), sys.argv[3], Path(sys.argv[4]).resolve()), indent=1))
    else:
        raise SystemExit('play.py regen <opeu> <scratch dir> | play.py run <opeu> <name> <dir>')
