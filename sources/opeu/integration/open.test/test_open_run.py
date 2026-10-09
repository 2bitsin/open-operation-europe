#!/usr/bin/env python3
"""Headless runs of the opening (OPEN.EXE) before MAIN.

  skip   --intro, a key during the prologue's tank picture (10 s): the
         opening aborts to the short title, which waits for song 1 to end
         (as DOSBox-X shows the original doing: exit about 79 s after the
         start); then MAIN draws its keyboard/mouse prompt.
  title  --intro, a key at the full title (140 s): the opening exits within
         two seconds (song 5 / fade / palette fade); then the same prompt,
         in the same palette, as after the skip.
  plain  --headless without --intro: the opening does not run (the smoke
         scripts and their PNGs are unchanged by it).

Run by `./buildutil test opeu-integration` (test_open_run below), or as
  test_open_run.py <opeu> <output dir>
"""
import os
import re
import shutil
import struct
import subprocess
import sys
import zlib
from pathlib import Path

# The original data files (sources/opeu/opeu/assets.embed).
DATA = ['--data', str(Path(__file__).resolve().parents[2] / 'opeu' / 'assets.embed')]


def read_png(path):
    """RGB rows of the port's PNG (8-bit truecolour, filter 0 per row)."""
    data = Path(path).read_bytes()
    pos, idat, size = 8, b'', None
    while pos < len(data):
        length, kind = struct.unpack_from('>I4s', data, pos)
        body = data[pos + 8:pos + 8 + length]
        if kind == b'IHDR':
            size = struct.unpack_from('>II', body)
        elif kind == b'IDAT':
            idat += body
        pos += 12 + length
    w, h = size
    raw = zlib.decompress(idat)
    stride = 1 + 3 * w
    return w, h, [raw[y * stride + 1:(y + 1) * stride] for y in range(h)]


def lit(rows, y0, y1, x0, x1):
    return sum(1 for y in range(y0, y1) for x in range(x0, x1) if any(rows[y][3 * x:3 * x + 3]))


def run(opeu, out, name, args, key_ms=None):
    log = out/f'{name}.log'
    cmd = [str(opeu), '--headless'] + DATA + args + ['--png', str(out/f'{name}.png')]
    if key_ms is not None:
        script = out/f'{name}-input.txt'
        script.write_text(f'{key_ms} key space\n')
        cmd += ['--input', str(script)]
    env = dict(os.environ, OPEU_TRACE='svc', OPEU_LOG=str(log))
    subprocess.run(cmd, check=True, capture_output=True, env=env)
    text = log.read_text()
    m = re.search(r'OPEN\.EXE: exit at (\d+) us', text)
    return 'OPEN.EXE: start' in text, (int(m.group(1)) / 1e6 if m else None)


def main(argv):
    opeu, out = Path(argv[1]), Path(argv[2])
    out.mkdir(parents=True, exist_ok=True)
    failures = []

    def check(cond, what):
        print(('ok    ' if cond else 'FAIL  ') + what)
        if not cond:
            failures.append(what)

    started, exit_s = run(opeu, out, 'skip', ['--intro', '--ms', '82000'], key_ms=10000)
    check(started and exit_s is not None and 78.0 <= exit_s <= 80.5,
          f'skip: the opening ran and exited after song 1 (exit at {exit_s} s; DOSBox-X ~79.2 s)')
    w, h, rows = read_png(out/'skip.png')
    check((w, h) == (640, 480), 'skip: 640x480 frame')
    prompt = lit(rows, 140, 158, 100, 540)
    check(prompt > 500 and lit(rows, 0, 130, 0, 640) == 0 and lit(rows, 170, 480, 0, 640) == 0,
          f'skip: MAIN\'s keyboard/mouse prompt on a black screen ({prompt} lit pixels in its line)')

    started, exit_s = run(opeu, out, 'title', ['--intro', '--ms', '145000'], key_ms=140000)
    check(started and exit_s is not None and 140.0 <= exit_s <= 142.0,
          f'title: a key at the title ends the opening (exit at {exit_s} s)')
    check((out/'title.png').read_bytes() == (out/'skip.png').read_bytes(),
          'title: the same prompt frame as after the skip')

    started, exit_s = run(opeu, out, 'plain', ['--ms', '3000'])
    check(not started, 'plain: --headless without --intro does not run the opening')

    print('open run: ' + ('PASS' if not failures else f'{len(failures)} FAILED'))
    return 1 if failures else 0


def test_open_run(tmp_path):
    opeu = shutil.which('opeu')
    assert opeu, 'opeu is not on PATH (buildutil puts <build>/bin there)'
    assert main(['test_open_run.py', opeu, str(tmp_path)]) == 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
