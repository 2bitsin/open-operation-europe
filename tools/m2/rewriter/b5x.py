"""The B5x pass (#13): the cross-directory shapes that repeat hundreds of times, rewritten token by token.

  - hosts: the text, UI and dialog host structs stop being handed around. `TextHost` is empty, so its parameters and
    arguments go; `UiHost` and `DialogHost` are members of `Game`, so their parameters and arguments go and a routine
    reads `game.Ui().field` / `game.Dialogs().field` where it read `host->field`.
  - display: the drawing library's calls become `game.Machine().Display()` methods over its value types (`Area`,
    `Image`, `Corners`, `Sprite`), an image pointer the span it starts (`port_far(s, o)` -> `FarBytes(game, s, o)`).
  - random: the synced draws (`port_rand_bounded`, `RandomBelow`, `RandomNext`, `RandomSeed`) become the `GameState`
    methods that load the data segment's random state into the generator and store it back; the direct draws
    (`opeu_rand_bounded`, `opeu_rand_seed`) stay unsynced, on the generator `GameState` owns.

Everything else B5x changes (the other Machine call sites, the directory lists) is done by hand: each shape occurs a few
times only (docs/port.md "M2 rewriter")."""
from __future__ import annotations

import re
from collections import Counter
from dataclasses import dataclass, field
from pathlib import Path

from .source import SOURCES

HOST_TYPES = r"(?:TextHost|UiHost|DialogHost)"
HOST_GETTERS = r"port_(?:textout|uiprim|dialogs)_host\(\)"

# `, static_cast<const DialogHost*>(port_dialogs_host())`: the argument, with its comma.
HOST_ARGUMENT = re.compile(rf",\s*static_cast<(?:const\s+)?{HOST_TYPES}(?:\s+const)?\s*\*>\({HOST_GETTERS}\)")
# `, [[maybe_unused]] const UiHost* host`: a parameter, named or not, with its comma.
HOST_PARAMETER = re.compile(rf",\s*(?:\[\[maybe_unused\]\]\s*)?(?:const\s+)?{HOST_TYPES}(?:\s+const)?\s*\*(?:\s*host\b)?")
# `game, host` handed on to the next routine; `host` is only ever a host there.
HOST_FORWARD = re.compile(r"\bgame,\s*host\b(?=\s*[,)])")
# `host->scratch`: a field of the dialogs' or the UI's host.
HOST_FIELD = re.compile(r"\bhost->(load|scratch|memory|ds_segment|tiles_segment)\b")
FIELD_OWNER = {"load": "Dialogs", "scratch": "Dialogs", "memory": "Ui", "ds_segment": "Ui", "tiles_segment": "Ui"}

RANDOM = (
  (re.compile(r"\bport_rand_bounded\(game,\s*"), "game.State().RandomBounded("),
  (re.compile(r"\bRandomBelow\(game,\s*"), "game.State().RandomBounded("),
  (re.compile(r"\bRandomNext\(game\)"), "game.State().RandomNext()"),
  (re.compile(r"\bRandomSeed\(game,\s*"), "game.State().RandomSeed("),
  (re.compile(r"\bopeu_rand_bounded\("), "game.State().Generator().Bounded("),
  (re.compile(r"\bopeu_rand_seed\("), "game.State().Generator().Seed("),
)

RANDOM_INCLUDES = re.compile(r'#include "(?:core|common)/random\.hpp"\n')

# Rewritten by hand: the generator itself and the glue the pass retires.
BY_HAND = {"core/random.cpp", "core/random.hpp", "common/random.hpp", "common/game-state.hpp", "common/random.test.cpp", "common/host.hpp", "link/glue.cpp", "link/glue.hpp", "link/hosts.cpp"}


@dataclass
class B5xResult:
  files: dict[str, Counter] = field(default_factory=dict)  # file -> kind -> sites
  left:  list[str]          = field(default_factory=list)  # drawing calls whose arguments fit no shape: by hand

  def Counts(self) -> dict[str, int]:
    total: Counter = Counter()
    for counts in self.files.values():
      total.update(counts)
    return dict(sorted(total.items()))


