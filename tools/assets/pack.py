#!/usr/bin/env python3
"""Build the classic asset pack from the shipped data files.

    python3 tools/assets/pack.py [--out DIR] [--archive FILE] [--assets DIR] [--probe PATH] [--only ID ...]
    python3 tools/assets/pack.py --check [--assets DIR] [--probe PATH]
    python3 tools/assets/pack.py --check-remaster [DIR]

Decodes the shipped files (sources/opeu/opeu/assets.embed) through the extract-assets probe, the assets module's
decoders, and writes the pack twice:

    DIR (default sources/opeu/assets/packs.embed/classic)   the directory form, checked in and compiled into the game:
                                                           pack.json, <class>/<readable name>.png (naming.py),
                                                           tables/*.json (the data that is no picture) and
                                                           texts/*.json (the messages and the code table)
    FILE (default _local/packs/classic.opeupack)            the archive form: the same files in one file

`--check` writes nothing: it decodes the pack into a scratch directory and exits non-zero, naming the files, when the
checked-in pack differs from it. `--check-remaster [DIR]` checks a remaster drop-in directory (default
sources/opeu/assets/packs.embed/remaster) against the classic pack, lists what is still missing and, when nothing is
wrong, writes its pack.json (remaster.py).

A pack is a manifest (pack.json: every asset's stable id, class, geometry, palettes, recolouring, animation frames
and fonts, and the palettes) and PNG images: RGBA in the asset's palette, and an 8-bit index map for the assets the
game recolours. `--only` keeps the given ids (and their variants). Both outputs are replaced, and the same inputs
give the same bytes. docs/asset-pack.md describes the format.
"""
import argparse
import json
import shutil
import struct
import sys
import tempfile
from collections import defaultdict
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from extraction import ROOT, SHIPPED, find_probe, run_probe, write_tables  # noqa: E402
from naming import Naming  # noqa: E402

PACKS = ROOT / 'sources' / 'opeu' / 'assets' / 'packs.embed'
CLASSIC = PACKS / 'classic'
REMASTER = PACKS / 'remaster'
ARCHIVE = ROOT / '_local' / 'packs' / 'classic.opeupack'

FORMAT = 'opeu-pack/1'
ARCHIVE_FORMAT = 'opeu-archive/1'
ARCHIVE_MAGIC = b'OPEUPACK'
MANIFEST = 'pack.json'
HOURS = ['main/dawn', 'main/day', 'main/dusk', 'main/night']
ONE_BIT = {'bitmap', 'runs-bitmap'}
SIDE_ZERO = 'side-0'


def recolour(record):
    """Why the game shows the asset in colours other than its image's, or None: the hour's palette (colours 8..15),
    the pen it draws a one-bit shape with, or the opening's palette animation."""
    if record.get('hour_coloured'):
        return 'hour'
    if record['layout'] in ONE_BIT:
        return 'ink'
    if record['palette'].startswith('open/'):
        return 'scene'
    return None


def entry(record, asset_id, image, description):
    """The manifest entry of one image of a probe record (the asset itself or a variant)."""
    out = {
        'id': asset_id,
        'image': image,
        'class': record['group'],
        'role': record['role'],
        'width': record['width'],
        'height': record['height'],
        'scale': 1,
        'palette': record['palette'],
        'palettes': HOURS if record.get('hour_coloured') else [record['palette']],
        'mask': record['mask'],
        'colours': record['colours'],
    }
    if 'animation' in record:
        out['animation'] = record['animation']
        out['frame'] = record['frame']
    out['use'] = record['use']
    out['description'] = description
    return out


