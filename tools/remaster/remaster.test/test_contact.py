"""contact.py: one page set per class with an index, and the side-by-side comparison for the remastered ids."""
from PIL import Image

import contact
from conftest import ENTRIES
from packs import ClassicPack


def test_the_sheets_show_every_asset_by_class(classic, tmp_path):
    written = contact.sheets(ClassicPack.open(classic), None, tmp_path / 'sheets', None, contact.PER_PAGE)
    assert sorted(written) == sorted({e['class'] for e in ENTRIES})
    shown = sorted(asset for pages in written.values() for _, ids in pages for asset in ids)
    assert shown == sorted(e['id'] for e in ENTRIES)
    assert (tmp_path / 'sheets' / 'index.md').read_text(encoding='utf-8').count('](') == sum(len(p) for p in written.values())
    with Image.open(tmp_path / 'sheets' / 'battle-01.png') as page:
        assert page.mode == 'RGB' and page.width > 4 * 4


def test_a_large_class_is_split_into_pages(classic, tmp_path):
    written = contact.sheets(ClassicPack.open(classic), None, tmp_path / 'sheets', ['battle'], 2)
    assert [len(ids) for _, ids in written['battle']] == [2, 2]


def test_the_comparison_shows_the_remastered_ids_only(classic, remaster, image_of, tmp_path):
    image_of('portrait/000').unlink()
    written = contact.sheets(ClassicPack.open(classic), remaster, tmp_path / 'compare', None, contact.PER_PAGE)
    shown = {asset for pages in written.values() for _, ids in pages for asset in ids}
    assert 'portrait/000' not in shown and 'portraits' not in written
    assert shown == {e['id'] for e in ENTRIES} - {'portrait/000'}


def test_a_remaster_is_shown_at_the_classic_cell_size():
    small = Image.new('RGBA', (160, 104))
    assert contact.resized(small, (40, 26)).size == (40, 26)
    assert contact.shown_scale({'width': 40, 'height': 26}) == 4 and contact.shown_scale({'width': 640, 'height': 400}) == 1
