"""The `svc_*` dispatchers: a switch over a service id whose cases call the routine behind each id, or a forwarder to
another dispatcher or to the link table (`port_service`)."""
from __future__ import annotations

import re
from dataclasses import dataclass, field

from .constants import Constants
from .functions import Function
from .program import Program
from .source import SourceFile
from .syntax import Arguments, Compact, Span, SplitTopLevel, Statements
from .tokens import IDENTIFIER
from .words import FindAccessors, WordContext, WordUse

LINK_CALLS = {"port_call", "port_service", "port_call_named", "btl_reorder"}
RESULT_TYPES = {"uint16_t", "uint32_t", "int", "unsigned"}
STACK_CALLS = {"WithStack", "WithBattleStack", "port_stack_enter", "port_frame_enter", "port_scratch_enter"}

# Case kinds; only SINGLE is substituted by B2/B3, B0.3 rewrites the others into SINGLE.
SINGLE = "single"
MULTI = "multi-statement"
FORWARDING = "svc-forwarding"
RAW = "raw-word"
LINKING = "link-calling"
EMPTY = "empty"

# How a single-statement case yields the service's result.
RETURNS = "return"     # return EXPR;
ASSIGNS = "assign"     # result = EXPR; break;
STATEMENT = "none"     # EXPR; break;  (the service returns the result variable's initial value, 0)


@dataclass
class Case:
  labels:     list[str]
  values:     list[int | None]
  body:       Span
  statements: list[Span]
  kind:       str = EMPTY
  form:       str = STATEMENT
  expression: Span | None = None
  words:      WordUse = field(default_factory=WordUse)
  stack:      bool = False
  line:       int = 0
  reason:     str = ""
  forward:    "Forward | None" = None  # a case handing the words on to another dispatcher or to the link table
  locals:     dict[str, Span] = field(default_factory=dict)  # a local the statement uses once: NAME -> its expression

  def Summary(self, source: SourceFile) -> dict:
    return {"labels": self.labels, "values": self.values, "kind": self.kind, "form": self.form, "line": self.line,
            "statements": len(self.statements), "stack": self.stack, "reads": sorted(set(self.words.Indices())),
            "reason": self.reason, "reorder": list(self.forward.reorder) if self.forward and self.forward.reorder else None,
            "expression": Compact(source.tokens, self.expression) if self.expression is not None else None}


@dataclass
class Forward:
  """A dispatcher (or one of its cases) handing the words on: to another svc_* or to the link table. The id handed on
  is the one received, or `id_expression` evaluated on it (a constant, or a constexpr table indexed by the id); an id
  for which one of the `unless` predicates holds is not handed on (the dispatcher reports it unhandled)."""
  target:        str                           # a svc_* name or "port_service"
  reorder:       tuple[int, ...] | None = None  # btl_reorder's word order
  id_expression: str | None = None             # the id handed on, as text over the received id
  unless:        tuple[str, ...] = ()          # predicates on the received id that keep it from being handed on


@dataclass
class Guard:
  """An `if (CONDITION) ...` on the id before the switch or the forward: a forward when the branch only forwards, a
  case when the condition is `id == K` and the branch does the work itself."""
  condition: Span
  forward:   Forward | None
  line:      int
  case:      Case | None = None


