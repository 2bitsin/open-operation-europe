"""Structure over a token list: matching brackets, top-level lists and statements, expression text."""
from __future__ import annotations

from dataclasses import dataclass

from .tokens import IDENTIFIER, Token

OPENING = {"(": ")", "[": "]", "{": "}"}
CLOSING = {")", "]", "}"}


@dataclass(frozen=True)
class Span:
  """Tokens [first, last) of one file's token list."""
  first: int
  last:  int

  def __len__(self) -> int:
    return self.last - self.first

  def Contains(self, index: int) -> bool:
    return self.first <= index < self.last


def Partners(tokens: list[Token]) -> dict[int, int]:
  """Index of the matching bracket for every (, [, { and its closer (unbalanced brackets are left out)."""
  partners: dict[int, int] = {}
  stack: list[int] = []
  for index, token in enumerate(tokens):
    if token.text in OPENING:
      stack.append(index)
    elif token.text in CLOSING and stack and OPENING[tokens[stack[-1]].text] == token.text:
      opener = stack.pop()
      partners[opener], partners[index] = index, opener
  return partners


def SplitTopLevel(tokens: list[Token], span: Span, separator: str) -> list[Span]:
  """The parts of span separated by separator outside every bracket (an empty span gives no parts)."""
  if not len(span):
    return []
  parts, start, depth = [], span.first, 0
  for index in range(span.first, span.last):
    text = tokens[index].text
    depth += (text in OPENING) - (text in CLOSING)
    if depth == 0 and text == separator:
      parts.append(Span(start, index))
      start = index + 1
  parts.append(Span(start, span.last))
  return parts


def Arguments(tokens: list[Token], partners: dict[int, int], open_paren: int) -> list[Span]:
  """The comma-separated arguments inside the parenthesis at open_paren (template angle brackets are not split on,
  since a top-level comma inside `<...>` never occurs in a call's argument list in this tree)."""
  return SplitTopLevel(tokens, Span(open_paren + 1, partners[open_paren]), ",")


def Text(tokens: list[Token], span: Span, source: str) -> str:
  """The source text of a span, exactly as written (whitespace inside kept)."""
  if not len(span):
    return ""
  return source[tokens[span.first].start:tokens[span.last - 1].end]


def Compact(tokens: list[Token], span: Span) -> str:
  """The span's tokens joined by single spaces where two words would otherwise merge: a canonical spelling."""
  out: list[str] = []
  previous: Token | None = None
  for token in tokens[span.first:span.last]:
    if previous is not None and _Joins(previous, token):
      out.append(" ")
    out.append(token.text)
    previous = token
  return "".join(out)


def _Joins(left: Token, right: Token) -> bool:
  word = lambda token: token.kind == IDENTIFIER or token.text[0].isdigit()
  if word(left) and word(right):
    return True
  return left.text == "," or (left.text in ("return", "case") and right.text not in ";:")


def IsIdentifier(token: Token) -> bool:
  return token.kind == IDENTIFIER


def Statements(tokens: list[Token], partners: dict[int, int], span: Span) -> list[Span]:
  """The top-level statements of a block's inside: each ends at a `;` or at the brace closing a compound statement
  (an `else` after it continues the statement, a `do` block runs to its `;`, a brace followed by `;`, `,` or `)` is an
  initialiser's or a lambda's)."""
  found, start, index = [], span.first, span.first
  while index < span.last:
    text = tokens[index].text
    if text in OPENING and index in partners:
      close = partners[index]
      following = tokens[close + 1].text if close + 1 < span.last else ""
      index = close + 1
      if text == "{" and following not in (";", ",", ")", "else", ".", "->") and tokens[start].text != "do":
        found.append(Span(start, index))
        start = index
      continue
    if text == ";":
      found.append(Span(start, index + 1))
      start = index + 1
    index += 1
  if start < span.last:
    found.append(Span(start, span.last))
  return found


def Splice(source: str, tokens: list[Token], span: Span, replacements: list[tuple[Span, str]]) -> str:
  """The text of span with each (sub-span, text) replaced; sub-spans must not overlap."""
  if not len(span):
    return ""
  out, position = [], tokens[span.first].start
  for part, text in sorted(replacements, key=lambda item: item[0].first):
    out.append(source[position:tokens[part.first].start])
    out.append(text)
    position = tokens[part.last - 1].end
  out.append(source[position:tokens[span.last - 1].end])
  return "".join(out)
