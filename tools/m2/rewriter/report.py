"""What the dry run writes: the model as JSON, the B2/B3 substitution plans (JSON and a readable listing) and the
review listings."""
from __future__ import annotations

import json
from collections import Counter, defaultdict
from pathlib import Path

from .model import Model
from .substitute import CalleeOf, Substitution
from .syntax import Compact, Text


def WriteModel(model: Model, out: Path) -> dict:
  program = model.program
  dispatchers = []
  for dispatcher in model.dispatchers.values():
    source = program.Source(dispatcher.function.file)
    dispatchers.append({
      "name": dispatcher.name, "file": str(dispatcher.function.file), "line": dispatcher.function.line, "battle": dispatcher.battle,
      "forward": dispatcher.forward.target if dispatcher.forward else None, "irregular": dispatcher.irregular,
      "guards": [Compact(source.tokens, guard.condition) for guard in dispatcher.guards],
      "cases": [case.Summary(source) for case in dispatcher.cases],
      "default": dispatcher.default.Summary(source) if dispatcher.default else None,
    })
  handlers = []
  addresses = defaultdict(list)
  for entry in model.links.entries.values():
    if entry.handler:
      addresses[entry.handler].append(entry.address)
  for handler in model.links.handlers.values():
    source = program.Source(handler.function.file)
    summary = handler.case.Summary(source)
    handlers.append({"name": handler.name, "file": str(handler.function.file), "line": handler.function.line,
                     "addresses": [f"0x{a:05x}" for a in addresses.get(handler.name, [])],
                     "keys": [model.links.KeyName(a) for a in addresses.get(handler.name, [])],
                     "kind": summary["kind"], "reason": summary["reason"], "callee": _Callee(handler.case, source),
                     "argument_words": _ArgumentOrder(handler.case), "expression": summary["expression"]})
  sites = [_Site(model, site) for site in model.sites]
  document = {"headline": model.Headline(), "dispatchers": dispatchers, "handlers": handlers, "call_sites": sites}
  _Json(out / "model.json", document)
  return document


def _Callee(case, source) -> str:
  return CalleeOf(source, case.expression) if case.expression is not None else ""


def _ArgumentOrder(case) -> list[int]:
  """The words the handler reads, in the order its routine call names them (`Arg(1), Arg(0)` -> [1, 0])."""
  return [read.index for read in sorted(case.words.reads, key=lambda read: read.span.first)]


def _Site(model: Model, site) -> dict:
  source = model.program.Source(site.file)
  target = model.targets[site.Key()]
  substitution = model.substitutions.get(site.Key())
  return {"file": str(site.file), "line": site.line, "function": site.function.name, "call": Compact(source.tokens, site.span),
          "used": site.used, "battle": target.battle, "chain": target.chain, "callbacks": target.callbacks,
          "resolved": target.resolved, "failure": target.failure,
          "callee": substitution.callee if substitution else None}


def WritePlans(model: Model, out: Path) -> dict[str, int]:
  plans = {"b2": [], "b3": []}
  for substitution in model.substitutions.values():
    plans["b2" if substitution.target.battle else "b3"].append(substitution)
  for name, entries in plans.items():
    entries.sort(key=lambda s: (str(s.site.file), s.site.span.first))
    _Json(out / f"plan-{name}.json", [_PlanEntry(model, s) for s in entries])
    _Listing(out / f"plan-{name}.txt", [_Readable(model, s) for s in entries])
  return {name: len(entries) for name, entries in plans.items()}


def _PlanEntry(model: Model, substitution: Substitution) -> dict:
  site = substitution.site
  source = model.program.Source(site.file)
  return {"file": f"sources/opeu/{site.file}", "line": site.line, "function": site.function.name,
          "start": source.tokens[site.span.first].start, "end": source.tokens[site.span.last - 1].end,
          "original": Text(source.tokens, site.span, source.text), "replacement": substitution.text,
          "hoisted": [{"word": index, "text": text} for index, text in substitution.hoisted],
          "callee": substitution.callee, "chain": substitution.target.chain, "used": site.used,
          "past_end": substitution.past_end, "void_used": substitution.void_used, "result_cast": substitution.result_cast,
          "order": substitution.order, "invisible": substitution.invisible, "includes": substitution.includes,
          "local": substitution.local}


