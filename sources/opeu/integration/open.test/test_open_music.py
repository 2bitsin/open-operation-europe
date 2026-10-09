#!/usr/bin/env python3
"""The opening's music in the port against the original FM driver.

1. Runs the port headless through the opening (--intro) with a key at the
   title (default 140 s), logging every INT 66h call with the output frame
   at which it happened (OPEU_TRACE=fm) and every OPL2 register write
   (--opl-log).
2. Replays exactly those driver calls, each after the same number of PIT
   ticks (tick k is due at output frame ceil(k * 4096 * 49716 / 1193182),
   port/services/fm.c), on the original FMDRV.COM under the Unicorn harness
   of tests/validate_fmdrv.py (the original is the fixture; the port never
   runs it), with OPEN's song bank (src/open/data/ofmusic.c, written by
   port/tests/dump_open_bank.c) in the caller's memory.
3. Requires the port's register stream (tick, register, value) to equal the
   original driver's write for write, and every call to return the same
   AX/BX (the opening branches on the status and countdown results).

Memory model of the replay: the bank is the caller's memory until the
shutdown's AH=01h AL=05h. Song 5 lies past the bank's three-song table; its
track offsets (F407 1388 5002 0003 0000) point into OPEN's heap, DGROUP's
stack area and the bank, so from that call on the replay holds OPEN's 64 KB
from the bank segment as the port had it at the exit (OPEU_OPEN_DUMP). When
the opening exits, the port loads MAIN's image, which has nothing at the
bank's addresses (in DOS the game's code is loaded there); the replay puts
the same bytes (MAIN's initial arena, opeu --dump-arena) at the bank's
segment from the first call after the exit on. See port/README.md "OPEN.EXE", sound.

Usage: open_music_diff.py <opeu> <dump_open_bank> <out dir> [--key-ms=N] [--ms=N]
Needs a Python with unicorn (exit 77 = skipped otherwise).
"""
import json
import os
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT/'tests'))
try:
    import validate_fmdrv as V  # noqa: E402
except ImportError as missing:
    print(f'open_music_diff: skipped ({missing})')
    sys.exit(77)

RATE = 49716
PIT = 1193182
DIVISOR = 4096
OPEN_BANK = 0x0710     # OPEN_BANK_SEGMENT (port/open/open_process.h, load segment 0)
CALL = re.compile(r'\[fm\] frame (\d+) int66 ax=([0-9a-f]+) bx=([0-9a-f]+) dx=([0-9a-f]+) ds=([0-9a-f]+)'
                  r' -> ax=([0-9a-f]+) bx=([0-9a-f]+)')


def ticks_at(frame):
    return frame * PIT // (DIVISOR * RATE)


def run_port(opeu, out, key_ms, stop_ms):
    out.mkdir(parents=True, exist_ok=True)
    script = out/'title-key.txt'
    script.write_text(f'{key_ms} key space\n')
    log, opl = out/'fm-trace.log', out/'intro.opl'
    env = dict(os.environ, OPEU_TRACE='fm,svc', OPEU_LOG=str(log), OPEU_OPEN_DUMP=str(out/'open-memory.bin'))
    subprocess.run([str(opeu), '--headless', '--intro', '--ms', str(stop_ms), '--input', str(script),
                    '--opl-log', str(opl), '--wav', str(out/'intro.wav'), '--png', str(out/'end.png')],
                   env=env, check=True, capture_output=True)
    calls, exit_index, exit_frame = [], None, None
    for line in log.read_text().splitlines():
        m = CALL.search(line)
        if m:
            calls.append(tuple(int(v, 16) if i else int(v) for i, v in enumerate(m.groups())))
        elif 'OPEN.EXE: exit at' in line:
            exit_index = len(calls)
            exit_frame = int(line.split()[-2]) * RATE // 1000000
    writes = []
    for line in opl.read_text().splitlines():
        if line.startswith('#'):
            continue
        frame, reg, value = line.split()
        writes.append((ticks_at(int(frame)), int(reg, 16), int(value, 16)))
    end_frame = stop_ms * RATE // 1000
    return calls, (exit_index, exit_frame), writes, end_frame


