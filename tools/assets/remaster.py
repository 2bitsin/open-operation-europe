"""A remaster drop-in directory checked against the classic pack, and its manifest written.

A remaster directory (default sources/opeu/assets/packs.embed/remaster) holds PNGs at the classic pack's file names
(`portraits/portrait-005-busch.png`, ...) for the assets it replaces, each an integer number of times the classic
size both ways (4x is the convention; any whole factor works, per image). `pack.py --check-remaster DIR` checks every
image, lists what is still missing per class, and, when nothing is wrong, writes the remaster's pack.json: the classic
entries of the replaced ids with their own `scale` and without index maps. The game embeds the directory and draws it
over the classic pack (assets::OpenGamePack). docs/asset-pack.md, "Remastering", is the workflow.

Errors:
    unknown-image     the file name is no classic image's (an index map, *.index.png, is never a remaster's)
    not-png           the file is not a PNG
    not-rgba          not 8-bit RGBA
    wrong-size        not a whole multiple of the classic size, or not the same multiple both ways
    unexpected-alpha  transparency where the classic image has none (the game shows no background through it)
    no-alpha          fully opaque where the classic image lets the background show
"""
import json
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path

from PIL import Image, UnidentifiedImageError

MANIFEST = 'pack.json'
FORMAT = 'opeu-pack/1'
OPAQUE = 255
INDEX_SUFFIX = '.index.png'


@dataclass(frozen=True)
class Finding:
    check: str
    image: str
    detail: str

    def __str__(self):
        return f'{self.check}: {self.image}: {self.detail}'


@dataclass(frozen=True)
class Checked:
    """What a remaster directory holds against the classic pack."""
    findings: list   # Finding, empty when the remaster is sound
    replaced: dict   # classic id -> the remaster image's scale
    missing: dict    # class -> how many of its images the remaster does not replace yet
    total: dict      # class -> how many images it has

    def summary(self):
        lines = [f'{len(self.replaced)} of {sum(self.total.values())} images replaced, {len(self.findings)} errors']
        lines += [f'  {name}: {self.total[name] - self.missing.get(name, 0)} of {self.total[name]} replaced' for name in sorted(self.total)]
        return '\n'.join(lines + [f'  {finding}' for finding in self.findings])


def classic_entries(classic):
    manifest = json.loads((Path(classic) / MANIFEST).read_text(encoding='utf-8'))
    return {entry['image']: entry for entry in manifest['assets']}


def remaster_images(root):
    root = Path(root)
    return sorted(path.relative_to(root).as_posix() for path in root.rglob('*.png'))


def scale_of(size, entry):
    """The image's whole scale over the classic size, or None."""
    width, height = size
    if width % entry['width'] or height % entry['height'] or width // entry['width'] != height // entry['height']:
        return None
    return width // entry['width'] or None


def check_pixels(path, entry, image):
    """The findings of one remaster image whose file name is a classic image's, and its scale."""
    if image.format != 'PNG':
        return [Finding('not-png', path, f'a {image.format} file')], None
    if image.mode != 'RGBA':
        return [Finding('not-rgba', path, f'mode {image.mode}; save it as 8-bit RGBA')], None
    scale = scale_of(image.size, entry)
    if scale is None:
        return [Finding('wrong-size', path, f'{image.size[0]}x{image.size[1]} is no whole multiple of {entry["width"]}x{entry["height"]}')], None
    lowest = image.getchannel('A').getextrema()[0]
    if not entry['mask'] and lowest != OPAQUE:
        return [Finding('unexpected-alpha', path, f'alpha down to {lowest}; the classic image is opaque')], scale
    if entry['mask'] and lowest == OPAQUE:
        return [Finding('no-alpha', path, 'fully opaque; the classic image lets the background show')], scale
    return [], scale


def check_image(root, path, entries):
    if path.endswith(INDEX_SUFFIX) or path not in entries:
        return [Finding('unknown-image', path, 'no classic image has this file name')], None
    try:
        with Image.open(Path(root) / path) as image:
            image.load()
            return check_pixels(path, entries[path], image)
    except UnidentifiedImageError:
        return [Finding('not-png', path, 'not an image')], None


def check_remaster(root, classic):
    """Every image of the remaster at `root` against the classic pack at `classic`."""
    entries = classic_entries(classic)
    findings, replaced = [], {}
    for path in remaster_images(root):
        found, scale = check_image(root, path, entries)
        findings += found
        if scale is not None and not found:
            replaced[entries[path]['id']] = scale
    total, missing = defaultdict(int), defaultdict(int)
    for entry in entries.values():
        total[entry['class']] += 1
        missing[entry['class']] += entry['id'] not in replaced
    return Checked(findings, replaced, dict(missing), dict(total))


def remaster_entry(entry, scale):
    """A classic entry as the remaster lists it: its own scale, no index map (a remaster image is RGBA only)."""
    return {**{key: value for key, value in entry.items() if key != 'indexed'}, 'scale': scale}


def write_manifest(root, classic, checked):
    """The remaster's pack.json, one line per asset; the pack's scale is the largest of its images'."""
    by_id = {entry['id']: entry for entry in classic_entries(classic).values()}
    rows = [remaster_entry(by_id[asset_id], scale) for asset_id, scale in sorted(checked.replaced.items())]
    head = {'format': FORMAT, 'name': 'remaster', 'scale': max(checked.replaced.values(), default=1), 'classes': sorted({r['class'] for r in rows}),
            'palettes': {}, 'animations': {}, 'fonts': {}}
    compact = lambda value: json.dumps(value, ensure_ascii=False, separators=(', ', ': '))  # noqa: E731
    body = ',\n'.join(f'  {compact(row)}' for row in rows)
    text = '{\n' + ',\n'.join(f' {compact(k)}: {compact(v)}' for k, v in head.items()) + f',\n "assets": [\n{body}\n ]\n}}\n'
    (Path(root) / MANIFEST).write_text(text, encoding='utf-8')
    return Path(root) / MANIFEST
