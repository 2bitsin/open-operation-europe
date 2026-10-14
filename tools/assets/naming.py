"""Readable names and one-line descriptions for the classic pack's images, so a person or an image generator can tell
from a file name and the manifest what each picture shows.

A file name is the asset's id with '/' turned into '-', followed, where the game's data says what the picture is, by
that: `portrait-005-rommel`, `silhouette-024-tiger-i`, `view-tile-map-2-17-kursk-1943-forest`,
`battle-unit-class-03-facing-1-light-tank`, `event-27-pontoon-crossing`, `text-glyph-065-capital-a`. The id keeps the
name unique and sorted; the words make it readable. The knowledge the words come from is tools/assets/asset-names.json
(maps, terrain classes, unit icon classes, equipment types, event pictures) and the scenario set-ups the probe decodes
(who each portrait is). docs/asset-pack.md, "File names", is the rule.
"""
import json
import re
from collections import Counter, defaultdict
from pathlib import Path

NAMES = Path(__file__).resolve().parent / 'asset-names.json'
TERRAIN_BITS = 0x0F
GLYPH_WORDS = {
    ' ': 'space', '!': 'exclamation', '"': 'quote', '#': 'hash', '$': 'dollar', '%': 'percent', '&': 'ampersand',
    "'": 'apostrophe', '(': 'left-parenthesis', ')': 'right-parenthesis', '*': 'asterisk', '+': 'plus', ',': 'comma',
    '-': 'hyphen', '.': 'full-stop', '/': 'slash', ':': 'colon', ';': 'semicolon', '<': 'less-than', '=': 'equals',
    '>': 'greater-than', '?': 'question', '@': 'at', '[': 'left-bracket', '\\': 'backslash', ']': 'right-bracket',
    '^': 'caret', '_': 'underscore', '`': 'grave', '{': 'left-brace', '|': 'bar', '}': 'right-brace', '~': 'tilde',
    '\x7f': 'delete',
}


def slug(text):
    """Lower case, every run of other characters one hyphen: 'Pzkpfw IVF2' -> 'pzkpfw-ivf2'."""
    return re.sub(r'[^a-z0-9]+', '-', text.lower()).strip('-')


def article(noun):
    return 'an' if noun[0] in 'aeiou' else 'a'


def character_words(code):
    """What a text glyph's character is called in a file name: 'capital-a', 'small-a', 'digit-7', 'comma'."""
    character = chr(code)
    if character.isupper():
        return f'capital-{character.lower()}'
    if character.islower():
        return f'small-{character}'
    if character.isdigit():
        return f'digit-{character}'
    return GLYPH_WORDS[character]


def character_description(code):
    character = chr(code)
    if character.isalpha():
        return f'the letter {character}'
    if character.isdigit():
        return f'the digit {character}'
    return f'the character {GLYPH_WORDS[character].replace("-", " ")}'


