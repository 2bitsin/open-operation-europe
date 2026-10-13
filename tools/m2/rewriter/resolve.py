"""Which code a `Call(` site runs: its callback's dispatcher, followed through forwarders and the link table to the
single-statement case or handler whose routine call replaces the site."""
from __future__ import annotations

import re
from dataclasses import dataclass, field

from .calls import Bindings, CallSite
from .dispatch import LINKING, SINGLE, Case, Dispatcher, Forward
from .links import LinkTable
from .program import Program
from .syntax import Compact

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

  def Same(self, other: "Target") -> bool:
    return self.case is other.case and self.words == other.words and self.count == other.count

  @property
  def resolved(self) -> bool:
    return self.case is not None and self.case.kind == SINGLE and not self.failure


@dataclass
class Resolver:
  program:     Program
  dispatchers: dict[str, Dispatcher]
  links:       LinkTable
  bindings:    Bindings

  def Resolve(self, site: CallSite) -> Target:
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
    value = self.program.ConstantsOf(site.file).Evaluate(service)
    if value is None:
      return Target(None, None, words, len(words), bound, failure=f"service {service} is not a constant")
    targets = [self._FromDispatcher(name, value, words, []) for name in bound]
    target = targets[0]
    target.callbacks, target.unbound = bound, len(bound) != len(callbacks)
    if not all(target.Same(other) and not other.failure for other in targets[1:]):
      target.failure = target.failure or "callback bound to dispatchers that reach different code: " + "; ".join(
        " > ".join(other.chain) for other in targets)
    return target

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
    if guarded is not None:
      return self._Forward(dispatcher, guarded, value, words, chain)
    if dispatcher.forward is not None:
      return self._Forward(dispatcher, dispatcher.forward, value, words, chain)
    if not dispatcher.cases and dispatcher.default is None:
      return Target(None, dispatcher, words, len(words), chain, failure=f"{name}: {dispatcher.irregular or 'no switch'}")
    case = dispatcher.CaseFor(value) or dispatcher.default
    if case is None:
      return Target(None, dispatcher, words, len(words), chain, failure=f"{name} has no case 0x{value:x} and no default")
    if case.forward is not None:
      return self._Forward(dispatcher, case.forward, value, words, chain)
    if case.kind == LINKING and case.forward is None:
      return Target(case, dispatcher, words, len(words), chain, failure=f"case {case.labels[0]} calls the link table by itself")
    failure = "" if case.kind == SINGLE else f"case {case.labels[0]} of {name} is {case.kind} ({case.reason})"
    return Target(case, dispatcher, words, len(words), chain + [case.labels[0]], failure=failure)

  def _Guard(self, dispatcher: Dispatcher, value: int) -> Forward | str | None:
    """The forward of the first guard the id passes, a failure when that guard does more or cannot be evaluated."""
    for guard in dispatcher.guards:
      source = self.program.Source(dispatcher.function.file)
      text = Compact(source.tokens, guard.condition)
      for name, span in dispatcher.locals.items():
        text = re.sub(rf"\b{name}\b", f"({Compact(source.tokens, span)})", text)
      passed = dispatcher.constants.Evaluate(re.sub(rf"\b{dispatcher.id_name}\b", str(value), text))
      if passed is None:
        return f"{dispatcher.name}: guard on line {guard.line} cannot be evaluated ({text})"
      if passed:
        return guard.forward or f"{dispatcher.name}: guard on line {guard.line} takes 0x{value:x} itself"
    return None

  def _Forward(self, dispatcher: Dispatcher, forward: Forward, value: int, words: list[int | None], chain: list[str]) -> Target:
    if forward.reorder is not None:
      words = [words[i] if i < len(words) else None for i in forward.reorder]
    if forward.service is not None:
      value = forward.service
    if forward.target != "port_service":
      return self._FromDispatcher(forward.target, value, words, chain)
    handler = self.links.HandlerAt(value)
    chain = chain + [self.links.KeyName(value)]
    if handler is None:
      return Target(None, dispatcher, words, len(words), chain, battle=True, failure=f"no handler linked at 0x{value:05x}")
    case = handler.case
    failure = "" if case.kind == SINGLE else f"handler {handler.name} is {case.kind} ({case.reason})"
    return Target(case, handler.dispatcher, words, len(words), chain + [handler.name], battle=True, failure=failure)
