"""The `Call(ds, event, ID, words...)` sites, and which `svc_*` dispatcher each routine's callback parameter is bound
to by its callers (followed through callers that forward their own callback)."""
from __future__ import annotations

import re
from dataclasses import dataclass, field
from pathlib import PurePosixPath

from .constants import IsIntegerLiteral
from .functions import Function
from .program import Program
from .source import SourceFile
from .syntax import Arguments, Compact, Span, SplitTopLevel, Text
from .tokens import IDENTIFIER

CALLBACK_TYPES = ("HostEvent", "HostWordsEvent", "HostService", "HostNotify", "BattleEvent", "BattleService", "PendingEvent",
                  "SideColorEvent", "UiEvent", "ArrivalEvent", "SupplyEvent", "UtilService", "TextEvent", "EmsEvent", "SoundService")
CALL_TEMPLATE = PurePosixPath("game/common/memory.hpp")
# The calls that pack their words for an event as Call does: Call(game, event, ID, words...), CallWords (an event
# taking four scalar words, missing ones 0), List and Input (io's word arrays, Input's padded with zeros to seven).
CALL_LIKE = {"Call", "CallWords", "List", "Input"}
# The event types whose words are scalars: a raw `event(game, ID, words...)` is a site like Call's.
SCALAR_EVENTS = {"PendingEvent", "SideColorEvent", "BattleService", "ArrivalEvent"}
# The event types whose words are an array (with a count, but HostService's): a raw `event(game, ID, NAME, N)` over a
# local array NAME of 16-bit words is a site whose words are NAME's, read where the case reads them.
ARRAY_EVENTS = {"HostEvent", "HostWordsEvent", "HostNotify", "HostService"}
COUNTLESS_EVENTS = {"HostService"}
STATEMENT_STARTS = {";", "{", "}", "else", "do"}
CONTROL_WORDS = {"if", "while", "for", "switch"}
UNBOUND = "<unbound>"


@dataclass
class CallSite:
  file:      PurePosixPath
  line:      int
  name:      Span        # the `Call` token
  span:      Span        # Call(...) as a whole
  function:  Function    # the enclosing definition
  ds:        Span
  event:     Span
  service:   Span
  words:     list[Span]
  used:      bool        # the call's value is used
  array:     str = ""    # a raw call over a local array: its name (the words are NAME[0], NAME[1], ...)
  pointer:   str = ""    # the array as a pointer to its words (NAME, or NAME.data() of a std::array)
  size:      int = 0     # the array's length
  elements:  list[Span] | None = None  # a const array declared just before the site's statement for it alone: its
  declaration: Span | None = None      # initializer's elements and its declaration (the words may fold into the call)

  def Text(self, source: SourceFile) -> str:
    return Text(source.tokens, self.span, source.text)

  def Key(self) -> str:
    return f"{self.file}:{self.line}:{self.span.first}"


def IsCallback(type_text: str) -> bool:
  """A parameter type that is one of the callback types (`HostEvent`, `HostEvent const`, ...)."""
  return any(word in CALLBACK_TYPES for word in re.findall(r"\w+", type_text))


def FindCallSites(program: Program) -> list[CallSite]:
  sites: list[CallSite] = []
  for path, source in program.tree.files.items():
    if path == CALL_TEMPLATE:
      continue
    tokens, partners = source.tokens, source.partners
    for index, token in enumerate(tokens[:-1]):
      if tokens[index + 1].text != "(" or index + 1 not in partners or tokens[index - 1].text in (".", "->"):
        continue
      if token.text in CALL_LIKE:
        site = _WrappedSite(program, source, index)
      elif token.kind == IDENTIFIER:
        site = _RawSite(program, source, index)
      else:
        site = None
      if site is not None:
        sites.append(site)
  return sites


def _WrappedSite(program: Program, source: SourceFile, index: int) -> CallSite | None:
  """`Call(game, event, ID, words...)` (or another of CALL_LIKE) inside a function."""
  tokens, partners = source.tokens, source.partners
  function = program.Enclosing(source.path, index)
  arguments = Arguments(tokens, partners, index + 1)
  if function is None or len(arguments) < 3 or function.name in CALL_LIKE:
    return None
  start = index - 2 if tokens[index - 1].text == "::" else index
  span = Span(start, partners[index + 1] + 1)
  return CallSite(source.path, tokens[index].line, Span(index, index + 1), span, function, arguments[0], arguments[1],
                  arguments[2], arguments[3:], ValueUsed(source, span))


