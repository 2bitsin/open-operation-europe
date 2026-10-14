#!/usr/bin/env python3
"""The prompt templates of tools/remaster/templates/: one Markdown file per asset class, YAML front matter for the
tools (ids covered, scale, background, recolouring, style references, validation rules) and a `## Prompt` section
the generator script fills per asset.

    python3 tools/remaster/templates.py ID [--pack DIR] [--scale N]   the filled prompt of ID and the images to attach
    python3 tools/remaster/templates.py --check [--pack DIR]           every asset covered once, every reference real

Placeholders in the prompt: {{id}} {{use}} {{width}} {{height}} {{scale}} {{target_width}} {{target_height}}
{{palette}} {{source}} {{references}} {{variant}} {{animation}} {{frame}}. docs/remaster.md is the workflow.
"""
import argparse
import re
import sys
from dataclasses import dataclass
from fnmatch import fnmatchcase
from pathlib import Path

import yaml

sys.path.insert(0, str(Path(__file__).resolve().parent))
from packs import CLASSIC, ClassicPack  # noqa: E402

TEMPLATES = Path(__file__).resolve().parent / 'templates'
FRONT_MATTER = re.compile(r'\A---\n(.*?)\n---\n(.*)\Z', re.DOTALL)
PROMPT = re.compile(r'^## Prompt\n(.*?)(?=^## |\Z)', re.DOTALL | re.MULTILINE)
PLACEHOLDER = re.compile(r'\{\{(\w+)\}\}')
DEFAULT_RULES = {
    'mask_tolerance':    0.15,  # most share of the covered game pixels whose coverage may differ from the classic's
    'neutral':           False,  # grey only: the game draws the shape in a pen colour
    'chroma_tolerance':  16,     # most max(r, g, b) - min(r, g, b) of a neutral pixel
    'neutral_share':     0.01,   # share of covered pixels that may exceed the chroma tolerance
    'grid':              False,  # stays inside the classic's extent (grown by grid_margin game pixels)
    'grid_margin':       1,
    'daylight':          False,  # an hour-tinted image: warn when much darker or lighter than the classic day image
    'daylight_tolerance': 0.25,  # most difference of mean luminance, 0..1
    'variant_tolerance': 0.02,   # most share of pixels whose coverage differs between a variant and its base
}


@dataclass(frozen=True)
class Template:
    """One asset class's template: the front matter and the Markdown body."""
    path: Path
    meta: dict
    body: str

    @classmethod
    def read(cls, path):
        found = FRONT_MATTER.match(path.read_text(encoding='utf-8'))
        if not found:
            raise ValueError(f'{path}: no YAML front matter')
        return cls(path, yaml.safe_load(found.group(1)), found.group(2))

    @property
    def name(self):
        return self.meta['template']

    @property
    def rules(self):
        return {**DEFAULT_RULES, **(self.meta.get('rules') or {})}

    def covers(self, asset_id):
        return any(fnmatchcase(asset_id, pattern) for pattern in self.meta['ids'])

    def prompt_text(self):
        found = PROMPT.search(self.body)
        if not found:
            raise ValueError(f'{self.path}: no ## Prompt section')
        return found.group(1).strip()

    def fill(self, values):
        """The template's prompt with every placeholder replaced; an unknown placeholder is an error."""
        def value(match):
            if match.group(1) not in values:
                raise KeyError(f'{self.path.name}: unknown placeholder {match.group(0)}')
            return str(values[match.group(1)])
        return ' '.join(PLACEHOLDER.sub(value, self.prompt_text()).split())

    def variant_text(self, entry):
        texts = self.meta.get('variant_text') or {}
        return texts.get(entry.get('variant', 'base'), entry.get('variant', 'none'))


@dataclass(frozen=True)
class Templates:
    """Every template of a directory; answers which template an asset id belongs to."""
    templates: tuple

    @classmethod
    def load(cls, directory=TEMPLATES):
        return cls(tuple(Template.read(path) for path in sorted(Path(directory).glob('*.md'))))

    def covering(self, asset_id):
        return [template for template in self.templates if template.covers(asset_id)]

    def of(self, asset_id):
        found = self.covering(asset_id)
        if len(found) != 1:
            raise LookupError(f'{asset_id}: covered by {len(found)} templates ({", ".join(t.name for t in found)})')
        return found[0]

    def coverage_faults(self, ids):
        """The ids covered by no template or by several: each asset has exactly one template."""
        return [f'{asset_id}: {len(self.covering(asset_id))} templates' for asset_id in ids if len(self.covering(asset_id)) != 1]

    def reference_faults(self, pack):
        """The references that name no asset of the pack or that belong to another template."""
        faults = []
        for template in self.templates:
            for reference in template.meta.get('references', []):
                if reference not in pack.entries or not template.covers(reference):
                    faults.append(f'{template.name}: reference {reference} is not one of its assets')
        return faults


def reference_images(pack, template):
    """The image paths of the template's style references the pack holds (a pack built with --only may lack some)."""
    return [pack.entries[r]['image'] for r in template.meta.get('references', []) if r in pack.entries]


def placeholders(pack, template, entry, scale):
    """The values a prompt is filled with for one asset."""
    return {
        'id':            entry['id'],
        'use':           entry.get('use') or 'the game',
        'width':         entry['width'],
        'height':        entry['height'],
        'scale':         scale,
        'target_width':  entry['width'] * scale,
        'target_height': entry['height'] * scale,
        'palette':       entry['palette'],
        'source':        entry['image'],
        'references':    ', '.join(reference_images(pack, template)),
        'variant':       template.variant_text(entry),
        'animation':     entry.get('animation', 'none'),
        'frame':         entry.get('frame', 0),
    }


@dataclass(frozen=True)
class Prompt:
    """What the generator is given for one asset: the template, the filled prompt (one paragraph, the images named by
    their pack paths) and the images to attach, the classic asset first, then the class's style references."""
    template: Template
    text: str
    attachments: tuple


def prompt_for(pack, templates, asset_id, scale=None):
    entry = pack.entries[asset_id]
    template = templates.of(asset_id)
    text = template.fill(placeholders(pack, template, entry, scale or template.meta['scale']))
    references = [image for image in reference_images(pack, template) if image != entry['image']]
    return Prompt(template, text, tuple(pack.root / image for image in [entry['image'], *references]))


def arguments():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('id', nargs='?', help='the asset to fill the prompt for')
    parser.add_argument('--pack', type=Path, default=CLASSIC, help='the classic pack (default sources/opeu/assets/packs.embed/classic)')
    parser.add_argument('--templates', type=Path, default=TEMPLATES)
    parser.add_argument('--scale', type=int, help="the remaster's scale (default the template's)")
    parser.add_argument('--check', action='store_true', help='check coverage and references against the pack')
    return parser.parse_args()


def check(pack, templates):
    faults = templates.coverage_faults(sorted(pack.entries)) + templates.reference_faults(pack)
    print('\n'.join(faults) if faults else f'{len(templates.templates)} templates cover {len(pack.entries)} assets once each')
    return 1 if faults else 0


def main():
    args = arguments()
    pack = ClassicPack.open(args.pack)
    templates = Templates.load(args.templates)
    if args.check or not args.id:
        return check(pack, templates)
    prompt = prompt_for(pack, templates, args.id, args.scale)
    print(f'# {args.id}: {prompt.template.name} ({prompt.template.path})\n\n{prompt.text}\n\nAttach:')
    print('\n'.join(str(path) for path in prompt.attachments))
    return 0


if __name__ == '__main__':
    sys.exit(main())
