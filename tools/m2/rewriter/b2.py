"""B2 (#13): what the battle path's direct calls leave behind, removed from the scratch tree.

`--apply b2` first replaces every `Call(game, event, BattleRoutine::K, words...)` with the routine call K's link
handler makes (substitute.py). This pass then applies, to the rewritten tree:

  parameter  a `BattleEvent` parameter that is not used, or only handed on to other routines' dropped `BattleEvent`
             parameters, is dropped from the definition, from every declaration and from every call (a fixed point:
             one used for anything else, or of a routine whose address is taken, keeps its callers' parameters)
  forwarder  a battle dispatcher (`svc_*` taking the game, an address, the words and their count) nothing names any
             more is deleted, then every function only deleted functions named (port_service, btl_reorder, ...)
  handler    a `port_routine_*` handler at a BattleRoutine address is deleted when nothing names the handler or that
             address's enumerator any more and no far code pointer `{offset, segment}` in game/ can reach it; its
             link-table entry keeps the routine's name with `nullptr`
  key        a BattleRoutine enumerator nothing names is deleted, and `BattleEvent` itself once nothing names it
  include    `common/battle-routine.hpp` and `link/link.hpp` go from files that no longer name what they declare

A deleted function takes its comment with it when it was the last code of its paragraph. Every kept parameter and
handler is listed with its reason (B2Result.kept)."""
from __future__ import annotations

import re
from dataclasses import dataclass, field
from pathlib import Path, PurePosixPath

from .constants import ConstantScopes, IsIntegerLiteral
from .functions import Function
from .links import BATTLE_KEYS, TABLE, ReadLinkTable
from .program import Program
from .source import SOURCES, SourceFile, SourceTree
from .syntax import Arguments, Span, SplitTopLevel, Text
from .tokens import IDENTIFIER

EVENT = "BattleEvent"
EVENTS_HEADER = PurePosixPath("game/common/events.hpp")
DISPATCHER_SHAPE = ("Game&", "uint32_t", "uint16_t const*", "unsigned")
TIGHT = {".", "->", "::"}
SEGMENT_NAME = re.compile(r"seg", re.I)
LINK_HEADER = "link/link.hpp"
LINK_NAMES = {"port_call", "port_service", "port_routine_name", "port_routine_address", "PortHandler", "PortRoutine", "PortLink"}
KEYS_HEADER = "common/battle-routine.hpp"
KEYS_ENUM = "BattleRoutine"
DELETE = ("delete", 0, 0)  # the edit that deletes a whole file


@dataclass
class B2Result:
  dropped:  list[str] = field(default_factory=list)  # routine(parameter) whose BattleEvent parameter went
  sites:    int = 0                                  # call arguments removed with them
  deleted:  list[str] = field(default_factory=list)  # functions deleted (forwarders and what only they named)
  handlers: list[str] = field(default_factory=list)  # link handlers deleted
  keys:     list[str] = field(default_factory=list)  # BattleRoutine enumerators deleted
  includes: list[str] = field(default_factory=list)  # file: include removed
  kept:     list[str] = field(default_factory=list)  # what stays, and why

  def Counts(self) -> dict[str, int]:
    return {"dropped": len(self.dropped), "arguments": self.sites, "deleted": len(self.deleted), "handlers": len(self.handlers),
            "keys": len(self.keys), "includes": len(self.includes), "kept": len(self.kept)}


def B2Pass(root: Path) -> B2Result:
  """Runs every rule on the tree under root (a scratch export), writing each stage's files before the next reads them."""
  result = B2Result()
  _Write(root, *DropEvents(Program(SourceTree(root)), result))
  _Write(root, *DeleteDead(Program(SourceTree(root)), result))
  _Write(root, *DeleteHandlers(Program(SourceTree(root)), result))
  _Write(root, *DeleteDead(Program(SourceTree(root)), result))
  _Write(root, *DeleteKeys(Program(SourceTree(root)), result))
  _Write(root, *DropIncludes(SourceTree(root), result))
  return result