def RewriteHosts(text: str) -> tuple[str, Counter]:
  """Drops host arguments and parameters, forwards and field reads; returns the text and the counts per kind."""
  counts: Counter = Counter()
  text, counts["host_argument"] = HOST_ARGUMENT.subn("", text)
  text, counts["host_parameter"] = HOST_PARAMETER.subn("", text)
  text, counts["host_forward"] = HOST_FORWARD.subn("game", text)
  text, counts["host_field"] = HOST_FIELD.subn(lambda m: f"game.{FIELD_OWNER[m.group(1)]}().{m.group(1)}", text)
  return text, +counts


def RewriteRandom(text: str) -> tuple[str, Counter]:
  """The synced draws become GameState methods, the direct ones the generator's; the generator headers' includes go
  (game-state.hpp declares both)."""
  counts: Counter = Counter()
  for pattern, replacement in RANDOM:
    text, counts[pattern.pattern] = pattern.subn(replacement, text)
  text, counts["include"] = RANDOM_INCLUDES.subn("", text)
  return text, +counts


def B5xPass(tree: Path) -> B5xResult:
  """Rewrites every game/ source of the tree in place."""
  result = B5xResult()
  game = tree / SOURCES / "game"
  for path in sorted(p for p in game.rglob("*") if p.suffix in (".cpp", ".hpp") and "gen" not in p.parts):
    if path.relative_to(game).as_posix() in BY_HAND:
      continue
    before = path.read_text()
    text, hosts = RewriteHosts(before)
    text, randoms = RewriteRandom(text)
    text, drawn, left = RewriteDisplay(text)
    text, capped, uncapped = RewriteCapped(text)
    name = path.relative_to(game).as_posix()
    result.left += [f"{name}: {call}" for call in left + uncapped]
    if text != before:
      path.write_text(text)
      result.files[name] = hosts + randoms + drawn + capped
  return result


# ---- machine calls --------------------------------------------------------------------------------------------------
# The drawing library's free functions become Display methods (docs/platform-surface.md section 5). Each shape says how
# the old words map onto the method's value types; a far or near image pointer becomes the span it starts.

DISPLAY = "game.Machine().Display()"

# name -> (method, shape)
DISPLAY_CALLS: dict[str, tuple[str, str]] = {
  "koei_2fea":                  ("EnterGraphics", "none"),
  "opeu_text_mode":             ("LeaveGraphics", "none"),
  "opeu_vga_clear":             ("Clear", "none"),
  "koei_vga_identity_palette":  ("ShowIdentityPalette", "none"),
  "opeu_bitmap_restore":        ("DrawColumns", "area_image"),
  "koei_266c":                  ("DrawColumns", "area_image"),
  "opeu_blit_far":              ("DrawThreeByteColumns", "area_image"),
  "koei_27c2":                  ("DrawThreeByteColumns", "area_image"),
  "opeu_draw_far":              ("DrawBits", "area_image"),
  "opeu_draw_near":             ("DrawBits", "area_image"),
  "opeu_code_0000_28c5":        ("DrawBitsOnPaper", "area_image"),
  "opeu_gfx_blit":              ("DrawShiftedBits", "area_image"),
  "opeu_vga_glyph":             ("DrawShiftedBits", "area_image"),
  "koei_2e57":                  ("DrawShiftedColumns", "area_bytes"),
  "opeu_bitmap_save":           ("SaveColumns", "area_bytes"),
  "koei_2e72":                  ("SaveColumns", "area_bytes"),
  "opeu_screen_copy":           ("CopyColumns", "area_point"),
  "opeu_vga_copy":              ("CopyColumns", "area_point"),
  "opeu_picture_decode":        ("DecodePicture", "point_reader"),
  "opeu_picture_decode_masked": ("DecodeMaskedPicture", "point_reader"),
  "opeu_ui_box":                ("Box", "area_colour"),
  "opeu_vga_fill_rect":         ("Box", "area_colour"),
  "opeu_ui_highlight":          ("Highlight", "area_colour"),
  "opeu_ui_line":               ("Line", "corners_colour"),
  "opeu_ui_fill":               ("Fill", "corners_colour"),
  "koei_3bc5":                  ("CopyCellBlock", "two_points"),
  "opeu_sprite_compose":        ("ComposeSprite", "sprite"),
  "koei_3f26":                  ("ComposeShiftedSprite", "sprite"),
}