@dataclass
class Dispatcher:
  function:  Function
  context:   WordContext | None
  id_name:   str
  battle:    bool  # keyed by a linear routine address (BattleRoutine), not a module-local id
  cases:     list[Case] = field(default_factory=list)
  default:   Case | None = None
  forward:   Forward | None = None
  result:    str = ""  # the result variable the cases assign
  out:       str = ""  # the parameter the result is stored through when the dispatcher returns whether it served the id
  irregular: str = ""  # why the body is neither a switch nor a forwarder
  guards:    list[Guard] = field(default_factory=list)
  locals:    dict[str, Span] = field(default_factory=dict)  # constant locals the guards may compare with
  constants: Constants = field(default_factory=Constants)  # the enumerators and constants its file sees
  declared:  set[str] = field(default_factory=set)  # the locals it declares before the switch

  @property
  def name(self) -> str:
    return self.function.name

  def Inlined(self, source: SourceFile) -> dict[str, Span]:
    """The `auto const NAME{ EXPR };` locals whose EXPR only names constants and pure calls: a case using NAME is
    substituted with EXPR in its place."""
    return {name: span for name, span in self.locals.items() if not _Impure(source, span) and self.context is not None
            and not any(t.text in (self.context.args, self.context.count) or t.text in self.context.accessors for t in source.tokens[span.first:span.last])}

  def WordLocals(self, source: SourceFile) -> dict[str, Span]:
    """The `auto const NAME{ EXPR };` locals whose EXPR only reads words and converts them (`x{ static_cast<uint8_t>(
    Arg(args, count, 0)) }`): a case naming NAME reads the word where it names it."""
    if self.context is None:
      return {}
    return {name: span for name, span in self.locals.items() if not _Impure(source, span, READS_AND_CASTS)
            and any(t.text in ("Arg", "Arg32") for t in source.tokens[span.first:span.last]) and "[" not in {t.text for t in source.tokens[span.first:span.last]}}

  def CaseFor(self, value: int) -> Case | None:
    return next((case for case in self.cases if value in case.values), None)

  def Evaluate(self, source: SourceFile, text: str, value: int) -> int | None:
    """An expression over the id (Compact text) evaluated for one id value, the dispatcher's constant locals in place."""
    for name, span in self.locals.items():
      text = re.sub(rf"\b{name}\b", f"({Compact(source.tokens, span)})", text)
    return self.constants.Evaluate(re.sub(rf"(?<![\w:]){self.id_name}\b", str(value), text)) if self.id_name else None


def FindDispatchers(program: Program) -> dict[str, Dispatcher]:
  found: dict[str, Dispatcher] = {}
  for name, definitions in sorted(program.definitions.items()):
    if name.startswith("svc_"):
      for function in definitions:
        found.setdefault(name, _Dispatcher(program, function))
  return found


def _Dispatcher(program: Program, function: Function) -> Dispatcher:
  source = program.Source(function.file)
  parameters = function.parameters
  id_at = next((i for i, parameter in enumerate(parameters) if _IsId(parameter.type)), None)
  id_name = parameters[id_at].name if id_at is not None else ""
  pointer = next((p for p in parameters if p.type.replace("const", "").replace(" ", "").endswith("uint16_t*")), None)
  count = parameters[parameters.index(pointer) + 1].name if pointer is not None and parameters.index(pointer) + 1 < len(parameters) else ""
  context = WordContext(pointer.name, count) if pointer is not None and pointer.name else None
  battle = id_at is not None and "uint32_t" in parameters[id_at].type
  dispatcher = Dispatcher(function, context, id_name, battle, out=_OutName(parameters))
  body = Span(function.body.first + 1, function.body.last - 1)
  statements = Statements(source.tokens, source.partners, body)
  if context is not None:
    context.accessors = FindAccessors(source, statements, context)
  constants = dispatcher.constants = program.ConstantsOf(function.file)
  dispatcher.declared = {name for s in statements if source.tokens[s.first].text != "switch" for name in DeclaredNames(source, s)}
  dispatcher.result = _ResultName(source, statements, dispatcher.out)
  statements = _ReadGuards(source, statements, dispatcher)
  switch = next((s for s in statements if source.tokens[s.first].text == "switch"), None)
  if switch is None:
    _ReadForwarder(source, statements, dispatcher)
  else:
    if "BattleRoutine" in Compact(source.tokens, Span(switch.first, source.partners[switch.first + 1] + 1)):
      dispatcher.battle = True
    _ReadSwitch(program, source, switch, dispatcher)
  return dispatcher


