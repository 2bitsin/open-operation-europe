#!/usr/bin/env python3
"""Contact sheets of a pack, one set of pages per asset class, for reviewing a remaster.

    python3 tools/remaster/contact.py [--pack DIR] [--compare DIR] [--out DIR] [--class NAME ...] [--per-page N]

Every asset is drawn nearest-neighbour at a readable scale (4x for the small ones, less for the large pictures) over
a dark checkerboard where it is transparent (the pen-coloured shapes are light grey), on a grid labelled with its file name, game size, palette, recolouring and the
scale shown. `--compare DIR` puts a remaster beside the pack (the classic pack by default) asset by asset, for the
ids the remaster holds: classic upscaled on the left, the remaster brought to the same size on the right.

Output (replaced on every run), default _local/remaster/sheets/<pack name>/ or .../compare-<remaster name>/:

    <class>-<page>.png    the pages
    index.md              the classes, their pages and the ids on each page
"""
import argparse
import shutil
import sys
from dataclasses import dataclass
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, str(Path(__file__).resolve().parent))
from packs import CLASSIC, REMASTER, ClassicPack, load_rgba, remaster_images  # noqa: E402

SHEETS = REMASTER / 'sheets'
LARGEST_SHOWN = 320
MOST_SCALE = 4
PAGE_WIDTH = 2400
PAGE_HEIGHT = 2400
PER_PAGE = 240
GAP = 8
LABEL_WIDTH = 176
LINE_HEIGHT = 13
CHECKER = 8
CHECKER_LIGHT = (72, 72, 80, 255)
CHECKER_DARK = (40, 40, 48, 255)
PAPER = (255, 255, 255, 255)
INK = (0, 0, 0, 255)
HEADER_HEIGHT = 24


@dataclass(frozen=True)
class Cell:
    """One asset on a sheet: its picture (already scaled and over the checkerboard) and the label under it."""
    asset_id: str
    picture: Image.Image
    label: tuple

    def size(self):
        width = max(self.picture.width, LABEL_WIDTH)
        return width, self.picture.height + LINE_HEIGHT * len(self.label) + GAP


