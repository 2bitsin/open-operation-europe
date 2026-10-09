#!/usr/bin/env python3
"""The port's opening against DOSBox-X golden frames of the original.

Golden frames: port/open/tests/dosbox_open_capture.py (KOEI -> FMDRV ->
OPEN.EXE under DOSBox-X; scenes.json lists every still scene held >= 300 ms,
with a PNG). Native frames: opeu --headless --intro --shots DIR 100.

Timing is not compared frame for frame: DOSBox-X spends emulated time
decoding pictures (a 386 at fixed cycles) that the port does not, so the
port runs ahead by a growing margin; scene holds are FM-tick waits. Each
golden still scene of the first loop is matched to the native frame with
the fewest differing pixels inside a window around its expected time
(golden time minus the KOEI.COM start-up and an allowed lead), in scene
order, and the RGB images are compared pixel for pixel.

Usage: compare_open_frames.py <golden run dir> <native frames dir> <out dir>
       [--loop-end-ms=150700] [--lead-ms=8000]
Writes <out>/frames.json and side-by-side PNGs of every scene
(golden | native | difference in red). Exit 0 when every non-black scene
has an identical native frame.
"""
import json
import sys
from pathlib import Path

import numpy as np
from PIL import Image


def load(path):
    return np.asarray(Image.open(path).convert('RGB'))


def main(argv):
    args = [a for a in argv[1:] if not a.startswith('--')]
    opts = dict(a[2:].split('=', 1) for a in argv[1:] if a.startswith('--') and '=' in a)
    golden, native_dir, out = Path(args[0]), Path(args[1]), Path(args[2])
    loop_end = float(opts.get('loop-end-ms', 150700))
    lead = float(opts.get('lead-ms', 8000))
    out.mkdir(parents=True, exist_ok=True)
    scenes = json.loads((golden/'scenes.json').read_text())
    origin = scenes['meta']['first_640x480_ms']          # OPEN.EXE (KOEI's mode switch) ~ port t=0
    native = sorted((int(p.stem), p) for p in native_dir.glob('*.png'))
    cache = {}

    def frame(i):
        if i not in cache:
            cache[i] = load(native[i][1])
        return cache[i]

    results, cursor = [], 0
    for n, scene in enumerate(s for s in scenes['scenes'] if s['start_ms'] < loop_end):
        g = load(golden/scene['png'])
        t = scene['start_ms'] - origin
        best = None
        for i in range(cursor, len(native)):
            ms = native[i][0]
            if ms < t - lead - 2000:
                continue
            if ms > t + 2000:
                break
            diff = int(np.any(frame(i) != g, axis=2).sum())
            if best is None or diff < best[0]:
                best = (diff, i)
                if diff == 0:
                    break
        entry = {'scene': n, 'golden_ms': round(scene['start_ms'], 1), 'golden_png': scene['png'],
                 'black': scene['black'], 'expected_open_ms': round(t)}
        if best is None:
            entry['native'] = None
        else:
            diff, i = best
            entry.update(native_ms=native[i][0], native_png=native[i][1].name, differing_pixels=diff,
                         lead_ms=round(t - native[i][0]))
            if not scene['black']:
                cursor = i
            nat = frame(i)
            mask = np.any(nat != g, axis=2)
            red = np.zeros_like(g)
            red[mask] = (255, 0, 0)
            side = np.concatenate([g, nat, red], axis=1)
            Image.fromarray(side).save(out/f'scene{n:02d}_{int(scene["start_ms"]):07d}ms.png')
            if diff:
                ys, xs = np.nonzero(mask)
                entry['diff_box'] = [int(xs.min()), int(ys.min()), int(xs.max()), int(ys.max())]
        results.append(entry)
        cache = {k: v for k, v in cache.items() if k >= cursor}

    nonblack = [r for r in results if not r['black']]
    identical = [r for r in nonblack if r.get('differing_pixels') == 0]
    summary = {'scenes': len(results), 'non_black': len(nonblack), 'identical': len(identical),
               'different': [{k: r.get(k) for k in ('scene', 'golden_ms', 'native_ms', 'differing_pixels', 'diff_box')}
                             for r in nonblack if r.get('differing_pixels') != 0]}
    (out/'frames.json').write_text(json.dumps({'summary': summary, 'scenes': results}, indent=1))
    print(json.dumps(summary, indent=1))
    return 0 if len(identical) == len(nonblack) else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv))
