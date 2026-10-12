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