# ---- parameter ---------------------------------------------------------------------------------------------------------

@dataclass(frozen=True)
class _Slot:
  """One BattleEvent parameter of one definition."""
  file:  PurePosixPath
  name:  str
  index: int


def DropEvents(program: Program, result: B2Result) -> tuple[Program, dict]:
  slots, edges, live = _EventSlots(program)
  _Propagate(edges, live)
  dropped = {slot for slot in slots if slot not in live}
  result.kept += sorted(f"{slot.file} {slot.name} parameter {slot.index}: {live[slot]}" for slot in slots if slot in live)
  result.dropped += sorted(f"{slot.file} {slot.name}({_Definition(program, slot).parameters[slot.index].name or EVENT})" for slot in dropped)
  edits: dict = {}
  for slot in dropped:
    definition = _Definition(program, slot)
    _RemoveListItem(program.Source(slot.file), definition.name_index + 1, slot.index, edits)
    for declaration in _Declarations(program, definition):
      _RemoveListItem(program.Source(declaration.file), declaration.name_index + 1, slot.index, edits)
  result.sites += _DropArguments(program, dropped, edits)
  return program, edits


def _EventSlots(program: Program) -> tuple[set[_Slot], dict[_Slot, set[_Slot]], dict[_Slot, str]]:
  """Every BattleEvent parameter, the parameters each one is handed on to, and those used otherwise (with why)."""
  slots, edges, live = set(), {}, {}
  taken = _AddressTaken(program)
  for name, definitions in program.definitions.items():
    for function in definitions:
      for index, parameter in enumerate(function.parameters):
        if parameter.type != EVENT:
          continue
        slot = _Slot(function.file, name, index)
        slots.add(slot)
        edges[slot] = set()
        if (function.file, name) in taken:
          live[slot] = f"the routine's address is taken ({taken[function.file, name]})"
          continue
        if parameter.name:
          _Uses(program, function, parameter.name, slot, (edges, live))
  return slots, edges, live


def _Uses(program: Program, function: Function, name: str, slot: _Slot, graph: tuple[dict, dict]) -> None:
  edges, live = graph
  source = program.Source(function.file)
  tokens = source.tokens
  for index in range(function.body.first, function.body.last):
    if tokens[index].text != name or tokens[index - 1].text in TIGHT:
      continue
    target = _PassedTo(program, source, index)
    if target is None:
      live.setdefault(slot, f"{function.file}:{tokens[index].line} uses it: {_Line(source.text, tokens[index].start)}")
    else:
      edges[slot].add(target)


def _PassedTo(program: Program, source: SourceFile, index: int) -> _Slot | None:
  """The BattleEvent parameter of a routine the token at index is passed to as a whole argument, if it is one."""
  tokens, partners = source.tokens, source.partners
  if tokens[index - 1].text not in ("(", ",") or tokens[index + 1].text not in (")", ","):
    return None
  paren = _OpenParen(tokens, partners, index)
  if paren is None or tokens[paren - 1].kind != IDENTIFIER or tokens[paren - 2].text in TIGHT:
    return None
  callee = program.Resolve(tokens[paren - 1].text, source.path)
  position = next((i for i, part in enumerate(Arguments(tokens, partners, paren)) if part.first == index), None)
  if callee is None or position is None or position >= len(callee.parameters) or callee.parameters[position].type != EVENT:
    return None
  return _Slot(callee.file, callee.name, position)


def _OpenParen(tokens, partners: dict[int, int], index: int) -> int | None:
  at = index - 1
  while at > 0:
    text = tokens[at].text
    if text in (")", "]", "}") and at in partners:
      at = partners[at] - 1
      continue
    if text == "(":
      return at
    if text in ("{", "[", ";"):
      return None
    at -= 1
  return None


