"""tools/assets/pack.py: the classic pack is complete, byte-stable in both forms, the checked-in pack the game embeds
is what the decoders make (pack.py --check), every image has a readable name and a description, the C++ loader reads
every image of the whole pack as the decoders draw it, the renderer bakes every texture of it exactly as from the
decoders, and --check-remaster checks a drop-in directory and writes its manifest."""
import hashlib
import importlib.util
import json
import os
import re
import shutil
import subprocess
from pathlib import Path

import pytest
from PIL import Image

HERE = Path(__file__).resolve().parents[1]
ROOT = HERE.parents[1]
SPEC = importlib.util.spec_from_file_location('pack', HERE / 'pack.py')
pack = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(pack)
REMASTER_SPEC = importlib.util.spec_from_file_location('remaster', HERE / 'remaster.py')
remaster_tool = importlib.util.module_from_spec(REMASTER_SPEC)
REMASTER_SPEC.loader.exec_module(remaster_tool)

MODULE = ROOT / 'sources' / 'opeu' / 'assets'
RENDER = ROOT / 'sources' / 'opeu' / 'render'
REMASTER_SAMPLE = MODULE / 'assets.test' / 'remaster-sample'
PROGRAM_GLYPHS = 96 + 11 + 11 + 2  # the program's text font, digits, sideways digits and markers
COUNTS = {'battle': 244 + 44, 'dialogs': 6, 'glyphs': 1298 + PROGRAM_GLYPHS, 'logos': 4, 'masks': 20, 'pictures': 95,
          'portraits': 120, 'silhouettes': 159, 'tiles': 1152}
FONTS = {'digit-glyph': ([6, 6], 0, 11), 'ending-glyph': ([16, 14], 0, 218), 'game-glyph': ([16, 14], 0, 1080),
         'marker-glyph': ([8, 8], 0, 2), 'sideways-digit-glyph': ([8, 5], 0, 11), 'text-glyph': ([8, 16], 32, 96)}


def tree(root):
    return {p.relative_to(root).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(root.rglob('*')) if p.is_file()}


def digests(files):
    return {path: hashlib.sha256(data).hexdigest() for path, data in files.items()}


def module_tests(module):
    """A module's gtest executable of this build (BUILDUTIL_BUILD_DIR), else the newest one built."""
    name = f'opeu-{module}-tests'
    build = os.environ.get('BUILDUTIL_BUILD_DIR')
    found = [Path(build) / 'sources' / 'opeu' / module / name] if build else []
    found += sorted(ROOT.glob(f'_build/*/sources/opeu/{module}/{name}'), key=lambda p: p.stat().st_mtime, reverse=True)
    present = [p for p in found if p.is_file()]
    assert present, f'no {name}; build with ./buildutil test'
    return present[0]


def run_gtest(module, directory, pattern, pack_path):
    """The module's gtest cases matching `pattern`, run over the pack at `pack_path`."""
    result = subprocess.run([str(module_tests(module)), f'--gtest_filter={pattern}'], cwd=directory, capture_output=True, text=True,
                            env={**os.environ, 'OPEU_CLASSIC_PACK': str(pack_path)}, check=False)
    assert result.returncode == 0, result.stdout[-4000:] + result.stderr[-2000:]
    return result.stdout


@pytest.fixture(scope='module')
def classic(tmp_path_factory):
    out = tmp_path_factory.mktemp('packs')
    counts = pack.build(out / 'classic', out / 'classic.opeupack')
    return out / 'classic', out / 'classic.opeupack', counts


def test_the_classic_pack_holds_every_asset(classic):
    directory, _, counts = classic
    assert counts == COUNTS
    manifest = json.loads((directory / 'pack.json').read_text(encoding='utf-8'))
    assert manifest['format'] == 'opeu-pack/1' and manifest['name'] == 'classic' and manifest['scale'] == 1
    assert len(manifest['palettes']) == 121
    ids = [a['id'] for a in manifest['assets']]
    assert len(ids) == len(set(ids)) == sum(COUNTS.values())
    files = tree(directory)
    for asset in manifest['assets']:
        assert asset['image'] in files, asset['id']
        assert ('indexed' in asset) == ('recolour' in asset), asset['id']
        assert 'indexed' not in asset or asset['indexed'] in files, asset['id']
        assert asset['palette'] in manifest['palettes'] and set(asset['palettes']) <= set(manifest['palettes'])
    assert sum(1 for a in manifest['assets'] if 'variant_of' in a) == 44
    assert {name: (font['cell'], font['first'], len(font['glyphs'])) for name, font in manifest['fonts'].items()} == FONTS
    assert manifest['fonts']['text-glyph']['glyphs'][33] == 'text-glyph/065'
    assert manifest['animations']['opening-blast-mask'] == [f'opening-blast-mask/{k}' for k in range(6)]


