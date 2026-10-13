"""validate.py: a remaster that keeps every rule passes and gets a loadable manifest; each kind of fault is found."""
import json
import subprocess
import sys
from pathlib import Path

from PIL import Image

import validate
from conftest import ENTRIES, SCALE
from packs import ClassicPack
from templates import Templates

TOOL = Path(validate.__file__)


def findings(classic, remaster, scale=SCALE):
    report = validate.validate(validate.Remaster.read(remaster, ClassicPack.open(classic), scale), Templates.load())
    return {(f.check, f.asset) for f in report.findings}, report


def checks(classic, remaster):
    return {check for check, _ in findings(classic, remaster)[0]}


def opened(path):
    with Image.open(path) as image:
        return image.convert('RGBA')


def test_a_faithful_remaster_passes(classic, remaster):
    found, report = findings(classic, remaster)
    assert report.errors() == []
    assert sum(report.replaced.values()) == len(ENTRIES)


def test_fix_manifest_lists_the_replaced_ids_without_index_maps(classic, remaster, tmp_path):
    result = subprocess.run([sys.executable, TOOL, remaster, '--classic', classic, '--report', tmp_path / 'reports', '--fix-manifest'],
                            capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr
    manifest = json.loads((remaster / 'pack.json').read_text(encoding='utf-8'))
    assert manifest['format'] == 'opeu-pack/1' and manifest['scale'] == SCALE and manifest['name'] == 'remaster'
    assert [a['id'] for a in manifest['assets']] == sorted(e['id'] for e in ENTRIES)
    assert all('indexed' not in a for a in manifest['assets'])
    assert {'palettes', 'animations', 'fonts', 'classes'} <= manifest.keys()
    assert json.loads((tmp_path / 'reports' / 'remaster.json').read_text(encoding='utf-8'))['errors'] == 0


def test_a_failing_remaster_exits_non_zero_and_writes_no_manifest(classic, remaster, image_of, tmp_path):
    opened(image_of('portrait/000')).resize((7, 7)).save(image_of('portrait/000'))
    result = subprocess.run([sys.executable, TOOL, remaster, '--classic', classic, '--report', tmp_path, '--fix-manifest'], capture_output=True)
    assert result.returncode == 1
    assert not (remaster / 'pack.json').exists()
    assert 'wrong-size' in (tmp_path / 'remaster.md').read_text(encoding='utf-8')


def test_a_wrong_size_is_found(classic, remaster, image_of):
    opened(image_of('portrait/000')).resize((6 * 3, 4 * 3)).save(image_of('portrait/000'))
    assert ('wrong-size', 'portrait/000') in findings(classic, remaster)[0]


def test_a_scale_other_than_the_images_is_found(classic, remaster):
    assert 'wrong-size' in {check for check, _ in findings(classic, remaster, scale=2)[0]}


def test_an_rgb_image_is_found(classic, remaster, image_of):
    opened(image_of('portrait/000')).convert('RGB').save(image_of('portrait/000'))
    assert ('not-rgba', 'portrait/000') in findings(classic, remaster)[0]


def test_a_jpeg_under_a_png_name_is_found(classic, remaster, image_of):
    opened(image_of('portrait/000')).convert('RGB').save(image_of('portrait/000'), format='JPEG')
    assert ('not-png', 'portrait/000') in findings(classic, remaster)[0]


def test_transparency_on_an_opaque_asset_is_found(classic, remaster, image_of):
    image = opened(image_of('view-tile/map-0/00'))
    image.putpixel((0, 0), (0, 0, 0, 0))
    image.save(image_of('view-tile/map-0/00'))
    assert ('unexpected-alpha', 'view-tile/map-0/00') in findings(classic, remaster)[0]


def test_a_mask_that_is_not_the_classic_silhouette_is_found(classic, remaster, image_of):
    Image.new('RGBA', (16, 16), (30, 60, 160, 255)).save(image_of('battle-blast/large/0'))
    assert ('mask-mismatch', 'battle-blast/large/0') in findings(classic, remaster)[0]


def test_colour_in_a_pen_coloured_glyph_is_found(classic, remaster, image_of):
    image = opened(image_of('game-glyph/0000'))
    alpha = image.getchannel('A')
    Image.merge('RGBA', (*Image.new('RGB', image.size, (255, 0, 0)).split(), alpha)).save(image_of('game-glyph/0000'))
    assert ('not-neutral', 'game-glyph/0000') in findings(classic, remaster)[0]


def test_a_glyph_outside_its_cell_extent_is_found(classic, remaster, image_of):
    image = opened(image_of('game-glyph/0000'))
    image.paste((255, 255, 255, 255), (7 * SCALE, 7 * SCALE, 8 * SCALE, 8 * SCALE))
    image.save(image_of('game-glyph/0000'))
    assert ('off-grid', 'game-glyph/0000') in findings(classic, remaster)[0]


def test_an_image_of_no_classic_asset_is_found(classic, remaster):
    (remaster / 'portraits' / 'portrait').mkdir(parents=True, exist_ok=True)
    Image.new('RGBA', (24, 16)).save(remaster / 'portraits' / 'portrait' / '999.png')
    assert ('unknown-image', 'portraits/portrait/999.png') in findings(classic, remaster)[0]


def test_an_index_map_is_found(classic, remaster):
    Image.new('L', (16, 16)).save(remaster / 'tiles' / 'view-tile' / 'map-0' / '00.index.png')
    assert ('index-map', 'tiles/view-tile/map-0/00.index.png') in findings(classic, remaster)[0]


def test_an_asset_without_its_variant_is_found(classic, remaster, image_of):
    image_of('battle-unit/class-00/facing-0/side-0').unlink()
    assert ('incomplete-variants', 'battle-unit/class-00/facing-0') in findings(classic, remaster)[0]


def test_a_variant_without_its_asset_is_found(classic, remaster, image_of):
    image_of('battle-unit/class-00/facing-0').unlink()
    assert ('incomplete-variants', 'battle-unit/class-00/facing-0') in findings(classic, remaster)[0]


def test_a_variant_with_another_silhouette_is_found(classic, remaster, image_of):
    image = opened(image_of('battle-unit/class-00/facing-0/side-0'))
    image.paste((200, 40, 40, 255), (0, 0, 4, 16))
    image.save(image_of('battle-unit/class-00/facing-0/side-0'))
    assert ('variant-silhouette', 'battle-unit/class-00/facing-0/side-0') in findings(classic, remaster)[0]


def test_a_stated_scale_that_differs_is_found(classic, remaster):
    (remaster / 'pack.json').write_text(json.dumps({'format': 'opeu-pack/1', 'scale': 2}), encoding='utf-8')
    assert ('scale-mismatch', 'pack.json') in findings(classic, remaster)[0]


def test_a_night_image_for_a_day_tile_is_a_warning(classic, remaster, image_of):
    Image.new('RGBA', (16, 16), (0, 0, 10, 255)).save(image_of('view-tile/map-0/00'))
    found, report = findings(classic, remaster)
    assert ('not-daylight', 'view-tile/map-0/00') in found and report.errors() == []


def test_half_an_animation_is_a_warning(classic, remaster, image_of):
    image_of('battle-blast/large/1').unlink()
    found, report = findings(classic, remaster)
    assert ('partial-animation', 'battle-blast/large') in found and report.errors() == []