def shown_scale(entry):
    """The scale an asset is shown at: as large as fits LARGEST_SHOWN, at most MOST_SCALE, at least 1."""
    return max(1, min(MOST_SCALE, LARGEST_SHOWN // max(entry['width'], entry['height'])))


def checkerboard(size):
    board = Image.new('RGBA', size, CHECKER_LIGHT)
    draw = ImageDraw.Draw(board)
    for y in range(0, size[1], CHECKER):
        for x in range((y // CHECKER % 2) * CHECKER, size[0], CHECKER * 2):
            draw.rectangle((x, y, x + CHECKER - 1, y + CHECKER - 1), fill=CHECKER_DARK)
    return board


def over_checkerboard(image):
    return Image.alpha_composite(checkerboard(image.size), image)


def resized(image, size):
    """Nearest-neighbour when growing by a whole factor (the pixels stay pixels), Lanczos when shrinking."""
    if image.size == size:
        return image
    filter_ = Image.Resampling.NEAREST if image.width <= size[0] else Image.Resampling.LANCZOS
    return image.resize(size, filter_)


def wrapped(name, width_in_chars, separator):
    lines, line = [], ''
    for part in name.split(separator):
        piece = part if not line else separator + part
        if line and len(line) + len(piece) > width_in_chars:
            lines.append(line)
            piece = separator + part
            line = ''
        line += piece
    return lines + [line]


def label(entry, scale):
    """The image's file name (what a remaster saves under the same name), its size, palette and recolouring."""
    flags = [flag for flag in ('mask' if entry['mask'] else '', entry.get('recolour', ''), entry.get('variant', '')) if flag]
    return (*wrapped(Path(entry['image']).stem, 30, '-'), f"{entry['width']}x{entry['height']} {entry['palette']} at {scale}x",
            ' '.join(flags))


def classic_cell(pack, entry):
    scale = shown_scale(entry)
    shown = resized(pack.image(entry), (entry['width'] * scale, entry['height'] * scale))
    return Cell(entry['id'], over_checkerboard(shown), label(entry, scale))


def side_by_side(left, right):
    pair = Image.new('RGBA', (left.width + GAP + right.width, max(left.height, right.height)), PAPER)
    pair.paste(left, (0, 0))
    pair.paste(right, (left.width + GAP, 0))
    return pair


def compare_cell(pack, entry, remaster_path):
    scale = shown_scale(entry)
    size = (entry['width'] * scale, entry['height'] * scale)
    classic = over_checkerboard(resized(pack.image(entry), size))
    remaster = over_checkerboard(resized(load_rgba(remaster_path), size))
    return Cell(entry['id'], side_by_side(classic, remaster), label(entry, scale) + ('classic | remaster',))


def paginate(cells, per_page):
    """Shelf packing: cells left to right in rows, a new page when a page is full or holds per_page cells."""
    pages, page, row, x, y, row_height = [], [], [], 0, HEADER_HEIGHT, 0
    for cell in cells:
        width, height = cell.size()
        if row and x + width > PAGE_WIDTH:
            x, y, row_height, row = 0, y + row_height + GAP, 0, []
        if page and (y + height > PAGE_HEIGHT or len(page) == per_page):
            pages.append(page)
            page, x, y, row_height = [], 0, HEADER_HEIGHT, 0
        page.append((cell, (x, y)))
        row.append(cell)
        x, row_height = x + width + GAP, max(row_height, height)
    return pages + [page] if page else pages


def draw_page(placed, title):
    width = max(at[0] + cell.size()[0] for cell, at in placed)
    height = max(at[1] + cell.size()[1] for cell, at in placed)
    page = Image.new('RGBA', (width + GAP, height + GAP), PAPER)
    draw = ImageDraw.Draw(page)
    font = ImageFont.load_default()
    draw.text((GAP, GAP // 2), title, fill=INK, font=font)
    for cell, (x, y) in placed:
        page.paste(cell.picture, (x + GAP, y))
        for number, line in enumerate(cell.label):
            draw.text((x + GAP, y + cell.picture.height + 2 + number * LINE_HEIGHT), line, fill=INK, font=font)
    return page.convert('RGB')


@dataclass(frozen=True)
class SheetSet:
    """Where a set of sheets goes and what it is called; writes the pages and the index."""
    out: Path
    name: str

    def write_class(self, asset_class, cells, per_page):
        pages = paginate(cells, per_page)
        written = []
        for number, placed in enumerate(pages, start=1):
            path = self.out / f'{asset_class}-{number:02}.png'
            draw_page(placed, f'{self.name}: {asset_class}, page {number} of {len(pages)}').save(path, optimize=True)
            written.append((path, [cell.asset_id for cell, _ in placed]))
        return written

    def write_index(self, written):
        lines = [f'# Contact sheets: {self.name}', '']
        for asset_class, pages in written.items():
            count = sum(len(ids) for _, ids in pages)
            lines += [f'## {asset_class} ({count} assets, {len(pages)} pages)', '']
            lines += [f'- [{path.name}]({path.name}): {ids[0]} .. {ids[-1]} ({len(ids)})' for path, ids in pages]
            lines.append('')
        (self.out / 'index.md').write_text('\n'.join(lines), encoding='utf-8')


def classic_cells(pack, asset_class):
    return [classic_cell(pack, entry) for entry in pack.in_class(asset_class)]


def compare_cells(pack, asset_class, remaster):
    images = remaster_images(remaster)
    return [compare_cell(pack, entry, images[entry['image']]) for entry in pack.in_class(asset_class) if entry['image'] in images]


def arguments():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--pack', type=Path, default=CLASSIC, help='the pack shown (default the classic pack)')
    parser.add_argument('--compare', type=Path, help='a remaster directory to show beside the pack')
    parser.add_argument('--out', type=Path, help='the output directory (replaced)')
    parser.add_argument('--class', dest='classes', nargs='+', help='only these classes')
    parser.add_argument('--per-page', type=int, default=PER_PAGE, help='most assets on a page')
    return parser.parse_args()


def sheets(pack, compare, out, classes, per_page):
    """Writes the sheets of the given classes; returns {class: [(page path, ids on it)]} for the classes with any."""
    shutil.rmtree(out, ignore_errors=True)
    out.mkdir(parents=True)
    name = f'{pack.manifest["name"]} vs {compare.name}' if compare else pack.manifest['name']
    sheet_set = SheetSet(out, name)
    written = {}
    for asset_class in classes or pack.classes():
        cells = compare_cells(pack, asset_class, compare) if compare else classic_cells(pack, asset_class)
        if cells:
            written[asset_class] = sheet_set.write_class(asset_class, cells, per_page)
    sheet_set.write_index(written)
    return written


def main():
    args = arguments()
    pack = ClassicPack.open(args.pack)
    out = args.out or SHEETS / (f'compare-{args.compare.name}' if args.compare else pack.manifest['name'])
    written = sheets(pack, args.compare, out, args.classes, args.per_page)
    for asset_class, pages in written.items():
        print(f'{asset_class}: {sum(len(ids) for _, ids in pages)} assets on {len(pages)} pages')
    print(f'{out / "index.md"}')


if __name__ == '__main__':
    main()
