"""B3 (#13): what the numbered services' direct calls leave behind, removed from the scratch tree.

`--apply b3` first replaces every resolved `Call(game, event, ID, words...)` with the routine call ID's case makes
(substitute.py; `--only` limits it to the sites whose callback is bound to dispatchers of some files). This pass then
applies, to the rewritten tree:

  parameter   a callback parameter (any event type: `HostEvent`, `HostService`, `HostWordsEvent`, `HostNotify`,
              `BattleService`, `PendingEvent`, ...) that is not used, or only handed on to other routines' dropped
              callback parameters, is dropped from the definition, from every declaration and from every call (a
              fixed point: one used for anything else, or of a routine whose address is taken, stays); a `(void)name;`
              that only silenced it goes with it
  dispatcher  a `svc_*` nothing names any more is deleted, and a routine only the dropped arguments named (an event
              written as a function, `LabelEvent`), then every function only deleted functions named
  enum        an enumeration whose enumerators were service ids (case labels of a dispatcher) is deleted when neither
              it nor any enumerator is named outside it any more, and a `constexpr` constant a site gave as its
              service id when nothing names it any more
  include     `common/events.hpp` goes from files that no longer name what it declares (the game's own name included)

Every kept parameter and enumeration is listed with its reason (B3Result.kept)."""
from __future__ import annotations

import re
from dataclasses import dataclass, field
from pathlib import Path, PurePosixPath

from .b2 import DELETE, EVENTS_HEADER, TIGHT, _AddressTaken, _Declarations, _DeleteFunctions, _Heads, _Line, _OpenParen, _References, _RemoveListItem, _Write
from .calls import IsCallback
from .constants import ConstantScopes
from .functions import Function
from .program import Program
from .source import SourceFile, SourceTree
from .syntax import Arguments, Span, Text
from .tokens import IDENTIFIER

EVENTS_INCLUDE = "common/events.hpp"


@dataclass
class B3Result:
  dropped:  list[str] = field(default_factory=list)  # file routine(parameter) whose callback parameter went
  sites:    int = 0                                  # call arguments removed with them
  deleted:  list[str] = field(default_factory=list)  # functions deleted (dispatchers and what only they named)
  enums:    list[str] = field(default_factory=list)  # service enumerations deleted
  constants: list[str] = field(default_factory=list)  # service id constants deleted
  includes: list[str] = field(default_factory=list)  # file: include removed
  kept:     list[str] = field(default_factory=list)  # what stays, and why
  unpassed: set[str] = field(default_factory=set)    # the routines dropped arguments named (deleted when unnamed now)

  def Counts(self) -> dict[str, int]:
    return {"dropped": len(self.dropped), "arguments": self.sites, "deleted": len(self.deleted), "enums": len(self.enums),
            "constants": len(self.constants), "includes": len(self.includes), "kept": len(self.kept)}


def B3Pass(root: Path, labels: set[str], ids: set[str] = frozenset()) -> B3Result:
  """Runs every rule on the tree under root (a scratch export); labels: the case labels of the dispatchers before the
  sites were replaced (the enumerations they name are the service ids); ids: the names the sites gave as service ids
  (the constants among them nothing names any more go)."""
  result = B3Result()
  _Write(root, None, DropCallbacks(Program(SourceTree(root)), result))
  _Write(root, None, DeleteDispatchers(Program(SourceTree(root)), result))
  _Write(root, None, DeleteEnums(SourceTree(root), labels, result))
  _Write(root, None, DeleteServiceConstants(SourceTree(root), ids, result))
  _Write(root, None, DeleteEmptyHeaders(SourceTree(root), result))
  _Write(root, None, DropEventsInclude(SourceTree(root), result))
  return result


# ---- parameter ---------------------------------------------------------------------------------------------------------

@dataclass(frozen=True)
class Slot:
  """One callback parameter of one definition."""
  file:  PurePosixPath
  name:  str
  index: int