def _RawSite(program: Program, source: SourceFile, index: int) -> CallSite | None:
  """`event(game, ID, words...)` through a callback parameter of a scalar-word event type."""
  tokens, partners = source.tokens, source.partners
  function = program.Enclosing(source.path, index)
  if function is None or tokens[index - 1].text == "::" or function.name in CALL_LIKE:
    return None
  position = function.ParameterIndex(tokens[index].text)
  types = set(re.findall(r"\w+", function.parameters[position].type)) if position is not None else set()
  arguments = Arguments(tokens, partners, index + 1)
  if not types & (SCALAR_EVENTS | ARRAY_EVENTS) or len(arguments) < 2:
    return None
  span = Span(index, partners[index + 1] + 1)
  if types & ARRAY_EVENTS:
    return _ArraySite(source, function, Span(index, index + 1), span, arguments, bool(types & COUNTLESS_EVENTS))
  return CallSite(source.path, tokens[index].line, Span(index, index + 1), span, function, arguments[0], Span(index, index + 1),
                  arguments[1], arguments[2:], ValueUsed(source, span))


def _ArraySite(source: SourceFile, function: Function, name: Span, span: Span, arguments: list[Span], countless: bool) -> CallSite | None:
  """`event(game, ID, NAME, N)` (`service(game, ID, NAME)` for a count-less event) over a local array of 16-bit words
  `std::uint16_t [const] NAME[S]{ ... }` or `std::array<std::uint16_t, S> NAME{ ... }` (passed as `NAME.data()`), N a
  literal or `std::size(NAME)`."""
  tokens = source.tokens
  if len(arguments) != (3 if countless else 4):
    return None
  given = [token.text for token in tokens[arguments[2].first:arguments[2].last]]
  array = given[0] if len(given) in (1, 5) and tokens[arguments[2].first].kind == IDENTIFIER else ""
  if not array or (len(given) == 5 and given[1:] != [".", "data", "(", ")"]):
    return None
  size = LocalArraySize(source, function, array, arguments[2].first, std_array=len(given) == 5)
  if size is None:
    return None
  count = size if countless else _Count(source, arguments[3], array, size)
  if count is None or count > size:
    return None
  site = CallSite(source.path, tokens[name.first].line, name, span, function, arguments[0], name, arguments[1], [arguments[2]] * count,
                  ValueUsed(source, span), array, "".join(given), size)
  site.declaration, site.elements = _OnlyFor(source, function, array, span.first) or (None, None)
  return site


FOLD_BETWEEN = {"if", "(", "!", "return"}


def _OnlyFor(source: SourceFile, function: Function, array: str, site: int) -> tuple[Span, list[Span]] | None:
  """`std::uint16_t const NAME[...]{ e0, e1, ... };` as the statement just before the one of the site (only `if (`,
  `!` or `return` between), NAME named nowhere else: the declaration and its elements."""
  tokens, partners = source.tokens, source.partners
  uses = [i for i in range(function.body.first, function.body.last) if tokens[i].text == array and tokens[i - 1].text not in (".", "->", "::")]
  if len(uses) != 2 or tokens[uses[0] + 1].text != "[" or tokens[uses[0] - 1].text != "const":
    return None
  close = partners.get(uses[0] + 1)
  if close is None or tokens[close + 1].text != "{" or close + 1 not in partners:
    return None
  end = partners[close + 1] + 1
  if tokens[end].text != ";" or any(token.text not in FOLD_BETWEEN for token in tokens[end + 1:site]):
    return None
  start = uses[0] - 1
  while start > function.body.first and tokens[start - 1].text not in (";", "{", "}"):
    start -= 1
  return Span(start, end + 1), SplitTopLevel(tokens, Span(close + 2, end - 1), ",")


def _Count(source: SourceFile, span: Span, array: str, size: int) -> int | None:
  texts = [token.text for token in source.tokens[span.first:span.last]]
  if len(texts) == 1 and IsIntegerLiteral(source.tokens[span.first]):
    return int(texts[0].rstrip("uUlL"), 0)
  if "".join(texts) in (f"std::size({array})", f"static_cast<uint16_t>(std::size({array}))", f"static_cast<std::uint16_t>(std::size({array}))"):
    return size
  return None


