"""The sources the rewriter reads: a `git archive` export of a commit (never the shared working tree) or a given tree."""
from __future__ import annotations

import io
import shutil
import subprocess
import tarfile
from dataclasses import dataclass, field
from functools import cached_property
from pathlib import Path, PurePosixPath

from .syntax import Partners
from .tokens import Token, Tokenize

ROOT = Path(__file__).resolve().parents[3]
SOURCES = PurePosixPath("sources/opeu")
SUFFIXES = (".cpp", ".hpp")
COMMIT_MARK = ".commit"


@dataclass
class SourceFile:
  path: PurePosixPath  # relative to sources/opeu, e.g. game/map/btlhit.cpp
  text: str

  @cached_property
  def tokens(self) -> list[Token]:
    return Tokenize(self.text)

  @cached_property
  def partners(self) -> dict[int, int]:
    return Partners(self.tokens)


@dataclass
class SourceTree:
  """The C++ files under sources/opeu/<scope> of one tree, by path relative to sources/opeu."""
  root:  Path
  scope: tuple[str, ...] = ("game", "glue", "services")  # glue, services: the headers game files include
  files: dict[PurePosixPath, SourceFile] = field(default_factory=dict)

  def __post_init__(self) -> None:
    base = self.root / SOURCES
    for directory in self.scope:
      for path in sorted((base / directory).rglob("*")):
        if path.suffix in SUFFIXES and path.is_file():
          relative = PurePosixPath(path.relative_to(base).as_posix())
          self.files[relative] = SourceFile(relative, path.read_text(errors="replace"))


def ExportCommit(revision: str, destination: Path) -> Path:
  """Exports sources/ of a commit into destination (reused when it already holds that commit); returns it."""
  commit = subprocess.run(["git", "rev-parse", "--verify", f"{revision}^{{commit}}"], cwd=ROOT, check=True, capture_output=True, text=True).stdout.strip()
  mark = destination / COMMIT_MARK
  if mark.exists() and mark.read_text().strip() == commit:
    return destination
  if destination.exists():
    shutil.rmtree(destination)
  destination.mkdir(parents=True)
  archive = subprocess.run(["git", "archive", "--format=tar", commit, "sources"], cwd=ROOT, check=True, capture_output=True).stdout
  with tarfile.open(fileobj=io.BytesIO(archive)) as tar:
    tar.extractall(destination)
  mark.write_text(commit + "\n")
  return destination


def CommitOf(tree: Path) -> str:
  mark = tree / COMMIT_MARK
  return mark.read_text().strip() if mark.exists() else "(not an export)"
