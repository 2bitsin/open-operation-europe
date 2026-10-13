"""The text that replaces one `Call(` site: its target's routine call with the site's words put where the target read
them (plan section 2.1), and the facts a reviewer must see (words past the end, used results of void cases, words
whose side effects move, names the site's file cannot see)."""
from __future__ import annotations

import re
from dataclasses import dataclass, field, replace

from .calls import CallSite
from .casts import ModernCasts
from .dispatch import SEGMENT_NAMES, STATEMENT
from .functions import Function
from .program import Program
from .resolve import Target, TernaryParts
from .source import SourceFile
from .syntax import Arguments, Compact, Span, Splice, Text
from .tokens import IDENTIFIER
from .words import SPAN, WIDE, WordRead

NARROW_PARAMETERS = {"uint16_t", "int16_t", "uint8_t", "int8_t", "short", "unsigned short", "unsigned char", "signed char", "char"}
UNSIGNED_NARROW = {"uint16_t", "uint8_t", "unsigned short", "unsigned char", "bool"}
RESULT_AS_IS = {"uint16_t", "unsigned short"}
PURE_CALLS = {"static_cast", "reinterpret_cast", "const_cast", "Local", "Near", "Data", "Record", "Word", "Byte", "offsetof",
              "sizeof", "to_underlying", "Arg", "Arg32", "U16", "Bits", "port_far", "PORT_DS", "port_dialogs_host", "port_textout_host", "port_uiprim_host", "port_numdlg_host", "uint16_t", "uint8_t", "int16_t", "Lo", "Hi",
              "At"}  # At: a ServiceLocals reserve's near address
MUTATING = {"++", "--", "=", "+=", "-=", "*=", "/=", "%=", "|=", "&=", "^=", "<<=", ">>="}
TIGHT = {".", "->", "::"}


@dataclass
class Substitution:
  site:        CallSite
  target:      Target
  text:        str                                   # the routine call that replaces the site (hoisted words by name)
  hoisted:     list[tuple[int, str]] = field(default_factory=list)  # (word index, its text) to evaluate first
  past_end:    list[int] = field(default_factory=list)                # positions the target reads past the words
  void_used:   bool = False                          # the site uses a result the target never sets (it is 0)
  result_cast: bool = False                          # the result is converted to 16 bits as AX gave it
  order:       str = ""                              # why the words must be evaluated before the call
  invisible:   list[str] = field(default_factory=list)  # names of the target's file the site's file cannot see
  includes:    list[str] = field(default_factory=list)  # headers the site's file must include for them
  local:       list[str] = field(default_factory=list)  # of those, names only the target's own .cpp defines
  cycle:       list[str] = field(default_factory=list)  # of those, headers that include the site's own header
  near:        list[str] = field(default_factory=list)  # near pointers made of a word (typed references in B5)
  callee:      str = ""


