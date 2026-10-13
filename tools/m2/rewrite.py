#!/usr/bin/env python3
"""The M2 rewriter (#13, #14): a token-level model of the game's service dispatch and the substitutions that replace
it with direct calls. Dry runs only; it never writes under sources/ of the checkout.

    rewrite.py [dry-run]                 model, B2/B3 plans and review listings of HEAD -> _local/m2/dry-run/
    rewrite.py --apply b2|b3|all --tree DIR   also rewrite a scratch export DIR (under _local/; made from --rev
                                         when missing) and report what was replaced
    rewrite.py b1 | b4                   the model and listing of the B1 (ds -> Game&) and B4 (code pointers ->
                                         enums) passes, not applied yet

The input is a `git archive` export of --rev (default HEAD) under _local/m2/tree, never the shared working tree;
--tree reads another tree. docs/port.md "M2 rewriter" describes the rules.
"""
from __future__ import annotations

import argparse
import json
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from rewriter import report  # noqa: E402
from rewriter.apply import Apply, CheckScratch, MainCheckout  # noqa: E402
from rewriter.check import Check, CompileDatabase  # noqa: E402
from rewriter.model import Model  # noqa: E402
from rewriter.source import ROOT, ExportCommit  # noqa: E402
from rewriter.stubs import B1Model, B4Model  # noqa: E402

DEFAULT_TREE = ROOT / "_local/m2/tree"
DEFAULT_OUT = ROOT / "_local/m2/dry-run"


def main(argv: list[str] | None = None) -> int:
  parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
  parser.add_argument("command", nargs="?", default="dry-run", choices=("dry-run", "b1", "b4"))
  parser.add_argument("--rev", default="HEAD", help="the commit to export and read (default HEAD)")
  parser.add_argument("--tree", type=Path, help="read (or with --apply rewrite) this tree instead of an export of --rev")
  parser.add_argument("--out", type=Path, default=DEFAULT_OUT, help="where the listings go (default _local/m2/dry-run)")
  parser.add_argument("--apply", choices=("b2", "b3", "all"), help="rewrite --tree with this batch's substitutions")
  parser.add_argument("--check", action="store_true", help="with --apply: compile every rewritten file (-fsyntax-only)")
  args = parser.parse_args(argv)
  if args.apply and args.tree is None:
    parser.error("--apply needs --tree: a scratch export under _local/, never the checkout")
  try:
    tree = _Tree(args)
  except MainCheckout as refusal:
    parser.error(str(refusal))
  started = time.monotonic()
  model = Model(tree)
  args.out.mkdir(parents=True, exist_ok=True)
  if args.command == "b1":
    return _Stub(args.out / "b1.json", B1Model(model))
  if args.command == "b4":
    return _Stub(args.out / "b4.json", B4Model(model))
  headline = report.WriteModel(model, args.out)["headline"]
  plans = report.WritePlans(model, args.out)
  reviews = report.WriteReviews(model, args.out)
  summary = {**headline, "plans": plans, "reviews": reviews}
  if args.apply:
    applied = Apply(model, args.apply)
    summary["applied"] = {"sites": sum(applied.replaced.values()), "files": len(applied.replaced), "skipped": len(applied.skipped)}
    (args.out / f"apply-{args.apply}.txt").write_text("\n".join(applied.skipped) + "\n")
    if args.check:
      summary["check"] = _Check(tree, applied, args.out / f"check-{args.apply}.txt")
  summary["seconds"] = round(time.monotonic() - started, 1)
  (args.out / "summary.json").write_text(json.dumps(summary, indent=1) + "\n")
  print(json.dumps(summary, indent=1))
  return 0


def _Check(tree: Path, applied, listing: Path) -> dict:
  database = CompileDatabase()
  if database is None:
    return {"skipped": "no compile database under _build/ (run ./buildutil build first)"}
  failures = Check(tree, sorted(applied.replaced), database)
  listing.write_text("".join(f"{file}\n" + "".join(f"  {error}\n" for error in errors) for file, errors in sorted(failures.items())))
  return {"files": len(applied.replaced), "failing": len(failures), "errors": sum(len(errors) for errors in failures.values())}


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