def _Propagate(edges: dict[_Slot, set[_Slot]], live: dict[_Slot, str]) -> None:
  """A parameter handed on to a kept parameter is kept too."""
  changed = True
  while changed:
    changed = False
    for slot, targets in edges.items():
      reached = next((target for target in sorted(targets, key=str) if target in live or target not in edges), None)
      if slot not in live and reached is not None:
        live[slot] = f"handed on to {reached.name} parameter {reached.index}, which stays"
        changed = True


def _AddressTaken(program: Program, names: set[str] | None = None) -> dict[tuple[PurePosixPath, str], str]:
  """(file, name) of the routines (by default those with a BattleEvent parameter) named other than in a call or a
  declaration where they are visible (their type is then fixed by what receives them)."""
  if names is None:
    names = {name for name, definitions in program.definitions.items() if any(p.type == EVENT for f in definitions for p in f.parameters)}
  heads = _Heads(program)
  taken = {}
  for path, source in program.tree.files.items():
    tokens = source.tokens
    for index, token in enumerate(tokens[:-1]):
      if token.text in names and tokens[index + 1].text != "(" and (path, index) not in heads and tokens[index - 1].text not in TIGHT:
        function = _Visible(program, token.text, path)
        if function is not None:
          taken.setdefault((function.file, token.text), f"{path}:{token.line}")
  return taken


def _Visible(program: Program, name: str, path: PurePosixPath) -> Function | None:
  """The definition a name in path reaches: one in path itself or in a file path includes."""
  closure = program.scopes.Closure(path)
  return next((f for file in closure for f in program.definitions.get(name, []) if f.file == file), None) or \
    next((f for f in program.definitions.get(name, []) for d in program.declarations.get(name, []) if d.file in closure), None)


def _Heads(program: Program) -> set[tuple[PurePosixPath, int]]:
  functions = [f for group in (program.definitions, program.declarations) for found in group.values() for f in found]
  return {(function.file, function.name_index) for function in functions}


def _Definition(program: Program, slot: _Slot) -> Function:
  return next(f for f in program.definitions[slot.name] if f.file == slot.file)


def _Declarations(program: Program, definition: Function) -> list[Function]:
  """The declarations of this definition: every one of its name, or when several definitions share the name, those in
  its own file or a header its file includes."""
  declarations = [d for d in program.declarations.get(definition.name, []) if len(d.parameters) == len(definition.parameters)]
  if len(program.definitions[definition.name]) == 1:
    return declarations
  closure = set(program.scopes.Closure(definition.file))
  return [declaration for declaration in declarations if declaration.file in closure]


def _DropArguments(program: Program, dropped: set[_Slot], edits: dict) -> int:
  """Removes the dropped parameters' arguments from every call by name; returns how many."""
  by_name: dict[str, set[_Slot]] = {}
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
          _CheckPure(source, index + 1, slot.index)
          _RemoveListItem(source, index + 1, slot.index, edits)
          removed += 1
  return removed


def _CheckPure(source: SourceFile, paren: int, position: int) -> None:
  parts = Arguments(source.tokens, source.partners, paren)
  if position < len(parts):
    text = Text(source.tokens, parts[position], source.text)
    if not re.fullmatch(r"[A-Za-z_]\w*", text):
      raise ValueError(f"{source.path}:{source.tokens[paren].line}: the dropped argument `{text}` is not a name")


def _RemoveListItem(source: SourceFile, paren: int, position: int, edits: dict) -> None:
  """Removes the item at position from the parenthesised list at paren, with its comma."""
  tokens = source.tokens
  parts = SplitTopLevel(tokens, Span(paren + 1, source.partners[paren]), ",")
  if position >= len(parts):
    return
  if position > 0:
    start, end = tokens[parts[position - 1].last - 1].end, tokens[parts[position].last - 1].end
  elif len(parts) > 1:
    start, end = tokens[parts[0].first].start, tokens[parts[1].first].start
  else:
    start, end = tokens[parts[0].first].start, tokens[parts[0].last - 1].end
  edits.setdefault(source.path, set()).add((start, end, ""))


