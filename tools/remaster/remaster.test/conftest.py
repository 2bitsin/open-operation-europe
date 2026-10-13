"""A tiny classic pack and its remaster, drawn by the tests: a map tile, a glyph, a unit icon with its side-0
variant, a portrait and a two-frame blast animation. The remaster is the classic upscaled nearest-neighbour, the
glyph white, so it passes every check; each test then breaks one thing."""
import json
from pathlib import Path

import pytest
from PIL import Image

SCALE = 4
GREY = (170, 170, 170, 255)
CLEAR = (0, 0, 0, 0)
GREEN = (40, 140, 60, 255)
BLUE = (30, 60, 160, 255)
RED = (200, 40, 40, 255)
TAN = (210, 170, 110, 255)
FILLED = {(x, y) for x in range(1, 3) for y in range(1, 4)}
PLUS = {(1, 0), (0, 1), (1, 1), (2, 1), (1, 2)}


def entry(asset_id, asset_class, size, mask, **extra):
    return {'id': asset_id, 'class': asset_class, 'role': 'sprite', 'width': size[0], 'height': size[1], 'palette': 'main/day',
            'palettes': ['main/day'], 'mask': mask, 'colours': [0], 'image': f'{asset_class}/{asset_id}.png', 'use': 'a test asset', **extra}


ENTRIES = [
    entry('view-tile/map-0/00', 'tiles', (4, 4), False, recolour='hour', indexed='tiles/view-tile/map-0/00.index.png'),
    entry('game-glyph/0000', 'glyphs', (8, 8), True, recolour='ink', indexed='glyphs/game-glyph/0000.index.png'),
    entry('battle-unit/class-00/facing-0', 'battle', (4, 4), True),
    entry('battle-unit/class-00/facing-0/side-0', 'battle', (4, 4), True, variant_of='battle-unit/class-00/facing-0', variant='side-0'),
    entry('portrait/000', 'portraits', (6, 4), False),
    entry('battle-blast/large/0', 'battle', (4, 4), True, animation='battle-blast/large', frame=0),
    entry('battle-blast/large/1', 'battle', (4, 4), True, animation='battle-blast/large', frame=1),
]


def drawn(size, colour, pixels=None, background=CLEAR):
    image = Image.new('RGBA', size, background if pixels is not None else colour)
    for pixel in pixels or ():
        image.putpixel(pixel, colour)
    return image


CLASSIC_IMAGES = {
    'view-tile/map-0/00':                   drawn((4, 4), GREEN),
    'game-glyph/0000':                      drawn((8, 8), GREY, {(x + 2, y + 2) for x, y in PLUS}),
    'battle-unit/class-00/facing-0':        drawn((4, 4), BLUE, FILLED),
    'battle-unit/class-00/facing-0/side-0': drawn((4, 4), RED, FILLED),
    'portrait/000':                         drawn((6, 4), TAN),
    'battle-blast/large/0':                 drawn((4, 4), RED, FILLED),
    'battle-blast/large/1':                 drawn((4, 4), RED, PLUS),
}


def write_classic(root):
    for item in ENTRIES:
        path = root / item['image']
        path.parent.mkdir(parents=True, exist_ok=True)
        CLASSIC_IMAGES[item['id']].save(path)
    manifest = {'format': 'opeu-pack/1', 'name': 'classic', 'scale': 1, 'classes': sorted({e['class'] for e in ENTRIES}),
                'palettes': {}, 'animations': {'battle-blast/large': ['battle-blast/large/0', 'battle-blast/large/1']}, 'fonts': {},
                'assets': ENTRIES}
    (root / 'pack.json').write_text(json.dumps(manifest), encoding='utf-8')
    return root


def remastered(item):
    image = CLASSIC_IMAGES[item['id']].resize((item['width'] * SCALE, item['height'] * SCALE), Image.Resampling.NEAREST)
    if item.get('recolour') == 'ink':
        image = Image.merge('RGBA', (*Image.new('RGB', image.size, (255, 255, 255)).split(), image.getchannel('A')))
    return image


def write_remaster(root):
    for item in ENTRIES:
        path = root / item['image']
        path.parent.mkdir(parents=True, exist_ok=True)
        remastered(item).save(path)
    return root


@pytest.fixture
def classic(tmp_path):
    return write_classic(tmp_path / 'classic')


@pytest.fixture
def remaster(tmp_path):
    return write_remaster(tmp_path / 'remaster')


@pytest.fixture
def image_of(remaster):
    def path(asset_id):
        return remaster / next(item['image'] for item in ENTRIES if item['id'] == asset_id)
    return path


@pytest.fixture
def root():
    return Path(__file__).resolve().parents[3]
