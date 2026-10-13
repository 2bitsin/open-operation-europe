"""The two passes the rewriter models but does not yet apply.

B1 (#14a): every `uint8_t* ds` / `uint8_t ds[65536]` parameter becomes `Game& game`, `Data(ds)` becomes
`Data(game)`, `ds` handed on becomes `game`; raw `ds[i]` / `ds + n` views become `Bytes(...)` and are listed.
B4 (#13): far code pointers stored in the data segment become an enum per table with a constexpr dispatch table; the
model lists every code table, the routine behind each initial `{offset, segment}`, and the sites that dispatch
through `segment * 16 + offset`."""
from __future__ import annotations

import re
from collections import Counter
from dataclasses import dataclass, field
from pathlib import PurePosixPath

from .model import Model
from .syntax import Compact, Span

SEGMENT_HEADER = PurePosixPath("game/common/data-segment.hpp")
CODE_WORDS = ("action", "handler", "hook", "initializer", "mission", "glyph_draw")
_FIELD = re.compile(r"(?:std::array<FarPointer,\s*(\d+)>|FarPointer)\s+(\w+);")
_PAIR = re.compile(r"\{\s*(0x[0-9a-fA-F]+|\d+)\s*,\s*(0x[0-9a-fA-F]+|\d+)\s*\}")


@dataclass
class DsUse:
  kind: str   # parameter, data, raw, argument
  file: str
  line: int
  text: str


def B1Model(model: Model) -> dict:
  """Every token B1 changes, by kind, and the raw views listed for review."""
  uses: list[DsUse] = []
  for path, source in sorted(model.program.tree.files.items()):
    if not str(path).startswith("game/"):
      continue
    uses += _DsUses(path, source)
  by_kind = Counter(use.kind for use in uses)
  by_directory = Counter((str(PurePosixPath(use.file).parts[1]) if len(PurePosixPath(use.file).parts) > 2 else "game", use.kind) for use in uses)
  return {"counts": dict(by_kind),
          "by_directory": {f"{directory} {kind}": count for (directory, kind), count in sorted(by_directory.items())},
          "rules": {"parameter": "uint8_t* ds / uint8_t ds[65536] -> Game& game", "data": "Data(ds) -> Data(game)",
                    "argument": "ds handed on -> game", "raw": "ds[i] / ds + n -> Bytes(near, size) (reviewed by hand)"},
          "raw": [f"{use.file}:{use.line}: {use.text}" for use in uses if use.kind == "raw"]}


def _DsUses(path: PurePosixPath, source) -> list[DsUse]:
  tokens = source.tokens
  found = []
  for index, token in enumerate(tokens):
    if token.text != "ds" or tokens[index - 1].text in (".", "->", "::"):
      continue
    before, after = tokens[index - 1].text, tokens[index + 1].text if index + 1 < len(tokens) else ""
    if before == "*" and tokens[index - 2].text == "uint8_t" or (before == "uint8_t" and after == "["):
      kind = "parameter"
    elif before == "(" and tokens[index - 2].text == "Data" and after == ")":
      kind = "data"
    elif after == "[" or (after == "+" and tokens[index + 2].text != "+"):
      kind = "raw"
    elif before in ("(", ",") and after in (")", ","):
      kind = "argument"
    else:
      kind = "other"
    found.append(DsUse(kind, str(path), token.line, Compact(tokens, Span(max(index - 3, 0), min(index + 4, len(tokens))))))
  return found


@dataclass
class CodeTable:
  name:     str
  size:     int
  enum:     str
  initial:  list[str] = field(default_factory=list)  # the routine behind each initial pointer


def B4Model(model: Model) -> dict:
  tables = _CodeTables(model)
  _Initialisers(model, tables)
  dispatch = []
  for path, source in sorted(model.program.tree.files.items()):
    text = source.text
    for match in re.finditer(r"port_call\s*\(([^;]*?\*\s*16u?\s*\+[^;]*?)\)\s*[;)]", text):
      dispatch.append(f"{path}:{text.count(chr(10), 0, match.start()) + 1}: {' '.join(match.group(0).split())}")
  return {"tables": [{"field": t.name, "slots": t.size, "enum": t.enum, "initial": t.initial} for t in tables],
          "code_slots": sum(t.size for t in tables), "dispatch_sites": dispatch,
          "rule": "FarPointer slot -> enum class <Enum> : std::uint32_t (4-byte slot kept); {off, seg} -> <Enum>::<ROUTINE>; "
                  "port_call(seg * 16 + off) -> constexpr std::array of routines indexed by std::to_underlying"}


def _CodeTables(model: Model) -> list[CodeTable]:
  if SEGMENT_HEADER not in model.program.tree.files:
    return []
  text = model.program.Source(SEGMENT_HEADER).text
  tables = []
  for match in _FIELD.finditer(text):
    name = match.group(2)
    if any(word in name for word in CODE_WORDS) and "text" not in name:
      tables.append(CodeTable(name, int(match.group(1) or 1), _EnumName(name)))
  return tables


def _EnumName(field_name: str) -> str:
  singular = re.sub(r"s$", "", field_name)
  return "".join(part.capitalize() for part in singular.split("_"))


def _Initialisers(model: Model, tables: list[CodeTable]) -> None:
  """The routine names of each table's `data.<field> = { { {off, seg}, ... } }` in game/data."""
  by_name = {table.name: table for table in tables}
  for path, source in model.program.tree.files.items():
    if not str(path).startswith("game/data/"):
      continue
    for match in re.finditer(r"data\.(\w+)\s*=\s*\{\s*\{(.*?)\}\s*\};", source.text, re.S):
      table = by_name.get(match.group(1))
      if table is None:
        continue
      for offset, segment in _PAIR.findall(match.group(2)):
        address = int(segment, 0) * 16 + int(offset, 0)
        entry = model.links.entries.get(address)
        table.initial.append("NONE" if address == 0 else entry.name if entry else f"0x{address:05x} (unlinked)")
