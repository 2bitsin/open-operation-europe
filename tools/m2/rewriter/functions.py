"""Function definitions and declarations at namespace or class scope, with their parameters and return types."""
from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import PurePosixPath

from .source import SourceFile
from .syntax import Compact, Span, SplitTopLevel
from .tokens import IDENTIFIER, Token

SPECIFIERS = {"static", "inline", "constexpr", "consteval", "constinit", "extern", "virtual", "explicit", "friend", "noexcept", "thread_local", "typename"}
BUILTIN_WORDS = {"int", "unsigned", "signed", "char", "short", "long", "bool", "void", "const", "volatile", "float", "double", "auto"}
NOT_FUNCTION_NAMES = {"if", "for", "while", "switch", "return", "sizeof", "alignof", "decltype", "static_assert", "alignas", "__attribute__", "requires"}


@dataclass(frozen=True)
class Parameter:
  type: str  # canonical spelling, e.g. "std::uint16_t const*"
  name: str  # "" when unnamed


@dataclass
class Function:
  name:        str
  file:        PurePosixPath
  parameters:  list[Parameter]
  returns:     str
  name_index:  int          # token index of the name
  body:        Span | None  # the braces of a definition, None for a declaration
  owner:       str = ""     # the enclosing class, for members
  is_template: bool = False
  line:        int = 0

  def ParameterIndex(self, name: str) -> int | None:
    found = [index for index, parameter in enumerate(self.parameters) if parameter.name == name]
    return found[0] if found else None


@dataclass
class FileFunctions:
  definitions:  list[Function] = field(default_factory=list)
  declarations: list[Function] = field(default_factory=list)


def ScanFunctions(source: SourceFile) -> FileFunctions:
  """Every function defined or declared at namespace or class scope of one file."""
  found = FileFunctions()
  _ScanScope(source, Span(0, len(source.tokens)), "", found)
  return found


def _ScanScope(source: SourceFile, span: Span, owner: str, found: FileFunctions) -> None:
  tokens, partners = source.tokens, source.partners
  head_start, index = span.first, span.first
  while index < span.last:
    text = tokens[index].text
    if text in (";", "}"):
      if text == ";":
        _Declaration(source, Span(head_start, index), owner, found)
      head_start = index = index + 1
    elif text == "{" and index in partners:
      index = _OpenBrace(source, Span(head_start, index), owner, found)
      head_start = index
    elif text in ("(", "[") and index in partners:
      index = partners[index] + 1
    else:
      index += 1


def _OpenBrace(source: SourceFile, head: Span, owner: str, found: FileFunctions) -> int:
  """Handles the brace closing head; returns the index after the construct."""
  tokens, partners = source.tokens, source.partners
  brace = head.last
  close = partners[brace]
  words = [token.text for token in tokens[head.first:head.last]]
  if words[:1] == ["namespace"] or words[:2] == ["inline", "namespace"] or (words[:1] == ["extern"] and len(words) == 2):
    _ScanScope(source, Span(brace + 1, close), owner, found)
    return close + 1
  kind = _ClassKind(tokens, head)
  if kind is not None:
    name = _ClassName(tokens, head)
    if kind != "enum":
      _ScanScope(source, Span(brace + 1, close), name or owner, found)
    return _SkipDeclarators(tokens, partners, close + 1)
  function = _FunctionHead(source, head, owner)
  if function is None:
    return _SkipDeclarators(tokens, partners, close + 1)
  body_brace = _BodyAfterInitializers(tokens, partners, head, brace)
  function.body = Span(body_brace, partners[body_brace] + 1)
  found.definitions.append(function)
  return partners[body_brace] + 1


def _SkipDeclarators(tokens: list[Token], partners: dict[int, int], index: int) -> int:
  """After an initialiser's or a class's closing brace: up to and past the `;` that ends the declaration, if any."""
  if index < len(tokens) and tokens[index].text == ";":
    return index + 1
  return index


def _ClassKind(tokens: list[Token], head: Span) -> str | None:
  depth = 0
  for token in tokens[head.first:head.last]:
    depth += (token.text in "([<") - (token.text in ")]>")
    if depth == 0 and token.text in ("class", "struct", "union", "enum"):
      return token.text
    if depth == 0 and token.text == "=":
      return None
  return None


def _ClassName(tokens: list[Token], head: Span) -> str:
  names = [token.text for token in tokens[head.first:head.last] if token.kind == IDENTIFIER and token.text not in ("class", "struct", "union", "enum", "final", "public", "private", "protected")]
  return names[0] if names else ""


