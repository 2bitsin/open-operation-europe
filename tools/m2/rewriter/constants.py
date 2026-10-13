"""Integer values of enumerators and constexpr constants, as each file sees them (its own and its includes')."""
from __future__ import annotations

import re
from dataclasses import dataclass, field
from pathlib import PurePosixPath

from .source import SourceFile, SourceTree
from .syntax import Span, SplitTopLevel
from .tokens import IDENTIFIER, NUMBER, Token

INCLUDE = re.compile(r'^[ \t]*#[ \t]*include[ \t]*"([^"]+)"', re.M)
_SAFE = re.compile(r"^[0-9a-fA-FxX+\-*/%|&^~<>=!()\s]*$")
_CONSTANT = re.compile(r"\bconstexpr\s+(?:std::)?(?:u?int(?:8|16|32)_t|unsigned|int|auto)\s+([A-Za-z_]\w*)\s*(?:\{\s*([^{};]+?)\s*\}|=\s*([^;{}]+?)\s*);")


@dataclass
class Constants:
  """name -> value; a scoped enumerator is also listed as Enum::NAME."""
  values: dict[str, int] = field(default_factory=dict)

  def Evaluate(self, expression: str) -> int | None:
    """The value of a constant expression of literals and known names (None when it is not one)."""
    text = re.sub(r"\bstatic_cast\s*<[^<>]*>", "", expression)
    text = re.sub(r"\bstd::to_underlying\b", "", text)
    text = re.sub(r"(?<![\w:])([A-Za-z_][\w:]*)", lambda match: str(self._Lookup(match.group(1))), text)
    text = re.sub(r"\b(0[xX][0-9a-fA-F]+|\d+)[uUlL]*\b", r"\1", text)
    if "None" in text or not _SAFE.match(text):
      return None
    text = text.replace("&&", " and ").replace("||", " or ").replace("/", "//")
    text = re.sub(r"!(?!=)", " not ", text)
    try:
      return int(eval(text, {"__builtins__": {}}))  # only digits, operators and parentheses reach here
    except (SyntaxError, TypeError, ZeroDivisionError):
      return None

  def _Lookup(self, name: str) -> int | None:
    """A name, or its last two parts when it is spelled with a namespace (opeu::game::Service::X)."""
    if name in self.values:
      return self.values[name]
    return self.values.get("::".join(name.split("::")[-2:])) if name.count("::") > 1 else None


def FileConstants(source: SourceFile) -> Constants:
  """The enumerators and constexpr integer constants one file defines."""
  constants = Constants()
  for match in _CONSTANT.finditer(source.text):
    value = constants.Evaluate(match.group(2) or match.group(3))
    if value is not None:
      constants.values[match.group(1)] = value
  tokens = source.tokens
  for index, token in enumerate(tokens):
    if token.text == "enum":
      _Enum(source, index, constants)
  return constants


def _Enum(source: SourceFile, index: int, constants: Constants) -> None:
  tokens, partners = source.tokens, source.partners
  scoped = tokens[index + 1].text in ("class", "struct")
  name_index = index + 2 if scoped else index + 1
  name = tokens[name_index].text if tokens[name_index].kind == IDENTIFIER else ""
  brace = next((i for i in range(index, min(index + 12, len(tokens))) if tokens[i].text in ("{", ";")), None)
  if brace is None or tokens[brace].text != "{" or brace not in partners:
    return
  value = -1
  for part in SplitTopLevel(tokens, Span(brace + 1, partners[brace]), ","):
    if not len(part) or tokens[part.first].kind != IDENTIFIER:
      continue
    enumerator = tokens[part.first].text
    if len(part) > 2 and tokens[part.first + 1].text == "=":
      local = Constants({**constants.values, **({f"{name}::{k}": v for k, v in constants.values.items()} if name else {})})
      evaluated = local.Evaluate(" ".join(token.text for token in tokens[part.first + 2:part.last]))
      value = evaluated if evaluated is not None else value + 1
    else:
      value += 1
    if name:
      constants.values[f"{name}::{enumerator}"] = value
    constants.values.setdefault(enumerator, value)  # also bare: unscoped, or reached through `using enum`


class ConstantScopes:
  """Per file: the constants it defines merged with those of every header it includes, transitively."""

  def __init__(self, tree: SourceTree):
    self._tree = tree
    self._own = {path: FileConstants(source) for path, source in tree.files.items()}
    self._by_name: dict[str, list[PurePosixPath]] = {}
    for path in tree.files:
      self._by_name.setdefault(path.name, []).append(path)
    self._cache: dict[PurePosixPath, Constants] = {}
    self._closures: dict[PurePosixPath, list[PurePosixPath]] = {}

  def Of(self, path: PurePosixPath) -> Constants:
    if path not in self._cache:
      merged: dict[str, int] = {}
      for included in reversed(self.Closure(path)):
        merged.update(self._own[included].values)
      self._cache[path] = Constants(merged)
    return self._cache[path]

  def DefiningFiles(self, name: str) -> list[PurePosixPath]:
    """The files that define a constant or enumerator of that name themselves."""
    return [path for path, constants in self._own.items() if name in constants.values]

  def Closure(self, path: PurePosixPath) -> list[PurePosixPath]:
    """The file and every file it includes, transitively, nearest first."""
    if path in self._closures:
      return self._closures[path]
    order, pending, seen = [], [path], {path}
    while pending:
      current = pending.pop(0)
      order.append(current)
      for include in INCLUDE.findall(self._tree.files[current].text):
        target = self._Resolve(include, current)
        if target is not None and target not in seen:
          seen.add(target)
          pending.append(target)
    self._closures[path] = order
    return order

  def _Resolve(self, include: str, includer: PurePosixPath) -> PurePosixPath | None:
    candidates = [p for p in self._by_name.get(PurePosixPath(include).name, []) if p.as_posix().endswith(include)]
    beside = [p for p in candidates if p.parent == includer.parent]
    return (beside or candidates or [None])[0]



def IsIntegerLiteral(token: Token) -> bool:
  return token.kind == NUMBER and re.fullmatch(r"(0[xX][0-9a-fA-F]+|\d+)[uUlL]*", token.text) is not None
