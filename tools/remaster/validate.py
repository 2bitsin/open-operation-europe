#!/usr/bin/env python3
"""Validate a remaster pack directory against the classic pack.

    python3 tools/remaster/validate.py REMASTER [--classic DIR] [--scale N] [--report DIR] [--fix-manifest] [--name NAME]

A remaster directory holds RGBA PNGs at the classic pack's image paths (<class>/<id>.png) for the ids it replaces,
nothing else is required. Every image is checked against its classic asset and its class's template
(tools/remaster/templates/*.md, the `rules` of the front matter):

    unknown-image        the path is no classic asset's image
    index-map            a remaster has no index maps (*.index.png)
    not-png, not-rgba    the file is not a PNG, or not 8-bit RGBA
    wrong-size           not the classic size times the scale
    unexpected-alpha     transparency where the classic has no mask
    mask-mismatch        where the classic has a mask: the coverage, brought to game pixels, differs from the
                         classic's in more than mask_tolerance of the covered pixels
    not-neutral          a pen-coloured (ink) class holds colour beyond chroma_tolerance
    off-grid             a glyph or tile draws outside the classic's extent grown by grid_margin game pixels
    incomplete-variants  an asset without its variants (side-0 ...), or a variant without its asset
    variant-silhouette   a variant's coverage differs from its asset's in more than variant_tolerance of the pixels
    scale-mismatch       the remaster's own pack.json states another scale
    not-daylight (warn)  an hour-tinted image much darker or lighter than the classic day image
    partial-animation (warn) some frames of an animation replaced, not all

The report goes to REPORT/<name>.md and .json (default _local/remaster/reports/); the exit code is 1 on any error.
`--fix-manifest` then writes the remaster's own pack.json (scale, the replaced ids' entries without index maps), so
`assets::FallbackPack` loads it over the classic pack. docs/remaster.md is the workflow.
"""
import argparse
import json
import sys
from collections import defaultdict
from dataclasses import asdict, dataclass
from pathlib import Path

from PIL import Image, ImageChops

sys.path.insert(0, str(Path(__file__).resolve().parent))
from packs import CLASSIC, FORMAT, MANIFEST, REMASTER, ClassicPack, index_maps, remaster_images  # noqa: E402
from templates import TEMPLATES, Templates  # noqa: E402

REPORTS = REMASTER / 'reports'
DEFAULT_SCALE = 4
OPAQUE = 255
HALF = 128
ERROR = 'error'
WARNING = 'warning'


@dataclass(frozen=True)
class Finding:
    severity: str
    check: str
    asset: str
    detail: str


@dataclass(frozen=True)
class Case:
    """One remaster image and everything it is checked against."""
    entry: dict
    rules: dict
    scale: int
    file: Image.Image
    image: Image.Image
    classic: Image.Image

    def error(self, check, detail):
        return Finding(ERROR, check, self.entry['id'], detail)

    def warning(self, check, detail):
        return Finding(WARNING, check, self.entry['id'], detail)


def binary(image, threshold):
    return image.point(lambda value: 255 if value >= threshold else 0).convert('1')


def coverage_at_game_size(image, size):
    """Which game pixels the image covers: its alpha averaged over each scale x scale block, at least half opaque."""
    return binary(image.getchannel('A').resize(size, Image.Resampling.BOX), HALF)


def classic_coverage(classic):
    return binary(classic.getchannel('A'), 1)


def count(mask):
    return mask.convert('L').histogram()[255]


def mismatch_share(left, right):
    """The share of the pixels either mask covers where the two disagree."""
    union = count(ImageChops.logical_or(left, right))
    return count(ImageChops.logical_xor(left, right)) / union if union else 0.0


def check_format(case):
    faults = [] if case.file.format == 'PNG' else [case.error('not-png', f'a {case.file.format} file')]
    return faults + ([] if case.file.mode == 'RGBA' else [case.error('not-rgba', f'mode {case.file.mode}')])


def check_size(case):
    wanted = (case.entry['width'] * case.scale, case.entry['height'] * case.scale)
    return [] if case.image.size == wanted else [case.error('wrong-size', f'{case.image.size[0]}x{case.image.size[1]}, wanted {wanted[0]}x{wanted[1]}')]


def check_alpha(case):
    if case.entry['mask']:
        return check_mask(case)
    lowest = case.image.getchannel('A').getextrema()[0]
    return [] if lowest == OPAQUE else [case.error('unexpected-alpha', f'alpha down to {lowest}; the classic asset is opaque')]


def check_mask(case):
    share = mismatch_share(coverage_at_game_size(case.image, case.classic.size), classic_coverage(case.classic))
    tolerance = case.rules['mask_tolerance']
    return [] if share <= tolerance else [case.error('mask-mismatch', f'coverage differs in {share:.0%} of the covered pixels (at most {tolerance:.0%})')]