def _ReadGuards(source: SourceFile, statements: list[Span], dispatcher: Dispatcher) -> list[Span]:
  """Takes the `if` statements on the id (and the constant locals they compare with) out of statements; an `if` that
  forwards in its condition (`if (P || !svc_x(...)) port_unhandled(...);`) is the dispatcher's forward."""
  tokens, partners = source.tokens, source.partners
  rest = []
  for statement in statements:
    first = tokens[statement.first]
    if first.text == "if" and dispatcher.id_name in {t.text for t in tokens[statement.first:partners[statement.first + 1]]}:
      forward = ForwardOf(source, dispatcher, statement)
      if forward is not None:
        dispatcher.forward = forward
      else:
        dispatcher.guards.append(_Guard(source, dispatcher, statement))
    elif first.text == "auto" and tokens[statement.first + 1].text == "const" and tokens[statement.first + 3].text == "{":
      dispatcher.locals[tokens[statement.first + 2].text] = Span(statement.first + 4, partners[statement.first + 3])
    elif first.text == "(" and tokens[statement.first + 1].text == "void":
      continue
    else:
      rest.append(statement)
  return rest


def _Guard(source: SourceFile, dispatcher: Dispatcher, statement: Span) -> Guard:
  """`if (CONDITION) BRANCH`: a forward when BRANCH only forwards, a case when CONDITION is `id == K`."""
  tokens, partners = source.tokens, source.partners
  close = partners[statement.first + 1]
  condition = Span(statement.first + 2, close)
  branch = Span(close + 1, statement.last)
  if tokens[branch.first].text == "{" and partners.get(branch.first) == branch.last - 1:
    branch = Span(branch.first + 1, branch.last - 1)
  inner = [s for s in Statements(tokens, partners, branch) if not IsFiller(source, s)]
  forward = ForwardOf(source, dispatcher, inner[0]) if len(inner) == 1 else None
  guard = Guard(condition, forward, tokens[statement.first].line)
  label = _EqualityLabel(source, dispatcher, condition)
  if forward is None and label:
    guard.case = Case([label], [dispatcher.constants.Evaluate(label)], branch, inner, line=guard.line)
    Classify(source, dispatcher, guard.case)
  return guard


def _EqualityLabel(source: SourceFile, dispatcher: Dispatcher, condition: Span) -> str:
  """K of a condition `id == K` (or `K == id`) where K is a constant, else empty."""
  sides = Compact(source.tokens, condition).split("==")
  if len(sides) != 2 or dispatcher.id_name not in sides:
    return ""
  label = sides[1] if sides[0] == dispatcher.id_name else sides[0]
  return label if dispatcher.constants.Evaluate(label) is not None else ""


PURE_LOCALS = {"static_cast", "port_dialogs_host", "port_textout_host", "port_uiprim_host", "port_numdlg_host", "std", "to_underlying"}
READS_AND_CASTS = {"static_cast", "Arg", "Arg32", "std", "to_underlying"}


def _Impure(source: SourceFile, span: Span, pure: set[str] = PURE_LOCALS) -> bool:
  tokens = source.tokens
  return any(tokens[i].kind == IDENTIFIER and tokens[i + 1].text == "(" and tokens[i].text not in pure for i in range(span.first, span.last - 1))


def DeclaredNames(source: SourceFile, statement: Span) -> set[str]:
  """The names a declaration statement declares (`auto const* dialogs{ ... };`, `uint16_t sp, r = 0;`)."""
  tokens, partners = source.tokens, source.partners
  if tokens[statement.first].text in ("return", "if", "switch", "for", "while", "do", "(") or tokens[statement.first].kind != IDENTIFIER:
    return set()
  names, index = set(), statement.first + 1
  while index < statement.last:
    token = tokens[index]
    following = tokens[index + 1].text if index + 1 < statement.last else ""
    if token.kind == IDENTIFIER and following in ("{", "=", ";", ",", "[") and tokens[index - 1].text not in (".", "->", "::", "(", "=", ","):
      names.add(token.text)
    elif token.kind == IDENTIFIER and following in ("{", "=", ";", ",", "[") and tokens[index - 1].text == ",":
      names.add(token.text)
    if token.text in ("(", "[", "{") and index in partners:
      index = partners[index] + 1
      continue
    index += 1
  return names


