#!/usr/bin/env python3
"""Register-stream comparison: native FM driver (port/services/fmdriver.c)
against the original FMDRV.COM.

The original driver runs under the Unicorn harness of tests/validate_fmdrv.py
(its Machine class, its scripted session and its synthetic banks are reused
unchanged; this script only subclasses Machine to cut the event log into
steps). Each step is one driver activity: install, INT 66h call, far call,
one PIT tick, an installer run. The same steps are replayed through
port/tests/fmdriver_trace.c (the native driver) and, per step, the OPL
register writes (register, value, in order) and the returned AX/BX/DX/DS
must be identical.

Sessions:
  validator   tests/validate_fmdrv.py's session (synthetic banks exercising
              songs, effects, fade, queue, callback, voice and rhythm
              commands, installer runs, a machine without a chip);
  main-songs  every song of MAIN's own music bank (FMUSIC data at 3D91:0000,
              taken from the port's data arena) played with repeat on, then
              faded out as MAIN does (AH=02h AL=14h);
  main-effects every sound effect of MAIN's effect bank (3D91:7BBA);
  open-songs  the three songs of OPEN.EXE's bank (src/open/data/ofmusic.c,
              written by port/tests/dump_open_bank.c).

Usage: fmdriver_diff.py <fmdriver_trace binary> [<opeu binary for main-*>]
       [--open-dump=<dump_open_bank binary>] [--quick] [--json=FILE]
Exit status 0 only when every step of every session matches.
"""
import json
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tests'))
try:
    import validate_fmdrv as V  # noqa: E402  (the original driver's harness)
except ImportError as missing:  # unicorn not installed: ctest reports the test as skipped
    print(f'fmdriver_diff: skipped ({missing}); configure with -DOPEU_TEST_PYTHON=<python with unicorn>')
    sys.exit(77)

BANK_SEGMENTS = (V.BANK, V.EFFECTS)


class Step:
    def __init__(self, kind, args=(), phase=None):
        self.kind = kind
        self.args = tuple(args)
        self.phase = phase
        self.opl = []
        self.result = None


class TracingMachine(V.Machine):
    """validate_fmdrv.Machine that records the steps it runs."""
    steps = None  # list shared by the machines of one session

    def __init__(self, image, opl_present=True):
        self._mark = None
        self._current = None
        self._resident = False
        self._programs = 0
        self.script = []
        super().__init__(image, opl_present)
        self._mark = len(self.events)
        self._begin('machine', [int(opl_present)])

    # step bookkeeping -------------------------------------------------------
    def _flush(self):
        if self._mark is None or self._current is None:
            return
        step = self._current
        step.opl += [(e[1], e[2]) for e in self.events[self._mark:] if e[0] == 'opl']
        self._mark = len(self.events)

    def _begin(self, kind, args=()):
        self._flush()
        step = Step(kind, args)
        TracingMachine.steps.append(step)
        self._current = step
        return step

    def finish(self):
        self._flush()

    # recorded operations ----------------------------------------------------
    def write(self, segment, offset, data):
        super().write(segment, offset, data)
        if self._mark is not None and segment in BANK_SEGMENTS:
            TracingMachine.steps.append(Step('mem', (segment, offset, bytes(data))))

    def run_program(self, segment, tail):
        self._programs += 1
        if self._programs == 1:
            self._begin('install')
            self._resident = True
        elif tail.strip(b' \t').upper() == b'OFF' and self._resident:
            self._begin('uninstall')      # the installer calls INT 66h AH=FFh
            self._resident = False
        else:
            self._begin('none')           # reports only (already installed, bad tail, ...)
        return super().run_program(segment, tail)

    def ticks(self, count):
        for _ in range(count):
            self._begin('tick')
            self.call(V.TIMER_STUB, self.vector(0x08))
        self.events.append(('ticks', count))

    def api(self, ax, **regs):
        step = self._begin('int', (ax, regs.get('bx', 0), regs.get('dx', 0), regs.get('ds', 0)))
        result = super().api(ax, **regs)
        step.result = (result[0], result[1], result[3], result[7])
        preserved = (result[2], result[4], result[5], result[6], result[8])
        if preserved != (regs.get('cx', 0), 0x1111, 0x2222, 0x3333, regs.get('es', 0)):
            step.result += ('clobbered',)
        return result

    def call(self, stub, target, ax=0, bx=0, cx=0, dx=0, ds=0, es=0, flags=0x202):
        if stub != V.FAR_STUB:
            return super().call(stub, target, ax=ax, bx=bx, cx=cx, dx=dx, ds=ds, es=es, flags=flags)
        step = self._begin('far', (ax, bx, dx, ds))
        result = super().call(stub, target, ax=ax, bx=bx, cx=cx, dx=dx, ds=ds, es=es, flags=flags)
        step.result = (result[0], result[1], result[3], result[7])
        return result