# ---- handler -----------------------------------------------------------------------------------------------------------

def DeleteHandlers(program: Program, result: B2Result) -> tuple[Program, dict]:
  links = ReadLinkTable(program)
  named = _NamedKeys(program, links.keys)
  far = FarTargets(program, links)
  numbers = _Numbers(program)
  heads = _Heads(program)
  edits: dict = {}
  for address in sorted(set(links.keys.values())):
    entry = links.entries.get(address)
    if entry is None or not entry.handler or entry.handler not in links.handlers:
      continue
    reason = named.get(address) or _Referenced(program, entry.handler, heads) or far.get(address) or numbers.get(address)
    if reason:
      result.kept.append(f"{entry.handler}: {reason}")
      continue
    _DeleteFunctions(program, [links.handlers[entry.handler].function], edits)
    _Unlink(program.Source(TABLE), entry.handler, edits)
    result.handlers.append(entry.handler)
  return program, edits


def _NamedKeys(program: Program, keys: dict[str, int]) -> dict[int, str]:
  """address -> where its enumerator is still named (outside the enumeration): as `BattleRoutine::NAME`, or bare in a
  file with `using enum BattleRoutine` (other enumerations reuse the names)."""
  named = {}
  for path, source in program.tree.files.items():
    if path == BATTLE_KEYS:
      continue
    tokens, bare = source.tokens, _UsingEnum(source)
    for index, token in enumerate(tokens):
      qualified = index > 1 and tokens[index - 1].text == "::" and tokens[index - 2].text == KEYS_ENUM
      if token.kind == IDENTIFIER and token.text in keys and (qualified or (bare and tokens[index - 1].text not in TIGHT)):
        named.setdefault(keys[token.text], f"BattleRoutine::{token.text} is still named at {path}:{token.line}")
  return named


def _StaticAsserts(source: SourceFile) -> list[Span]:
  tokens, partners = source.tokens, source.partners
  return [Span(index + 1, partners[index + 1]) for index, token in enumerate(tokens[:-1]) if token.text == "static_assert" and index + 1 in partners]


def _UsingEnum(source: SourceFile) -> bool:
  return re.search(rf"\busing\s+enum\s+{KEYS_ENUM}\s*;", source.text) is not None


def _Numbers(program: Program) -> dict[int, str]:
  """Every integer literal of game/ (but the link table and the keys) -> where: an address written as a number might
  be called by it."""
  found = {}
  for path, source in program.tree.files.items():
    if not _Scanned(path):
      continue
    asserted = {index for span in _StaticAsserts(source) for index in range(span.first, span.last)}
    for index, token in enumerate(source.tokens):
      if IsIntegerLiteral(token) and index not in asserted:
        found.setdefault(int(token.text.rstrip("uUlL"), 0), f"the address is written as a number at {path}:{token.line}")
  return found


def _Referenced(program: Program, name: str, heads: set) -> str:
  for path, source in program.tree.files.items():
    if path == TABLE:
      continue
    for index, token in enumerate(source.tokens):
      if token.text == name and (path, index) not in heads:
        return f"named at {path}:{token.line}"
  return ""


