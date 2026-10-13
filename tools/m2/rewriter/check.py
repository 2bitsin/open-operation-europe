"""The compiler as the checker: each rewritten file compiled with -fsyntax-only by its command from the build's
compile database, with the checkout's sources/ swapped for the scratch tree's."""
from __future__ import annotations

import json
import os
import re
import shlex
import subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

from .source import ROOT, SOURCES

ERROR = re.compile(r"^(\S+?):(\d+):\d+: error: (.*)$", re.M)


def CompileDatabase() -> Path | None:
  found = sorted((ROOT / "_build").glob("*/compile_commands.json"))
  return found[0] if found else None


def Check(tree: Path, files: list[str], database: Path) -> dict[str, list[str]]:
  """file (relative to sources/opeu) -> its compile errors in the scratch tree (only files that fail)."""
  commands = {Path(entry["file"]).resolve(): entry for entry in json.loads(database.read_text())}
  checkout = str((ROOT / "sources").resolve())
  scratch = str((tree / "sources").resolve())
  jobs = []
  for file in files:
    entry = commands.get((ROOT / SOURCES / file).resolve())
    if entry is not None:
      jobs.append((file, _Command(entry, checkout, scratch)))
  with ThreadPoolExecutor(max_workers=os.cpu_count() or 4) as pool:
    results = dict(zip([file for file, _ in jobs], pool.map(lambda job: _Run(job[1], tree), jobs)))
  return {file: errors for file, errors in results.items() if errors}


def _Command(entry: dict, checkout: str, scratch: str) -> list[str]:
  words = entry.get("arguments") or shlex.split(entry["command"])
  words = [word.replace(checkout, scratch) for word in words]
  if "-o" in words:
    at = words.index("-o")
    del words[at:at + 2]
  return words + ["-fsyntax-only"]


def _Run(command: list[str], tree: Path) -> list[str]:
  result = subprocess.run(command, cwd=tree, capture_output=True, text=True)
  if result.returncode == 0:
    return []
  return [f"{Path(match.group(1)).name}:{match.group(2)}: {match.group(3)}" for match in ERROR.finditer(result.stderr)] or ["(failed without an error line)"]