def run_original(session_fn, image):
    TracingMachine.steps = []
    machines = []
    real = V.Machine

    class Tracked(TracingMachine):
        def __init__(self, *a, **k):
            super().__init__(*a, **k)
            machines.append(self)

    V.Machine = Tracked
    try:
        record = session_fn(image)
    finally:
        V.Machine = real
    for m in machines:
        m.finish()
    if isinstance(record, tuple):       # validate_fmdrv.session returns (record, events)
        record = record[0]
    return TracingMachine.steps, record


def script_of(steps):
    out = []
    for s in steps:
        if s.kind == 'machine':
            out.append(f'machine {s.args[0]:x}')
        elif s.kind == 'mem':
            seg, off, data = s.args
            for i in range(0, len(data), 4096):
                out.append(f'mem {seg:x} {off + i:x} {data[i:i + 4096].hex()}')
        elif s.kind in ('int', 'far'):
            out.append(f'{s.kind} ' + ' '.join(f'{v:x}' for v in s.args))
        elif s.kind == 'tick':
            out.append('tick 1')
        else:
            out.append(s.kind)
    return '\n'.join(out) + '\n'


def run_native(trace_binary, steps):
    text = subprocess.run([str(trace_binary)], input=script_of(steps), capture_output=True,
                          text=True, check=True).stdout
    native = []
    for line in text.splitlines():
        if line.startswith('> '):
            native.append(Step(line[2:]))
        elif line.startswith('opl '):
            _, reg, val = line.split()
            native[-1].opl.append((int(reg, 16), int(val, 16)))
        elif line.startswith('ret '):
            native[-1].result = tuple(int(v, 16) for v in line.split()[1:])
    return native


def compare(name, steps, native, phases):
    """Per-phase step match counts; phases: list of (label, predicate)."""
    expected = [s for s in steps if s.kind != 'mem']
    if len(expected) != len(native):
        raise SystemExit(f'{name}: step count differs ({len(expected)} vs {len(native)})')
    report = {}
    phase_index, label = 0, phases[0][0] if phases else name
    first_mismatch = {}
    for i, (e, n) in enumerate(zip(expected, native)):
        while phase_index + 1 < len(phases) and phases[phase_index + 1][1](e):
            phase_index += 1
            label = phases[phase_index][0]
        entry = report.setdefault(label, {'steps': 0, 'matching_steps': 0, 'opl_writes': 0,
                                          'matching_writes': 0, 'ticks': 0, 'calls': 0})
        entry['steps'] += 1
        entry['ticks'] += e.kind == 'tick'
        entry['calls'] += e.kind in ('int', 'far')
        entry['opl_writes'] += len(e.opl)
        entry['matching_writes'] += sum(1 for a, b in zip(e.opl, n.opl) if a == b) if len(e.opl) == len(n.opl) else 0
        ok = e.kind == n.kind and e.opl == n.opl and (e.result is None or e.result == n.result)
        entry['matching_steps'] += ok
        if not ok and label not in first_mismatch:
            first_mismatch[label] = {'step': i, 'kind': e.kind, 'args': [hex(a) for a in e.args],
                                     'expected_opl': [f'{r:02x}={v:02x}' for r, v in e.opl][:24],
                                     'native_opl': [f'{r:02x}={v:02x}' for r, v in n.opl][:24],
                                     'expected_result': e.result and [hex(v) if isinstance(v, int) else v
                                                                      for v in e.result],
                                     'native_result': n.result and [hex(v) for v in n.result]}
    for label, mismatch in first_mismatch.items():
        report[label]['first_mismatch'] = mismatch
    return report


def find(ax):
    return lambda s: s.kind == 'int' and s.args[0] == ax


VALIDATOR_PHASES = [
    ('install', lambda s: True),
    ('version, countdowns', find(0xfe00)),
    ('song 1: callback, load, repeat, play', find(0x0900)),
    ('voice volume/transpose (80h/81h)', find(0x8000)),
    ('queue (08h) and repeat off', find(0x0802)),
    ('fade-out (02h AL=6)', find(0x0101)),
    ('effects (10h-13h)', find(0x1000)),
    ('effect voices (05h-07h)', find(0x0500)),
    ('rhythm commands (82h/83h)', find(0x8200)),
    ('invalid function, play/stop, far call', find(0x2000)),
    ('installer runs, uninstall', lambda s: s.kind == 'none'),
    ('no OPL chip', lambda s: s.kind == 'machine'),
]


def game_banks(opeu):
    """MAIN's song and effect banks from the port's initial data arena."""
    with tempfile.TemporaryDirectory() as tmp:
        dump = Path(tmp) / 'arena.bin'
        subprocess.run([str(opeu), '--headless', '--dump-arena', str(dump)], check=True,
                       capture_output=True)
        arena = dump.read_bytes()
    base = 0x3d91 * 16
    return arena[base:base + 0x7bba], arena[base + 0x7bba:base + 0x7bba + 847]