@dataclass
class Substituter:
  """Substitutes sites innermost first, so a site nested in another's words is replaced inside them."""
  program: Program
  _done:   dict[object, list["Substitution"]] = field(default_factory=dict)  # file -> the substitutions made there

  def Substitute(self, site: CallSite, target: Target) -> Substitution:
    substitution = self._Ternary(site, target) if target.branches else self._One(site, target, None)
    if "std::array" in substitution.text:
      substitution.includes.append("<array>")
    for inner in self._Inner(site):
      substitution.includes += [include for include in inner.includes if include not in substitution.includes]
      substitution.local += [name for name in inner.local if name not in substitution.local]
      substitution.cycle += [include for include in inner.cycle if include not in substitution.cycle]
    self._done.setdefault(site.file, []).append(substitution)
    return substitution

  def _One(self, site: CallSite, target: Target, hoisted: list[str] | None) -> Substitution:
    """The substitution of a site reaching one case; hoisted: the words already evaluated first, by name."""
    owner_source = self.program.Source(target.owner.function.file)
    case = target.case
    words = hoisted or [self._WordText(site, span) for span in site.words]
    spans = [case.expression, *case.locals.values()]
    reads = [read for read in case.words.reads if any(span.Contains(read.span.first) for span in spans)]
    substitution = Substitution(site, target, "", callee=CalleeOf(owner_source, case.expression))
    effects = [index for index, span in enumerate(site.words) if _SideEffects(self.program.Source(site.file), span)]
    substitution.order = "" if hoisted else _OrderHazard(owner_source, case.expression, reads, target, effects, words)
    if substitution.order:
      substitution.hoisted, words = _Hoisted(words)
    text = ModernCasts(self._Expression(site, target, case.expression, (words, substitution)))
    substitution.text = self._Result(site, target, text, substitution)
    self._Invisible(site, target, owner_source, substitution)
    return substitution

  def _Expression(self, site: CallSite, target: Target, span: Span, state: tuple[list[str], Substitution]) -> str:
    """The text of a span of the case: the site's words where it reads them, the site's names for ds and the id, and
    each local of the case replaced by its own expression."""
    words, substitution = state
    owner = self.program.Source(target.owner.function.file)
    case = target.case
    reads = [read for read in case.words.reads if span.Contains(read.span.first)]
    replacements = [(read.span, self._ReadText(site, target, read, words, substitution)) for read in reads]
    replacements += self._Renames(site, target, owner, span)
    for name, local in case.locals.items():
      uses = [index for index in range(span.first, span.last) if owner.tokens[index].text == name and owner.tokens[index - 1].text not in TIGHT]
      text = self._Expression(site, target, local, state) if uses else ""
      replacements += [(Span(index, index + 1), f"({text})" if _Operators(text) else text) for index in uses]
    return Splice(owner.text, owner.tokens, span, replacements)

  def _Ternary(self, site: CallSite, target: Target) -> Substitution:
    """A site whose id is `condition ? A : B`: the condition choosing between the two routine calls, as an expression
    when both give the same type (or the value is used: both give AX then), else as an if statement."""
    source = self.program.Source(site.file)
    condition, *alternatives = TernaryParts(source.tokens, site.service)
    words = [self._WordText(site, span) for span in site.words]
    effects = [index for index, span in enumerate(site.words) if _SideEffects(source, span)]
    hoisted, named = _Hoisted(words) if effects else ([], None)
    branches = [self._One(replace(site, service=part), branch, named) for part, branch in zip(alternatives, target.branches)]
    substitution = _Merged(site, target, branches)
    substitution.hoisted = hoisted
    substitution.order = "words with side effects are evaluated once, before the condition" if hoisted else ""
    condition_text = self._WordText(site, condition)
    test = f"({condition_text})" if _Operators(condition_text) else condition_text
    first, second = (branch.text for branch in branches)
    if site.used or self._SameType(branches):
      substitution.text = f"{test} ? {first} : {second}"
    elif _StatementStart(source, site.span):
      substitution.text = f"if ({condition_text}) {first}; else {second}"
    else:
      substitution.text = f"{test} ? static_cast<void>({first}) : static_cast<void>({second})"
    if site.used:
      substitution.text = f"({substitution.text})"
    return substitution

  def _SameType(self, branches: list[Substitution]) -> bool:
    types = set()
    for branch in branches:
      callee = self.program.Signature(branch.callee, branch.target.owner.function.file) if branch.callee else None
      types.add(_Bare(callee.returns) if callee is not None else "?")
    return len(types) == 1 and "?" not in types

  def _Inner(self, site: CallSite) -> list["Substitution"]:
    return [done for done in self._done.get(site.file, []) if site.span.first < done.site.span.first and done.site.span.last <= site.span.last]

  def _WordText(self, site: CallSite, span: Span) -> str:
    """A word's source text, with the replacements of the sites nested in it already made."""
    source = self.program.Source(site.file)
    return Splice(source.text, source.tokens, span, self._Nested(site, span))

  def _Nested(self, site: CallSite, span: Span) -> list[tuple[Span, str]]:
    found = [(done.site.span, done.text) for done in self._done.get(site.file, []) if span.first <= done.site.span.first and done.site.span.last <= span.last]
    return _Outermost(found)

  def _ReadText(self, site: CallSite, target: Target, read: WordRead, words: list[str], substitution: Substitution) -> str:
    if read.kind == SPAN:
      length = target.count if read.length is None else read.length
      items = [self._Word(target, position, words, substitution) for position in range(length)]
      return "std::span<std::uint16_t const>{}" if not items else f"std::array<std::uint16_t, {len(items)}>{{ {', '.join(_Cast(item) for item in items)} }}"
    if read.accessor:
      return self._Accessor(site, target, read.accessor, read.arguments, (words, substitution))
    low = self._Word(target, read.index, words, substitution)
    if read.kind == WIDE:
      high = self._Word(target, read.index + 1, words, substitution)
      return f"(static_cast<std::uint32_t>({_Cast(low)}) | static_cast<std::uint32_t>({_Cast(high)}) << 16)"
    return _Placed(low, self.program.Source(target.owner.function.file), read.span, self.program, site)

  def _Word(self, target: Target, position: int, words: list[str], substitution: Substitution) -> str:
    """The site's word at a target's word position, `0` when the target reads past what it receives."""
    index = target.words[position] if position < len(target.words) else None
    if index is None or index >= len(words):
      substitution.past_end.append(position)
      return "0"
    return words[index]

  def _Accessor(self, site: CallSite, target: Target, name: str, arguments: tuple, state: tuple[list[str], Substitution]) -> str:
    """An accessor call (`byte(2)`, `far(5, 6)`) expanded: its lambda's expression with each word read in place,
    accessors it calls expanded the same way."""
    words, substitution = state
    accessors = target.owner.context.accessors
    accessor = accessors[name]
    owner = self.program.Source(target.owner.function.file)
    tokens, partners = owner.tokens, owner.partners
    bound = dict(zip(accessor.parameters, arguments))
    replacements = []
    index = accessor.expression.first
    while index < accessor.expression.last - 1:
      inner = [Compact(tokens, part) for part in Arguments(tokens, partners, index + 1)] if tokens[index + 1].text == "(" else []
      if tokens[index].text in ("Arg", "Arg32") and inner and inner[-1] in bound:
        word = self._Word(target, bound[inner[-1]], words, substitution)
        replacements.append((Span(index, partners[index + 1] + 1), _Placed(word, owner, Span(index, partners[index + 1] + 1), self.program, site)))
      elif tokens[index].text in accessors and inner and all(argument in bound for argument in inner):
        text = self._Accessor(site, target, tokens[index].text, tuple(bound[argument] for argument in inner), state)
        replacements.append((Span(index, partners[index + 1] + 1), text))
      else:
        index += 1
        continue
      index = partners[index + 1] + 1
    text = Splice(owner.text, tokens, accessor.expression, replacements)
    text = f"({text})" if _Operators(Text(tokens, accessor.expression, owner.text)) else text
    if accessor.near and text not in substitution.near:
      substitution.near.append(text)
    return text

  def _Renames(self, site: CallSite, target: Target, owner: SourceFile, span: Span) -> list[tuple[Span, str]]:
    """The game (`ds` before B1) and the dispatcher's id where the target names them: the site's own expressions."""
    source = self.program.Source(site.file)
    names = dict.fromkeys(SEGMENT_NAMES, Text(source.tokens, site.ds, source.text))
    if target.owner.id_name:
      names[target.owner.id_name] = Text(source.tokens, site.service, source.text) if target.service is None else f"0x{target.service:x}"
    tokens = owner.tokens
    for name, local in target.owner.Inlined(owner).items():
      text = Text(tokens, local, owner.text)
      names[name] = f"({text})" if _Operators(text) else text
    reads = target.case.words.reads
    return [(Span(index, index + 1), names[tokens[index].text]) for index in range(span.first, span.last)
            if tokens[index].kind == IDENTIFIER and tokens[index].text in names and names[tokens[index].text] != tokens[index].text
            and tokens[index - 1].text not in TIGHT and not any(read.span.Contains(index) for read in reads)]

  def _Result(self, site: CallSite, target: Target, text: str, substitution: Substitution) -> str:
    if not site.used:
      return Discarded(text)
    if target.case.form == STATEMENT:
      substitution.void_used = True
      return f"({text}, std::uint16_t{{ 0 }})"
    callee = self.program.Signature(substitution.callee, target.owner.function.file) if substitution.callee else None
    returns = _Bare(callee.returns) if callee is not None else ""
    if returns in RESULT_AS_IS or re.match(r"^\(?\s*static_cast\s*<\s*(std::)?uint16_t\s*>", text) or _NarrowedToByte(self.program.Source(site.file), site.span):
      return text
    substitution.result_cast = True
    return f"static_cast<std::uint16_t>({text})"

  def _Invisible(self, site: CallSite, target: Target, owner: SourceFile, substitution: Substitution) -> None:
    """The names the replacement uses that the site's file cannot see, the headers that declare them, and those only
    the target's own .cpp defines (a file-local helper or constant: B0.3 or a hand edit moves it to a header)."""
    if target.owner.function.file == site.file:
      return
    visible = set(self.program.scopes.Closure(site.file))
    constants = self.program.ConstantsOf(site.file).values
    owner_constants = self.program.ConstantsOf(target.owner.function.file).values
    tokens = owner.tokens
    inlined = target.owner.Inlined(owner)
    used = {tokens[i].text for i in range(target.case.expression.first, target.case.expression.last)}
    accessors = target.owner.context.accessors if target.owner.context is not None else {}
    spans = [target.case.expression, *target.case.locals.values()] + [span for name, span in inlined.items() if name in used]
    spans += [accessor.expression for name, accessor in accessors.items() if name in used or any(read.accessor == name for read in target.case.words.reads)]
    for index in (i for span in spans for i in range(span.first, span.last)):
      token = tokens[index]
      if token.kind != IDENTIFIER or tokens[index - 1].text in TIGHT or token.text in substitution.invisible or token.text in inlined or token.text in accessors:
        continue
      known = self.program.definitions.get(token.text, []) + self.program.declarations.get(token.text, [])
      if known and not any(function.file in visible for function in known):
        files = [function.file for function in known]
      elif not known and token.text in owner_constants and token.text not in constants:
        files = self.program.scopes.DefiningFiles(token.text)
      else:
        continue
      substitution.invisible.append(token.text)
      header = next((file for file in files if file.suffix == ".hpp"), None)
      if header is None:
        substitution.local.append(token.text)
      elif site.file in self.program.scopes.Closure(header):
        substitution.cycle += [IncludeOf(header)] if IncludeOf(header) not in substitution.cycle else []
      elif IncludeOf(header) not in substitution.includes:
        substitution.includes.append(IncludeOf(header))