class Naming:
    """Names and descriptions for every asset of one decoding of the shipped files."""

    def __init__(self, records, names=NAMES):
        known = json.loads(Path(names).read_text(encoding='utf-8'))
        self._maps = known['maps']
        self._terrain = known['terrain_classes']
        self._unit_classes = known['unit_icon_classes']
        self._equipment = known['equipment']
        self._events = known['events']
        self._people = self._people_by_portrait([r for r in records if r['kind'] == 'setup'])
        self._tile_terrain = self._terrain_by_tile([r for r in records if r['kind'] == 'map'])

    def _people_by_portrait(self, setups):
        """Portrait number -> (name, the maps of the scenarios that show it), from the set-ups the menus start."""
        people = defaultdict(lambda: ['', []])
        for setup in sorted(setups, key=lambda s: s['setup']):
            if setup['setup'] == 0:
                continue  # set-up 0 is reached by no menu and its names are in KOEI's double-byte code
            for person in setup['people']:
                entry = people[person['portrait']]
                entry[0] = entry[0] or person['name']
                entry[1].append(setup['scenario'])
        return dict(people)

    def _terrain_by_tile(self, maps):
        """(map, tile) -> how many cells of each terrain class show that tile."""
        counts = defaultdict(Counter)
        for grid in maps:
            for cells, tiles in zip(grid['cells'], grid['tiles']):
                for cell, tile in zip(cells, tiles):
                    counts[(int(grid['map']), tile)][self._terrain[cell & TERRAIN_BITS]] += 1
        return counts

    def label(self, asset_id):
        """The words after the id in the file name, or '' where the data says nothing more than the id."""
        family, _, rest = asset_id.partition('/')
        return getattr(self, '_label_' + family.replace('-', '_'), lambda _: '')(rest)

    def stem(self, asset_id):
        """The file name without '.png': the id with '/' as '-', then the label."""
        words = self.label(asset_id)
        return asset_id.replace('/', '-') + (f'-{words}' if words else '')

    def describe(self, record):
        """One line saying what the picture shows, for a person and for an image generator's prompt."""
        family, _, rest = record['id'].partition('/')
        told = getattr(self, '_describe_' + family.replace('-', '_'), None)
        return told(rest) if told else (record['use'] or 'not drawn by the game')

    # --- portraits -------------------------------------------------------------------------------------------------

    def _person(self, rest):
        return self._people.get(int(rest), ['', []])

    def _label_portrait(self, rest):
        return slug(self._person(rest)[0])

    def _describe_portrait(self, rest):
        name, scenarios = self._person(rest)
        if not name:
            return 'portrait of a commander no scenario names'
        where = ', '.join(self._maps[s]['title'] for s in sorted(set(scenarios)))
        return f'portrait of {name}, a commander in {where}'

    # --- equipment -------------------------------------------------------------------------------------------------

    def _label_silhouette(self, rest):
        return slug(self._equipment[int(rest)]['name'])

    def _describe_silhouette(self, rest):
        kind = self._equipment[int(rest)]
        nations = '/'.join(kind['nations'])
        return f'side view of the {nations + " " if nations else ""}{kind["category"]} type {kind["name"]} (the equipment panel)'

    # --- battle sprites --------------------------------------------------------------------------------------------

    def _unit_class(self, rest):
        return self._unit_classes[int(rest.split('/')[0].removeprefix('class-'))]

    def _label_battle_unit(self, rest):
        return slug(self._unit_class(rest))

    def _describe_battle_unit(self, rest):
        facing = rest.split('/')[1].removeprefix('facing-')
        kind = self._unit_class(rest)
        return f'battle icon of {article(kind)} {kind} unit, facing {facing} of 4'

    def _describe_battle_terrain(self, rest):
        season, number = rest.split('/')
        return f'battle field object {int(number)} in {season}, a part of a 40x26 diamond cell'

    # --- map tiles -------------------------------------------------------------------------------------------------

    def _tile(self, rest):
        board, number = rest.split('/')
        index = int(board.removeprefix('map-'))
        return self._maps[index], self._tile_terrain.get((index, int(number)), Counter())

    def _label_view_tile(self, rest):
        board, terrain = self._tile(rest)
        return f'{board["slug"]}-{slug(terrain.most_common(1)[0][0]) if terrain else "unused"}'

    _label_overview_tile = _label_view_tile

    def _describe_tile(self, rest, view):
        board, terrain = self._tile(rest)
        cells = ', '.join(f'{count} {name}' for name, count in terrain.most_common())
        return f'{view} tile {int(rest.split("/")[1])} of the {board["title"]} map: ' + (f'{cells} cells' if cells else 'no cell shows it')

    def _describe_view_tile(self, rest):
        return self._describe_tile(rest, 'map view (24x24)')

    def _describe_overview_tile(self, rest):
        return self._describe_tile(rest, 'strategic overview (8x8)')

    # --- pictures and glyphs ---------------------------------------------------------------------------------------

    def _label_event(self, rest):
        return self._events[int(rest)]['slug']

    def _describe_event(self, rest):
        return f'event picture: {self._events[int(rest)]["description"]}'

    def _label_text_glyph(self, rest):
        return character_words(int(rest))

    def _describe_text_glyph(self, rest):
        return f'8x16 text font glyph: {character_description(int(rest))}'
