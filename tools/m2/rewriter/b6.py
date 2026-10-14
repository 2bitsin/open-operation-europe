"""The B6 pass (#14): the game's sprintf and printf take typed argument lists.

  - `opeu_request_format(game, std::array<std::uint16_t, N>{ destination, format, words... })` becomes
    `opeu_request_format(game, destination, std::array<FormatArgument, N - 1>{ format, words... })`: the destination
    is a TextTarget (a near address or a buffer the routine owns), the rest FormatArguments (a word, or a text a %s
    prints).
  - `opeu_cursor_print(game, std::array<std::uint16_t, N>{ ... })` keeps its words as `std::array<FormatArgument, N>`.
  - an argument of either call that is a whole `static_cast<std::uint16_t>(...)` loses the cast: a FormatArgument takes
    any integer as its 16-bit word, and a text (a name a routine wrote) must reach it as a text.
  - the advice routines' far text pointer (`const uint8_t* far_text`, named or not, and the advice checks' function
    type) becomes the span of the far advice texts, `std::span<std::uint8_t const>`.

The calls whose words are a named array, and the reserves themselves (ServiceLocals -> TextBuffer and typed locals),
are rewritten by hand: each shape occurs a few times only (docs/port.md "M2 rewriter")."""
from __future__ import annotations

import re
from collections import Counter
from dataclasses import dataclass, field
from pathlib import Path

from .source import SOURCES

ARRAY = re.compile(r"std::array<(?:std::)?uint16_t,\s*")
CALL = re.compile(r"\b(opeu_request_format|opeu_cursor_print)\(")
# `const uint8_t* far_text`, `uint8_t const* far_text`, an unnamed `const uint8_t*` beside a Game & in a parameter list.
FAR_TEXT = re.compile(r"\b(?:const\s+(?:std::)?uint8_t|(?:std::)?uint8_t\s+const)\s*\*(?=\s*far_text\b|\s*[,)])")
FAR_TEXT_TYPE = "std::span<std::uint8_t const>"


@dataclass
class B6Result:
  files: dict[str, Counter] = field(default_factory=dict)  # file -> kind -> sites
  left:  list[str]          = field(default_factory=list)  # calls whose words are no braced array: by hand

  def Counts(self) -> dict[str, int]:
    total: Counter = Counter()
    for counts in self.files.values():
      total.update(counts)
    return dict(sorted(total.items()))


def _Close(text: str, at: int) -> int:
  """The index of the bracket closing the one opened just before `at` (parentheses, braces and brackets are balanced;
  string and character literals are skipped)."""
  depth, i = 1, at
  while depth:
    c = text[i]
    if c in "\"'":
      i += 1
      while text[i] != c:
        i += 2 if text[i] == "\\" else 1
    elif c in "({[":
      depth += 1
    elif c in ")}]":
      depth -= 1
    i += 1
  return i - 1


def _TopLevelComma(text: str) -> int:
  """The index of the first comma of `text` outside any parenthesis, brace, bracket or template argument list (-1 when
  there is none)."""
  depth = 0
  for i, c in enumerate(text):
    if c in "({[" or (c == "<" and re.match(r"[\w:]", text[i - 1:i]) and not text[i + 1:i + 2] in "<="):
      depth += 1
    elif c in ")}]" or (c == ">" and depth > 0 and text[i - 1] != "-" and text[i + 1:i + 2] != "="):
      depth -= 1
    elif c == "," and depth == 0:
      return i
  return -1


def _Smaller(size: str) -> str:
  size = size.strip()
  return str(int(size) - 1) if size.isdigit() else f"{size} - 1"


def RewriteFormats(text: str) -> tuple[str, Counter, list[str]]:
  """Rewrites every sprintf and printf call whose words are a braced array; returns the text, the counts per kind and
  the calls left for the hand pass."""
  counts: Counter = Counter()
  left: list[str] = []
  out, at = [], 0
  for match in CALL.finditer(text):
    if match.start() < at:
      continue
    end = _Close(text, match.end())
    call = text[match.start():end + 1]
    rewritten = _Rewrite(match.group(1), call)
    if rewritten is None:
      left.append(" ".join(call.split()))
      continue
    out += [text[at:match.start()], rewritten]
    at = end + 1
    counts[match.group(1)] += 1
  out.append(text[at:])
  return "".join(out), +counts, left


def _Rewrite(name: str, call: str) -> str | None:
  """The call with its words rewritten, or None when they are no braced array of words."""
  head = call.index("(") + 1
  game_end = call.find(",", head)
  if game_end < 0:
    return None
  rest = call[game_end + 1:]
  array = ARRAY.search(rest)
  if array is None or rest[:array.start()].strip():
    return None
  size_end = _Close(rest.replace("<", "(").replace(">", ")"), array.end())
  size = rest[array.end():size_end]
  brace = rest.index("{", size_end)
  close = _Close(rest, brace + 1)
  if rest[close + 1:].strip() != ")":
    return None
  words = rest[brace + 1:close]
  prefix = call[:game_end + 1] + rest[:array.start()]
  if name == "opeu_cursor_print":
    return f"{prefix}std::array<FormatArgument, {size}>{{{words}}})"
  comma = _TopLevelComma(words)
  if comma < 0:
    return None
  destination = words[:comma].strip()
  remaining = words[comma + 1:]
  return f"{prefix}{destination}, std::array<FormatArgument, {_Smaller(size)}>{{{remaining}}})"


WORD_CAST = re.compile(r"static_cast<(?:std::)?uint16_t>\(")
TYPED_CALL = re.compile(r"\b(?:opeu_request_format|opeu_cursor_print)\((?=\s*_?game,)")


