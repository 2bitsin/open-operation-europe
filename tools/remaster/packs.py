"""A pack directory as the remaster tools see it: the manifest's entries by id and by image path, the variants of
each asset, and the images as Pillow RGBA images. docs/asset-pack.md is the format; docs/remaster.md the workflow."""
import json
from collections import defaultdict
from dataclasses import dataclass, field
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
LOCAL = ROOT / '_local'
PACKS = ROOT / 'sources' / 'opeu' / 'opeu' / 'packs.embed'
CLASSIC = PACKS / 'classic' if PACKS.is_dir() else LOCAL / 'packs' / 'classic'  # where pack.py writes it in either tree
REMASTER = LOCAL / 'remaster'
MANIFEST = 'pack.json'
FORMAT = 'opeu-pack/1'
INDEX_SUFFIX = '.index.png'


@dataclass(frozen=True)
class ClassicPack:
    """A pack with a manifest (the classic pack, or any finished pack): what a remaster is checked against."""
    root: Path
    manifest: dict
    entries: dict = field(repr=False)

    @classmethod
    def open(cls, root):
        root = Path(root)
        manifest = json.loads((root / MANIFEST).read_text(encoding='utf-8'))
        if manifest.get('format') != FORMAT:
            raise ValueError(f'{root / MANIFEST}: not {FORMAT}')
        return cls(root, manifest, {entry['id']: entry for entry in manifest['assets']})

    @property
    def scale(self):
        return self.manifest['scale']

    def by_image(self):
        """Image path (relative to the pack) -> entry: how a remaster image without a manifest is identified."""
        return {entry['image']: entry for entry in self.entries.values()}

    def variants(self):
        """Asset id -> the ids of its variants (side-0 ...), for the assets that have any."""
        found = defaultdict(list)
        for entry in self.entries.values():
            if 'variant_of' in entry:
                found[entry['variant_of']].append(entry['id'])
        return {asset: sorted(ids) for asset, ids in found.items()}

    def in_class(self, asset_class):
        return sorted((entry for entry in self.entries.values() if entry['class'] == asset_class), key=lambda e: e['id'])

    def classes(self):
        return sorted({entry['class'] for entry in self.entries.values()})

    def image(self, entry):
        return load_rgba(self.root / entry['image'])


def load_rgba(path):
    with Image.open(path) as image:
        return image.convert('RGBA')


def game_size(entry):
    return entry['width'], entry['height']


def scaled_size(entry, scale):
    return entry['width'] * scale, entry['height'] * scale


def remaster_images(root):
    """The images a remaster directory holds: path relative to the root -> absolute path (index maps excluded)."""
    root = Path(root)
    return {path.relative_to(root).as_posix(): path for path in sorted(root.rglob('*.png'))
            if not path.name.endswith(INDEX_SUFFIX)}


def index_maps(root):
    root = Path(root)
    return sorted(path.relative_to(root).as_posix() for path in root.rglob('*' + INDEX_SUFFIX))
