"""The pass the rewriter models but does not yet apply.

B4 (#13): far code pointers stored in the data segment become an enum per table with a constexpr dispatch table; the
model lists every code table, the routine behind each initial `{offset, segment}`, and the sites that dispatch
through `segment * 16 + offset`."""
from __future__ import annotations

import re
from dataclasses import dataclass, field
from pathlib import PurePosixPath

from .model import Model

SEGMENT_HEADER = PurePosixPath("game/common/data-segment.hpp")
CODE_WORDS = ("action", "handler", "hook", "initializer", "mission", "glyph_draw")
_FIELD = re.compile(r"(?:std::array<FarPointer,\s*(\d+)>|FarPointer)\s+(\w+);")
_PAIR = re.compile(r"\{\s*(0x[0-9a-fA-F]+|\d+)\s*,\s*(0x[0-9a-fA-F]+|\d+)\s*\}")


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