def _IsId(type_text: str) -> bool:
  return type_text.replace("std::", "") in ("uint16_t", "uint32_t")


def _OutName(parameters: list) -> str:
  """The `uint16_t* result` parameter (non-const) through which a dispatcher that returns a flag stores its result."""
  return next((p.name for p in parameters if p.type.replace("std::", "").replace(" ", "") == "uint16_t*"), "")


def _ResultName(source: SourceFile, statements: list[Span], out: str) -> str:
  """The local the cases assign their result to: declared before the switch (`uint16_t sp, r = 0;`) and returned
  after it (`return r;`) or stored through the out parameter (`*result = r;`)."""
  tokens = source.tokens
  declared: set[str] = set()
  for statement in statements:
    texts = [token.text for token in tokens[statement.first:statement.last]]
    start = statement.first + (2 if texts[:2] == ["std", "::"] else 0)
    if tokens[start].text in RESULT_TYPES:
      declared |= {tokens[part.first].text for part in SplitTopLevel(tokens, Span(start + 1, statement.last - 1), ",") if len(part)}
  returned = [tokens[s.first + 1].text for s in statements if tokens[s.first].text == "return" and len(s) == 3]
  stored = [tokens[s.first + 3].text for s in statements if out and len(s) == 5 and [t.text for t in tokens[s.first:s.first + 3]] == ["*", out, "="]]
  return next((name for name in returned + stored if name in declared), "")


def _ReadForwarder(source: SourceFile, statements: list[Span], dispatcher: Dispatcher) -> None:
  real = [s for s in statements if source.tokens[s.first].text != "static" and not _ResultBookkeeping(source, dispatcher, s)]
  if dispatcher.forward is not None and not real:
    return
  forward = ForwardOf(source, dispatcher, real[0]) if len(real) == 1 and dispatcher.forward is None else None
  if forward is None:
    dispatcher.irregular = f"{len(real)} statements, no switch" + (f" after {len(dispatcher.guards)} guards" if dispatcher.guards else "")
  dispatcher.forward = forward


def _ResultBookkeeping(source: SourceFile, dispatcher: Dispatcher, statement: Span) -> bool:
  """The result variable's declaration, `return result;` or `*out = result;`."""
  if not dispatcher.result:
    return False
  texts = [token.text for token in source.tokens[statement.first:statement.last]]
  return texts in (["return", dispatcher.result, ";"], ["*", dispatcher.out, "=", dispatcher.result, ";"]) or (
    dispatcher.result in DeclaredNames(source, statement) and texts[0] in RESULT_TYPES | {"std"})


def ForwardOf(source: SourceFile, dispatcher: Dispatcher, statement: Span) -> Forward | None:
  """A statement that hands the words on as a Forward, the id handed on being the dispatcher's own, a constant or a
  table entry indexed by it:
    [return | result =] svc_x(ds, [module,] ID, args, count [, &result]);
    [return] port_service(ds, ID, args, count);
    if (P || ... || !svc_x(ds, ID, args, count, &result)) port_unhandled(...);"""
  tokens = source.tokens
  if dispatcher.context is None:
    return None
  if tokens[statement.first].text == "if":
    return _PredicateForward(source, dispatcher, statement)
  first = statement.first + _LeadLength(source, dispatcher, statement)
  end = statement.last - (1 if tokens[statement.last - 1].text == ";" else 0)
  if source.partners.get(first + 1, -1) + 1 != end:
    return None
  return _CallForward(source, dispatcher, first)


def _LeadLength(source: SourceFile, dispatcher: Dispatcher, statement: Span) -> int:
  """How many tokens before the call: 1 for `return`, 2 for `result =`, else 0."""
  texts = [token.text for token in source.tokens[statement.first:statement.first + 2]]
  if texts[:1] == ["return"]:
    return 1
  return 2 if dispatcher.result and texts == [dispatcher.result, "="] else 0