def _Readable(model: Model, substitution: Substitution) -> str:
  site = substitution.site
  source = model.program.Source(site.file)
  lines = [f"{site.file}:{site.line} in {site.function.name}  [{' > '.join(substitution.target.chain)}]",
           f"  - {Compact(source.tokens, site.span)}", f"  + {' '.join(substitution.text.split())}"]
  lines += [f"    hoisted{index} = {' '.join(text.split())}" for index, text in substitution.hoisted]
  return "\n".join(lines)


def WriteReviews(model: Model, out: Path) -> dict[str, int]:
  substitutions = sorted(model.substitutions.values(), key=lambda s: (str(s.site.file), s.site.line))
  where = lambda s: f"{s.site.file}:{s.site.line} {s.site.function.name} -> {s.callee} [{' > '.join(s.target.chain)}]"
  reviews = {
    "past-end":      [f"{where(s)}: reads word position(s) {s.past_end} of {len(s.site.words)} given; becomes literal 0" for s in substitutions if s.past_end],
    "void-used":     [f"{where(s)}: the value is used but the case sets no result (it is 0)" for s in substitutions if s.void_used],
    "result-cast":   [f"{where(s)}: result of {s.callee} converted with static_cast<std::uint16_t>" for s in substitutions if s.result_cast],
    "order":         [f"{where(s)}: {s.order}; words {[i for i, _ in s.hoisted]} hoisted in order" for s in substitutions if s.order],
    "includes":      [f"{where(s)}: includes {', '.join(s.includes)}" for s in substitutions if s.includes],
    "local":         [f"{where(s)}: needs {', '.join(s.local)}, defined only in {s.target.owner.function.file}" for s in substitutions if s.local],
    "cycle":         [f"{where(s)}: needs {', '.join(s.cycle)}, which includes {s.site.file} (an include cycle)" for s in substitutions if s.cycle],
    "near":          [f"{where(s)}: near pointer {near}" for s in substitutions for near in s.near],
    "unbound":       [f"{where(s)}: some caller binds the callback where the tool cannot see; resolved by {s.target.callbacks}" for s in substitutions if s.target.unbound],
    "polymorphic":   _Polymorphic(model),
    "unresolved":    _Unresolved(model),
  }
  for name, lines in reviews.items():
    _Listing(out / f"review-{name}.txt", lines)
  return {name: len(lines) for name, lines in reviews.items()}


def _Polymorphic(model: Model) -> list[str]:
  """Callback parameters bound to more than one svc_*, and whether the Call sites through them reach one target."""
  divergent = defaultdict(list)
  for site in model.sites:
    target = model.targets[site.Key()]
    if len(target.callbacks) > 1:
      divergent[(str(site.function.file), site.function.name)].append("same target" if target.resolved else "DIFFERENT targets" if "different code" in target.failure else "unresolved")
  lines = []
  for key, names in sorted(model.bindings.Polymorphic().items()):
    file, routine = key[0].split("::")
    verdicts = Counter(divergent.get((file, routine), []))
    svcs = sorted(name for name in names if name.startswith("svc_"))
    lines.append(f"{file} {routine} parameter {key[1]}: {len(svcs)} dispatchers {svcs}; its Call sites: {dict(verdicts) or 'none'}")
  return lines


def _Unresolved(model: Model) -> list[str]:
  lines = []
  for site in sorted(model.sites, key=lambda s: (str(s.file), s.line)):
    target = model.targets[site.Key()]
    if not target.resolved:
      source = model.program.Source(site.file)
      lines.append(f"{site.file}:{site.line} {site.function.name}: {target.failure}  [{' > '.join(target.chain)}]  {' '.join(Compact(source.tokens, site.span).split())[:160]}")
  return lines


def _Json(path: Path, document) -> None:
  path.parent.mkdir(parents=True, exist_ok=True)
  path.write_text(json.dumps(document, indent=1) + "\n")


def _Listing(path: Path, lines: list[str]) -> None:
  path.parent.mkdir(parents=True, exist_ok=True)
  path.write_text("\n".join(lines) + ("\n" if lines else ""))