def FarTargets(program: Program, links) -> dict[int, str]:
  """Linked addresses a far code pointer written in game/ can hold: two neighbouring items of a bracketed list, an
  offset and a segment, either order; the segment a SEGMENT constant's value or one of a literal pair hitting the link
  table, or any of those when it is a variable named for a segment."""
  scopes = ConstantScopes(program.tree)
  pairs = [pair for path, source in program.tree.files.items() if _Scanned(path) for pair in _Pairs(source, scopes)]
  segments = {value for path in program.tree.files for name, value in scopes._own[path].values.items() if "SEGMENT" in name}
  segments |= {b for _, a, va, b_text, b in pairs if _Literal(a) and _Literal(b_text) and va is not None and b is not None
               and b * 16 + va in links.entries}
  segments.discard(0)
  found: dict[int, str] = {}
  for where, first, first_value, second, second_value in pairs:
    for offset, offset_value, segment, segment_value in ((first, first_value, second, second_value), (second, second_value, first, first_value)):
      if offset_value is None or offset_value in links.keys.values():
        continue
      candidates = [segment_value] if segment_value in segments else sorted(segments) if segment_value is None and _SegmentName(segment) else []
      for value in candidates:
        address = value * 16 + offset_value
        if address in links.entries:
          found.setdefault(address, f"a far pointer {{{offset}, {segment}}} at {where} can hold it")
  return found


def _Scanned(path: PurePosixPath) -> bool:
  return str(path).startswith("game/") and path not in (TABLE, BATTLE_KEYS) and path.name != "data-image.cpp"


def _Pairs(source: SourceFile, scopes: ConstantScopes) -> list[tuple[str, str, int | None, str, int | None]]:
  constants = scopes.Of(source.path)
  tokens, partners = source.tokens, source.partners
  found = []
  for opening, closing in partners.items():
    if opening > closing or tokens[opening].text not in ("(", "{"):
      continue
    items = [part for part in SplitTopLevel(tokens, Span(opening + 1, closing), ",") if len(part)]
    texts = [Text(tokens, part, source.text) for part in items]
    values = [constants.Evaluate(text) for text in texts]
    where = f"{source.path}:{tokens[opening].line}"
    found += [(where, texts[i], values[i], texts[i + 1], values[i + 1]) for i in range(len(items) - 1)]
  return found


def _Literal(text: str) -> bool:
  return re.fullmatch(r"\s*0[xX][0-9a-fA-F]+[uU]?\s*", text) is not None


def _SegmentName(text: str) -> bool:
  return SEGMENT_NAME.search(text) is not None and "(" not in text


def _Unlink(source: SourceFile, handler: str, edits: dict) -> None:
  """The handler's declaration line goes; its entry names nullptr instead."""
  text = source.text
  declaration = re.search(rf"^PortHandler {handler};\n", text, re.M)
  if declaration:
    edits.setdefault(source.path, set()).add((declaration.start(), declaration.end(), ""))
  for match in re.finditer(rf"(\"\w+\",\s*){handler}(\s*\}})", text):
    edits.setdefault(source.path, set()).add((match.start(), match.end(), f"{match.group(1)}nullptr{match.group(2)}"))


# ---- forwarder -----------------------------------------------------------------------------------------------------------

def DeleteDead(program: Program, result: B2Result) -> tuple[Program, dict]:
  """Battle dispatchers nothing names, then (to a fixed point) every function only deleted functions name."""
  references = _References(program)
  dead: set[str] = set()
  changed = True
  while changed:
    changed = False
    for name, definitions in program.definitions.items():
      if name in dead or not str(definitions[0].file).startswith("game/"):
        continue
      outside = [where for where, inside in references.get(name, []) if inside not in dead]
      dispatcher = all(_DispatcherShape(f) for f in definitions)
      if not outside and (dispatcher or references.get(name)):
        dead.add(name)
        changed = True
  edits: dict = {}
  for name in sorted(dead):
    _DeleteFunctions(program, program.definitions[name] + program.declarations.get(name, []), edits)
  result.deleted += sorted(dead)
  return program, edits


def _DispatcherShape(function: Function) -> bool:
  types = tuple(p.type.replace("std::", "").replace("const uint16_t*", "uint16_t const*").replace("unsigned int", "unsigned") for p in function.parameters)
  return types == DISPATCHER_SHAPE


