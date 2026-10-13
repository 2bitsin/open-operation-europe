"""Which code a `Call(` site runs: its callback's dispatcher, followed through forwarders and the link table to the
single-statement case or handler whose routine call replaces the site."""
from __future__ import annotations

from dataclasses import dataclass, field

from .calls import Bindings, CallSite
from .dispatch import LINKING, SEGMENT_NAMES, SINGLE, Case, Dispatcher, Forward
from .links import LinkTable
from .program import Program
from .syntax import Compact, Span, SplitTopLevel
from .tokens import IDENTIFIER

MAX_HOPS = 8


@dataclass
class Target:
  """The case (or link handler) a site reaches, the words it receives there and the way it got there."""
  case:       Case | None
  owner:      Dispatcher | None  # the dispatcher (or the handler's pseudo-dispatcher) holding case
  words:      list[int | None]   # for each word the case reads: the site's word index, None for a literal 0
  count:      int                # how many words the case receives
  chain:      list[str] = field(default_factory=list)
  battle:     bool = False       # reached through the link table
  failure:    str = ""
  callbacks:  list[str] = field(default_factory=list)  # the svc_* the site's callback may be (all reach this target)
  unbound:    bool = False       # some caller binds the callback where the tool cannot see (a pointer, a member)
  service:    int | None = None  # the id the case was reached with, when a forward changed the site's id
  condition:  Span | None = None  # a site whose id is `condition ? A : B`: the condition, and the two targets
  branches:   list["Target"] = field(default_factory=list)

  def Same(self, other: "Target") -> bool:
    return self.case is other.case and self.words == other.words and self.count == other.count

  @property
  def resolved(self) -> bool:
    if self.branches:
      return not self.failure and all(branch.resolved for branch in self.branches)
    return self.case is not None and (self.case.kind == SINGLE or self.case.storage) and not self.failure