def _BodyAfterInitializers(tokens: list[Token], partners: dict[int, int], head: Span, brace: int) -> int:
  """The body brace of a definition: brace itself, or the brace after a constructor's member-initialiser list."""
  colon = next((index for index in range(head.first, head.last) if tokens[index].text == ":" and _AfterParameters(tokens, head, index)), None)
  if colon is None:
    return brace
  index = brace
  while tokens[index].text == "{" and tokens[index - 1].kind == IDENTIFIER and tokens[index - 1].text not in ("const", "noexcept", "override", "final"):
    index = partners[index] + 1
    if tokens[index].text != ",":
      break
    index += 1
    while tokens[index].text != "{" and tokens[index].text != "(":
      index += 1
    if tokens[index].text == "(":
      index = partners[index] + 1
  return index


def _AfterParameters(tokens: list[Token], head: Span, index: int) -> bool:
  return any(token.text == ")" for token in tokens[head.first:index])


def _Declaration(source: SourceFile, head: Span, owner: str, found: FileFunctions) -> None:
  tokens = source.tokens
  words = [token.text for token in tokens[head.first:head.last]]
  if not words or words[0] in ("using", "typedef", "return", "static_assert", "template", "namespace") and "(" not in words:
    return
  if "=" in words[:words.index("(")] if "(" in words else True:
    return
  function = _FunctionHead(source, head, owner)
  if function is not None:
    found.declarations.append(function)


def _FunctionHead(source: SourceFile, head: Span, owner: str) -> Function | None:
  """The function a head declares (name, parameters, return type), or None when the head is something else."""
  tokens, partners = source.tokens, source.partners
  index, is_template = _SkipPrefix(tokens, partners, head)
  paren = next((i for i in range(index, head.last) if tokens[i].text == "(" and i in partners), None)
  if paren is None or paren == index:
    return None
  name_index = paren - 1
  name = tokens[name_index]
  if name.kind != IDENTIFIER or name.text in NOT_FUNCTION_NAMES or name.text in BUILTIN_WORDS:
    return None
  if any(token.text == "=" for token in tokens[index:paren]):
    return None
  if tokens[paren + 1].text in ("*", "&"):
    return None  # `type (*name)(...)`: a function pointer declarator, not a function named by its type
  qualifier = tokens[name_index - 1].text == "::" and name_index - 2 >= index
  member_of = tokens[name_index - 2].text if qualifier else owner
  leading = Span(index, name_index - 2 if qualifier else name_index)
  close = partners[paren]
  parameters = [_Parameter(tokens, part) for part in SplitTopLevel(tokens, Span(paren + 1, close), ",")]
  parameters = [parameter for parameter in parameters if parameter is not None]
  returns = _ReturnType(tokens, leading, Span(close + 1, head.last))
  return Function(name.text, source.path, parameters, returns, name_index, None, member_of, is_template, name.line)


def _SkipPrefix(tokens: list[Token], partners: dict[int, int], head: Span) -> tuple[int, bool]:
  """Index past template headers, attributes and access labels at the head's start; whether a template header was."""
  index, is_template = head.first, False
  while index < head.last:
    text = tokens[index].text
    if text == "template" and index + 1 < head.last and tokens[index + 1].text == "<":
      index, is_template = _SkipAngles(tokens, index + 1), True
    elif text == "[" and index + 1 < head.last and tokens[index + 1].text == "[":
      index = partners.get(index, index) + 1
    elif text in ("public", "private", "protected") and index + 1 < head.last and tokens[index + 1].text == ":":
      index += 2
    else:
      return index, is_template
  return index, is_template


def _SkipAngles(tokens: list[Token], index: int) -> int:
  depth = 0
  while index < len(tokens):
    depth += (tokens[index].text == "<") - (tokens[index].text == ">") - 2 * (tokens[index].text == ">>")
    index += 1
    if depth <= 0:
      return index
  return index


def _ReturnType(tokens: list[Token], leading: Span, after: Span) -> str:
  arrow = next((i for i in range(after.first, after.last) if tokens[i].text == "->"), None)
  if arrow is not None:
    end = next((i for i in range(arrow + 1, after.last) if tokens[i].text in ("requires", "override", "final")), after.last)
    return Compact(tokens, Span(arrow + 1, end))
  kept = [i for i in range(leading.first, leading.last) if tokens[i].text not in SPECIFIERS]
  return Compact(tokens, Span(kept[0], kept[-1] + 1)) if kept else ""


def _Parameter(tokens: list[Token], part: Span) -> Parameter | None:
  if not len(part):
    return None
  end = next((i for i in range(part.first, part.last) if tokens[i].text == "="), part.last)
  last = end
  while last > part.first and tokens[last - 1].text == "]":
    last = next(i for i in range(last - 1, part.first - 1, -1) if tokens[i].text == "[")
  words = tokens[part.first:last]
  if len(words) == 1 and words[0].text in ("void", "..."):
    return None
  name_token = words[-1] if words else None
  named = name_token is not None and name_token.kind == IDENTIFIER and len(words) > 1 and tokens[last - 2].text != "::" and name_token.text not in BUILTIN_WORDS and not name_token.text.endswith("_t")
  type_end = last - 1 if named else last
  return Parameter(Compact(tokens, Span(part.first, type_end)), name_token.text if named else "")