def _CallForward(source: SourceFile, dispatcher: Dispatcher, first: int) -> Forward | None:
  """The call at first as a Forward when it is one of a dispatcher or of the link table taking the words unchanged."""
  tokens = source.tokens
  name = tokens[first].text
  if tokens[first].kind != IDENTIFIER or tokens[first + 1].text != "(" or not (name.startswith("svc_") or name in ("port_service", "port_call")):
    return None
  arguments = [Compact(tokens, part) for part in Arguments(tokens, source.partners, first + 1)]
  if arguments and arguments[-1].startswith("&") and dispatcher.result and arguments[-1] == "&" + dispatcher.result:
    arguments = arguments[:-1]
  if len(arguments) < 4 or arguments[-2:] != [dispatcher.context.args, dispatcher.context.count]:
    return None
  target = "port_service" if name == "port_call" else name
  if arguments[-3] == dispatcher.id_name:
    return Forward(target)
  if dispatcher.Evaluate(source, arguments[-3], 0) is None and dispatcher.constants.Evaluate(arguments[-3]) is None:
    return None
  return Forward(target, id_expression=arguments[-3])


def _PredicateForward(source: SourceFile, dispatcher: Dispatcher, statement: Span) -> Forward | None:
  """`if (P || ... || !svc_x(ds, ID, args, count, &result)) port_unhandled(...);`: the call forwards unless a P holds."""
  tokens, partners = source.tokens, source.partners
  close = partners[statement.first + 1]
  body = [token.text for token in tokens[close + 1:close + 3]]
  if body != ["port_unhandled", "("]:
    return None
  terms = SplitTopLevel(tokens, Span(statement.first + 2, close), "||")
  last = terms[-1]
  if tokens[last.first].text != "!" or partners.get(last.first + 2, -1) != last.last - 1:
    return None
  forward = _CallForward(source, dispatcher, last.first + 1)
  if forward is None:
    return None
  forward.unless = tuple(Compact(tokens, term) for term in terms[:-1])
  return forward


def _ReadSwitch(program: Program, source: SourceFile, switch: Span, dispatcher: Dispatcher) -> None:
  tokens, partners = source.tokens, source.partners
  constants = program.ConstantsOf(dispatcher.function.file)
  brace = partners[switch.first + 1] + 1
  if tokens[brace].text != "{":
    dispatcher.irregular = "switch without a block"
    return
  for case in _Cases(source, Span(brace + 1, partners[brace]), constants):
    Classify(source, dispatcher, case)
    if case.labels == ["default"]:
      dispatcher.default = case
    else:
      dispatcher.cases.append(case)


def _Cases(source: SourceFile, span: Span, constants: Constants) -> list[Case]:
  """The switch's case groups: labels that share a body, and the body up to the next label."""
  tokens, partners = source.tokens, source.partners
  cases: list[Case] = []
  labels: list[str] = []
  body_start: int | None = None
  index = span.first
  while index < span.last:
    text = tokens[index].text
    if text in ("case", "default"):
      if body_start is not None and body_start < index:
        cases.append(_Case(source, labels, Span(body_start, index), constants))
        labels = []
      colon = _LabelColon(tokens, index, span.last)
      labels.append("default" if text == "default" else Compact(tokens, Span(index + 1, colon)))
      index = body_start = colon + 1
      continue
    index = partners[index] + 1 if text in ("(", "[", "{") and index in partners else index + 1
  if labels:
    cases.append(_Case(source, labels, Span(body_start, span.last), constants))
  return cases


def _LabelColon(tokens, index: int, end: int) -> int:
  """The colon ending the label at index (a `?:` inside the label's expression is skipped)."""
  depth, ternaries = 0, 0
  for at in range(index + 1, end):
    text = tokens[at].text
    if text in ("(", "["):
      depth += 1
    elif text in (")", "]"):
      depth -= 1
    elif text == "?":
      ternaries += 1
    elif text == ":" and depth == 0 and ternaries:
      ternaries -= 1
    elif text == ":" and depth == 0:
      return at
  return index + 1