def LocalArraySize(source: SourceFile, function: Function, name: str, before: int, std_array: bool = False) -> int | None:
  """The length of the local array of 16-bit words NAME declared in function before the token index, None when there
  is no such declaration (or its length is not a literal)."""
  tokens, partners = source.tokens, source.partners
  for index in range(before - 1, function.body.first, -1):
    if tokens[index].text != name or tokens[index - 1].text in (".", "->", "::"):
      continue
    if std_array:
      opener = next((i for i in range(index - 1, max(index - 12, 0), -1) if tokens[i].text in ("array", ";", "{", "}")), None)
      if opener is None or tokens[opener].text != "array" or tokens[index - 1].text != ">" or tokens[index + 1].text != "{":
        continue
      inside = [token for token in tokens[opener + 2:index - 1]]
      if len(inside) < 3 or inside[-3].text != "uint16_t" or inside[-2].text != "," or not IsIntegerLiteral(inside[-1]):
        return None
      return int(inside[-1].text.rstrip("uUlL"), 0)
    if tokens[index + 1].text != "[" or index + 1 not in partners:
      continue
    declared = [token.text for token in tokens[max(index - 3, 0):index]]
    if "uint16_t" not in declared or tokens[index - 1].text not in ("uint16_t", "const"):
      continue
    close = partners[index + 1]
    length = tokens[index + 2:close]
    if len(length) == 1 and IsIntegerLiteral(length[0]):
      return int(length[0].text.rstrip("uUlL"), 0)
    if not length and tokens[close + 1].text == "{" and close + 1 in partners:
      return len(SplitTopLevel(tokens, Span(close + 2, partners[close + 1]), ","))
    return None
  return None


def ValueUsed(source: SourceFile, span: Span) -> bool:
  """Whether the expression at span is not a whole expression statement (`Call(...);` alone, or under an unbraced
  if/else/loop, or cast to void)."""
  tokens, partners = source.tokens, source.partners
  if span.last < len(tokens) and tokens[span.last].text == "," and _InForHeader(tokens, partners, span.first):
    return False  # a left operand of the comma operator in a for header
  if span.last >= len(tokens) or tokens[span.last].text != ";":
    return True
  before = tokens[span.first - 1]
  if before.text in STATEMENT_STARTS:
    return False
  if before.text == ":" and _IsLabel(tokens, span.first - 1):
    return False
  if before.text == ")" and span.first - 1 in partners:
    opener = partners[span.first - 1]
    inside = [token.text for token in tokens[opener + 1:span.first - 1]]
    return not (tokens[opener - 1].text in CONTROL_WORDS or inside == ["void"])
  return True


def _InForHeader(tokens, partners: dict[int, int], index: int) -> bool:
  while index > 0:
    index -= 1
    text = tokens[index].text
    if text in (")", "]", "}") and index in partners:
      index = partners[index]
    elif text in ("{", "["):
      return False
    elif text == "(":
      return tokens[index - 1].text == "for"
  return False


def _IsLabel(tokens, colon: int) -> bool:
  """A `case X:` / `default:` / goto label (`name:` opening a statement) colon, not a ternary's or a bit-field's."""
  if colon >= 2 and tokens[colon - 1].kind == IDENTIFIER and tokens[colon - 2].text in (";", "{", "}") and tokens[colon + 1].text != ":":
    return True
  index = colon - 1
  while index > 0 and tokens[index].text not in (";", "{", "}", "case", "default", "?"):
    index -= 1
  return tokens[index].text in ("case", "default")


@dataclass
class Bindings:
  """(routine, parameter index) -> the svc_* names (or UNBOUND / '<expr>') its callers pass."""
  bound:   dict[tuple[str, int], set[str]] = field(default_factory=dict)
  callers: dict[tuple[str, int], list[str]] = field(default_factory=dict)  # where each binding comes from

  def Of(self, function: Function, parameter: str) -> set[str]:
    index = function.ParameterIndex(parameter)
    return self.bound.get((_Key(function), index), {UNBOUND}) if index is not None else {UNBOUND}

  def Polymorphic(self) -> dict[tuple[str, int], set[str]]:
    return {key: names for key, names in self.bound.items() if len({n for n in names if n.startswith("svc_")}) > 1}


