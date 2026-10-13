"""The model of one tree: dispatchers, link handlers, call sites, their targets and substitutions, built once."""
from __future__ import annotations

from collections import Counter
from dataclasses import dataclass, field
from functools import cached_property
from pathlib import Path

from .calls import CallSite, BindCallbacks, Bindings, FindCallSites
from .dispatch import Dispatcher, FindDispatchers
from .links import LinkTable, ReadLinkTable
from .program import Program
from .resolve import Resolver, Target
from .source import CommitOf, SourceTree
from .substitute import Substituter, Substitution
from .syntax import Compact


@dataclass
class Model:
  root:    Path
  program: Program = field(init=False)

  def __post_init__(self) -> None:
    self.program = Program(SourceTree(self.root))

  @cached_property
  def dispatchers(self) -> dict[str, Dispatcher]:
    return FindDispatchers(self.program)

  @cached_property
  def links(self) -> LinkTable:
    return ReadLinkTable(self.program)

  @cached_property
  def sites(self) -> list[CallSite]:
    return FindCallSites(self.program)

  @cached_property
  def bindings(self) -> Bindings:
    return BindCallbacks(self.program)

  @cached_property
  def targets(self) -> dict[str, Target]:
    resolver = Resolver(self.program, self.dispatchers, self.links, self.bindings)
    return {site.Key(): resolver.Resolve(site) for site in self.sites}

  @cached_property
  def substitutions(self) -> dict[str, Substitution]:
    """Every resolved site's substitution, innermost sites first so outer sites see their replaced words."""
    substituter = Substituter(self.program)
    ordered = sorted(self.sites, key=lambda site: len(site.span))
    return {site.Key(): substituter.Substitute(site, self.targets[site.Key()]) for site in ordered if self.targets[site.Key()].resolved}

  def Commit(self) -> str:
    return CommitOf(self.root)

  def Headline(self) -> dict:
    cases = [case for dispatcher in self.dispatchers.values() for case in dispatcher.cases]
    targets = list(self.targets.values())
    return {
      "commit":               self.Commit(),
      "files":                len(self.program.tree.files),
      "dispatchers":          len(self.dispatchers),
      "dispatchers_by_shape": dict(Counter(_Shape(d) for d in self.dispatchers.values())),
      "case_groups":          len(cases),
      "case_labels":          sum(len(case.labels) for case in cases),
      "case_labels_by_kind":  dict(Counter(case.kind for case in cases for _ in case.labels)),
      "cases_reserving_stack": sum(len(case.labels) for case in cases if case.stack),
      "link_entries":         len(self.links.entries),
      "link_entries_with_handler": sum(1 for entry in self.links.entries.values() if entry.handler),
      "handlers_defined":     len(self.links.handlers),
      "handlers_by_kind":     dict(Counter(handler.case.kind for handler in self.links.handlers.values())),
      "call_sites":           len(self.sites),
      "call_sites_battle_key": sum(1 for site in self.sites if "BattleRoutine" in Compact(self.program.Source(site.file).tokens, site.service)),
      "resolved":             sum(1 for target in targets if target.resolved),
      "resolved_battle_path": sum(1 for target in targets if target.resolved and target.battle),
      "resolved_numbered":    sum(1 for target in targets if target.resolved and not target.battle),
      "unresolved":           sum(1 for target in targets if not target.resolved),
      "unresolved_by_reason": dict(Counter(ReasonOf(target.failure) for target in targets if not target.resolved).most_common()),
    }


REASONS = (
  ("parameter pack",                 "a word is a parameter pack expansion"),
  ("service id is a parameter",      "service id is a parameter"),
  ("is not a constant",              "service id is not a constant"),
  ("bound to dispatchers that reach", "callback bound to dispatchers that reach different code"),
  ("callback",                       "callback not bound to a dispatcher"),
  ("multi-statement",                "multi-statement case or handler"),
  ("raw-word",                       "raw-word case or handler"),
  ("svc-forwarding",                 "forwarding case the tool cannot follow"),
  ("calls the link table by itself", "link-calling case"),
  ("link-calling",                   "link-calling case"),
  ("no handler linked",              "no link handler at the address"),
  ("not handed on",                  "id not served"),
  ("does not serve",                 "id not served"),
  ("has no case",                    "id not served"),
  ("guard",                          "guard the tool cannot follow"),
)


def ReasonOf(failure: str) -> str:
  """The kind of an unresolved site's failure, for the summary's counts."""
  return next((reason for mark, reason in REASONS if mark in failure), failure.split(":")[0])


def _Shape(dispatcher: Dispatcher) -> str:
  if dispatcher.cases or dispatcher.default:
    return "switch"
  if dispatcher.forward is not None:
    return "forward to " + ("port_service" if dispatcher.forward.target == "port_service" else "svc")
  return "irregular"