def run_original(bank, open_memory, after_exit, calls, exit, end_frame):
    exit_index, exit_frame = exit
    image = (ROOT/'binaries/FMDRV.COM').read_bytes()
    m = V.Machine(image)
    m.write(V.BANK, 2, bank)
    m.run_program(V.DRIVER, b'')
    install = [e for e in m.events if e[0] == 'opl']
    mark = len(m.events)
    writes, results, done = [], [], 0

    def collect(tick):
        nonlocal mark
        for e in m.events[mark:]:
            if e[0] == 'opl':
                writes.append((tick, e[1], e[2]))
        mark = len(m.events)

    def tick_until(due):
        nonlocal done
        while done < due:
            if exit_frame is not None and done == ticks_at(exit_frame):
                m.write(V.BANK, 0, after_exit)             # MAIN loaded over the bank
            m.ticks(1)
            done += 1
            collect(done)

    for index, (frame, ax, bx, dx, ds, _, _) in enumerate(calls):
        if ax >> 8 == 0x01 and ax & 0xff > 3 and open_memory:
            m.write(V.BANK, 0, open_memory)                # song 5: offsets into OPEN's memory
        if ax >> 8 == 0x00 and ds != OPEN_BANK:
            raise SystemExit(f'MAIN opened its own bank (call {index}); stop the run earlier (--ms)')
        tick_until(ticks_at(frame))
        if index == exit_index:
            m.write(V.BANK, 0, after_exit)                 # MAIN loaded over the bank
        segment = V.BANK if ax >> 8 in (0x00, 0x05, 0x10) else ds
        r = m.api(ax, bx=bx, dx=dx, ds=segment)
        results.append((r[0], r[1]))
        collect(done)
    tick_until(ticks_at(end_frame))
    return install, writes, results


def main(argv):
    args = [a for a in argv[1:] if not a.startswith('--')]
    opts = dict(a[2:].split('=', 1) for a in argv[1:] if a.startswith('--') and '=' in a)
    if len(args) < 3:
        raise SystemExit(__doc__)
    opeu, dump, out = Path(args[0]), Path(args[1]), Path(args[2])
    key_ms, stop_ms = int(opts.get('key-ms', 140000)), int(opts.get('ms', 150000))
    bank = subprocess.run([str(dump)], capture_output=True, check=True).stdout
    calls, exit_index, native, end_frame = run_port(opeu, out, key_ms, stop_ms)
    arena = out/'main-arena.bin'
    subprocess.run([str(opeu), '--dump-arena', str(arena)], check=True, capture_output=True)
    after_exit = arena.read_bytes()[OPEN_BANK*16:OPEN_BANK*16 + 0xfff0]
    dump = out/'open-memory.bin'
    open_memory = dump.read_bytes() if dump.exists() else None
    install, original, results = run_original(bank, open_memory, after_exit, calls, exit_index, end_frame)

    # The port installs the driver without the chip test (fm.c): its log
    # starts with the install's chip initialisation. Compare those writes
    # with the original install's minus the timer test (registers 2-4).
    init = [(r, v) for _, r, v in install]
    init_no_test = [(r, v) for r, v in init if r not in (0x02, 0x03, 0x04)]
    native_init = [(r, v) for _, r, v in native[:len(init_no_test)]]
    native_rest = native[len(init_no_test):]

    report = {'calls': len(calls), 'opening_exit_call': exit_index[0], 'opening_exit_tick': ticks_at(exit_index[1]),
              'ticks': ticks_at(end_frame),
              'install_writes_match': native_init == init_no_test,
              'opl_writes_original': len(original), 'opl_writes_port': len(native_rest)}
    mismatch = next((i for i, (a, b) in enumerate(zip(original, native_rest)) if a != b), None)
    if mismatch is None and len(original) != len(native_rest):
        mismatch = min(len(original), len(native_rest))
    report['first_write_mismatch'] = mismatch
    if mismatch is not None:
        report['context_original'] = [f'{t}:{r:02x}={v:02x}' for t, r, v in original[mismatch:mismatch+8]]
        report['context_port'] = [f'{t}:{r:02x}={v:02x}' for t, r, v in native_rest[mismatch:mismatch+8]]
    bad = [i for i, (c, r) in enumerate(zip(calls, results)) if (c[5], c[6]) != r]
    report['call_result_mismatches'] = len(bad)
    if bad:
        c = calls[bad[0]]
        report['first_result_mismatch'] = {'call': bad[0], 'ax_in': hex(c[1]), 'port': [hex(c[5]), hex(c[6])],
                                           'original': [hex(v) for v in results[bad[0]]]}
    starts = [(ticks_at(c[0]), c[1] & 0xff) for c in calls if c[1] >> 8 == 0x01]
    report['song_starts'] = [{'tick': t, 'seconds': round(t * DIVISOR / PIT, 3), 'song': s} for t, s in starts]
    # First sounding note: a melodic key-on (B0-B8 bit 5) or a rhythm hit
    # (BD with rhythm mode and an instrument bit).
    report['first_note_tick'] = next((t for t, r, v in native_rest
                                      if (0xb0 <= r <= 0xb8 and v & 0x20) or (r == 0xbd and v & 0x20 and v & 0x1f)), None)
    (out/'open-music-diff.json').write_text(json.dumps(report, indent=1))
    print(json.dumps(report, indent=1))
    ok = report['install_writes_match'] and mismatch is None and not bad
    print('open music: ' + ('IDENTICAL' if ok else 'DIFFERENT'))
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv))