@dataclass
class _Graph:
  slots:    set[Slot] = field(default_factory=set)
  edges:    dict[Slot, set[Slot]] = field(default_factory=dict)  # a parameter -> the parameters it is handed on to
  live:     dict[Slot, str] = field(default_factory=dict)        # a parameter used otherwise -> why
  silenced: dict[Slot, list[Span]] = field(default_factory=dict)  # its `(void)name;` statements


def DropCallbacks(program: Program, result: B3Result) -> dict:
  graph = CallbackGraph(program)
  Propagate(graph)
  dropped = {slot for slot in graph.slots if slot not in graph.live}
  result.kept += sorted(f"{slot.file} {slot.name} parameter {slot.index}: {graph.live[slot]}" for slot in graph.slots if slot in graph.live)
  result.dropped += sorted(f"{slot.file} {slot.name}({_Definition(program, slot).parameters[slot.index].name or 'unnamed'})" for slot in dropped)
  edits: dict = {}
  for slot in dropped:
    definition = _Definition(program, slot)
    _RemoveListItem(program.Source(slot.file), definition.name_index + 1, slot.index, edits)
    for declaration in _Declarations(program, definition):
      _RemoveListItem(program.Source(declaration.file), declaration.name_index + 1, slot.index, edits)
    for statement in graph.silenced.get(slot, []):
      _RemoveStatement(program.Source(slot.file), statement, edits)
  result.sites += _DropArguments(program, dropped, edits, result.unpassed)
  return edits


def CallbackGraph(program: Program) -> _Graph:
  """Every callback parameter, the parameters each one is handed on to, and those used otherwise (with why)."""
  graph = _Graph()
  taken = _AddressTaken(program, _CallbackRoutines(program))
  for name, definitions in program.definitions.items():
    for function in definitions:
      if not str(function.file).startswith("game/"):
        continue
      for index, parameter in enumerate(function.parameters):
        if not IsCallback(parameter.type):
          continue
        slot = Slot(function.file, name, index)
        graph.slots.add(slot)
        graph.edges[slot] = set()
        if (function.file, name) in taken:
          graph.live[slot] = f"the routine's address is taken ({taken[function.file, name]})"
        elif parameter.name:
          _Uses(program, function, parameter.name, slot, graph)
  return graph


def _CallbackRoutines(program: Program) -> set[str]:
  return {name for name, definitions in program.definitions.items() if any(IsCallback(p.type) for f in definitions for p in f.parameters)}


def _Uses(program: Program, function: Function, name: str, slot: Slot, graph: _Graph) -> None:
  source = program.Source(function.file)
  tokens = source.tokens
  for index in range(function.body.first, function.body.last):
    if tokens[index].text != name or tokens[index - 1].text in TIGHT:
      continue
    silenced = _Silenced(source, index)
    if silenced is not None:
      graph.silenced.setdefault(slot, []).append(silenced)
      continue
    target = PassedTo(program, source, index)
    if target is None:
      graph.live.setdefault(slot, f"{function.file}:{tokens[index].line} uses it: {_Line(source.text, tokens[index].start)}")
    else:
      graph.edges[slot].add(target)


def _Silenced(source: SourceFile, index: int) -> Span | None:
  """The statement `(void)name;` around the name at index, if that is all it is."""
  tokens = source.tokens
  texts = [token.text for token in tokens[index - 3:index + 2]]
  return Span(index - 3, index + 2) if texts[:3] == ["(", "void", ")"] and texts[4:] == [";"] and tokens[index - 4].text in (";", "{", "}") else None


def PassedTo(program: Program, source: SourceFile, index: int) -> Slot | None:
  """The callback parameter of a routine the token at index is passed to as a whole argument, if it is one."""
  tokens, partners = source.tokens, source.partners
  if tokens[index - 1].text not in ("(", ",") or tokens[index + 1].text not in (")", ","):
    return None
  paren = _OpenParen(tokens, partners, index)
  if paren is None or tokens[paren - 1].kind != IDENTIFIER or tokens[paren - 2].text in TIGHT:
    return None
  callee = program.Resolve(tokens[paren - 1].text, source.path)
  position = next((i for i, part in enumerate(Arguments(tokens, partners, paren)) if part.first == index), None)
  if callee is None or position is None or position >= len(callee.parameters) or not IsCallback(callee.parameters[position].type):
    return None
  return Slot(callee.file, callee.name, position)