def _References(program: Program) -> dict[str, list[tuple[str, str | None]]]:
  """name -> (where, the function whose body holds it) of every mention but the functions' own heads."""
  heads = _Heads(program)
  names = set(program.definitions)
  found: dict = {}
  for path, source in program.tree.files.items():
    for index, token in enumerate(source.tokens):
      if token.kind == IDENTIFIER and token.text in names and (path, index) not in heads and source.tokens[index - 1].text not in (".", "->"):
        enclosing = program.Enclosing(path, index)
        found.setdefault(token.text, []).append((f"{path}:{token.line}", enclosing.name if enclosing else None))
  return found


# ---- deletion of text --------------------------------------------------------------------------------------------------

def _DeleteFunctions(program: Program, functions: list[Function], edits: dict) -> None:
  for function in functions:
    source = program.Source(function.file)
    first = _HeadStart(source, function.name_index)
    last = function.body.last - 1 if function.body is not None else _DeclarationEnd(source, function.name_index)
    edits.setdefault(function.file, set()).add(("lines", source.tokens[first].line, source.tokens[last].line))


def _HeadStart(source: SourceFile, name_index: int) -> int:
  tokens, partners = source.tokens, source.partners
  at = name_index - 1
  while at >= 0:
    text = tokens[at].text
    if text in ("]", ">", ")") and at in partners:
      at = partners[at] - 1
      continue
    if text in (";", "{", "}"):
      return at + 1
    at -= 1
  return 0


def _DeclarationEnd(source: SourceFile, name_index: int) -> int:
  return next(i for i in range(name_index, len(source.tokens)) if source.tokens[i].text == ";")


def _DoomedLines(text: str, ranges: list[tuple[int, int]]) -> set[int]:
  """The lines (1-based) to delete for whole-line ranges: the comment right above a range goes with it unless code
  that stays follows (the comment then heads a group); a paragraph whose code lines all go loses its comments and one
  blank line beside it as well."""
  lines = text.split("\n")
  doomed = {line for first, last in ranges for line in range(first, last + 1)}
  for first, last in sorted(ranges):
    following = lines[last] if last < len(lines) else ""
    if following.strip() and not _Comment(following) and last + 1 not in doomed and following.strip() != "}":
      continue
    above = first - 1
    while above >= 1 and _Comment(lines[above - 1]):
      doomed.add(above)
      above -= 1
  paragraphs, start = [], 1
  for number, line in enumerate(lines + [""], start=1):
    if not line.strip():
      if number > start:
        paragraphs.append((start, number - 1))
      start = number + 1
  for first, last in paragraphs:
    code = [n for n in range(first, last + 1) if not _Comment(lines[n - 1])]
    if code and all(n in doomed for n in code):
      doomed |= set(range(first, last + 1))
      if last + 1 <= len(lines) and not lines[last].strip():
        doomed.add(last + 1)
      elif first > 1 and not lines[first - 2].strip():
        doomed.add(first - 1)
  return doomed


def _Comment(line: str) -> bool:
  return line.lstrip().startswith(("//", "/*", "* ", "*/")) or line.strip() == "*"


# ---- key -------------------------------------------------------------------------------------------------------------------

def DeleteKeys(program: Program, result: B2Result) -> tuple[Program, dict]:
  if BATTLE_KEYS not in program.tree.files:
    return program, {}
  keys = ReadLinkTable(program).keys
  named = set(_NamedKeys(program, keys))
  source = program.Source(BATTLE_KEYS)
  edits: dict = {}
  for key in sorted(keys):
    if keys[key] in named:
      continue
    match = re.search(rf"^\s*{key}\s*=\s*0x[0-9a-fA-F]+\s*,[^\n]*\n", source.text, re.M)
    if match:
      edits.setdefault(BATTLE_KEYS, set()).add((match.start(), match.end(), ""))
      result.keys.append(key)
  if keys and len(edits.get(BATTLE_KEYS, ())) == len(keys):
    edits[BATTLE_KEYS] = {DELETE}
    result.keys.append(KEYS_HEADER)
  if EVENTS_HEADER in program.tree.files and not _Mentioned(program, EVENT, EVENTS_HEADER):
    events = program.Source(EVENTS_HEADER)
    alias = re.search(rf"^using {EVENT}\b[^\n]*\n", events.text, re.M)
    if alias:
      edits.setdefault(EVENTS_HEADER, set()).add((alias.start(), alias.end(), ""))
      result.keys.append(EVENT)
  return program, edits