ARITY = {"none": 0, "area_image": 6, "area_bytes": 5, "area_point": 6, "point_reader": 3, "area_colour": 5, "corners_colour": 5,
         "two_points": 4, "sprite": 8}

FAR = re.compile(r"^port_far\((.*)\)$", re.S)
NEAR_DATA = re.compile(r"^(game\.State\(\)\.Bytes\(.*\))\.data\(\)$", re.S)
SPAN_DATA = re.compile(r"^(.*)\.data\(\)$", re.S)


def _Closing(text: str, open_at: int) -> int:
  """The index of the parenthesis closing the one at open_at (string and character literals skipped)."""
  depth = 0
  at = open_at
  while at < len(text):
    c = text[at]
    if c in "\"'":
      at = _LiteralEnd(text, at)
      continue
    if c in "([{":
      depth += 1
    elif c in ")]}":
      depth -= 1
      if depth == 0:
        return at
    at += 1
  raise ValueError(f"unbalanced call at {open_at}")


def _LiteralEnd(text: str, at: int) -> int:
  quote = text[at]
  at += 1
  while text[at] != quote:
    at += 2 if text[at] == "\\" else 1
  return at + 1


def SplitArguments(text: str) -> list[str]:
  """The top-level comma-separated arguments of a call's argument text (inside its parentheses)."""
  arguments, depth, start, at = [], 0, 0, 0
  while at < len(text):
    c = text[at]
    if c in "\"'":
      at = _LiteralEnd(text, at)
      continue
    if c in "([{":
      depth += 1
    elif c in ")]}":
      depth -= 1
    elif c == "," and depth == 0:
      arguments.append(text[start:at].strip())
      start = at + 1
    at += 1
  if text[start:].strip():
    arguments.append(text[start:].strip())
  return arguments


def Bytes(argument: str) -> str | None:
  """The span an image pointer starts: `port_far(s, o)` is FarBytes(game, s, o), a near view's `.data()` the view."""
  argument = " ".join(argument.split())
  if far := FAR.match(argument):
    return f"FarBytes(game, {far.group(1)})"
  if near := NEAR_DATA.match(argument):
    return near.group(1)
  if span := SPAN_DATA.match(argument):
    return span.group(1)
  return None


def DisplayCall(name: str, arguments: list[str]) -> str | None:
  """The Display method call replacing name(arguments), or None when the arguments do not fit the shape."""
  method, shape = DISPLAY_CALLS[name]
  if len(arguments) != ARITY[shape]:
    return None
  a = arguments
  call = f"{DISPLAY}.{method}"
  if shape == "none":
    return f"{call}()"
  if shape == "area_image":
    image = Bytes(a[5])
    return image and f"{call}({{ {a[0]}, {a[1]}, {a[2]}, {a[3]} }}, {{ {a[4]}, {image} }})"
  if shape == "area_bytes":
    image = Bytes(a[4])
    return image and f"{call}({{ {a[0]}, {a[1]}, {a[2]}, {a[3]} }}, {image})"
  if shape == "area_point":
    return f"{call}({{ {a[0]}, {a[1]}, {a[2]}, {a[3]} }}, {{ {a[4]}, {a[5]} }})"
  if shape == "point_reader":
    return f"{call}({{ {a[0]}, {a[1]} }}, {a[2]})"
  if shape == "area_colour":
    return f"{call}({{ {a[0]}, {a[1]}, {a[2]}, {a[3]} }}, {a[4]})"
  if shape == "corners_colour":
    return f"{call}({{ {{ {a[0]}, {a[1]} }}, {{ {a[2]}, {a[3]} }} }}, {a[4]})"
  if shape == "two_points":
    return f"{call}({{ {a[0]}, {a[1]} }}, {{ {a[2]}, {a[3]} }})"
  masks = [Bytes(a[4]), Bytes(a[5]), Bytes(a[6])]
  if None in masks:
    return None
  return f"{call}({{ {a[0]}, {a[1]}, {a[2]}, {a[3]} }}, {{ .and_mask = {masks[0]}, .or_image = {masks[1]}, .buffer = {masks[2]}, .flags = {a[7]} }})"