@dataclass
class Resolver:
  program:     Program
  dispatchers: dict[str, Dispatcher]
  links:       LinkTable
  bindings:    Bindings
  _storage:    bool = field(default=False, init=False)  # the site being resolved lends its own words array

  def Resolve(self, site: CallSite) -> Target:
    self._storage = bool(site.array)
    source = self.program.Source(site.file)
    event = Compact(source.tokens, site.event)
    callbacks = self._Callbacks(site, event)
    words: list[int | None] = list(range(len(site.words)))
    bound = sorted(name for name in callbacks if name.startswith("svc_"))
    if not bound:
      return Target(None, None, words, len(words), [event], failure=f"callback {event} bound to {sorted(callbacks)}")
    if any(Compact(source.tokens, word).endswith("...") for word in site.words):
      return Target(None, None, words, len(words), bound, failure="a word is a parameter pack expansion (the count is not fixed)")
    service = Compact(source.tokens, site.service)
    if site.function.ParameterIndex(service) is not None:
      return Target(None, None, words, len(words), bound, failure=f"service id is a parameter ({service} of {site.function.name})")
    if len(site.service) == 1 and source.tokens[site.service.first].text.islower():
      return Target(None, None, words, len(words), bound, failure=f"service id is a parameter ({service}, a local {site.function.name} computes from its parameters)")
    ternary = TernaryParts(source.tokens, site.service)
    if ternary is not None:
      return self._Ternary(site, ternary, bound, words)
    value = self.program.ConstantsOf(site.file).Evaluate(service)
    if value is None:
      return Target(None, None, words, len(words), bound, failure=f"service {service} is not a constant")
    target = self._Bound(bound, value, words)
    target.callbacks, target.unbound = bound, len(bound) != len(callbacks)
    return target

  def _Bound(self, bound: list[str], value: int, words: list[int | None]) -> Target:
    """The target of an id through every dispatcher the callback may be (a failure when they reach different code)."""
    targets = [self._FromDispatcher(name, value, words, []) for name in bound]
    target = targets[0]
    if not all((target.Same(other) or self._Equivalent(target, other)) and not other.failure for other in targets[1:]):
      target.failure = target.failure or "callback bound to dispatchers that reach different code: " + "; ".join(
        " > ".join(other.chain) for other in targets)
    return target

  def _Equivalent(self, one: Target, other: Target) -> bool:
    """Cases of different dispatchers that are the same text over their own names for the game, the id, the words and
    their count, every routine they call being the same one seen from both files (none of the dispatchers' locals)."""
    if one.case is None or other.case is None or one.case.expression is None or other.case.expression is None:
      return False
    if (one.words, one.count, one.case.form, one.case.locals, other.case.locals) != (other.words, other.count, other.case.form, {}, {}):
      return False
    return self._Shape(one) is not None and self._Shape(one) == self._Shape(other)

  def _Shape(self, target: Target) -> tuple | None:
    owner, case = target.owner, target.case
    source = self.program.Source(owner.function.file)
    tokens = source.tokens
    context = owner.context
    roles = {name: role for name, role in ((owner.id_name, "<id>"), (context.args if context else "", "<words>"), (context.count if context else "", "<count>")) if name}
    roles.update(dict.fromkeys(SEGMENT_NAMES, "<game>"))
    shape = []
    for index in range(case.expression.first, case.expression.last):
      token = tokens[index]
      if token.kind == IDENTIFIER and token.text in owner.declared:
        return None
      if token.kind == IDENTIFIER and index + 1 < len(tokens) and tokens[index + 1].text == "(" and tokens[index - 1].text not in (".", "->", "::"):
        callee = self.program.Resolve(token.text, owner.function.file)
        shape.append((str(callee.file), callee.name) if callee is not None else token.text)
      else:
        shape.append(roles.get(token.text, token.text) if token.kind == IDENTIFIER else token.text)
    return tuple(shape)

  def _Ternary(self, site: CallSite, parts: tuple[Span, Span, Span], bound: list[str], words: list[int | None]) -> Target:
    """A site whose id is `condition ? A : B` with A and B constants: both targets, resolved only when both are."""
    source = self.program.Source(site.file)
    constants = self.program.ConstantsOf(site.file)
    condition, *alternatives = parts
    values = [constants.Evaluate(Compact(source.tokens, part)) for part in alternatives]
    if None in values:
      return Target(None, None, words, len(words), bound, failure=f"service {Compact(source.tokens, site.service)} is not a constant")
    branches = [self._Bound(bound, value, words) for value in values]
    first = branches[0]
    failure = "; ".join(f"{'AB'[i]}: {branch.failure}" for i, branch in enumerate(branches) if branch.failure)
    chain = [" | ".join(" > ".join(branch.chain) for branch in branches)]
    return Target(first.case, first.owner, words, len(words), chain, battle=first.battle, failure=failure, callbacks=bound,
                  condition=condition, branches=branches)

  def _Callbacks(self, site: CallSite, event: str) -> set[str]:
    if event.startswith("svc_"):
      return {event}
    if site.function.ParameterIndex(event) is not None:
      return self.bindings.Of(site.function, event)
    return {f"<{event}>"}

  def _FromDispatcher(self, name: str, value: int, words: list[int | None], chain: list[str]) -> Target:
    chain = chain + [name]
    dispatcher = self.dispatchers.get(name)
    if dispatcher is None or len(chain) > MAX_HOPS:
      return Target(None, None, words, len(words), chain, failure=f"no dispatcher {name}" if dispatcher is None else "forwarding loop")
    guarded = self._Guard(dispatcher, value)
    if isinstance(guarded, str):
      return Target(None, dispatcher, words, len(words), chain, failure=guarded)
    if isinstance(guarded, Case):
      target = self._Reached(dispatcher, guarded, words, chain)
      target.battle = dispatcher.battle  # a battle dispatcher's own branch for a routine address: B2's
      return target
    if guarded is not None:
      return self._Forward(dispatcher, guarded, value, words, chain)
    if dispatcher.forward is not None:
      return self._Forward(dispatcher, dispatcher.forward, value, words, chain)
    if not dispatcher.cases and dispatcher.default is None:
      return Target(None, dispatcher, words, len(words), chain, failure=f"{name}: {dispatcher.irregular or 'no switch'}")
    case = dispatcher.CaseFor(value) or dispatcher.default
    if case is None:
      return Target(None, dispatcher, words, len(words), chain, failure=f"{name} has no case 0x{value:x} and no default")
    if case is dispatcher.default and dispatcher.out:
      return Target(None, dispatcher, words, len(words), chain, failure=f"{name} does not serve 0x{value:x} (its default reports it unserved)")
    if case.forward is not None:
      return self._Forward(dispatcher, case.forward, value, words, chain)
    return self._Reached(dispatcher, case, words, chain)

  def _Reached(self, dispatcher: Dispatcher, case: Case, words: list[int | None], chain: list[str]) -> Target:
    """The target of a case (or a guard's branch) that does the work itself."""
    if case.kind == LINKING and case.forward is None:
      return Target(case, dispatcher, words, len(words), chain, failure=f"case {case.labels[0]} calls the link table by itself")
    single = case.kind == SINGLE or (case.storage and self._storage)
    failure = "" if single else f"case {case.labels[0]} of {dispatcher.name} is {case.kind} ({case.reason})"
    return Target(case, dispatcher, words, len(words), chain + [case.labels[0]], failure=failure)

  def _Guard(self, dispatcher: Dispatcher, value: int) -> Forward | Case | str | None:
    """What the first guard the id passes does (a forward or a case of its own), a failure when it cannot be told."""
    source = self.program.Source(dispatcher.function.file)
    for guard in dispatcher.guards:
      text = Compact(source.tokens, guard.condition)
      passed = dispatcher.Evaluate(source, text, value)
      if passed is None:
        return f"{dispatcher.name}: guard on line {guard.line} cannot be evaluated ({text})"
      if passed:
        return guard.forward or guard.case or f"{dispatcher.name}: guard on line {guard.line} takes 0x{value:x} itself"
    return None

  def _Forward(self, dispatcher: Dispatcher, forward: Forward, value: int, words: list[int | None], chain: list[str]) -> Target:
    if forward.reorder is not None:
      words = [words[i] if i < len(words) else None for i in forward.reorder]
    source = self.program.Source(dispatcher.function.file)
    for predicate in forward.unless:
      held = dispatcher.Evaluate(source, predicate, value)
      if held is None or held:
        reason = "cannot be evaluated" if held is None else "holds"
        return Target(None, dispatcher, words, len(words), chain, failure=f"{dispatcher.name}: 0x{value:x} is not handed on ({predicate} {reason})")
    remapped = value
    if forward.id_expression is not None:
      remapped = dispatcher.Evaluate(source, forward.id_expression, value)
      if remapped is None:
        return Target(None, dispatcher, words, len(words), chain, failure=f"{dispatcher.name}: id {forward.id_expression} for 0x{value:x} is not a constant")
    if forward.target != "port_service":
      target = self._FromDispatcher(forward.target, remapped, words, chain)
      target.service = target.service if target.service is not None or remapped == value else remapped
      return target
    value = remapped
    handler = self.links.HandlerAt(value)
    chain = chain + [self.links.KeyName(value)]
    if handler is None:
      return Target(None, dispatcher, words, len(words), chain, battle=True, failure=f"no handler linked at 0x{value:05x}")
    case = handler.case
    failure = "" if case.kind == SINGLE else f"handler {handler.name} is {case.kind} ({case.reason})"
    return Target(case, handler.dispatcher, words, len(words), chain + [handler.name], battle=True, failure=failure)


def TernaryParts(tokens, span: Span) -> tuple[Span, Span, Span] | None:
  """(condition, A, B) of an expression `condition ? A : B` at the top level of span, None when it is not one."""
  question = SplitTopLevel(tokens, span, "?")
  if len(question) != 2:
    return None
  alternatives = SplitTopLevel(tokens, question[1], ":")
  return (question[0], alternatives[0], alternatives[1]) if len(alternatives) == 2 else None