def _NarrowedToByte(source: SourceFile, span: Span) -> bool:
  """The site is the whole operand of a conversion to 8 bits (`static_cast<uint8_t>(site)`, `(uint8_t)site`), which
  drops whatever the 16-bit conversion of its result would."""
  tokens = source.tokens
  before = [token.text for token in tokens[max(span.first - 7, 0):span.first]]
  after = tokens[span.last].text if span.last < len(tokens) else ""
  functional = before[-5:] == ["static_cast", "<", "uint8_t", ">", "("] or before[-7:] == ["static_cast", "<", "std", "::", "uint8_t", ">", "("]
  return (functional and after == ")") or before[-3:] == ["(", "uint8_t", ")"] or before[-5:] == ["(", "std", "::", "uint8_t", ")"]


def Discarded(text: str) -> str:
  """A call whose value the site discards, without the conversion of its result (`(uint16_t)f(x)` -> `f(x)`): a cast
  left in a statement of its own would be an unused value."""
  match = re.fullmatch(r"\s*(?:\((?:std::)?u?int(?:8|16|32)_t\)|static_cast\s*<\s*(?:std::)?u?int(?:8|16|32)_t\s*>)\s*(.*)", text, re.S)
  if match is None:
    return text
  inner = match.group(1)
  if inner.startswith("(") and _Closes(inner, 0) == len(inner) - 1:
    inner = inner[1:-1]
  call = re.match(r"[A-Za-z_][\w:]*\s*\(", inner)
  return inner if call and _Closes(inner, call.end() - 1) == len(inner) - 1 else text


