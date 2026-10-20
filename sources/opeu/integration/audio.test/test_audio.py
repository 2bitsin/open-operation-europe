#!/usr/bin/env python3
"""Headless audio check of the native port: the game's music must sound.

Runs `opeu --headless` with a scripted input log, writing the audio to a WAV
file (--wav), every OPL2 register write with its frame (--opl-log) and the
FM driver calls with their frames (OPEU_TRACE=fm). Then checks:

  1. the game started songs (INT 66h AH=01h with AL != 0);
  2. after each song start the driver keys on OPL channels within one
     second (the first music tick comes after at most 60/120*... a few
     tempo ticks), and keeps doing so while the song plays;
  3. the WAV is audible after the first key-on: at least 60% of the
     100 ms windows have an RMS above -50 dBFS, and it is not clipped flat;
  4. before the first song start the WAV is silent.

Run by `./buildutil test opeu-integration` (test_audio below, on the smoke
script), or as
  test_audio.py <opeu> <input script> <output dir> [<ms>]
"""
import os
import re
import shutil
import struct
import subprocess
import sys
from pathlib import Path

OPEU = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(OPEU.parents[1] / 'tools' / 'testing'))
import game_data  # noqa: E402

RATE = 49716


def read_wav(path):
    data = Path(path).read_bytes()
    assert data[:4] == b'RIFF' and data[8:16] == b'WAVEfmt ', 'not a WAV file'
    channels, rate, bits = struct.unpack_from('<H', data, 22)[0], struct.unpack_from('<I', data, 24)[0], \
        struct.unpack_from('<H', data, 34)[0]
    assert (channels, rate, bits) == (2, RATE, 16), (channels, rate, bits)
    size = struct.unpack_from('<I', data, 40)[0]
    pcm = data[44:44 + size]
    left = struct.unpack(f'<{len(pcm) // 2}h', pcm)[0::2]
    return left


def windows(samples, start, end, size):
    out = []
    for at in range(start, min(end, len(samples)) - size + 1, size):
        chunk = samples[at:at + size]
        mean = sum(chunk) / size
        rms = (sum((s - mean) ** 2 for s in chunk) / size) ** 0.5
        out.append(rms)
    return out


def main(argv):
    if len(argv) < 4:
        raise SystemExit(__doc__)
    opeu, script, outdir = argv[1], argv[2], Path(argv[3])
    ms = int(argv[4]) if len(argv) > 4 else 30000
    outdir.mkdir(parents=True, exist_ok=True)
    wav, log, trace = outdir / 'audio.wav', outdir / 'opl.log', outdir / 'fm-trace.log'
    env = dict(os.environ, OPEU_TRACE='fm', OPEU_LOG=str(trace))
    subprocess.run([opeu, '--headless', '--saves', str(outdir / 'saves'), '--ms', str(ms), '--input', script, '--wav', str(wav),
                    '--opl-log', str(log), '--png', str(outdir / 'last.png')], check=True, env=env,
                   stdout=subprocess.DEVNULL)

    starts = []
    for line in trace.read_text().splitlines():
        m = re.search(r'frame (\d+) int66 ax=(\w{4})', line)
        if m and m.group(2)[:2] == '01' and m.group(2)[2:] != '00':
            starts.append((int(m.group(1)), int(m.group(2)[2:], 16)))
    key_ons = []
    for line in log.read_text().splitlines():
        if line.startswith('#'):
            continue
        frame, reg, value = line.split()
        reg, value = int(reg, 16), int(value, 16)
        if (0xb0 <= reg <= 0xb8 and value & 0x20) or (reg == 0xbd and value & 0x1f):
            key_ons.append(int(frame))
    samples = read_wav(wav)

    problems = []
    if not starts:
        problems.append('the game started no song (INT 66h AH=01h AL!=0)')
    for frame, song in starts:
        later = [k for k in key_ons if k >= frame]
        if not later or later[0] - frame > RATE:
            problems.append(f'song {song} started at {frame / RATE:.3f} s but no key-on within 1 s')
    first = key_ons[0] if key_ons else len(samples)
    before = windows(samples, 0, starts[0][0] if starts else 0, RATE // 10)
    after = windows(samples, first, len(samples), RATE // 10)
    audible = [r for r in after if r > 32768 * 10 ** (-50 / 20)]
    loud = max(after) if after else 0
    peak = max((abs(s) for s in samples[first:]), default=0)
    if before and max(before) > 1.0:
        problems.append(f'sound before the first song start (RMS {max(before):.1f})')
    if not after or len(audible) < 0.6 * len(after):
        problems.append(f'only {len(audible)} of {len(after)} windows after the first key-on are audible')
    if peak >= 32767:
        problems.append('the output clips')
    print(f'song starts: {[(round(f / RATE, 3), s) for f, s in starts]}')
    print(f'OPL key-on writes: {len(key_ons)}, first at {first / RATE:.3f} s')
    print(f'WAV: {len(samples) / RATE:.1f} s, audible windows after the first key-on: '
          f'{len(audible)}/{len(after)}, max window RMS {loud:.0f}, peak {peak}')
    if problems:
        print('AUDIO CHECK FAILED:\n  ' + '\n  '.join(problems))
        return 1
    print('audio check passed')
    return 0


def test_audio(tmp_path):
    game_data.require_data()  # without the game's data the check skips; opeu finds OPEU_DATA_DIR's itself
    opeu = shutil.which('opeu')
    assert opeu, 'opeu is not on PATH (buildutil puts <build>/bin there)'
    script = OPEU / 'integration' / 'fixtures' / 'smoke-input.txt'
    assert main(['test_audio.py', opeu, str(script), str(tmp_path)]) == 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