def _Case(source: SourceFile, labels: list[str], body: Span, constants: Constants) -> Case:
  values = [None if label == "default" else constants.Evaluate(label) for label in labels]
  statements = Statements(source.tokens, source.partners, body)
  if len(statements) == 1 and source.tokens[statements[0].first].text == "{":
    block = statements[0]
    statements = Statements(source.tokens, source.partners, Span(block.first + 1, block.last - 1))
  statements = [s for s in statements if not IsFiller(source, s)]
  return Case(labels, values, body, statements, line=source.tokens[body.first - 1].line)


def IsFiller(source: SourceFile, statement: Span) -> bool:
  """`break;`, an empty statement or a `(void)name;` that only silences an unused parameter."""
  texts = [token.text for token in source.tokens[statement.first:statement.last]]
  return texts in (["break", ";"], [";"]) or (len(texts) == 5 and texts[:3] == ["(", "void", ")"] and texts[4] == ";")


def Classify(source: SourceFile, dispatcher: Dispatcher, case: Case) -> None:
  tokens = source.tokens
  if not case.statements:
    case.kind = EMPTY
    return
  names = {tokens[i].text for s in case.statements for i in range(s.first, s.last) if tokens[i].kind == IDENTIFIER}
  called = {tokens[i].text for s in case.statements for i in range(s.first, s.last - 1) if tokens[i].kind == IDENTIFIER and tokens[i + 1].text == "("}
  case.stack = bool(names & STACK_CALLS)
  if dispatcher.context is not None:
    case.locals = {name: span for name, span in dispatcher.WordLocals(source).items() if name in names}
    case.words = _Words(source, dispatcher.context, case.statements + list(case.locals.values()))
  statements = _DropTrailingResultReturn(source, dispatcher, case.statements)
  if called & LINK_CALLS:
    case.kind, case.reason = LINKING, ", ".join(sorted(called & LINK_CALLS))
    case.forward = _LinkForward(source, dispatcher, statements) if len(statements) == 1 else None
  elif any(name.startswith("svc_") for name in called):
    case.forward = ForwardOf(source, dispatcher, statements[0]) if len(statements) == 1 else None
    case.kind, case.reason = FORWARDING, ", ".join(sorted(name for name in called if name.startswith("svc_")))
  elif case.words.raw:
    case.kind, case.reason = RAW, f"{len(case.words.raw)} raw uses"
  elif len(statements) == 2 and _LocalUsedOnce(source, statements, case):
    _Single(source, dispatcher, case, statements[1])
  elif len(statements) != 1:
    case.kind, case.reason = MULTI, f"{len(statements)} statements"
  else:
    _Single(source, dispatcher, case, statements[0])


def _LocalUsedOnce(source: SourceFile, statements: list[Span], case: Case) -> bool:
  """`auto [const] NAME{ EXPR };` then a statement naming NAME once, EXPR evaluated where NAME stood without moving a
  side effect: EXPR only reads words and converts, or the statement is `return NAME;`. Records NAME in case.locals."""
  tokens, partners = source.tokens, source.partners
  declaration, use = statements
  at = declaration.first + (2 if [t.text for t in tokens[declaration.first:declaration.first + 2]] == ["auto", "const"] else 1)
  if tokens[declaration.first].text != "auto" or tokens[at].kind != IDENTIFIER or tokens[at + 1].text != "{":
    return False
  if partners.get(at + 1) != declaration.last - 2 or tokens[declaration.last - 1].text != ";":
    return False
  name = tokens[at].text
  if sum(1 for t in tokens[use.first:use.last] if t.text == name) != 1:
    return False
  expression = Span(at + 2, declaration.last - 2)
  if any(t.text == name for t in tokens[expression.first:expression.last]):
    return False
  if _Impure(source, expression, READS_AND_CASTS) and [t.text for t in tokens[use.first:use.last]] != ["return", name, ";"]:
    return False
  case.locals[name] = expression
  return True


def _Words(source: SourceFile, context: WordContext, statements: list[Span]) -> WordUse:
  use = WordUse()
  for statement in statements:
    part = context.Scan(source, statement)
    use.reads += part.reads
    use.raw += part.raw
  return use