DISPLAY_NAME = re.compile(r"\b(" + "|".join(sorted(DISPLAY_CALLS, key=len, reverse=True)) + r")\(")


def RewriteDisplay(text: str) -> tuple[str, Counter, list[str]]:
  """Every drawing-library call of text as its Display method; the counts per function and the calls left as they
  were (their arguments do not fit the shape: rewritten by hand)."""
  counts: Counter = Counter()
  left: list[str] = []
  out, at = [], 0
  for match in DISPLAY_NAME.finditer(text):
    if match.start() < at or (match.start() > 0 and (text[match.start() - 1] in ".>:" )):
      continue
    close = _Closing(text, match.end() - 1)
    replacement = DisplayCall(match.group(1), SplitArguments(text[match.end():close]))
    if replacement is None:
      left.append(" ".join(text[match.start():close + 1].split()))
      continue
    out.append(text[at:match.start()])
    out.append(replacement)
    at = close + 1
    counts[match.group(1)] += 1
  out.append(text[at:])
  return "".join(out), counts, left


# ---- capped arithmetic ----------------------------------------------------------------------------------------------
# core's helpers that cap a byte or word through a pointer become value-returning (core/numeric.hpp):
# `opeu_add_byte(amount, limit, &x)` -> `x = AddByte(x, amount, limit)`, `opeu_sub_word(amount, &x)` ->
# `x = SubWord(x, amount)`; a call whose value is used keeps it as the parenthesised assignment.

CAPPED = {"opeu_add_byte": "AddByte", "opeu_sub_byte": "SubByte", "opeu_add_word": "AddWord", "opeu_sub_word": "SubWord"}
CAPPED_NAME = re.compile(r"\b(opeu_(?:add|sub)_(?:byte|word))\(")


def _IsStatement(text: str, start: int, close: int) -> bool:
  """Whether the call at start..close is a whole expression statement: it opens its line or follows a statement's end."""
  line = text[text.rfind("\n", 0, start) + 1:start]
  before = text[:start].rstrip()
  opens = not line.strip() or before[-1:] in (";", "{", "}", ":", ")") or before.endswith("else")
  return opens and text[close + 1:].lstrip().startswith(";")


def RewriteCapped(text: str) -> tuple[str, Counter, list[str]]:
  """Every capped add or subtract through `&lvalue` as an assignment of the value-returning helper; the calls whose
  pointer is not `&lvalue` are left (by hand)."""
  counts: Counter = Counter()
  left: list[str] = []
  out, at = [], 0
  for match in CAPPED_NAME.finditer(text):
    if match.start() < at or text[match.start() - 1:match.start()] in (".", ">", ":"):
      continue
    close = _Closing(text, match.end() - 1)
    arguments = SplitArguments(text[match.end():close])
    if not arguments[-1].startswith("&") or text[:match.start()].rstrip().endswith(("uint8_t", "uint16_t")):
      left.append(" ".join(text[match.start():close + 1].split()))
      continue
    target = arguments[-1][1:].strip()
    call = f"{target} = {CAPPED[match.group(1)]}({', '.join([target, *arguments[:-1]])})"
    out.append(text[at:match.start()])
    out.append(call if _IsStatement(text, match.start(), close) else f"({call})")
    at = close + 1
    counts[match.group(1)] += 1
  out.append(text[at:])
  return "".join(out), counts, left
