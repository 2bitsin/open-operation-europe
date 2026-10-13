"""Applies a batch's substitutions to a scratch tree (never the main checkout): each outermost resolved site is
replaced, and words that must be evaluated first are declared before the statement holding the site."""
from __future__ import annotations

import re
from collections import Counter
from dataclasses import dataclass, field
from pathlib import Path

from .model import Model
from .source import ROOT, SOURCES, SourceFile
from .substitute import Substitution
from .tokens import IDENTIFIER

SHORT_CIRCUIT = {"&&", "||", "?"}
LOOPS = {"while", "for", "do"}


class MainCheckout(Exception):
  """The tree to rewrite is the shared checkout (or inside it but not under _local/)."""


@dataclass
class FileEdits:
  """The byte edits made in one file so far, and how many sites of each function hoisted words (their names differ)."""
  edits:   list[tuple[int, int, str]] = field(default_factory=list)
  hoisted: Counter = field(default_factory=Counter)


@dataclass
class Applied:
  replaced: dict[str, int] = field(default_factory=dict)  # file -> sites replaced
  skipped:  list[str] = field(default_factory=list)       # sites left as they are, with the reason


def CheckScratch(tree: Path) -> None:
  resolved = tree.resolve()
  scratch = (ROOT / "_local").resolve()
  if resolved == ROOT.resolve() or (ROOT.resolve() in resolved.parents and scratch not in resolved.parents):
    raise MainCheckout(f"{tree} is the shared checkout; --apply rewrites only an export under _local/")


def Apply(model: Model, batch: str) -> Applied:
  """Rewrites model.root's files for batch ('b2', 'b3' or 'all'); returns what was replaced and what was not."""
  CheckScratch(model.root)
  chosen = [s for s in model.substitutions.values() if batch == "all" or (batch == "b2") == s.target.battle]
  applied = Applied()
  by_file: dict = {}
  for substitution in _Outermost(chosen):
    by_file.setdefault(substitution.site.file, []).append(substitution)
  for path, substitutions in sorted(by_file.items()):
    source = model.program.Source(path)
    done, includes = FileEdits(), []
    for substitution in sorted(substitutions, key=lambda s: s.site.span.first):
      edit = _Edits(source, substitution, done)
      if isinstance(edit, str):
        applied.skipped.append(f"{path}:{substitution.site.line} {edit}")
        continue
      done.edits += edit
      includes += [include for include in substitution.includes if include not in includes]
    done.edits += _Includes(source.text, includes)
    _Write(model.root / SOURCES / path, source.text, done.edits)
    applied.replaced[str(path)] = len(substitutions) - sum(1 for line in applied.skipped if line.startswith(f"{path}:"))
  return applied


def _Outermost(substitutions: list[Substitution]) -> list[Substitution]:
  """Sites not nested in another chosen site's words (those carry the nested replacement in their text)."""
  spans: dict = {}
  for substitution in substitutions:
    spans.setdefault(substitution.site.file, []).append(substitution.site.span)
  return [s for s in substitutions if not any(o != s.site.span and o.first <= s.site.span.first and s.site.span.last <= o.last
                                              for o in spans[s.site.file])]


def _Edits(source: SourceFile, substitution: Substitution, done: FileEdits) -> list[tuple[int, int, str]] | str:
  """(start, end, text) byte edits of one site, or why it cannot be applied mechanically. The second and later sites
  of a function that hoist words name them `hoisted<word>_<n>`, so the names of one block never collide."""
  site = substitution.site
  tokens = source.tokens
  if substitution.local:
    return f"needs {', '.join(substitution.local)}, local to {substitution.target.owner.function.file}"
  if substitution.cycle:
    return f"needs {', '.join(substitution.cycle)}, which includes this header (an include cycle); left for a hand edit"
  edits = [(tokens[site.span.first].start, tokens[site.span.last - 1].end, substitution.text)]
  if not substitution.hoisted:
    return edits
  start = _StatementStart(source, site.span.first)
  reason = _Unhoistable(source, start, site.span.first)
  if reason:
    return f"words must be evaluated first but {reason}; left for a hand edit"
  position = tokens[start].start
  if any(edit[0] == edit[1] == position for edit in done.edits):
    return "another site of the same statement already hoists its words; left for a hand edit"
  hoists = done.hoisted[site.function.name]
  done.hoisted[site.function.name] += 1
  suffix = f"_{hoists}" if hoists else ""
  declarations = "".join(f"auto const hoisted{index}{suffix}{{ static_cast<std::uint16_t>({text}) }}; " for index, text in substitution.hoisted)
  text = re.sub(r"\bhoisted(\d+)\b", rf"hoisted\g<1>{suffix}", substitution.text)
  return [(edits[0][0], edits[0][1], text), (position, position, declarations)]


def _Includes(text: str, includes: list[str]) -> list[tuple[int, int, str]]:
  """Inserts the missing `#include` lines after the file's last include (system headers after the last <...> one)."""
  edits = []
  for include in includes:
    line = f"#include {include}" if include.startswith("<") else f'#include "{include}"'
    if line in text:
      continue
    quoted = [m.end() for m in re.finditer(r'^#include ".*$', text, re.M)]
    system = [m.end() for m in re.finditer(r"^#include <.*$", text, re.M)]
    at = (system or quoted or [0])[-1] if include.startswith("<") else (quoted or system or [0])[-1]
    edits.append((at, at, "\n" + line))
  return edits


def _StatementStart(source: SourceFile, index: int) -> int:
  """The first token of the statement holding index (walking out of the brackets it is nested in)."""
  tokens, partners = source.tokens, source.partners
  at = index - 1
  while at >= 0:
    text = tokens[at].text
    if text in (")", "]") and at in partners:
      at = partners[at] - 1
      continue
    if text == "{" and _Initializer(tokens, at):
      at -= 1
      continue
    if text in (";", "{", "}"):
      return at + 1
    if text == ":" and at > 0 and tokens[at - 1].text != ":" and _Label(tokens, at):
      return at + 1
    at -= 1
  return 0


def _Initializer(tokens, brace: int) -> bool:
  """The brace opens an initialiser (`name{`, `= {`, `type<...>{`, an argument), not a block."""
  before = tokens[brace - 1]
  if before.kind == IDENTIFIER:
    return before.text not in ("else", "do", "try")
  return before.text in ("=", "(", ",", ">")


def _Label(tokens, colon: int) -> bool:
  at = colon - 1
  while at > 0 and tokens[at].text not in (";", "{", "}", "case", "default", "?"):
    at -= 1
  return tokens[at].text in ("case", "default")


def _Unhoistable(source: SourceFile, start: int, site: int) -> str:
  tokens, partners = source.tokens, source.partners
  first = tokens[start].text
  if first in LOOPS:
    return f"the site is in a {first} loop's head or body"
  if first in ("if", "switch", "else") and not (start + 1 in partners and start + 1 < site < partners[start + 1]):
    return f"the site is in the body of an unbraced {first}"
  if any(tokens[i].text in SHORT_CIRCUIT for i in range(start, site)):
    return "a short-circuit or conditional operator comes before it"
  return ""


def _Write(path: Path, text: str, edits: list[tuple[int, int, str]]) -> None:
  for start, end, replacement in sorted(edits, key=lambda edit: (edit[0], edit[1]), reverse=True):
    text = text[:start] + replacement + text[end:]
  path.write_text(text)