def Propagate(graph: _Graph) -> None:
  """A parameter handed on to a kept parameter (or one the tool does not model) is kept too."""
  changed = True
  while changed:
    changed = False
    for slot, targets in graph.edges.items():
      reached = next((target for target in sorted(targets, key=str) if target in graph.live or target not in graph.edges), None)
      if slot not in graph.live and reached is not None:
        graph.live[slot] = f"handed on to {reached.name} parameter {reached.index}, which stays"
        changed = True


def _Definition(program: Program, slot: Slot) -> Function:
  return next(f for f in program.definitions[slot.name] if f.file == slot.file)


def _DropArguments(program: Program, dropped: set[Slot], edits: dict, unpassed: set[str]) -> int:
  """Removes the dropped parameters' arguments from every call by name, adding the names they were to unpassed;
  returns how many."""
  by_name: dict[str, set[Slot]] = {}
  for slot in dropped:
    by_name.setdefault(slot.name, set()).add(slot)
  heads = _Heads(program)
  removed = 0
  for path, source in program.tree.files.items():
    tokens = source.tokens
    for index, token in enumerate(tokens[:-1]):
      if token.text not in by_name or tokens[index + 1].text != "(" or (path, index) in heads or tokens[index - 1].text in (".", "->"):
        continue
      callee = program.Resolve(token.text, path)
      for slot in sorted(by_name[token.text], key=lambda s: -s.index):
        if callee is not None and callee.file == slot.file:
          unpassed.add(_CheckDroppable(source, index + 1, slot.index))
          _RemoveListItem(source, index + 1, slot.index, edits)
          removed += 1
  return removed


def _CheckDroppable(source: SourceFile, paren: int, position: int) -> str:
  """A dropped argument is a name (a dispatcher or a callback handed on): dropping it loses no side effect. Returns
  the name."""
  parts = Arguments(source.tokens, source.partners, paren)
  if position >= len(parts):
    return ""
  text = Text(source.tokens, parts[position], source.text)
  if not re.fullmatch(r"[A-Za-z_]\w*", text):
    raise ValueError(f"{source.path}:{source.tokens[paren].line}: the dropped argument `{text}` is not a name")
  return text


def _RemoveStatement(source: SourceFile, statement: Span, edits: dict) -> None:
  """Removes a statement; its whole line when nothing else is on it."""
  tokens, text = source.tokens, source.text
  start, end = tokens[statement.first].start, tokens[statement.last - 1].end
  line_start = text.rfind("\n", 0, start) + 1
  line_end = text.find("\n", end)
  if not text[line_start:start].strip() and not text[end:line_end].strip():
    start, end = line_start, line_end + 1
  edits.setdefault(source.path, set()).add((start, end, ""))


# ---- dispatcher ----------------------------------------------------------------------------------------------------------

def DeleteDispatchers(program: Program, result: B3Result) -> dict:
  """`svc_*` functions of game/ nothing names, and the routines only the dropped arguments named, then (to a fixed
  point) every function only deleted functions name."""
  references = _References(program)
  dead: set[str] = set()
  changed = True
  while changed:
    changed = False
    for name, definitions in program.definitions.items():
      if name in dead or not all(str(f.file).startswith("game/") for f in definitions):
        continue
      outside = [where for where, inside in references.get(name, []) if inside not in dead]
      if not outside and (name.startswith("svc_") or name in result.unpassed or references.get(name)):
        dead.add(name)
        changed = True
  edits: dict = {}
  for name in sorted(dead):
    _DeleteFunctions(program, program.definitions[name] + program.declarations.get(name, []), edits)
  result.deleted += sorted(dead)
  return edits


# ---- enum ----------------------------------------------------------------------------------------------------------------

@dataclass
class Enumeration:
  file:        PurePosixPath
  name:        str             # "" for an unnamed one
  scoped:      bool            # enum class
  enumerators: list[str]
  span:        Span            # from `enum` to the `;` after the closing brace


