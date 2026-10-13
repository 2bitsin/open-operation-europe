"""B1 (#14a): `Game& game` replaces the data segment parameter in game/.

Token rules, applied to every C++ file under game/ (and to glue/include/glue.hpp, which moves to game/link/glue.hpp:
it declares what game/link defines, and glue cannot name the game's types):

  parameter   `[const] [std::]uint8_t [const]* ds`        -> `Game& game` (`Game const& game` when const)
              `[const] [std::]uint8_t ds[N]`              -> `Game& game` (`Game const& game` when const)
              an unnamed first `[std::]uint8_t*` of a function-pointer type `(*)(...)` or of a routine -> `Game&`
  byte        `ds[i]`                                     -> `Byte(game, i)`
  view        `ds + n`, `PORT_DS(n)`                      -> `game.State().Bytes(n).data()` (listed for review)
  name        every other `ds` (arguments, `Data(ds)`, captures, `(void)ds`) -> `game`
  includes    `"glue.hpp"` -> `"link/glue.hpp"`; `"common/game.hpp"` where the file uses the game; `"link/link.hpp"`
              where it names the link table (port_call, port_service, PortHandler, ...)

Left alone: `ds` after `.`, `->` or `::` (register fields), a field declared in a list (`..., ss, ds;`), and the files
in HAND, which define the accessors the rules target; the hand patch (_local/m2/b1-hand.patch) does those and every
site the listing names."""
from __future__ import annotations

import re
from collections import Counter
from dataclasses import dataclass, field
from pathlib import Path, PurePosixPath

from .source import SOURCES, SourceFile, SourceTree
from .tokens import IDENTIFIER, Token

SCOPE = "game"
MOVED = {PurePosixPath("glue/include/glue.hpp"): PurePosixPath("game/link/glue.hpp")}
HAND = {
  PurePosixPath("game/common/data-segment.hpp"),  # Data(bytes): the segment's typed view, which GameState uses
  PurePosixPath("game/common/memory.hpp"),        # Word/SetWord/Byte on raw bytes become the Game overloads
}
GAME_HEADER = "common/game.hpp"
EVENTS_HEADER = "common/events.hpp"
NAMES_GAME = re.compile(r"\bGame\b")
ROUTINE = re.compile(r"(?:opeu|svc|port_routine)_\w+$")
GAME_BODY = re.compile(r"\b(?:Data|Byte|Word|SetWord)\s*\(\s*game\b|\bgame\s*\.")
LINK_HEADER = "link/link.hpp"
GLUE_HEADER = "link/glue.hpp"
LINK_NAMES = re.compile(r"\b(?:port_call|port_service|port_routine_name|port_routine_address|PortHandler|PortRoutine|PortLink)\b")
GAME_USE = re.compile(r"\bgame\s*(?:\.|,|\))|\bGame\b")
TYPE_WORDS = {"std", "::", "uint8_t", "const"}
STOPS = {",", ")", ";", "]", "}", "?", ":"}
OPENING = {"(": ")", "[": "]", "{": "}"}
DECLARATION_STARTS = {"(", ",", ";", "{", "}"}  # a parameter, or a member or local declared on its own

PARAMETER = "parameter"
BYTE = "byte"
VIEW = "view"
NAME = "name"


@dataclass
class Edit:
  start: int
  end:   int
  text:  str


@dataclass
class Site:
  kind: str
  file: str
  line: int
  text: str


@dataclass
class FilePass:
  """The edits of one file and the sites they come from."""
  source: SourceFile
  edits:  list[Edit] = field(default_factory=list)
  sites:  list[Site] = field(default_factory=list)

  def Record(self, kind: str, index: int, edits: list[Edit]) -> None:
    token = self.source.tokens[index]
    self.edits += edits
    self.sites.append(Site(kind, str(self.source.path), token.line, _Line(self.source.text, token.start)))


@dataclass
class B1Result:
  files: dict[str, str] = field(default_factory=dict)  # destination path (relative to sources/opeu) -> new text
  moved: dict[str, str] = field(default_factory=dict)  # old path -> new path
  sites: list[Site] = field(default_factory=list)

  def Counts(self) -> dict[str, int]:
    return dict(sorted(Counter(site.kind for site in self.sites).items()))

  def Listing(self, kind: str) -> list[str]:
    return [f"{site.file}:{site.line}: {site.text}" for site in self.sites if site.kind == kind]


