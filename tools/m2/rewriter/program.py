"""The whole program as the rewriter sees it: every file's functions, indexed by name, and the constants in scope."""
from __future__ import annotations

from dataclasses import dataclass, field
from functools import cached_property
from pathlib import PurePosixPath

from .constants import ConstantScopes, Constants
from .functions import Function, ScanFunctions
from .source import SourceFile, SourceTree


@dataclass
class Program:
  tree:         SourceTree
  definitions:  dict[str, list[Function]] = field(default_factory=dict)
  declarations: dict[str, list[Function]] = field(default_factory=dict)
  by_file:      dict[PurePosixPath, list[Function]] = field(default_factory=dict)

  def __post_init__(self) -> None:
    for path, source in self.tree.files.items():
      found = ScanFunctions(source)
      self.by_file[path] = sorted(found.definitions, key=lambda function: function.body.first)
      for function in found.definitions:
        self.definitions.setdefault(function.name, []).append(function)
      for function in found.declarations:
        self.declarations.setdefault(function.name, []).append(function)

  @cached_property
  def scopes(self) -> ConstantScopes:
    return ConstantScopes(self.tree)

  def Source(self, path: PurePosixPath) -> SourceFile:
    return self.tree.files[path]

  def ConstantsOf(self, path: PurePosixPath) -> Constants:
    return self.scopes.Of(path)

  def Enclosing(self, path: PurePosixPath, index: int) -> Function | None:
    """The function definition whose body holds token index (the innermost, for a local class's member)."""
    holding = [function for function in self.by_file.get(path, []) if function.body.Contains(index)]
    return max(holding, key=lambda function: function.body.first) if holding else None

  def Resolve(self, name: str, from_file: PurePosixPath) -> Function | None:
    """The definition a call by name reaches from a file: the only one there is, else the one in the nearest of the file
  and its includes (an internal helper of another file is not visible)."""
    found = self.definitions.get(name, [])
    if len(found) <= 1:
      return found[0] if found else None
    for path in self.scopes.Closure(from_file):
      visible = [function for function in found if function.file == path]
      if visible:
        return visible[0]
    return None

  def Signature(self, name: str, from_file: PurePosixPath) -> Function | None:
    """A definition or, failing that, a declaration of name (for parameter and return types)."""
    defined = self.Resolve(name, from_file)
    if defined is not None:
      return defined
    declared = self.declarations.get(name, [])
    return declared[0] if declared else None
