"""The prompt templates: every asset of the classic pack has exactly one, their front matter is complete, their
references are their own assets, and every prompt fills without a placeholder left."""
import re

import pytest

from packs import ClassicPack
from templates import Templates, prompt_for

CLASSIC_TOTAL = 3262
REQUIRED = {'template', 'title', 'pack_class', 'ids', 'count', 'scale', 'background', 'recolour', 'references', 'rules'}
TEMPLATES = Templates.load()


def sample(root):
    return ClassicPack.open(root / 'sources' / 'opeu' / 'assets' / 'assets.test' / 'packs' / 'classic-sample')


def test_every_template_has_its_front_matter():
    assert len(TEMPLATES.templates) == 11
    for template in TEMPLATES.templates:
        assert REQUIRED <= template.meta.keys(), template.name
        assert template.path.stem == template.name
        assert template.meta['background'] in {'opaque', 'transparent'}
        assert template.meta['scale'] == 4


def test_the_counts_add_up_to_the_classic_pack():
    assert sum(template.meta['count'] for template in TEMPLATES.templates) == CLASSIC_TOTAL


def test_every_sample_asset_has_exactly_one_template(root):
    assert TEMPLATES.coverage_faults(sorted(sample(root).entries)) == []


def test_every_reference_is_an_asset_of_its_own_template():
    for template in TEMPLATES.templates:
        assert template.meta['references'], template.name
        assert all(template.covers(reference) for reference in template.meta['references']), template.name


@pytest.mark.parametrize('template', TEMPLATES.templates, ids=lambda t: t.name)
def test_every_prompt_fills_completely(template):
    values = {'id': 'i', 'use': 'u', 'width': 1, 'height': 1, 'scale': 4, 'target_width': 4, 'target_height': 4, 'palette': 'p',
              'source': 's', 'references': 'r', 'variant': 'v', 'animation': 'a', 'frame': 0}
    filled = template.fill(values)
    assert not re.search(r'\{\{|\}\}', filled) and '\n' not in filled
    assert 'Output 4x4 pixels' in filled


def test_a_prompt_names_the_classic_asset_and_attaches_it_first(root):
    pack = sample(root)
    unit = next(i for i in sorted(pack.entries) if i.startswith('battle-unit/') and i.endswith('side-0'))
    prompt = prompt_for(pack, TEMPLATES, unit)
    assert prompt.template.name == 'unit-icons'
    assert pack.entries[unit]['image'] in prompt.text and 'red side' in prompt.text
    assert prompt.attachments[0] == pack.root / pack.entries[unit]['image']


def test_an_unknown_placeholder_is_an_error():
    with pytest.raises(KeyError):
        TEMPLATES.templates[0].fill({})