def _Arguments(text: str) -> list[str]:
  """The top-level comma-separated pieces of an argument list (whitespace kept)."""
  pieces, at = [], 0
  while True:
    comma = _TopLevelComma(text[at:])
    if comma < 0:
      return pieces + [text[at:]]
    pieces.append(text[at:at + comma])
    at += comma + 1


def _Uncast(argument: str) -> str:
  """The argument without a word cast that spans all of it."""
  stripped = argument.strip()
  cast = WORD_CAST.match(stripped)
  if cast is None or _Close(stripped, cast.end()) != len(stripped) - 1:
    return argument
  inner = stripped[cast.end():-1]
  return argument[:len(argument) - len(argument.lstrip())] + inner + argument[len(argument.rstrip()):]


def RewriteCasts(text: str) -> tuple[str, Counter]:
  """Drops the word casts that span a whole argument of a typed sprintf or printf call (its destination and the
  braced words)."""
  count = 0
  out, at = [], 0
  for match in TYPED_CALL.finditer(text):
    if match.start() < at:
      continue
    end = _Close(text, match.end())
    call = text[match.end():end]
    if "FormatArgument" not in call:
      continue
    brace = call.index("{", call.index("FormatArgument"))
    close = _Close(call, brace + 1)
    head = _Arguments(call[:call.rindex(",", 0, call.index("std::array<FormatArgument"))])
    words = _Arguments(call[brace + 1:close])
    new_head = [_Uncast(piece) for piece in head]
    new_words = [_Uncast(piece) for piece in words]
    count += sum(a != b for a, b in zip(head + words, new_head + new_words))
    rebuilt = ",".join(new_head) + call[call.rindex(",", 0, call.index("std::array<FormatArgument")):brace + 1] + ",".join(new_words) + call[close:]
    out += [text[at:match.end()], rebuilt]
    at = end
  out.append(text[at:])
  return "".join(out), Counter({"word_cast": count}) if count else Counter()


def RewriteFarText(text: str) -> tuple[str, Counter]:
  """The far advice text pointers become spans; returns the text and the count."""
  text, count = FAR_TEXT.subn(FAR_TEXT_TYPE, text)
  return text, Counter({"far_text": count}) if count else Counter()


# Reserves holding one local that is no text: bytes a DS service reads (sprites, masks, a request slot), a selection
# word, the death notice whose text overruns into its side byte. They are rewritten by hand.
HAND = {
  "core/airshow.cpp":  {"mask"},
  "core/evwound.cpp":  {"at"},
  "core/rqcommon.cpp": {"at"},
  "core/supstep.cpp":  {"selection"},
  "map/btlicon.cpp":   {"buffer"},
  "map/mapover.cpp":   {"buffer"},
  "map/mapview.cpp":   {"sprite_buffer"},
  "units/xferstep.cpp": {"selection"},
}
RESERVE = re.compile(r"^[ \t]*ServiceLocals const (\w+)\{ ([^;]+?) \};[^\n]*\n", re.M)


def _ScopeEnd(text: str, at: int) -> int:
  """The index of the brace closing the block the declaration at `at` is in."""
  depth, i = 0, at
  while True:
    c = text[i]
    if c == "{":
      depth += 1
    elif c == "}":
      if depth == 0:
        return i
      depth -= 1
    i += 1


def RewriteReserves(text: str, hand: frozenset[str] = frozenset()) -> tuple[str, Counter]:
  """Turns every reserve holding one text, named once and otherwise only cleared, into a TextBuffer the routine
  owns; the locals named in `hand` stay."""
  count = 0
  at = 0
  while (reserve := RESERVE.search(text, at)) is not None:
    name, size = reserve.groups()
    end = _ScopeEnd(text, reserve.end())
    scope = text[reserve.end():end]
    named = list(re.finditer(rf"^[ \t]*(?:auto|(?:std::)?uint16_t) const\s+(\w+)\{{ {name}\.At\(0\) \}};[^\n]*\n", scope, re.M))
    clears = list(re.finditer(rf"^[ \t]*{name}\.Clear\(\w+\);[^\n]*\n", scope, re.M))
    uses = len(re.findall(rf"\b{name}\.", scope))
    if len(named) != 1 or uses != 1 + len(clears) or named[0].group(1) in hand:
      at = reserve.end()
      continue
    for match in sorted(named + clears, key=lambda m: m.start(), reverse=True):
      scope = scope[:match.start()] + scope[match.end():]
    indent = reserve.group(0)[:len(reserve.group(0)) - len(reserve.group(0).lstrip())]
    declaration = f"{indent}TextBuffer<{size}> {named[0].group(1)};\n"
    text = text[:reserve.start()] + declaration + scope + text[end:]
    at = reserve.start() + len(declaration)
    count += 1
  return text, Counter({"text_buffer": count}) if count else Counter()


def B6Pass(tree: Path) -> B6Result:
  """Rewrites every game/ source of the tree in place."""
  result = B6Result()
  game = tree / SOURCES / "game"
  for path in sorted(p for p in game.rglob("*") if p.suffix in (".cpp", ".hpp") and "gen" not in p.parts):
    before = path.read_text()
    text, counts, left = RewriteFormats(before)
    text, spans = RewriteFarText(text)
    text, casts = RewriteCasts(text)
    name = path.relative_to(game).as_posix()
    text, buffers = RewriteReserves(text, frozenset(HAND.get(name, ())))
    counts += spans + casts + buffers
    result.left += [f"{name}: {call}" for call in left]
    if text != before:
      path.write_text(text)
      result.files[name] = counts
  return result