def FindEnumerations(source: SourceFile) -> list[Enumeration]:
  tokens, partners = source.tokens, source.partners
  found = []
  for index, token in enumerate(tokens):
    if token.text != "enum" or tokens[index - 1].text in TIGHT:
      continue
    brace = next((i for i in range(index + 1, min(index + 12, len(tokens))) if tokens[i].text in ("{", ";")), None)
    if brace is None or tokens[brace].text != "{" or brace not in partners:
      continue
    scoped = tokens[index + 1].text in ("class", "struct")
    name_at = index + (2 if scoped else 1)
    name = tokens[name_at].text if tokens[name_at].kind == IDENTIFIER else ""
    close = partners[brace]
    enumerators = [tokens[i].text for i in range(brace + 1, close) if tokens[i].kind == IDENTIFIER and tokens[i - 1].text in ("{", ",")]
    end = close + 1 if close + 1 < len(tokens) and tokens[close + 1].text == ";" else close
    found.append(Enumeration(source.path, name, scoped, enumerators, Span(index, end + 1)))
  return found


def DeleteEnums(tree: SourceTree, labels: set[str], result: B3Result) -> dict:
  """Deletes the service enumerations nothing names any more (enumerations with an enumerator among labels)."""
  scopes = ConstantScopes(tree)
  edits: dict = {}
  for path, source in tree.files.items():
    if not str(path).startswith("game/"):
      continue
    for enumeration in FindEnumerations(source):
      if not set(enumeration.enumerators) & labels:
        continue
      where = _EnumerationNamed(tree, scopes, enumeration)
      described = f"{path} {enumeration.name or '(unnamed)'}"
      if where:
        result.kept.append(f"{described}: {where}")
        continue
      first, last = source.tokens[enumeration.span.first].line, source.tokens[enumeration.span.last - 1].line
      edits.setdefault(path, set()).add(("lines", first, last))
      edits[path] |= _ReExports(source, [enumeration.name, *enumeration.enumerators])
      result.enums.append(described)
  return edits


def _ReExports(source: SourceFile, names: list[str]) -> set:
  """The re-export lines `using detail::x::NAME;` of names in the file's own re-export block."""
  wanted = "|".join(re.escape(name) for name in names if name)
  if not wanted:
    return set()
  return {(match.start(), match.end(), "") for match in re.finditer(rf"^[ \t]*using[ \t]+[\w:]+::(?:{wanted});[ \t]*\n", source.text, re.M)}


def DeleteServiceConstants(tree: SourceTree, ids: set[str], result: B3Result) -> dict:
  """Deletes `constexpr TYPE NAME{ ... };` (or `= ...;`) declarations of service ids the sites gave that nothing names
  any more."""
  scopes = ConstantScopes(tree)
  edits: dict = {}
  for path, source in tree.files.items():
    if not str(path).startswith("game/"):
      continue
    for name, span in _Constants(source, ids):
      if _EnumerationNamed(tree, scopes, Enumeration(path, "", False, [name], span)):
        continue
      first, last = source.tokens[span.first].line, source.tokens[span.last - 1].line
      edits.setdefault(path, set()).add(("lines", first, last))
      result.constants.append(f"{path} {name}")
  return edits


def _Constants(source: SourceFile, ids: set[str]) -> list[tuple[str, Span]]:
  """The `constexpr` declarations of one name among ids that stand alone on their lines: (name, span)."""
  tokens, partners = source.tokens, source.partners
  found = []
  for index, token in enumerate(tokens):
    if token.text != "constexpr" or tokens[index - 1].text not in (";", "{", "}", "inline", "static"):
      continue
    end = index
    while end < len(tokens) and tokens[end].text not in ("{", "=", ";", "("):
      end += 1
    if end >= len(tokens) or tokens[end].text not in ("{", "=") or tokens[end - 1].kind != IDENTIFIER or tokens[end - 1].text not in ids:
      continue
    close = partners[end] + 1 if tokens[end].text == "{" else next(i for i in range(end, len(tokens)) if tokens[i].text == ";")
    if close < len(tokens) and tokens[close].text == ";" and tokens[close].line == tokens[index].line:
      start = index - 1 if tokens[index - 1].text in ("inline", "static") else index
      found.append((tokens[end - 1].text, Span(start, close + 1)))
  return found


