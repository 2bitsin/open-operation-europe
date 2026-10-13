#!/usr/bin/env python3
"""The M2 rewriter (#13, #14): a token-level model of the game's service dispatch and the substitutions that replace
it with direct calls. Dry runs only; it never writes under sources/ of the checkout.

    rewrite.py [dry-run]                 model, B2/B3 plans and review listings of HEAD -> _local/m2/dry-run/
    rewrite.py --apply b2|b3|all --tree DIR   also rewrite a scratch export DIR (under _local/; made from --rev
                                         when missing) and report what was replaced; b2 then drops the battle
                                         events and deletes what only the battle path used (rewriter/b2.py), b3
                                         the callback parameters, dispatchers and service ids (rewriter/b3.py)
    rewrite.py --apply b3 --only 'io/*,map/*' --tree DIR
                                         only the sites whose callback is bound to dispatchers of those files
                                         (globs relative to sources/opeu/game): one sub-batch
    rewrite.py b1 [--tree DIR --apply [--check]]
                                         the B1 pass (ds -> Game&, rewriter/b1.py): its counts and review listings;
                                         with --apply it rewrites the scratch export DIR
    rewrite.py b4                        the model and listing of the B4 (code pointers -> enums) pass, not applied yet

The input is a `git archive` export of --rev (default HEAD) under _local/m2/tree, never the shared working tree;
--tree reads another tree. docs/port.md "M2 rewriter" describes the rules.
"""
from __future__ import annotations

import argparse
import fnmatch
import json
import re
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from rewriter import report  # noqa: E402
from rewriter.apply import Apply, CheckScratch, MainCheckout  # noqa: E402
from rewriter.b1 import B1Pass, WriteB1  # noqa: E402
from rewriter.b2 import B2Pass  # noqa: E402
from rewriter.b3 import B3Pass  # noqa: E402
from rewriter.check import Check, CompileDatabase  # noqa: E402
from rewriter.model import Model  # noqa: E402
from rewriter.source import ROOT, SOURCES, ExportCommit, SourceTree  # noqa: E402
from rewriter.stubs import B4Model  # noqa: E402

DEFAULT_TREE = ROOT / "_local/m2/tree"
DEFAULT_OUT = ROOT / "_local/m2/dry-run"


def main(argv: list[str] | None = None) -> int:
  parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
  parser.add_argument("command", nargs="?", default="dry-run", choices=("dry-run", "b1", "b4"))
  parser.add_argument("--rev", default="HEAD", help="the commit to export and read (default HEAD)")
  parser.add_argument("--tree", type=Path, help="read (or with --apply rewrite) this tree instead of an export of --rev")
  parser.add_argument("--out", type=Path, default=DEFAULT_OUT, help="where the listings go (default _local/m2/dry-run)")
  parser.add_argument("--apply", nargs="?", const="b1", choices=("b1", "b2", "b3", "all"),
                      help="rewrite --tree with this batch's substitutions (b1: `rewrite.py b1 --tree DIR --apply`)")
  parser.add_argument("--check", action="store_true", help="with --apply: compile every rewritten file (-fsyntax-only)")
  parser.add_argument("--only", default="", help="with --apply b3: comma-separated globs of the dispatchers' files (relative to game/) whose sites to replace")
  args = parser.parse_args(argv)
  if args.apply and args.tree is None:
    parser.error("--apply needs --tree: a scratch export under _local/, never the checkout")
  try:
    tree = _Tree(args)
  except MainCheckout as refusal:
    parser.error(str(refusal))
  started = time.monotonic()
  args.out.mkdir(parents=True, exist_ok=True)
  if args.command == "b1":
    return _B1(tree, args, started)
  model = Model(tree)
  if args.command == "b4":
    return _Stub(args.out / "b4.json", B4Model(model))
  headline = report.WriteModel(model, args.out)["headline"]
  plans = report.WritePlans(model, args.out)
  reviews = report.WriteReviews(model, args.out)
  summary = {**headline, "plans": plans, "reviews": reviews}
  if args.apply:
    applied = Apply(model, args.apply, _Selection(model, args.only))
    summary["applied"] = {"sites": sum(applied.replaced.values()), "files": len(applied.replaced), "skipped": len(applied.skipped)}
    (args.out / f"apply-{args.apply}.txt").write_text("\n".join(applied.skipped) + "\n")
    checked = sorted(applied.replaced)
    if args.apply == "b2":
      summary["b2"] = _B2(tree, args.out)
      checked = _GameFiles(tree)
    if args.apply == "b3":
      summary["b3"] = _B3(tree, _Labels(model), args.out)
      checked = _GameFiles(tree)
    if args.check:
      summary["check"] = _Check(tree, checked, args.out / f"check-{args.apply}.txt")
  summary["seconds"] = round(time.monotonic() - started, 1)
  (args.out / "summary.json").write_text(json.dumps(summary, indent=1) + "\n")
  print(json.dumps(summary, indent=1))
  return 0