def entries(record, naming):
    """(manifest entry, {pack path: extracted image}) for an asset and its side-0 variant. The image is
    <class>/<readable name>.png (naming.py); its index map and its variant sit beside it under the same name."""
    asset_id, group = record['id'], record['group']
    stem = f'{group}/{naming.stem(asset_id)}'
    description = naming.describe(record)
    main = entry(record, asset_id, f'{stem}.png', description + (", in side 1's colours" if record.get('side_coloured') else ''))
    files = {main['image']: record['image']}
    reason = recolour(record)
    if reason:
        main['recolour'] = reason
        main['indexed'] = f'{stem}.index.png'
        files[main['indexed']] = record['indices']
    yield main, files
    if record.get('side_coloured'):
        variant_id = f'{asset_id}/{SIDE_ZERO}'
        variant = entry(record, variant_id, f'{stem}-{SIDE_ZERO}.png', f"{description}, in side 0's colours")
        variant['colours'] = sorted({c & 0xFC | (c & 1) << 1 | c >> 1 & 1 for c in record['colours']})
        variant['variant_of'] = asset_id
        variant['variant'] = SIDE_ZERO
        yield variant, {variant['image']: record['variants'][SIDE_ZERO]}


def sequences(records):
    """Each animation's frame ids in frame order."""
    frames = defaultdict(list)
    for record in records:
        if 'animation' in record:
            frames[record['animation']].append((record['frame'], record['id']))
    return {name: [i for _, i in sorted(found)] for name, found in sorted(frames.items())}


def fonts(records):
    """Each glyph font (the glyphs whose ids share a first segment) with its cell size, the code of its first glyph
    (the number its id ends in: 32 for the text font's space) and its glyph ids in code order."""
    glyphs = defaultdict(list)
    for record in records:
        if record['role'] == 'glyph':
            glyphs[record['id'].split('/')[0]].append(record)
    return {name: {'cell': [found[0]['width'], found[0]['height']], 'first': int(found[0]['id'].rsplit('/', 1)[1]),
                   'glyphs': [r['id'] for r in found]}
            for name, found in sorted(glyphs.items())}


def manifest(listed, graphics, palettes):
    """pack.json: the pack's name and scale, its classes, palettes, animations and fonts, and every entry."""
    return {
        'format': FORMAT,
        'name': 'classic',
        'scale': 1,
        'classes': sorted({a['class'] for a in listed}),
        'palettes': {p['name']: p['colours'] for p in palettes},
        'animations': sequences(graphics),
        'fonts': fonts(graphics),
        'assets': listed,
    }


def compact(value):
    return json.dumps(value, ensure_ascii=False, separators=(', ', ': '))


def encode(pack):
    """pack.json as a table: one line per asset, palette, animation and font, so a diff shows the entries changed."""
    def member(key, value):
        if isinstance(value, list) and value and isinstance(value[0], dict):
            rows = ',\n'.join(f'  {compact(row)}' for row in value)
            return f' {compact(key)}: [\n{rows}\n ]'
        if isinstance(value, dict) and value:
            rows = ',\n'.join(f'  {compact(name)}: {compact(row)}' for name, row in value.items())
            return f' {compact(key)}: {{\n{rows}\n }}'
        return f' {compact(key)}: {compact(value)}'
    return '{\n' + ',\n'.join(member(key, value) for key, value in pack.items()) + '\n}\n'


def write_archive(directory, archive):
    """The directory's files in one file: magic, the directory's length, the directory (JSON), the bytes."""
    files = sorted(p.relative_to(directory).as_posix() for p in directory.rglob('*') if p.is_file())
    paths = [MANIFEST] + [path for path in files if path != MANIFEST]
    contents = [(path, (directory / path).read_bytes()) for path in paths]
    listing, at = [], 0
    for path, data in contents:
        listing.append({'path': path, 'at': at, 'bytes': len(data)})
        at += len(data)
    index = json.dumps({'format': ARCHIVE_FORMAT, 'files': listing}, separators=(',', ':')).encode('utf-8')
    archive.parent.mkdir(parents=True, exist_ok=True)
    archive.write_bytes(ARCHIVE_MAGIC + struct.pack('<I', len(index)) + index + b''.join(d for _, d in contents))


def read_archive(archive):
    """{path: bytes} of an archive written by write_archive."""
    data = Path(archive).read_bytes()
    if data[:len(ARCHIVE_MAGIC)] != ARCHIVE_MAGIC:
        raise ValueError(f'{archive}: not an asset pack archive')
    length = struct.unpack_from('<I', data, len(ARCHIVE_MAGIC))[0]
    start = len(ARCHIVE_MAGIC) + 4
    index = json.loads(data[start:start + length])
    body = start + length
    return {f['path']: data[body + f['at']:body + f['at'] + f['bytes']] for f in index['files']}