def test_the_build_is_byte_stable(classic, tmp_path):
    directory, archive, _ = classic
    again, again_archive = tmp_path / 'classic', tmp_path / 'classic.opeupack'
    pack.build(again, again_archive)
    pack.build(again, again_archive)  # over its own earlier output
    assert tree(again) == tree(directory)
    assert again_archive.read_bytes() == archive.read_bytes()


def test_the_archive_holds_the_directory(classic):
    directory, archive, _ = classic
    files = pack.read_archive(archive)
    assert next(iter(files)) == 'pack.json'
    assert digests(files) == tree(directory)


def test_the_checked_in_pack_is_what_the_decoders_make(classic):
    directory, _, _ = classic
    stale = pack.differences(pack.tree(directory), pack.tree(pack.CLASSIC))
    assert not stale, f'{len(stale)} files differ ({stale[:5]}); rebuild with python3 tools/assets/pack.py'


def test_every_image_has_a_readable_unique_name_and_a_description(classic):
    directory, _, _ = classic
    manifest = json.loads((directory / 'pack.json').read_text(encoding='utf-8'))
    images = [a['image'] for a in manifest['assets']]
    assert len({i.lower() for i in images}) == len(images)  # unique on a case-blind file system too
    assert all(re.fullmatch(r'[a-z]+/[a-z0-9-]+\.png', i) for i in images)
    assert all(a['description'] and a['scale'] == 1 for a in manifest['assets'])
    by_id = {a['id']: a for a in manifest['assets']}
    assert by_id['portrait/010']['image'] == 'portraits/portrait-010-rommel.png'
    assert by_id['silhouette/028']['image'] == 'silhouettes/silhouette-028-tiger-i.png'
    assert by_id['text-glyph/065']['image'] == 'glyphs/text-glyph-065-capital-a.png'
    assert by_id['view-tile/map-4/17']['image'].startswith('tiles/view-tile-map-4-17-ardennes-1944-')


def test_a_remaster_is_checked_and_listed(tmp_path):
    remaster = tmp_path / 'remaster'
    shutil.copytree(REMASTER_SAMPLE, remaster)
    (remaster / 'pack.json').unlink()
    checked = remaster_tool.check_remaster(remaster, pack.CLASSIC)
    assert not checked.findings and checked.replaced == {'portrait/118': 4, 'dialog/yes': 2}
    assert checked.missing['portraits'] == 119 and checked.total['portraits'] == 120
    remaster_tool.write_manifest(remaster, pack.CLASSIC, checked)
    assert (remaster / 'pack.json').read_bytes() == (REMASTER_SAMPLE / 'pack.json').read_bytes()


def test_a_remaster_image_of_the_wrong_size_or_name_is_refused(tmp_path):
    remaster = tmp_path / 'remaster'
    (remaster / 'portraits').mkdir(parents=True)
    Image.new('RGBA', (100, 100)).save(remaster / 'portraits' / 'portrait-010-rommel.png')
    Image.new('RGBA', (64, 80)).save(remaster / 'portraits' / 'rommel.png')
    Image.new('RGB', (128, 160)).save(remaster / 'portraits' / 'portrait-005-busch.png')
    checks = sorted(f.check for f in remaster_tool.check_remaster(remaster, pack.CLASSIC).findings)
    assert checks == ['not-rgba', 'unknown-image', 'wrong-size']


def test_the_renderers_sample_is_current(tmp_path):
    sample = RENDER / 'render.test' / 'packs' / 'classic.opeupack'
    listed = json.loads(pack.read_archive(sample)['pack.json'])['assets']
    pack.build(tmp_path / 'render', tmp_path / 'render.opeupack', only={a['id'] for a in listed if 'variant_of' not in a})
    assert (tmp_path / 'render.opeupack').read_bytes() == sample.read_bytes(), 'regenerate it: see render.test/headless.hpp'


@pytest.mark.parametrize('form', ['directory', 'archive'])
def test_the_loader_reads_every_image_as_the_decoders_draw_it(classic, form):
    directory, archive, _ = classic
    given = directory if form == 'directory' else archive
    assert '[  PASSED  ] 2 tests' in run_gtest('assets', MODULE, 'ClassicPack.*', given)


def test_the_renderer_bakes_every_texture_as_from_the_decoders(classic):
    _, archive, _ = classic
    assert '[  PASSED  ]' in run_gtest('render', RENDER, 'PackParity.*', archive)