def _Key(function: Function) -> str:
  return f"{function.file}::{function.name}"


def BindCallbacks(program: Program) -> Bindings:
  """Fixed point over every call of a routine that takes a callback: a `svc_*` argument binds the parameter, the
  caller's own callback parameter forwards the caller's binding, anything else binds '<expr>'."""
  takes = {_Key(f): f for defs in program.definitions.values() for f in defs if any(IsCallback(p.type) for p in f.parameters)}
  edges: list[tuple[tuple[str, int], str, tuple[str, int] | None, str]] = []
  for path, source in program.tree.files.items():
    _CollectEdges(program, source, takes, edges)
  bindings = Bindings()
  for target, value, forwarded, where in edges:
    bindings.bound.setdefault(target, set())
    bindings.callers.setdefault(target, []).append(where)
    if forwarded is None:
      bindings.bound[target].add(value)
  changed = True
  while changed:
    changed = False
    for target, value, forwarded, where in edges:
      if forwarded is not None:
        incoming = bindings.bound.get(forwarded, {UNBOUND})
        if not incoming <= bindings.bound[target]:
          bindings.bound[target] |= incoming
          changed = True
  return bindings


def _CollectEdges(program: Program, source: SourceFile, takes: dict[str, Function], edges: list) -> None:
  """Edges of every call by name of a routine that takes a callback, and of every call through a function parameter
  to which such a routine is passed (`RunTableRows(ds, svc_x, opeu_draw, ...)` calling `draw(ds, event, ...)`)."""
  tokens, partners = source.tokens, source.partners
  for function in program.by_file.get(source.path, []):
    for index in range(function.body.first, function.body.last - 1):
      token = tokens[index]
      if token.kind != IDENTIFIER or tokens[index + 1].text != "(" or tokens[index - 1].text in (".", "->"):
        continue
      callee = program.Resolve(token.text, source.path)
      if callee is None:
        continue
      arguments = Arguments(tokens, partners, index + 1)
      if _Key(callee) in takes:
        edges += [_Edge(program, source, function, callee, position, arguments[position])
                  for position, parameter in enumerate(callee.parameters) if position < len(arguments) and IsCallback(parameter.type)]
      edges += _PointerEdges(program, source, callee, arguments, takes)


def _PointerEdges(program: Program, source: SourceFile, callee: Function, arguments: list[Span], takes: dict[str, Function]) -> list:
  """For each argument naming a routine that takes a callback: the edges of callee's calls through that parameter."""
  found = []
  for position, argument in enumerate(arguments):
    if len(argument) != 1 or position >= len(callee.parameters) or not callee.parameters[position].name:
      continue
    pointee = program.Resolve(source.tokens[argument.first].text, source.path)
    if pointee is None or _Key(pointee) not in takes:
      continue
    found += _CallsThrough(program, callee, callee.parameters[position].name, pointee)
  return found


def _CallsThrough(program: Program, function: Function, parameter: str, pointee: Function) -> list:
  source = program.Source(function.file)
  tokens, partners = source.tokens, source.partners
  found = []
  for index in range(function.body.first, function.body.last - 1):
    if tokens[index].text == parameter and tokens[index + 1].text == "(" and tokens[index - 1].text not in (".", "->"):
      arguments = Arguments(tokens, partners, index + 1)
      found += [_Edge(program, source, function, pointee, position, arguments[position])
                for position, p in enumerate(pointee.parameters) if position < len(arguments) and IsCallback(p.type)]
  return found


def _Edge(program: Program, source: SourceFile, caller: Function, callee: Function, position: int, argument: Span):
  tokens = source.tokens
  target = (_Key(callee), position)
  where = f"{source.path}:{tokens[argument.first].line} {caller.name}"
  text = Compact(tokens, argument)
  if text.startswith("svc_") and len(argument) == 1:
    return target, text, None, where
  own = caller.ParameterIndex(text) if len(argument) == 1 and tokens[argument.first].kind == IDENTIFIER else None
  if own is not None and IsCallback(caller.parameters[own].type):
    return target, "", (_Key(caller), own), where
  return target, f"<{text}>", None, where