def B1Pass(tree: SourceTree) -> B1Result:
  """The rewritten text of every game/ file the pass changes (and of the moved header), with the sites changed."""
  result = B1Result()
  for path, source in sorted(tree.files.items()):
    destination = MOVED.get(path, path)
    if not str(destination).startswith(SCOPE + "/") or destination in HAND:
      continue
    rewritten, sites = RewriteFile(source)
    if rewritten != source.text or destination != path:
      result.files[str(destination)] = rewritten
      result.sites += sites
    if destination != path:
      result.moved[str(path)] = str(destination)
  return result


def WriteB1(root: Path, result: B1Result) -> None:
  for path, text in result.files.items():
    target = root / SOURCES / path
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text)
  for old in result.moved:
    (root / SOURCES / old).unlink(missing_ok=True)


def RewriteFile(source: SourceFile) -> tuple[str, list[Site]]:
  file_pass = FilePass(source)
  _Scan(file_pass)
  text = _Splice(source.text, file_pass.edits)
  text = text.replace('#include "glue.hpp"', f'#include "{GLUE_HEADER}"')
  return _AddIncludes(source.path, text), file_pass.sites


def _Scan(file_pass: FilePass) -> None:
  tokens = file_pass.source.tokens
  skip_to = 0
  for index, token in enumerate(tokens):
    if index < skip_to:
      continue
    if token.text == "PORT_DS" and _At(tokens, index + 1) == "(":
      _PortDs(file_pass, index)
    elif token.text == "*" and _UnnamedFirst(tokens, index):
      _UnnamedParameter(file_pass, index)
    elif token.text == "ds" and _At(tokens, index - 1) not in (".", "->", "::") and not _FieldInList(tokens, index):
      skip_to = _Ds(file_pass, index)


def _Ds(file_pass: FilePass, index: int) -> int:
  """Rewrites the `ds` at index; returns the first token index the scan resumes at."""
  tokens = file_pass.source.tokens
  type_start = _TypeStart(tokens, index)
  if type_start is not None and _At(tokens, index - 1) == "*":
    const = "const" in (token.text for token in tokens[type_start:index])
    file_pass.Record(PARAMETER, index, [Edit(tokens[type_start].start, tokens[index].end, "Game const& game" if const else "Game& game")])
    return index + 1
  if type_start is not None and _At(tokens, index + 1) == "[":
    close = file_pass.source.partners[index + 1]
    const = "const" in (token.text for token in tokens[type_start:index])
    file_pass.Record(PARAMETER, index, [Edit(tokens[type_start].start, tokens[close].end, "Game const& game" if const else "Game& game")])
    return close + 1
  if _At(tokens, index + 1) == "[":
    close = file_pass.source.partners[index + 1]
    file_pass.Record(BYTE, index, [Edit(tokens[index].start, tokens[index + 1].end, "Byte(game, "), Edit(tokens[close].start, tokens[close].end, ")")])
    return index + 2
  if _At(tokens, index + 1) == "+":
    end = _ExpressionEnd(tokens, file_pass.source.partners, index + 2)
    file_pass.Record(VIEW, index, [Edit(tokens[index].start, tokens[index + 2].start, "game.State().Bytes("), Edit(tokens[end - 1].end, tokens[end - 1].end, ").data()")])
    return index + 2
  file_pass.Record(NAME, index, [Edit(tokens[index].start, tokens[index].end, "game")])
  return index + 1


def _PortDs(file_pass: FilePass, index: int) -> None:
  tokens = file_pass.source.tokens
  close = file_pass.source.partners[index + 1]
  file_pass.Record(VIEW, index, [Edit(tokens[index].start, tokens[index + 1].end, "game.State().Bytes("), Edit(tokens[close].start, tokens[close].end, ").data()")])


def _UnnamedParameter(file_pass: FilePass, star: int) -> None:
  tokens = file_pass.source.tokens
  start = _TypeStart(tokens, star + 1)
  file_pass.Record(PARAMETER, star, [Edit(tokens[start].start, tokens[star].end, "Game&")])