def check_neutral(case):
    if not case.rules['neutral']:
        return []
    covered = [(r, g, b) for r, g, b, a in case.image.getdata() if a]
    coloured = sum(1 for pixel in covered if max(pixel) - min(pixel) > case.rules['chroma_tolerance'])
    share = coloured / len(covered) if covered else 0.0
    return [] if share <= case.rules['neutral_share'] else [case.error('not-neutral', f'{share:.0%} of the pixels coloured; a pen-coloured asset is grey only')]


def grown(box, margin, size):
    return max(box[0] - margin, 0), max(box[1] - margin, 0), min(box[2] + margin, size[0]), min(box[3] + margin, size[1])


def check_grid(case):
    if not case.rules['grid']:
        return []
    drawn = coverage_at_game_size(case.image, case.classic.size).getbbox()
    allowed = classic_coverage(case.classic).getbbox()
    if not drawn or not allowed:
        return []
    limit = grown(allowed, case.rules['grid_margin'], case.classic.size)
    inside = limit[0] <= drawn[0] and limit[1] <= drawn[1] and drawn[2] <= limit[2] and drawn[3] <= limit[3]
    return [] if inside else [case.error('off-grid', f'draws over {drawn}, the cell allows {limit} (game pixels)')]


def mean_luminance(image):
    grey = image.convert('L')
    alpha = image.getchannel('A')
    pixels = [value for value, cover in zip(grey.getdata(), alpha.getdata()) if cover]
    return sum(pixels) / len(pixels) / 255 if pixels else 0.0


def check_daylight(case):
    if not case.rules['daylight']:
        return []
    drift = mean_luminance(case.image) - mean_luminance(case.classic)
    tolerance = case.rules['daylight_tolerance']
    return [] if abs(drift) <= tolerance else [case.warning('not-daylight', f'mean luminance {drift:+.0%} against the classic day image')]


PIXEL_CHECKS = (check_alpha, check_neutral, check_grid, check_daylight)


def check_image(case):
    faults = check_format(case) + check_size(case)
    if any(fault.check == 'wrong-size' for fault in faults):
        return faults
    return faults + [fault for check in PIXEL_CHECKS for fault in check(case)]


@dataclass(frozen=True)
class Remaster:
    """A remaster directory read against the classic pack: the replaced entries and their images."""
    root: Path
    classic: ClassicPack
    scale: int
    images: dict

    @classmethod
    def read(cls, root, classic, scale):
        return cls(Path(root), classic, scale, remaster_images(root))

    def replaced(self):
        """Classic entry -> the remaster image, for every image at a classic asset's path."""
        by_image = self.classic.by_image()
        return {by_image[path]['id']: file for path, file in self.images.items() if path in by_image}

    def stray_images(self):
        by_image = self.classic.by_image()
        return [path for path in self.images if path not in by_image]

    def stated_scale(self):
        manifest = self.root / MANIFEST
        return json.loads(manifest.read_text(encoding='utf-8')).get('scale') if manifest.is_file() else None


def case_for(remaster, templates, asset_id, path):
    entry = remaster.classic.entries[asset_id]
    with Image.open(path) as file:
        file.load()
        return Case(entry, templates.of(asset_id).rules, remaster.scale, file, file.convert('RGBA'), remaster.classic.image(entry))


def check_images(remaster, templates):
    replaced = remaster.replaced()
    return [fault for asset_id, path in sorted(replaced.items()) for fault in check_image(case_for(remaster, templates, asset_id, path))]


def check_files(remaster):
    faults = [Finding(ERROR, 'unknown-image', path, 'no classic asset has this image path') for path in remaster.stray_images()]
    faults += [Finding(ERROR, 'index-map', path, 'a remaster carries RGBA images only') for path in index_maps(remaster.root)]
    stated = remaster.stated_scale()
    if stated is not None and stated != remaster.scale:
        faults.append(Finding(ERROR, 'scale-mismatch', MANIFEST, f'pack.json states {stated}, validating at {remaster.scale}'))
    return faults


def check_variants(remaster, templates):
    replaced = remaster.replaced()
    faults = []
    for asset_id, variants in sorted(remaster.classic.variants().items()):
        family = [asset_id, *variants]
        present = [member for member in family if member in replaced]
        if present and len(present) != len(family):
            missing = ', '.join(member for member in family if member not in replaced)
            faults.append(Finding(ERROR, 'incomplete-variants', asset_id, f'missing {missing}'))
        elif present:
            faults += check_silhouettes(remaster, templates, family)
    return faults


def check_silhouettes(remaster, templates, family):
    replaced = remaster.replaced()
    base = load_alpha_mask(replaced[family[0]])
    tolerance = templates.of(family[0]).rules['variant_tolerance']
    faults = []
    for variant in family[1:]:
        shape = load_alpha_mask(replaced[variant])
        share = mismatch_share(base, shape) if shape.size == base.size else 1.0
        if share > tolerance:
            faults.append(Finding(ERROR, 'variant-silhouette', variant, f'coverage differs from {family[0]} in {share:.0%} (at most {tolerance:.0%})'))
    return faults