def _B1(tree: Path, args, started: float) -> int:
  """The B1 pass over tree: counts and listings into --out; with --apply the rewrite of the scratch tree (and --check
  compiling every game/ file, since a rewritten header reaches all of them)."""
  result = B1Pass(SourceTree(tree))
  summary = {"files": len(result.files), "moved": result.moved, "sites": result.Counts()}
  for kind in ("view", "byte"):
    (args.out / f"b1-{kind}s.txt").write_text("".join(f"{line}\n" for line in result.Listing(kind)))
  if args.apply:
    WriteB1(tree, result)
    if args.check:
      summary["check"] = _Check(tree, _GameFiles(tree), args.out / "check-b1.txt")
  summary["seconds"] = round(time.monotonic() - started, 1)
  (args.out / "b1.json").write_text(json.dumps(summary, indent=1) + "\n")
  print(json.dumps(summary, indent=1))
  return 0


def _B2(tree: Path, out: Path) -> dict:
  """The B2 cleanup after the battle sites are direct: counts, and one listing per kind into out."""
  result = B2Pass(tree)
  for kind in ("dropped", "deleted", "handlers", "keys", "includes", "kept"):
    (out / f"b2-{kind}.txt").write_text("".join(f"{line}\n" for line in getattr(result, kind)))
  return result.Counts()


def _B3(tree: Path, labels: set[str], out: Path) -> dict:
  """The B3 cleanup after the selected sites are direct: counts, and one listing per kind into out."""
  result = B3Pass(tree, labels)
  for kind in ("dropped", "deleted", "enums", "includes", "kept"):
    (out / f"b3-{kind}.txt").write_text("".join(f"{line}\n" for line in getattr(result, kind)))
  return result.Counts()


def _Labels(model: Model) -> set[str]:
  """Every name in a case label of a dispatcher (the service ids, as enumerators)."""
  return {name for dispatcher in model.dispatchers.values() for case in dispatcher.cases for label in case.labels
          for name in re.findall(r"[A-Za-z_]\w*", label)}


def _Selection(model: Model, only: str):
  """The predicate choosing a substitution by the file of the dispatcher its site's callback is bound to."""
  patterns = [pattern.strip() for pattern in only.split(",") if pattern.strip()]
  if not patterns:
    return None
  def Chosen(substitution) -> bool:
    bound = substitution.target.callbacks or substitution.target.chain[:1]
    files = [str(model.dispatchers[name].function.file.relative_to("game")) for name in bound if name in model.dispatchers]
    return bool(files) and all(any(fnmatch.fnmatch(file, pattern) for pattern in patterns) for file in files)
  return Chosen


def _GameFiles(tree: Path) -> list[str]:
  game = tree / SOURCES / "game"
  return sorted(f"game/{path.relative_to(game).as_posix()}" for path in game.rglob("*.cpp"))


def _Check(tree: Path, files: list[str], listing: Path) -> dict:
  database = CompileDatabase()
  if database is None:
    return {"skipped": "no compile database under _build/ (run ./buildutil build first)"}
  failures = Check(tree, files, database)
  listing.write_text("".join(f"{file}\n" + "".join(f"  {error}\n" for error in errors) for file, errors in sorted(failures.items())))
  return {"files": len(files), "failing": len(failures), "errors": sum(len(errors) for errors in failures.values())}


def _Tree(args) -> Path:
  if args.tree is None:
    return ExportCommit(args.rev, DEFAULT_TREE)
  if args.apply:
    CheckScratch(args.tree)
    if not args.tree.exists():
      ExportCommit(args.rev, args.tree)
  return args.tree


def _Stub(path: Path, document: dict) -> int:
  path.write_text(json.dumps(document, indent=1) + "\n")
  print(json.dumps({key: value for key, value in document.items() if not isinstance(value, list)}, indent=1))
  print(f"wrote {path} (model and listing only: this pass is not applied yet)")
  return 0


if __name__ == "__main__":
  sys.exit(main())
