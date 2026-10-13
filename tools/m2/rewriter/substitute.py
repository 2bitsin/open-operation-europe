"""The text that replaces one `Call(` site: its target's routine call with the site's words put where the target read
them (plan section 2.1), and the facts a reviewer must see (words past the end, used results of void cases, words
whose side effects move, names the site's file cannot see)."""
from __future__ import annotations

import re
from dataclasses import dataclass, field

from .calls import CallSite
from .dispatch import STATEMENT
from .functions import Function
from .program import Program
from .resolve import Target
from .source import SourceFile
from .syntax import Arguments, Compact, Span, Splice, Text
from .tokens import IDENTIFIER
from .words import SPAN, WIDE, WordRead

NARROW_PARAMETERS = {"uint16_t", "int16_t", "uint8_t", "int8_t", "short", "unsigned short", "unsigned char", "signed char", "char"}
UNSIGNED_NARROW = {"uint16_t", "uint8_t", "unsigned short", "unsigned char", "bool"}
RESULT_AS_IS = {"uint16_t", "unsigned short"}
PURE_CALLS = {"static_cast", "reinterpret_cast", "const_cast", "Local", "Near", "Data", "Record", "Word", "Byte", "offsetof",
              "sizeof", "to_underlying", "Arg", "Arg32", "U16", "Bits", "port_far", "PORT_DS", "port_dialogs_host", "port_textout_host", "port_uiprim_host", "port_numdlg_host", "uint16_t", "uint8_t", "int16_t", "Lo", "Hi"}
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
  callee:      str = ""


@dataclass
class Substituter:
  """Substitutes sites innermost first, so a site nested in another's words is replaced inside them."""
  program: Program
  _done:   dict[object, list["Substitution"]] = field(default_factory=dict)  # file -> the substitutions made there

  def Substitute(self, site: CallSite, target: Target) -> Substitution:
    owner_source = self.program.Source(target.owner.function.file)
    case = target.case
    words = [self._WordText(site, span) for span in site.words]
    reads = [read for read in case.words.reads if case.expression.Contains(read.span.first)]
    substitution = Substitution(site, target, "", callee=CalleeOf(owner_source, case.expression))
    effects = [index for index, span in enumerate(site.words) if _SideEffects(self.program.Source(site.file), span)]
    substitution.order = _OrderHazard(owner_source, case.expression, reads, target, effects, words)
    if substitution.order:
      substitution.hoisted = [(index, words[index]) for index in range(len(words)) if not _Constant(words[index])]
      words = [f"hoisted{index}" if not _Constant(text) else text for index, text in enumerate(words)]
    replacements = [(read.span, self._ReadText(site, target, read, words, substitution)) for read in reads]
    replacements += self._Renames(site, target, owner_source)
    text = Splice(owner_source.text, owner_source.tokens, case.expression, replacements)
    substitution.text = self._Result(site, target, text, substitution)
    self._Invisible(site, target, owner_source, substitution)
    if "std::array" in substitution.text:
      substitution.includes.append("<array>")
    for inner in self._Inner(site):
      substitution.includes += [include for include in inner.includes if include not in substitution.includes]
      substitution.local += [name for name in inner.local if name not in substitution.local]
    self._done.setdefault(site.file, []).append(substitution)
    return substitution

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
      items = [self._Word(target, position, words, substitution) for position in range(target.count)]
      return "std::span<std::uint16_t const>{}" if not items else f"std::array<std::uint16_t, {len(items)}>{{ {', '.join(_Cast(item) for item in items)} }}"
    low = self._Word(target, read.index, words, substitution)
    if read.kind == WIDE:
      high = self._Word(target, read.index + 1, words, substitution)
      return f"(static_cast<std::uint32_t>({_Cast(low)}) | static_cast<std::uint32_t>({_Cast(high)}) << 16)"
    owner = self.program.Source(target.owner.function.file)
    if read.accessor:
      return self._Accessor(target, read, low, owner)
    return _Placed(low, owner, read.span, self.program, site)

  def _Word(self, target: Target, position: int, words: list[str], substitution: Substitution) -> str:
    """The site's word at a target's word position, `0` when the target reads past what it receives."""
    index = target.words[position] if position < len(target.words) else None
    if index is None or index >= len(words):
      substitution.past_end.append(position)
      return "0"
    return words[index]

  def _Accessor(self, target: Target, read: WordRead, word: str, owner: SourceFile) -> str:
    """An accessor call (`byte(2)`) expanded: its lambda's expression with the word in place of its own read."""
    accessor = target.owner.context.accessors[read.accessor]
    tokens = owner.tokens
    replacements = []
    for index in range(accessor.expression.first, accessor.expression.last):
      if tokens[index].text in ("Arg", "Arg32") and tokens[index + 1].text == "(":
        close = owner.partners[index + 1]
        replacements.append((Span(index, close + 1), _Placed(word, owner, Span(index, close + 1), self.program, None)))
        break
    text = Splice(owner.text, tokens, accessor.expression, replacements)
    return f"({text})" if _Operators(Text(tokens, accessor.expression, owner.text)) else text

  def _Renames(self, site: CallSite, target: Target, owner: SourceFile) -> list[tuple[Span, str]]:
    """`ds` and the dispatcher's id where the target names them: the site's own expressions."""
    source = self.program.Source(site.file)
    names = {"ds": Text(source.tokens, site.ds, source.text)}
    if target.owner.id_name:
      names[target.owner.id_name] = Text(source.tokens, site.service, source.text)
    tokens = owner.tokens
    for name, span in target.owner.Inlined(owner).items():
      text = Text(tokens, span, owner.text)
      names[name] = f"({text})" if _Operators(text) else text
    reads = target.case.words.reads
    return [(Span(index, index + 1), names[tokens[index].text]) for index in range(target.case.expression.first, target.case.expression.last)
            if tokens[index].kind == IDENTIFIER and tokens[index].text in names and names[tokens[index].text] != tokens[index].text
            and tokens[index - 1].text not in TIGHT and not any(read.span.Contains(index) for read in reads)]

  def _Result(self, site: CallSite, target: Target, text: str, substitution: Substitution) -> str:
    if not site.used:
      return text
    if target.case.form == STATEMENT:
      substitution.void_used = True
      return f"({text}, std::uint16_t{{ 0 }})"
    callee = self.program.Signature(substitution.callee, target.owner.function.file) if substitution.callee else None
    returns = _Bare(callee.returns) if callee is not None else ""
    if returns in RESULT_AS_IS or re.match(r"^\(?\s*static_cast\s*<\s*(std::)?uint16_t\s*>", text):
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
    spans = [target.case.expression] + [span for name, span in inlined.items() if name in used]
    for index in (i for span in spans for i in range(span.first, span.last)):
      token = tokens[index]
      if token.kind != IDENTIFIER or tokens[index - 1].text in TIGHT or token.text in substitution.invisible or token.text in inlined:
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
      elif IncludeOf(header) not in substitution.includes:
        substitution.includes.append(IncludeOf(header))


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
  used = [target.words[read.index] if read.index < len(target.words) else None for read in reads if read.kind != SPAN]
  if any(read.kind == SPAN for read in reads):
    return ""  # a span lists every word in order
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