def build(out, archive, assets=SHIPPED, probe=None, only=None):
    """Writes the classic pack to the directory `out` and the archive `archive`; returns the entry count per class."""
    out, archive = Path(out), Path(archive)
    out.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=out.parent, prefix=f'.{out.name}-') as staging:
        staging = Path(staging)
        records = run_probe(find_probe(probe), assets, staging)
        graphics = [r for r in records if r['kind'] == 'asset' and (not only or r['id'] in only)]
        missing = set(only or ()) - {r['id'] for r in graphics}
        if missing:
            sys.exit(f'pack.py: no asset {", ".join(sorted(missing))}')
        if out.exists():
            shutil.rmtree(out)
        listed = []
        naming = Naming(records)
        for main, files in (pair for record in graphics for pair in entries(record, naming)):
            listed.append(main)
            for target, source in files.items():
                (out / target).parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(staging / source, out / target)
        if not only:
            write_tables(out, records)
        pack = manifest(listed, graphics, [r for r in records if r['kind'] == 'palette'])
        (out / MANIFEST).write_text(encode(pack), encoding='utf-8')
    write_archive(out, archive)
    counts = defaultdict(int)
    for asset in listed:
        counts[asset['class']] += 1
    return dict(sorted(counts.items()))


def tree(root):
    """{path: bytes} of every file under `root`, paths '/'-separated and relative to it."""
    return {p.relative_to(root).as_posix(): p.read_bytes() for p in sorted(Path(root).rglob('*'))
            if p.is_file() and not p.name.startswith('.')}  # a desktop's .DS_Store is no file of the pack


def differences(expected, found):
    """The paths whose bytes differ between two trees, or that only one holds."""
    return sorted(path for path in expected.keys() | found.keys() if expected.get(path) != found.get(path))


def check(pack=CLASSIC, assets=SHIPPED, probe=None):
    """The files in which the pack at `pack` differs from what the decoders make of `assets` (empty when current)."""
    scratch = ROOT / '_local' / 'packs'
    scratch.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=scratch, prefix='.check-') as staging:
        staging = Path(staging)
        build(staging / 'classic', staging / 'classic.opeupack', assets, probe)
        return differences(tree(staging / 'classic'), tree(pack))


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--out', type=Path, default=CLASSIC)
    parser.add_argument('--archive', type=Path, default=ARCHIVE)
    parser.add_argument('--assets', type=Path, default=SHIPPED)
    parser.add_argument('--probe')
    parser.add_argument('--only', nargs='+', metavar='ID')
    parser.add_argument('--check', action='store_true', help='compare the checked-in pack with the decoders\' output')
    parser.add_argument('--check-remaster', type=Path, nargs='?', const=REMASTER, metavar='DIR',
                        help='check a remaster directory against the classic pack and write its pack.json')
    args = parser.parse_args()
    if args.check_remaster:
        from remaster import check_remaster, write_manifest  # Pillow: only this command needs it
        checked = check_remaster(args.check_remaster, args.out)
        print(checked.summary())
        if checked.findings:
            sys.exit(1)
        print(f'pack.py: wrote {write_manifest(args.check_remaster, args.out, checked)}')
        return
    if args.check:
        stale = check(args.out, args.assets, args.probe)
        if stale:
            sys.exit(f'pack.py: {args.out} is not what the decoders make ({len(stale)} files differ: '
                     f'{", ".join(stale[:10])}); rebuild it with tools/assets/pack.py')
        print(f'pack.py: {args.out} is current')
        return
    counts = build(args.out, args.archive, args.assets, args.probe, set(args.only or ()))
    print(f'pack.py: {sum(counts.values())} assets in {args.out} and {args.archive} '
          f'({args.archive.stat().st_size:,} bytes): ' + ', '.join(f'{c} {n}' for c, n in counts.items()))


if __name__ == '__main__':
    main()
