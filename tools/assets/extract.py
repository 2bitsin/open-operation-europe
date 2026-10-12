#!/usr/bin/env python3
"""Extract every shipped asset of Operation Europe to open formats.

    python3 tools/assets/extract.py [--out DIR] [--assets DIR] [--probe PATH]

Runs the extract-assets probe (sources/opeu/probes/extract-assets) over the shipped data files
(sources/opeu/opeu/assets.embed) and writes, under DIR (default _local/assets-extracted):

    images/<id>.png            the asset in its palette, transparent where its mask keeps the background
    images/<id>.index.png      its palette indices as grey levels (255 = transparent)
    images/<id>.side-0.png     unit icons only: as side 0 draws them
    manifest.json              the format version, the source files with their SHA-256, the manifests below
    assets/<group>.json        one per group: every asset's id, geometry, layout, colour roles, mask, frames
    palettes.json              the 121 classic palettes
    text/scenarios.json        the scenario descriptions and victory conditions, UTF-8
    tables/setups.json         the seven scenario set-ups
    tables/maps.json           the six maps' cell, tile and battle-field layers
    tables/battle-fields.json  the 537 battle fields
    tables/epilogue-token.json ENDGRP.NPK's one-shot token

The output directory is replaced, so a run is idempotent: the same inputs give the same bytes.
docs/asset-formats.md describes the formats and the manifest.
"""
import argparse
import hashlib
import shutil
import sys
from collections import defaultdict
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from extraction import ROOT, SHIPPED, find_probe, run_probe, write_json  # noqa: E402

FORMAT = 'opeu-assets/1'


def group_assets(records):
    groups = defaultdict(list)
    for record in records:
        record = dict(record)
        del record['kind']
        groups[record.pop('group')].append(record)
    manifests = {}
    for group, assets in groups.items():
        animations = defaultdict(list)
        for asset in assets:
            if 'animation' in asset:
                animations[asset['animation']].append((asset['frame'], asset['id']))
        manifests[group] = {
            'format': FORMAT,
            'group': group,
            'assets': assets,
            'animations': {name: [i for _, i in sorted(frames)] for name, frames in sorted(animations.items())},
        }
    return manifests


def strip_kind(records):
    return [{k: v for k, v in r.items() if k != 'kind'} for r in records]


def extract(out, assets, probe):
    out = Path(out)
    if out.exists():
        shutil.rmtree(out)
    out.mkdir(parents=True)
    records = run_probe(probe, assets, out)
    by_kind = defaultdict(list)
    for record in records:
        by_kind[record['kind']].append(record)

    manifests = group_assets(by_kind['asset'])
    for group, manifest in manifests.items():
        write_json(out / 'assets' / f'{group}.json', manifest)
    write_json(out / 'palettes.json', {'format': FORMAT, 'palettes': strip_kind(by_kind['palette'])})
    write_json(out / 'text' / 'scenarios.json', {'format': FORMAT, 'scenarios': strip_kind(by_kind['scenario-text'])})
    write_json(out / 'tables' / 'setups.json', {'format': FORMAT, 'setups': strip_kind(by_kind['setup'])})
    write_json(out / 'tables' / 'maps.json', {'format': FORMAT, 'maps': strip_kind(by_kind['map'])})
    write_json(out / 'tables' / 'battle-fields.json', {'format': FORMAT, 'fields': strip_kind(by_kind['battle-field'])})
    write_json(out / 'tables' / 'epilogue-token.json', {'format': FORMAT, **strip_kind(by_kind['epilogue-token'])[0]})

    files = {p.name: {'bytes': p.stat().st_size, 'sha256': hashlib.sha256(p.read_bytes()).hexdigest()}
             for p in sorted(Path(assets).iterdir()) if p.is_file()}
    write_json(out / 'manifest.json', {
        'format': FORMAT,
        'files': files,
        'assets': {group: f'assets/{group}.json' for group in sorted(manifests)},
        'counts': {group: len(m['assets']) for group, m in sorted(manifests.items())},
        'palettes': 'palettes.json',
        'text': ['text/scenarios.json'],
        'tables': ['tables/setups.json', 'tables/maps.json', 'tables/battle-fields.json',
                   'tables/epilogue-token.json'],
    })
    return {group: len(m['assets']) for group, m in manifests.items()}


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--out', default=ROOT / '_local' / 'assets-extracted')
    parser.add_argument('--assets', default=SHIPPED)
    parser.add_argument('--probe')
    args = parser.parse_args()
    counts = extract(args.out, args.assets, find_probe(args.probe))
    print(f'extract.py: {sum(counts.values())} assets in {args.out}: ' +
          ', '.join(f'{group} {n}' for group, n in sorted(counts.items())))


if __name__ == '__main__':
    main()