def song_count(bank):
    """MAIN's song table is opeu_music_tracks (190 bytes at 3D91:0000,
    port/gen/data_image.c): 19 entries of five words; the instrument data
    follows it, so the count cannot be read from the first track offset."""
    return 190 // 10


def effect_count(bank):
    """Entries until the first track (each entry's words are relative to it)."""
    end, k = len(bank), 0
    while 4 * k < end:
        for word in struct.unpack_from('<2H', bank, 4 * k):
            if word:
                end = min(end, word + 4 * k)
        k += 1
    return k


def main_session(music, effects, song_ticks, fade_ticks, effect_ticks, songs=None, label='MAIN'):
    songs = songs or song_count(music)
    n_effects = effect_count(effects) if effects else 0
    phases = [('install and load', lambda s: True)]

    def session(image):
        m = V.Machine(image)
        m.write(V.BANK, 0, music)
        if effects:
            m.write(V.EFFECTS, 0, effects)
        m.run_program(V.DRIVER, b'')
        m.api(0x0000, dx=0, ds=V.BANK)
        if effects:
            m.api(0x1000, dx=0, ds=V.EFFECTS)
        for n in range(1, songs + 1):
            m.api(0x0401)
            m.api(0x0100 | n)
            m.ticks(song_ticks)
            m.api(0x0300)
            m.api(0x0214)               # opeu_music_stop: fade 20
            m.ticks(fade_ticks)
            m.api(0x0300)
        for n in range(1, n_effects + 1):
            m.api(0x1100 | n)
            m.ticks(effect_ticks)
            m.api(0x1300)
        return {}

    for n in range(1, songs + 1):
        phases.append((f'{label} song {n}', (lambda k: lambda s: s.kind == 'int' and s.args[0] == 0x0100 | k)(n)))
    for n in range(1, n_effects + 1):
        phases.append((f'{label} effect {n}', (lambda k: lambda s: s.kind == 'int' and s.args[0] == 0x1100 | k)(n)))
    return session, phases


def main(argv):
    args = [a for a in argv[1:] if not a.startswith('--')]
    quick = '--quick' in argv
    json_path = next((a.split('=', 1)[1] for a in argv if a.startswith('--json=')), None)
    if not args:
        raise SystemExit(__doc__)
    trace = Path(args[0])
    image = (ROOT / 'binaries/FMDRV.COM').read_bytes()
    sessions = [('validator', V.session, VALIDATOR_PHASES)]
    if len(args) > 1:
        music, effects = game_banks(Path(args[1]))
        ticks = (1500, 300, 200) if quick else (8000, 600, 300)
        fn, phases = main_session(music, effects, *ticks)
        sessions.append(('main-songs-and-effects', fn, phases))
    open_dump = next((a.split('=', 1)[1] for a in argv if a.startswith('--open-dump=')), None)
    if open_dump:
        bank = subprocess.run([open_dump], capture_output=True, check=True).stdout
        ticks = (1500, 300, 0) if quick else (9000, 600, 0)
        fn, phases = main_session(bank, b'', *ticks, songs=3, label='OPEN')
        sessions.append(('open-songs', fn, phases))
    results = {}
    failed = False
    for name, fn, phases in sessions:
        steps, record = run_original(fn, image)
        native = run_native(trace, steps)
        report = compare(name, steps, native, phases)
        if name == 'validator':
            # count the callbacks before the second (chip-less) machine starts
            lines = script_of(steps).splitlines()
            second = [i for i, l in enumerate(lines) if l.startswith('machine')][1]
            text = '\n'.join(lines[:second] + ['callbacks']) + '\n'
            out = subprocess.run([str(trace)], input=text, capture_output=True,
                                 text=True, check=True).stdout.splitlines()
            native_callbacks = int(out[-1].split()[1], 16)
            report['client callback count'] = {'expected': record['callback_count'], 'native': native_callbacks,
                                               'match': record['callback_count'] == native_callbacks}
            failed |= record['callback_count'] != native_callbacks
        results[name] = report
        print(f'session {name}:')
        for label, entry in report.items():
            if 'steps' not in entry:
                print(f'  {label}: expected {entry["expected"]}, native {entry["native"]}')
                continue
            rate = 100.0 * entry['matching_steps'] / entry['steps']
            print(f'  {label:44s} {entry["matching_steps"]:6d}/{entry["steps"]:<6d} steps '
                  f'({rate:6.2f}%), {entry["opl_writes"]:6d} OPL writes, {entry["ticks"]} ticks')
            if entry['matching_steps'] != entry['steps']:
                failed = True
                print('    first mismatch:', json.dumps(entry['first_mismatch']))
    if json_path:
        Path(json_path).write_text(json.dumps(results, indent=2) + '\n')
    if failed:
        print('FM driver register streams DIFFER')
        return 1
    print('FM driver register streams identical')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