def _EnumerationNamed(tree: SourceTree, scopes: ConstantScopes, enumeration: Enumeration) -> str:
  """Where the enumeration or one of its enumerators is still named outside its own definition (in its file, or in a
  file that includes its header), else ""."""
  names = set(enumeration.enumerators)
  for path, source in tree.files.items():
    tokens = source.tokens
    own = path == enumeration.file
    if not own and (enumeration.file.suffix != ".hpp" or enumeration.file not in scopes.Closure(path)):
      continue
    for index, token in enumerate(tokens):
      if own and enumeration.span.Contains(index):
        continue
      if token.kind != IDENTIFIER or tokens[index - 1].text in (".", "->"):
        continue
      if enumeration.name and token.text == enumeration.name and tokens[index - 1].text != "::":
        return f"{enumeration.name} is named at {path}:{token.line}"
      qualified = tokens[index - 1].text == "::" and tokens[index - 2].text == enumeration.name
      bare = tokens[index - 1].text != "::" and (own or not enumeration.scoped)
      if token.text in names and (qualified or bare):
        return f"{token.text} is named at {path}:{token.line}"
  return ""


# ---- header --------------------------------------------------------------------------------------------------------------

EMPTY_NAMESPACE = re.compile(r"^namespace[ \t]+[\w:]+[ \t]*\{\s*\}[ \t]*(//[^\n]*)?\n?", re.M)


def DeleteEmptyHeaders(tree: SourceTree, result: B3Result) -> dict:
  """A game header left with nothing but comments, `#pragma once`, includes and empty namespaces is deleted, and every
  `#include` of it; a header that still declares something loses its empty namespace blocks."""
  edits: dict = {}
  for path, source in tree.files.items():
    if not str(path).startswith("game/") or path.suffix != ".hpp":
      continue
    empties = list(EMPTY_NAMESPACE.finditer(source.text))
    rest = EMPTY_NAMESPACE.sub("", source.text)
    if _Declares(rest):
      edits.setdefault(path, set()).update((match.start(), match.end(), "") for match in empties)
      continue
    edits[path] = {DELETE}
    result.deleted.append(f"{path} (empty header)")
    include = str(path.relative_to("game"))
    for other, text in ((other, s.text) for other, s in tree.files.items() if other != path):
      for match in re.finditer(rf'^#include "{re.escape(include)}"[^\n]*\n', text, re.M):
        edits.setdefault(other, set()).add((match.start(), match.end(), ""))
  return edits


def _Declares(text: str) -> bool:
  code = re.sub(r"//[^\n]*|/\*.*?\*/", "", text, flags=re.S)
  code = re.sub(r"^[ \t]*#[^\n]*$", "", code, flags=re.M)
  return bool(code.strip())


# ---- include -------------------------------------------------------------------------------------------------------------

EVENT_NAMES_PATTERN = re.compile(r"^using (\w+)\s*=", re.M)


def DropEventsInclude(tree: SourceTree, result: B3Result) -> dict:
  """`#include "common/events.hpp"` goes from game files that name none of its aliases and not the game (whose
  unqualified name at global scope it declares)."""
  events = tree.files.get(EVENTS_HEADER)
  if events is None:
    return {}
  aliases = set(EVENT_NAMES_PATTERN.findall(events.text)) | {"Game"}
  edits: dict = {}
  for path, source in tree.files.items():
    if path == EVENTS_HEADER or not str(path).startswith("game/"):
      continue
    line = re.search(rf'^#include "{re.escape(EVENTS_INCLUDE)}"\n', source.text, re.M)
    if line is None:
      continue
    names = {token.text for token in source.tokens if token.kind == IDENTIFIER}
    if names & aliases:
      continue
    edits.setdefault(path, set()).add((line.start(), line.end(), ""))
    result.includes.append(f"{path}: {EVENTS_INCLUDE}")
  return edits