def _Closes(text: str, opening: int) -> int:
  """The index of the parenthesis closing the one at opening (-1 when unbalanced)."""
  depth = 0
  for index in range(opening, len(text)):
    depth += (text[index] == "(") - (text[index] == ")")
    if depth == 0:
      return index
  return -1


def _Hoisted(words: list[str]) -> tuple[list[tuple[int, str]], list[str]]:
  """The words to evaluate first (every one that is not a constant) and the names that stand for them."""
  hoisted = [(index, text) for index, text in enumerate(words) if not _Constant(text)]
  return hoisted, [f"hoisted{index}" if not _Constant(text) else text for index, text in enumerate(words)]


def _Merged(site: CallSite, target: Target, branches: list[Substitution]) -> Substitution:
  """One substitution carrying what each branch of a conditional id needs and reports."""
  merged = Substitution(site, target, "", callee=" / ".join(dict.fromkeys(branch.callee for branch in branches)))
  for branch in branches:
    merged.past_end += [position for position in branch.past_end if position not in merged.past_end]
    merged.void_used |= branch.void_used
    merged.result_cast |= branch.result_cast
    for name in ("invisible", "includes", "local", "cycle", "near"):
      getattr(merged, name).extend(item for item in getattr(branch, name) if item not in getattr(merged, name))
  return merged