def load_alpha_mask(path):
    with Image.open(path) as image:
        return binary(image.convert('RGBA').getchannel('A'), HALF)


def check_animations(remaster):
    replaced = remaster.replaced()
    faults = []
    for animation, frames in sorted(remaster.classic.manifest.get('animations', {}).items()):
        present = [frame for frame in frames if frame in replaced]
        if present and len(present) != len(frames):
            faults.append(Finding(WARNING, 'partial-animation', animation, f'{len(present)} of {len(frames)} frames replaced'))
    return faults


@dataclass(frozen=True)
class Report:
    remaster: str
    classic: str
    scale: int
    replaced: dict
    findings: list

    def errors(self):
        return [finding for finding in self.findings if finding.severity == ERROR]

    def as_json(self):
        return json.dumps({**asdict(self), 'errors': len(self.errors())}, indent=1, sort_keys=True) + '\n'

    def as_markdown(self):
        lines = [f'# Remaster validation: {Path(self.remaster).name}', '', f'- remaster: `{self.remaster}`',
                 f'- classic: `{self.classic}`', f'- scale: {self.scale}x',
                 f'- replaced: {sum(self.replaced.values())} assets ({", ".join(f"{c} {n}" for c, n in sorted(self.replaced.items())) or "none"})',
                 f'- errors: {len(self.errors())}, warnings: {len(self.findings) - len(self.errors())}', '']
        if self.findings:
            lines += ['| severity | check | asset | detail |', '|---|---|---|---|']
            lines += [f'| {f.severity} | {f.check} | `{f.asset}` | {f.detail} |' for f in self.findings]
        return '\n'.join(lines) + '\n'

    def write(self, directory):
        directory.mkdir(parents=True, exist_ok=True)
        name = Path(self.remaster).name
        (directory / f'{name}.json').write_text(self.as_json(), encoding='utf-8')
        (directory / f'{name}.md').write_text(self.as_markdown(), encoding='utf-8')
        return directory / f'{name}.md'


def validate(remaster, templates):
    findings = check_files(remaster) + check_images(remaster, templates) + check_variants(remaster, templates) + check_animations(remaster)
    replaced = defaultdict(int)
    for asset_id in remaster.replaced():
        replaced[remaster.classic.entries[asset_id]['class']] += 1
    return Report(str(remaster.root), str(remaster.classic.root), remaster.scale, dict(replaced), findings)


def remaster_entry(entry):
    """A classic entry as the remaster lists it: the same id, geometry, palettes and recolouring, no index map."""
    return {key: value for key, value in entry.items() if key != 'indexed'}


def remaster_manifest(remaster, name):
    entries = [remaster_entry(remaster.classic.entries[asset_id]) for asset_id in sorted(remaster.replaced())]
    return {'format': FORMAT, 'name': name, 'scale': remaster.scale, 'classes': sorted({e['class'] for e in entries}),
            'palettes': {}, 'animations': {}, 'fonts': {}, 'assets': entries}


def write_manifest(remaster, name):
    """The remaster's pack.json in the classic pack's layout: one line per asset, so a diff shows what changed."""
    manifest = remaster_manifest(remaster, name)
    head = {key: value for key, value in manifest.items() if key != 'assets'}
    body = ',\n'.join(' ' + json.dumps(entry, sort_keys=False) for entry in manifest['assets'])
    text = json.dumps(head)[:-1] + (', "assets": [\n' + body + '\n]}\n' if body else ', "assets": []}\n')
    (remaster.root / MANIFEST).write_text(text, encoding='utf-8')
    return remaster.root / MANIFEST


def arguments():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('remaster', type=Path, help='the remaster pack directory')
    parser.add_argument('--classic', type=Path, default=CLASSIC, help='the classic pack (default sources/opeu/assets/packs.embed/classic)')
    parser.add_argument('--templates', type=Path, default=TEMPLATES)
    parser.add_argument('--scale', type=int, default=DEFAULT_SCALE, help=f'image pixels per game pixel (default {DEFAULT_SCALE})')
    parser.add_argument('--report', type=Path, default=REPORTS, help='where the report goes')
    parser.add_argument('--fix-manifest', action='store_true', help="write the remaster's pack.json when it validates")
    parser.add_argument('--name', help="the remaster pack's name (default the directory's)")
    return parser.parse_args()


def main():
    args = arguments()
    remaster = Remaster.read(args.remaster, ClassicPack.open(args.classic), args.scale)
    report = validate(remaster, Templates.load(args.templates))
    print(f'{report.write(args.report)}: {sum(report.replaced.values())} replaced, {len(report.errors())} errors, '
          f'{len(report.findings) - len(report.errors())} warnings')
    if report.errors():
        return 1
    if args.fix_manifest:
        print(write_manifest(remaster, args.name or args.remaster.name))
    return 0


if __name__ == '__main__':
    sys.exit(main())