def _DropTrailingResultReturn(source: SourceFile, dispatcher: Dispatcher, statements: list[Span]) -> list[Span]:
  """A `return result;` or `return 0;` closing a case adds nothing to the call."""
  if len(statements) > 1:
    last = [token.text for token in source.tokens[statements[-1].first:statements[-1].last]]
    if last in (["return", dispatcher.result, ";"], ["return", "0", ";"]):
      return statements[:-1]
  return statements


def _Single(source: SourceFile, dispatcher: Dispatcher, case: Case, statement: Span) -> None:
  tokens = source.tokens
  texts = [token.text for token in tokens[statement.first:statement.first + 2]]
  end = statement.last - 1 if tokens[statement.last - 1].text == ";" else statement.last
  if texts[0] == "return":
    case.form, case.expression = RETURNS, Span(statement.first + 1, end)
  elif dispatcher.result and texts == [dispatcher.result, "="]:
    case.form, case.expression = ASSIGNS, Span(statement.first + 2, end)
  else:
    case.form, case.expression = STATEMENT, Span(statement.first, end)
  foreign = _DispatcherLocals(source, dispatcher, case.expression, case.locals)
  if foreign:
    case.kind, case.reason = MULTI, "uses " + ", ".join(sorted(foreign))
  elif tokens[statement.first].text in ("if", "for", "while", "switch", "do"):
    case.kind, case.reason = MULTI, "control statement"
  else:
    case.kind = SINGLE


def _DispatcherLocals(source: SourceFile, dispatcher: Dispatcher, span: Span, case_locals: dict[str, Span]) -> set[str]:
  """Names of the dispatcher's own parameters and locals (other than ds, the words and the id) an expression uses."""
  parameters = {p.name for p in dispatcher.function.parameters} - {"ds", dispatcher.id_name}
  if dispatcher.context is not None:
    parameters -= {dispatcher.context.args, dispatcher.context.count}
  tokens = source.tokens
  used = {tokens[i].text for i in range(span.first, span.last) if tokens[i].kind == IDENTIFIER and tokens[i - 1].text not in (".", "->", "::")}
  accessors = set(dispatcher.context.accessors) if dispatcher.context is not None else set()
  locals_ = (parameters | (dispatcher.declared - set(dispatcher.Inlined(source))) | ({dispatcher.result} - {""})) & used
  return locals_ - accessors - set(case_locals)


def _LinkForward(source: SourceFile, dispatcher: Dispatcher, statements: list[Span]) -> Forward | None:
  """`return port_service(ds, id, args, count);` or `return btl_reorder(ds, id, args, count, ORDER, n);` with ORDER a
  `static constexpr unsigned char ORDER[]{ ... };` of the dispatcher, as a Forward to the link table."""
  tokens, partners = source.tokens, source.partners
  statement = statements[0]
  first = statement.first + (1 if tokens[statement.first].text == "return" else 0)
  name = tokens[first].text
  if name not in ("port_service", "port_call", "btl_reorder") or tokens[first + 1].text != "(":
    return None
  arguments = Arguments(tokens, partners, first + 1)
  if name != "btl_reorder":
    return Forward("port_service")
  table = Compact(tokens, arguments[4]) if len(arguments) == 6 else ""
  order = _OrderTable(source, dispatcher, table)
  return Forward("port_service", order) if order is not None else None


def _OrderTable(source: SourceFile, dispatcher: Dispatcher, name: str) -> tuple[int, ...] | None:
  tokens, partners = source.tokens, source.partners
  body = dispatcher.function.body
  for index in range(body.first, body.last - 2):
    if tokens[index].text == name and tokens[index + 1].text == "[" and tokens[index + 2].text == "]":
      brace = index + 3 if tokens[index + 3].text == "{" else index + 4
      if brace in partners:
        return tuple(int(tokens[i].text.rstrip("uU"), 0) for i in range(brace + 1, partners[brace]) if tokens[i].text != ",")
  return None
