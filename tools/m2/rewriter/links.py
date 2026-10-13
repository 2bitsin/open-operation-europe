"""The routine link table: its entries (address, name, handler), the `BattleRoutine` keys, and the `port_routine_*`
handlers with the words each passes to its routine."""
from __future__ import annotations

import re
from dataclasses import dataclass, field
from pathlib import PurePosixPath

from .dispatch import Case, Classify, DeclaredNames, Dispatcher, IsFiller
from .program import Program
from .syntax import Span, Statements
from .words import FindAccessors, WordContext

TABLE = PurePosixPath("game/link/gen/link-table.cpp")
BATTLE_KEYS = PurePosixPath("game/common/battle-routine.hpp")
_ENTRY = re.compile(r'\{\s*0x([0-9a-fA-F]+)u?\s*,\s*"(\w+)"\s*,\s*(\w+)\s*\}')
_KEY = re.compile(r"^\s*([A-Z0-9_]+)\s*=\s*0x([0-9a-fA-F]+)\s*,", re.M)


@dataclass(frozen=True)
class LinkEntry:
  address: int
  name:    str
  handler: str | None


@dataclass
class Handler:
  function:   object
  case:       Case
  dispatcher: Dispatcher  # the handler seen as a one-case dispatcher (its words and parameters)

  @property
  def name(self) -> str:
    return self.function.name


@dataclass
class LinkTable:
  entries:  dict[int, LinkEntry] = field(default_factory=dict)
  keys:     dict[str, int] = field(default_factory=dict)  # BattleRoutine enumerator -> address
  handlers: dict[str, Handler] = field(default_factory=dict)

  def HandlerAt(self, address: int) -> Handler | None:
    entry = self.entries.get(address)
    return self.handlers.get(entry.handler) if entry is not None and entry.handler else None

  def KeyName(self, address: int) -> str:
    return next((name for name, value in self.keys.items() if value == address), f"0x{address:05x}")


def ReadLinkTable(program: Program) -> LinkTable:
  table = LinkTable()
  if TABLE in program.tree.files:
    for match in _ENTRY.finditer(program.Source(TABLE).text):
      handler = None if match.group(3) == "nullptr" else match.group(3)
      table.entries[int(match.group(1), 16)] = LinkEntry(int(match.group(1), 16), match.group(2), handler)
  if BATTLE_KEYS in program.tree.files:
    table.keys = {name: int(value, 16) for name, value in _KEY.findall(program.Source(BATTLE_KEYS).text)}
  for name, definitions in program.definitions.items():
    if name.startswith("port_routine_"):
      dispatcher, case = _HandlerCase(program, definitions[0])
      table.handlers[name] = Handler(definitions[0], case, dispatcher)
  return table


def _HandlerCase(program: Program, function) -> tuple[Dispatcher, Case]:
  """A handler's body classified like a dispatcher case: its routine call and the words it reads, in order."""
  source = program.Source(function.file)
  parameters = function.parameters
  args = parameters[1].name if len(parameters) > 2 else ""
  count = parameters[2].name if len(parameters) > 2 else ""
  context = WordContext(args or "<args>", count or "<count>")
  body = Span(function.body.first + 1, function.body.last - 1)
  statements = Statements(source.tokens, source.partners, body)
  context.accessors = FindAccessors(source, statements, context)
  dispatcher = Dispatcher(function, context, "", True, constants=program.ConstantsOf(function.file))
  dispatcher.declared = {name for s in statements for name in DeclaredNames(source, s)}
  case = Case([function.name], [None], body, [s for s in statements if not _Accessor(source, s, context) and not IsFiller(source, s)], line=function.line)
  Classify(source, dispatcher, case)
  dispatcher.cases.append(case)
  return dispatcher, case


def _Accessor(source, statement: Span, context: WordContext) -> bool:
  first = source.tokens[statement.first:statement.first + 3]
  return bool(first) and first[0].text == "auto" and any(token.text in context.accessors for token in first)
