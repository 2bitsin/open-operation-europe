"""The B5x pass (#13): the cross-directory shapes that repeat hundreds of times, rewritten token by token.

  - hosts: the text, UI and dialog host structs stop being handed around. `TextHost` is empty, so its parameters and
    arguments go; `UiHost` and `DialogHost` are members of `Game`, so their parameters and arguments go and a routine
    reads `game.Ui().field` / `game.Dialogs().field` where it read `host->field`.
  - random: the synced draws (`port_rand_bounded`, `RandomBelow`, `RandomNext`, `RandomSeed`) become the `GameState`
    methods that load the data segment's random state into the generator and store it back; the direct draws
    (`opeu_rand_bounded`, `opeu_rand_seed`) stay unsynced, on the generator `GameState` owns.

Everything else B5x changes (the Machine call sites, the directory lists) is done by hand: each shape occurs a few
times only (docs/port.md "M2 rewriter")."""
from __future__ import annotations

import re
from collections import Counter
from dataclasses import dataclass, field
from pathlib import Path

from .source import SOURCES

HOST_TYPES = r"(?:TextHost|UiHost|DialogHost)"
HOST_GETTERS = r"port_(?:textout|uiprim|dialogs)_host\(\)"

# `, static_cast<const DialogHost*>(port_dialogs_host())`: the argument, with its comma.
HOST_ARGUMENT = re.compile(rf",\s*static_cast<(?:const\s+)?{HOST_TYPES}(?:\s+const)?\s*\*>\({HOST_GETTERS}\)")
# `, [[maybe_unused]] const UiHost* host`: a parameter, named or not, with its comma.
HOST_PARAMETER = re.compile(rf",\s*(?:\[\[maybe_unused\]\]\s*)?(?:const\s+)?{HOST_TYPES}(?:\s+const)?\s*\*(?:\s*host\b)?")
# `game, host` handed on to the next routine; `host` is only ever a host there.
HOST_FORWARD = re.compile(r"\bgame,\s*host\b(?=\s*[,)])")
# `host->scratch`: a field of the dialogs' or the UI's host.
HOST_FIELD = re.compile(r"\bhost->(load|scratch|memory|ds_segment|tiles_segment)\b")
FIELD_OWNER = {"load": "Dialogs", "scratch": "Dialogs", "memory": "Ui", "ds_segment": "Ui", "tiles_segment": "Ui"}

RANDOM = (
  (re.compile(r"\bport_rand_bounded\(game,\s*"), "game.State().RandomBounded("),
  (re.compile(r"\bRandomBelow\(game,\s*"), "game.State().RandomBounded("),
  (re.compile(r"\bRandomNext\(game\)"), "game.State().RandomNext()"),
  (re.compile(r"\bRandomSeed\(game,\s*"), "game.State().RandomSeed("),
  (re.compile(r"\bopeu_rand_bounded\("), "game.State().Generator().Bounded("),
  (re.compile(r"\bopeu_rand_seed\("), "game.State().Generator().Seed("),
)

RANDOM_INCLUDES = re.compile(r'#include "(?:core|common)/random\.hpp"\n')

# Rewritten by hand: the generator itself and the glue the pass retires.
BY_HAND = {"core/random.cpp", "core/random.hpp", "common/random.hpp", "common/host.hpp", "link/glue.cpp", "link/glue.hpp", "link/hosts.cpp"}


@dataclass
class B5xResult:
  files:    dict[str, Counter] = field(default_factory=dict)  # file -> kind -> sites

  def Counts(self) -> dict[str, int]:
    total: Counter = Counter()
    for counts in self.files.values():
      total.update(counts)
    return dict(sorted(total.items()))


def RewriteHosts(text: str) -> tuple[str, Counter]:
  """Drops host arguments and parameters, forwards and field reads; returns the text and the counts per kind."""
  counts: Counter = Counter()
  text, counts["host_argument"] = HOST_ARGUMENT.subn("", text)
  text, counts["host_parameter"] = HOST_PARAMETER.subn("", text)
  text, counts["host_forward"] = HOST_FORWARD.subn("game", text)
  text, counts["host_field"] = HOST_FIELD.subn(lambda m: f"game.{FIELD_OWNER[m.group(1)]}().{m.group(1)}", text)
  return text, +counts


def RewriteRandom(text: str) -> tuple[str, Counter]:
  """The synced draws become GameState methods, the direct ones the generator's; the generator headers' includes go
  (game-state.hpp declares both)."""
  counts: Counter = Counter()
  for pattern, replacement in RANDOM:
    text, counts[pattern.pattern] = pattern.subn(replacement, text)
  text, counts["include"] = RANDOM_INCLUDES.subn("", text)
  return text, +counts


def B5xPass(tree: Path) -> B5xResult:
  """Rewrites every game/ source of the tree in place."""
  result = B5xResult()
  game = tree / SOURCES / "game"
  for path in sorted(p for p in game.rglob("*") if p.suffix in (".cpp", ".hpp") and "gen" not in p.parts):
    if path.relative_to(game).as_posix() in BY_HAND:
      continue
    before = path.read_text()
    text, hosts = RewriteHosts(before)
    text, randoms = RewriteRandom(text)
    if text != before:
      path.write_text(text)
      result.files[path.relative_to(game).as_posix()] = hosts + randoms
  return result
