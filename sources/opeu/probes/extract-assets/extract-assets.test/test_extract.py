"""tools/assets/extract.py over the shipped files: every asset written, and a second run byte-identical."""
import hashlib
import importlib.util
import json
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[5]
SPEC = importlib.util.spec_from_file_location('extract', ROOT / 'tools' / 'assets' / 'extract.py')
extract = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(extract)

ASSETS = ROOT / 'sources' / 'opeu' / 'opeu' / 'assets.embed'
PROGRAM_GLYPHS = 96 + 11 + 11 + 2  # the program's text font, digits, sideways digits and markers
COUNTS = {'battle': 244, 'dialogs': 6, 'glyphs': 1298 + PROGRAM_GLYPHS, 'logos': 4, 'masks': 20, 'pictures': 95,
          'portraits': 120, 'silhouettes': 159, 'tiles': 1152}


def tree(root):
    return {str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(root.rglob('*')) if p.is_file()}


def test_extraction_is_complete_and_byte_stable(tmp_path):
    probe = shutil.which('extract-assets')
    assert probe, 'extract-assets is not on PATH (buildutil puts <build>/bin there)'
    first, second = tmp_path / 'first', tmp_path / 'second'
    assert extract.extract(first, ASSETS, probe) == COUNTS
    extract.extract(second, ASSETS, probe)
    extract.extract(second, ASSETS, probe)  # over its own earlier output
    files = tree(first)
    assert files == tree(second)
    manifest = json.loads((first / 'manifest.json').read_text(encoding='utf-8'))
    assert manifest['format'] == 'opeu-assets/1'
    assert len(manifest['files']) == 26
    for group in COUNTS:
        for asset in json.loads((first / 'assets' / f'{group}.json').read_text(encoding='utf-8'))['assets']:
            assert asset['image'] in files and asset['indices'] in files, asset['id']
    setups = json.loads((first / 'tables' / 'scenarios.json').read_text(encoding='utf-8'))['scenario_file']['setups']
    assert [f"{s['year']}-{s['month']:02}-{s['day']:02}" for s in setups[1:]] == [
        '1940-05-10', '1942-05-26', '1943-07-05', '1944-06-06', '1944-12-16', '1945-04-16']
