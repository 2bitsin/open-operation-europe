"""What tools/assets/extract.py and tools/assets/pack.py share: the shipped files, the extract-assets probe that
decodes them (sources/opeu/probes/extract-assets, the assets module's decoders), and how JSON is written."""
import json
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SHIPPED = ROOT / 'sources' / 'opeu' / 'opeu' / 'assets.embed'


def find_probe(given=None):
    """The extract-assets binary: the one given, else the one on PATH, else the newest one built under _build/."""
    if given:
        return Path(given)
    found = shutil.which('extract-assets')
    if found:
        return Path(found)
    built = sorted(ROOT.glob('_build/*/bin/extract-assets'), key=lambda p: p.stat().st_mtime)
    if not built:
        sys.exit('tools/assets: no extract-assets binary; build with ./buildutil build or pass --probe')
    return built[-1]


def run_probe(probe, assets, out):
    """Decodes every shipped file in `assets`, writes the images under `out`/images/, returns the probe's records."""
    result = subprocess.run([str(probe), str(assets), str(out)], capture_output=True, text=True, check=False)
    if result.returncode != 0:
        sys.exit(f'tools/assets: {probe} failed: {result.stderr.strip()}')
    return [json.loads(line) for line in result.stdout.splitlines() if line.strip()]


def write_json(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=1, ensure_ascii=False) + '\n', encoding='utf-8')


def _flat(value):
    """Whether a value is written on one line: a scalar, a list of scalars, or a record of those."""
    def scalar_or_row(v):
        return not isinstance(v, (list, dict)) or (isinstance(v, list) and not any(isinstance(i, (list, dict)) for i in v))
    if isinstance(value, dict):
        return all(scalar_or_row(v) for v in value.values())
    return scalar_or_row(value)


def table_text(value, depth=0):
    """A table file as text: every record and every grid row on a line of its own, so a diff shows the records changed."""
    if _flat(value):
        return json.dumps(value, ensure_ascii=False, separators=(', ', ': '))
    inner = ' ' * (depth + 1)
    if isinstance(value, list):
        rows = ',\n'.join(inner + table_text(item, depth + 1) for item in value)
        return f'[\n{rows}\n{" " * depth}]'
    rows = ',\n'.join(f'{inner}{json.dumps(key, ensure_ascii=False)}: {table_text(item, depth + 1)}' for key, item in value.items())
    return f'{{\n{rows}\n{" " * depth}}}'


def write_tables(out, records):
    """Writes each table record of the probe to `out`/<its path>; returns the paths written."""
    paths = []
    for record in (r for r in records if r['kind'] == 'table'):
        path = Path(out) / record['path']
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(table_text(record['table']) + '\n', encoding='utf-8')
        paths.append(record['path'])
    return paths