def _Mentioned(program: Program, name: str, besides: PurePosixPath) -> bool:
  return any(token.text == name for path, source in program.tree.files.items() if path != besides for token in source.tokens)


# ---- include -------------------------------------------------------------------------------------------------------------

def DropIncludes(tree: SourceTree, result: B2Result) -> tuple[None, dict]:
  keys_source = tree.files.get(BATTLE_KEYS)
  keys = set(re.findall(r"^\s*([A-Z0-9_]+)\s*=", keys_source.text, re.M)) | {"BattleRoutine"} if keys_source else set()
  edits: dict = {}
  for path, source in tree.files.items():
    gone = keys_source is None and re.search(rf'^#include "{re.escape(KEYS_HEADER)}"\n', source.text, re.M)
    if gone:
      edits.setdefault(path, set()).add((gone.start(), gone.end(), ""))
      result.includes.append(f"{path}: {KEYS_HEADER} (deleted)")
    if not str(path).startswith("game/") or path.suffix != ".cpp":
      continue
    names = {token.text for token in source.tokens if token.kind == IDENTIFIER}
    using = re.search(rf"^[ \t]*using[ \t]+enum[ \t]+{KEYS_ENUM}[ \t]*;\n", source.text, re.M)
    if using and not names & (keys - {KEYS_ENUM}):
      edits.setdefault(path, set()).add((using.start(), using.end(), ""))
      result.includes.append(f"{path}: using enum {KEYS_ENUM}")
      names -= {KEYS_ENUM}
    for header, declared in ((KEYS_HEADER, keys), (LINK_HEADER, LINK_NAMES)):
      if PurePosixPath(header).stem == path.stem:
        continue  # a file's own header stays
      line = re.search(rf'^#include "{re.escape(header)}"\n', source.text, re.M)
      if line and declared and not names & declared:
        edits.setdefault(path, set()).add((line.start(), line.end(), ""))
        result.includes.append(f"{path}: {header}")
  return None, edits


# ---- writing -------------------------------------------------------------------------------------------------------------

def _Write(root: Path, _program, edits: dict) -> None:
  for path, changes in edits.items():
    target = root / SOURCES / path
    if DELETE in changes:
      target.unlink()
      continue
    text = target.read_text()
    target.write_text(ApplyEdits(text, changes))


def ApplyEdits(text: str, changes: set) -> str:
  """Applies (start, end, replacement) byte edits and ("lines", first, last) line deletions to text; an edit inside a
  deleted line goes with it."""
  starts = [0] + [index + 1 for index, character in enumerate(text) if character == "\n"]
  doomed = _DoomedLines(text, [(c[1], c[2]) for c in changes if c[0] == "lines"])
  removed = [(starts[n - 1], starts[n] if n < len(starts) else len(text), "") for n in sorted(doomed)]
  spans = [c for c in changes if c[0] != "lines" and not any(r[0] <= c[0] and c[1] <= r[1] for r in removed)]
  for start, end, replacement in sorted(spans + removed, key=lambda c: (c[0], c[1]), reverse=True):
    text = text[:start] + replacement + text[end:]
  return text


def _Line(text: str, offset: int) -> str:
  start = text.rfind("\n", 0, offset) + 1
  end = text.find("\n", offset)
  return " ".join(text[start:end if end >= 0 else len(text)].split())