def _StatementStart(source: SourceFile, span: Span) -> bool:
  """The expression at span is a whole statement where a statement may begin (not the body of an unbraced if)."""
  tokens = source.tokens
  following = tokens[span.last].text if span.last < len(tokens) else ""
  return following == ";" and tokens[span.first - 1].text in (";", "{", "}")


def IncludeOf(header) -> str:
  """How a game file includes a header: relative to game/ (`map/btlspend.hpp`), glue and services ones by name."""
  parts = header.parts
  return "/".join(parts[1:]) if parts[0] == "game" else header.name

def _Outermost(found: list[tuple[Span, str]]) -> list[tuple[Span, str]]:
  return [(span, text) for span, text in found if not any(o.first <= span.first and span.last <= o.last and o != span for o, _ in found)]


def CalleeOf(source: SourceFile, expression: Span) -> str:
  """The routine the target calls: the first name called in its expression that is not a wrapper or a cast."""
  tokens = source.tokens
  for index in range(expression.first, expression.last - 1):
    token = tokens[index]
    if token.kind == IDENTIFIER and tokens[index + 1].text == "(" and token.text not in PURE_CALLS | {"WithStack", "WithBattleStack", "return"}:
      return token.text
  return ""


def _Bare(type_text: str) -> str:
  return type_text.replace("std::", "").replace("const", "").strip()


def _Constant(text: str) -> bool:
  return re.fullmatch(r"\s*(0[xX][0-9a-fA-F]+|\d+)[uU]?\s*|[A-Z][A-Z0-9_]*(::[A-Z][A-Z0-9_]*)?", text) is not None


def _Cast(text: str) -> str:
  return text if re.fullmatch(r"(0[xX][0-9a-fA-F]+|\d+)[uU]?|hoisted\d+", text.strip()) else f"static_cast<std::uint16_t>({text})"


def _Operators(text: str) -> bool:
  """Whether text has an operator outside brackets (so it needs parentheses where a primary expression stood)."""
  depth = 0
  text = re.sub(r"\b(?:static|reinterpret|const)_cast\s*<[^<>()]*>", "", text)
  for character in re.sub(r"->|::|\.", "", re.sub(r'"(?:\\.|[^"\\])*"', '""', text)):
    depth += (character in "([{") - (character in ")]}")
    if depth == 0 and character in "+-*/%<>=!&|^?:,~":
      return True
  return False


def _Placed(word: str, owner: SourceFile, read: Span, program: Program, site: CallSite | None) -> str:
  """The word where the target read it: converted to 16 bits as `Call` did unless the place converts it to 16 or 8
  bits anyway, parenthesised where it stands as an operand."""
  narrow = _NarrowPlace(owner, read, program)
  if not narrow and not _NarrowWord(word, program, site) and not _Constant(word):
    return f"static_cast<std::uint16_t>({word})"
  return f"({word})" if _Operators(word) and not _WholeOperand(owner, read) else word


def _NarrowPlace(source: SourceFile, read: Span, program: Program) -> bool:
  """The read is the whole operand of a cast to a 16- or 8-bit type, or a whole argument of a narrow parameter."""
  tokens = source.tokens
  before, after = tokens[read.first - 1], tokens[read.last] if read.last < len(tokens) else None
  if before.text == ")" and tokens[read.first - 2].text in NARROW_PARAMETERS | {"uint16_t", "uint8_t"}:
    return True  # (uint8_t)Arg(...)
  if before.text == "(" and after is not None and after.text == ")":
    cast = tokens[read.first - 2]
    if cast.text == ">" and _Bare(Compact(tokens, Span(_AngleOpen(tokens, read.first - 2) + 1, read.first - 2))) in NARROW_PARAMETERS:
      return True
  return _ArgumentType(source, read, program) in NARROW_PARAMETERS


