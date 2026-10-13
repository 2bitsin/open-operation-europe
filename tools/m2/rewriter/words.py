"""The argument words a dispatcher case or a link handler reads: `Arg(args, count, i)`, `Arg32(args, count, i)` and
the dispatcher's own accessor lambdas (`word(i)`, `byte(i)`), and every other use of the raw word array."""
from __future__ import annotations

from dataclasses import dataclass, field

from .constants import IsIntegerLiteral
from .source import SourceFile
from .syntax import Arguments, Span
from .tokens import IDENTIFIER

WIDE_READS = {"Arg32"}


@dataclass(frozen=True)
class Accessor:
  """A lambda over the words: `auto const word{ [&](unsigned i) { return Arg(args, count, i); } };`."""
  name:       str
  parameter:  str
  expression: Span  # the lambda's return expression


WORD = "word"  # Arg(args, count, i), word(i), args[i]: one word, 0 past the end
WIDE = "wide"  # Arg32(args, count, i): words i and i + 1 as one 32-bit value
SPAN = "span"  # { args, count }: every word, as a span


@dataclass(frozen=True)
class WordRead:
  span:     Span    # the whole read expression, e.g. `Arg(args, count, 2)`, `word(2)` or `{ args, count }`
  index:    int     # the word read (the low word of an Arg32; 0 for a span)
  kind:     str = WORD
  accessor: str = ""

  @property
  def wide(self) -> bool:
    return self.kind == WIDE


@dataclass
class WordUse:
  reads: list[WordRead] = field(default_factory=list)
  raw:   list[int] = field(default_factory=list)  # token indices of other uses of the array or count, or of a read
  #                                                 whose index is not a constant

  def Indices(self) -> list[int]:
    return [index for read in self.reads if read.kind != SPAN for index in ((read.index, read.index + 1) if read.wide else (read.index,))]

  def Spans(self) -> bool:
    return any(read.kind == SPAN for read in self.reads)


@dataclass
class WordContext:
  """The names under which a dispatcher or handler receives its words."""
  args:      str
  count:     str
  accessors: dict[str, Accessor] = field(default_factory=dict)

  def Scan(self, source: SourceFile, span: Span) -> WordUse:
    """Every read of the words inside span (accessor bodies are not part of span)."""
    tokens, partners = source.tokens, source.partners
    use, index = WordUse(), span.first
    while index < span.last:
      token = tokens[index]
      following = tokens[index + 1].text if index + 1 < len(tokens) else ""
      read = self._ReadAt(source, index)
      if isinstance(read, WordRead):
        use.reads.append(read)
        index = read.span.last
        continue
      if read is not None or (token.kind == IDENTIFIER and token.text in (self.args, self.count) and not _Member(tokens, index)):
        use.raw.append(index)
      index += 1
    return use

  def _ReadAt(self, source: SourceFile, index: int) -> WordRead | int | None:
    tokens, partners = source.tokens, source.partners
    token = tokens[index]
    following = tokens[index + 1].text if index + 1 < len(tokens) else ""
    if token.kind == IDENTIFIER and following == "(" and index + 1 in partners and not _Member(tokens, index):
      return self._Read(source, index)
    if token.text == "{" and index in partners and [t.text for t in tokens[index + 1:partners[index]]] == [self.args, ",", self.count]:
      return WordRead(Span(index, partners[index] + 1), 0, SPAN)
    if token.text == self.args and following == "[" and index + 1 in partners and not _Member(tokens, index):
      return self._Indexed(source, index)
    return None

  def _Indexed(self, source: SourceFile, index: int) -> WordRead | int:
    """`args[N]` read as a value (not assigned to, not addressed)."""
    tokens, partners = source.tokens, source.partners
    close = partners[index + 1]
    inside = tokens[index + 2:close]
    after = tokens[close + 1].text if close + 1 < len(tokens) else ""
    before = tokens[index - 1].text
    if len(inside) == 1 and IsIntegerLiteral(inside[0]) and after not in ("=", "+=", "-=", "|=", "&=", "++", "--") and before not in ("&", "++", "--"):
      return WordRead(Span(index, close + 1), _Value(inside[0].text))
    return -1

  def _Read(self, source: SourceFile, index: int) -> WordRead | int | None:
    """A word read starting at index, an int (a raw use) when it reads by a non-constant index, None otherwise."""
    tokens, partners = source.tokens, source.partners
    name, close = tokens[index].text, partners[index + 1]
    arguments = Arguments(tokens, partners, index + 1)
    if name in ("Arg", *WIDE_READS) and len(arguments) == 3 and self._Names(tokens, arguments):
      word = arguments[2]
      if len(word) == 1 and IsIntegerLiteral(tokens[word.first]):
        return WordRead(Span(index, close + 1), _Value(tokens[word.first].text), WIDE if name in WIDE_READS else WORD)
      return -1
    if name in self.accessors and len(arguments) == 1:
      word = arguments[0]
      if len(word) == 1 and IsIntegerLiteral(tokens[word.first]):
        return WordRead(Span(index, close + 1), _Value(tokens[word.first].text), WORD, name)
      return -1
    return None

  def _Names(self, tokens, arguments: list[Span]) -> bool:
    return (len(arguments[0]) == 1 and tokens[arguments[0].first].text == self.args and len(arguments[1]) == 1
            and tokens[arguments[1].first].text == self.count)


def FindAccessors(source: SourceFile, statements: list[Span], context: WordContext) -> dict[str, Accessor]:
  """The accessor lambdas declared among statements: `auto [const] NAME{ [..](T i) [-> R] { return EXPR; } };` or the
  same with `= [..] ... ;`, whose EXPR reads the words."""
  found: dict[str, Accessor] = {}
  tokens, partners = source.tokens, source.partners
  for statement in statements:
    words = [token.text for token in tokens[statement.first:statement.last]]
    if not words or words[0] != "auto":
      continue
    name_at = statement.first + (2 if len(words) > 1 and words[1] == "const" else 1)
    capture = next((i for i in range(name_at, statement.last) if tokens[i].text == "["), None)
    if tokens[name_at].kind != IDENTIFIER or capture is None or capture not in partners:
      continue
    parameters = partners[capture] + 1
    if tokens[parameters].text != "(":
      continue
    names = [tokens[i].text for i in range(parameters + 1, partners[parameters]) if tokens[i].kind == IDENTIFIER]
    body = next((i for i in range(partners[parameters], statement.last) if tokens[i].text == "{"), None)
    if not names or body is None or tokens[body + 1].text != "return":
      continue
    expression = Span(body + 2, partners[body] - 1)
    if any(token.text == context.args for token in tokens[expression.first:expression.last]):
      found[tokens[name_at].text] = Accessor(tokens[name_at].text, names[-1], expression)
  return found


def _Member(tokens, index: int) -> bool:
  return index > 0 and tokens[index - 1].text in (".", "->")


def _Value(text: str) -> int:
  return int(text.rstrip("uUlL"), 0)