def _TypeStart(tokens: list[Token], index: int) -> int | None:
  """The first token of the `[const] [std::]uint8_t [const] [*]` type written right before index, if it is one."""
  at = index - 1
  if _At(tokens, at) == "*":
    at -= 1
  start = at + 1
  while at >= 0 and tokens[at].text in TYPE_WORDS:
    start = at
    at -= 1
  words = [token.text for token in tokens[start:index] if token.text != "*"]
  return start if "uint8_t" in words and _At(tokens, start - 1) in DECLARATION_STARTS else None


def _UnnamedFirst(tokens: list[Token], star: int) -> bool:
  """The `*` ends the first, unnamed parameter of a function-pointer type `(*[name])( [std::]uint8_t* ,` or of a game
  routine `opeu_x( [std::]uint8_t* ,` (also svc_*, port_routine_*)."""
  if _At(tokens, star + 1) not in (",", ")"):
    return False
  start = _TypeStart(tokens, star + 1)
  if start is None or _At(tokens, start - 1) != "(":
    return False
  if ROUTINE.match(_At(tokens, start - 2)):
    return True  # a routine's definition that ignores the segment
  if _At(tokens, start - 2) != ")":
    return False
  named = tokens[start - 3].kind == IDENTIFIER
  return [t.text for t in tokens[start - 4 - named:start - 2 - named]] == ["(", "*"]


def _FieldInList(tokens: list[Token], index: int) -> bool:
  """`..., ss, ds;`: a register field declared in a list, not the data segment."""
  return _At(tokens, index - 1) == "," and _At(tokens, index + 1) == ";"


def _ExpressionEnd(tokens: list[Token], partners: dict[int, int], index: int) -> int:
  """One past the last token of the additive operand starting at index (it runs to a separator at its own depth)."""
  at = index
  while at < len(tokens) and tokens[at].text not in STOPS:
    at = partners[at] + 1 if tokens[at].text in OPENING and at in partners else at + 1
  return at


def _At(tokens: list[Token], index: int) -> str:
  return tokens[index].text if 0 <= index < len(tokens) else ""


def _Line(text: str, offset: int) -> str:
  start = text.rfind("\n", 0, offset) + 1
  end = text.find("\n", offset)
  return text[start:end if end >= 0 else len(text)].strip()


def _Splice(text: str, edits: list[Edit]) -> str:
  for edit in sorted(edits, key=lambda edit: (edit.start, edit.end), reverse=True):
    text = text[:edit.start] + edit.text + text[edit.end:]
  return text


def _AddIncludes(path: PurePosixPath, text: str) -> str:
  wanted = []
  if path.suffix == ".cpp" and GAME_USE.search(_Code(text)) and not path.name.startswith("game"):
    wanted.append(GAME_HEADER)
  if path.suffix == ".hpp" and GAME_BODY.search(_Code(text)) and not path.name.startswith("game"):
    wanted.append(GAME_HEADER)  # an inline body reaches the game's state
  elif path.suffix == ".hpp" and NAMES_GAME.search(_Code(text)) and str(path) != f"game/{EVENTS_HEADER}" and not _Includes(text, (EVENTS_HEADER, GAME_HEADER)):
    wanted.append(EVENTS_HEADER)  # a declaration needs only the name: events.hpp declares it
  if LINK_NAMES.search(_Code(text)) and str(path) != f"game/{LINK_HEADER}":
    wanted.append(LINK_HEADER)
  for header in wanted:
    if f'#include "{header}"' not in text:
      text = _Insert(text, f'#include "{header}"')
  return text


def _Includes(text: str, headers: tuple[str, ...]) -> bool:
  return any(f'#include "{header}"' in text for header in headers)


def _Insert(text: str, line: str) -> str:
  """After the file's last quoted include (or its first line when it has none)."""
  quoted = [match.end() for match in re.finditer(r'^#include ".*$', text, re.M)]
  at = quoted[-1] if quoted else text.find("\n") + 1
  return text[:at] + "\n" + line + text[at:] if quoted else text[:at] + line + "\n" + text[at:]


def _Code(text: str) -> str:
  """The text without comments and string literals (so a word in prose does not ask for an include)."""
  return re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\\n])*"', " ", text, flags=re.S)