def _AngleOpen(tokens, close: int) -> int:
  index = close
  while index > 0 and tokens[index].text != "<":
    index -= 1
  return index


def _ArgumentType(source: SourceFile, read: Span, program: Program) -> str:
  """The type of the parameter the read is passed to as a whole argument (empty when it is not one)."""
  tokens, partners = source.tokens, source.partners
  if tokens[read.first - 1].text not in ("(", ",") or read.last >= len(tokens) or tokens[read.last].text not in (")", ","):
    return ""
  depth, index = 0, read.first - 1
  while index > 0:
    text = tokens[index].text
    if text in (")", "]", "}") and index in partners:
      index = partners[index] - 1
      continue
    if text == "(":
      break
    if text in ("{", "[", ";"):
      return ""
    index -= 1
  name = tokens[index - 1]
  if name.kind != IDENTIFIER:
    return ""
  position = next((i for i, part in enumerate(Arguments(tokens, partners, index)) if part.first == read.first), None)
  callee = program.Signature(name.text, source.path)
  if callee is None or position is None or position >= len(callee.parameters):
    return ""
  return _Bare(callee.parameters[position].type)


def _WholeOperand(source: SourceFile, read: Span) -> bool:
  tokens = source.tokens
  return tokens[read.first - 1].text in ("(", ",", "{") and read.last < len(tokens) and tokens[read.last].text in (")", ",", "}")


def _NarrowWord(word: str, program: Program, site: CallSite | None) -> bool:
  """A word whose value already fits 16 bits unsigned: a cast to one, or a parameter or local declared so."""
  if re.fullmatch(r"\s*(static_cast\s*<\s*(std::)?(uint16_t|uint8_t)\s*>\s*\(.*\)|hoisted\d+)\s*", word, re.S):
    return True
  if site is None or not re.fullmatch(r"[A-Za-z_]\w*", word.strip()):
    return False
  return _Bare(_DeclaredType(program, site.function, word.strip())) in UNSIGNED_NARROW


def _DeclaredType(program: Program, function: Function, name: str) -> str:
  index = function.ParameterIndex(name)
  if index is not None:
    return function.parameters[index].type
  source = program.Source(function.file)
  tokens = source.tokens
  for at in range(function.body.first, function.body.last - 1):
    if tokens[at].text == name and tokens[at + 1].text in ("{", "=", ";") and tokens[at - 1].kind == IDENTIFIER:
      previous = at - 1 if tokens[at - 1].text != "const" else at - 2
      if tokens[previous].text == "auto" and tokens[at + 1].text == "{" and tokens[at + 2].text == "Call":
        return "uint16_t"
      return tokens[previous].text
  return ""


def _SideEffects(source: SourceFile, span: Span) -> bool:
  tokens = source.tokens
  for index in range(span.first, span.last):
    token = tokens[index]
    if token.text in MUTATING:
      return True
    if token.kind == IDENTIFIER and index + 1 < span.last and tokens[index + 1].text == "(" and token.text not in PURE_CALLS:
      return True
  return False


def _OrderHazard(owner: SourceFile, expression: Span, reads: list[WordRead], target: Target, effects: list[int], words: list[str]) -> str:
  """Why the site's words must be evaluated first, in order (empty when the call evaluates them as `Call` did)."""
  if not effects:
    return ""
  used = [target.words[index] if index < len(target.words) else None for read in reads if read.kind != SPAN for index in read.Indices()]
  spans = [read for read in reads if read.kind == SPAN]
  if spans:
    shortest = min(len(target.words) if read.length is None else read.length for read in spans)
    dropped = [index for index in effects if index >= shortest]
    return f"word {dropped[0]} has side effects and is past the span's end" if dropped else ""  # a span lists its words in order
  for index in effects:
    if used.count(index) != 1:
      return f"word {index} has side effects and is read {used.count(index)} times"
  variable = [index for index in used if index is not None and not _Constant(words[index])]
  if variable != sorted(variable):
    return f"words with side effects are read in the order {variable}"
  calls = sum(1 for i in range(expression.first, expression.last - 1) if owner.tokens[i].kind == IDENTIFIER
              and owner.tokens[i + 1].text == "(" and owner.tokens[i].text not in PURE_CALLS)
  if calls > 1 + sum(1 for read in reads if read.accessor):
    return "the target evaluates other calls among the words"
  return ""
