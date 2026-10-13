"""The argument words a dispatcher case or a link handler reads: `Arg(args, count, i)`, `Arg32(args, count, i)` and
the dispatcher's own accessor lambdas (`word(i)`, `byte(i)`), and every other use of the raw word array."""
from __future__ import annotations

from dataclasses import dataclass, field

from .constants import IsIntegerLiteral
from .source import SourceFile
from .syntax import Arguments, Compact, Span, SplitTopLevel
from .tokens import IDENTIFIER

WIDE_READS = {"Arg32"}


NEAR_MARKS = {"PORT_DS", "Bytes", "port_ds_word_ptr"}  # PORT_DS before B1; the game's byte views and word helper after it


@dataclass(frozen=True)
class Accessor:
  """A lambda over the words: `auto const word{ [&](unsigned i) { return Arg(args, count, i); } };`, or one over
  other accessors (`far(segment, offset)` returning `port_far(w(segment), w(offset))`)."""
  name:       str
  parameters: tuple[str, ...]
  expression: Span             # the lambda's return expression
  order:      tuple[int, ...]  # the parameters whose words the expression reads, in the order it reads them
  near:       bool = False     # it makes a near pointer into the data segment of a word


WORD = "word"  # Arg(args, count, i), word(i), args[i]: one word, 0 past the end
WIDE = "wide"  # Arg32(args, count, i): words i and i + 1 as one 32-bit value
SPAN = "span"  # { args, count }: every word, as a span


@dataclass(frozen=True)
class WordRead:
  span:      Span    # the whole read expression, e.g. `Arg(args, count, 2)`, `word(2)` or `{ args, count }`
  index:     int     # the word read (the low word of an Arg32; 0 for a span; an accessor's first word)
  kind:      str = WORD
  accessor:  str = ""
  arguments: tuple[int, ...] = ()  # an accessor's literal arguments
  words:     tuple[int, ...] = ()  # the words an accessor reads, in order
  length:    int | None = None     # a span of a literal length (`{ args, 3 }`); None: every word the site gives

  @property
  def wide(self) -> bool:
    return self.kind == WIDE

  def Indices(self) -> tuple[int, ...]:
    if self.accessor:
      return self.words
    return (self.index, self.index + 1) if self.wide else (self.index,)


@dataclass
class WordUse:
  reads: list[WordRead] = field(default_factory=list)
  raw:   list[int] = field(default_factory=list)  # token indices of other uses of the array or count, or of a read
  #                                                 whose index is not a constant

  def Indices(self) -> list[int]:
    return [index for read in self.reads if read.kind != SPAN for index in read.Indices()]

  def Spans(self) -> bool:
    return any(read.kind == SPAN for read in self.reads)


@dataclass
class WordContext:
  """The names under which a dispatcher or handler receives its words: an array and its count, or (a dispatcher of
  an event that passes its words as scalars, `svc_x(game, id, a0, a1, a2, a3)`) one name per word."""
  args:      str
  count:     str
  accessors: dict[str, Accessor] = field(default_factory=dict)
  named:     tuple[str, ...] = ()

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
    inside = [t for t in tokens[index + 1:partners[index]]] if token.text == "{" and index in partners else []
    if self.args and [t.text for t in inside] == [self.args, ",", self.count]:
      return WordRead(Span(index, partners[index] + 1), 0, SPAN)
    if self.args and len(inside) == 3 and inside[0].text == self.args and inside[1].text == "," and IsIntegerLiteral(inside[2]):
      return WordRead(Span(index, partners[index] + 1), 0, SPAN, length=_Value(inside[2].text))
    if token.text == self.args and following == "[" and index + 1 in partners and not _Member(tokens, index):
      return self._Indexed(source, index)
    if token.kind == IDENTIFIER and token.text in self.named and not _Member(tokens, index):
      return self._Named(source, index)
    return None

  def _Named(self, source: SourceFile, index: int) -> WordRead | int:
    """A scalar word's name read as a value (not assigned to, not addressed)."""
    tokens = source.tokens
    after = tokens[index + 1].text if index + 1 < len(tokens) else ""
    if after in ("=", "+=", "-=", "|=", "&=", "++", "--") or tokens[index - 1].text in ("&", "++", "--"):
      return -1
    return WordRead(Span(index, index + 1), self.named.index(tokens[index].text))

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
    accessor = self.accessors.get(name)
    if accessor is not None and len(arguments) == len(accessor.parameters):
      if not all(len(word) == 1 and IsIntegerLiteral(tokens[word.first]) for word in arguments):
        return -1
      literals = tuple(_Value(tokens[word.first].text) for word in arguments)
      words = tuple(literals[position] for position in accessor.order)
      return WordRead(Span(index, close + 1), words[0] if words else literals[0], WORD, name, literals, words)
    return None

  def _Names(self, tokens, arguments: list[Span]) -> bool:
    return (len(arguments[0]) == 1 and tokens[arguments[0].first].text == self.args and len(arguments[1]) == 1
            and tokens[arguments[1].first].text == self.count)


def FindAccessors(source: SourceFile, statements: list[Span], context: WordContext) -> dict[str, Accessor]:
  """The accessor lambdas declared among statements: `auto [const] NAME{ [..](T i, ...) [-> R] { return EXPR; } };` or
  the same with `= [..] ... ;`, whose EXPR reads the words or calls an accessor declared before it."""
  found: dict[str, Accessor] = {}
  for statement in statements:
    accessor = _Accessor(source, statement, context, found)
    if accessor is not None:
      found[accessor.name] = accessor
  return found


def _Accessor(source: SourceFile, statement: Span, context: WordContext, known: dict[str, Accessor]) -> Accessor | None:
  tokens, partners = source.tokens, source.partners
  words = [token.text for token in tokens[statement.first:statement.last]]
  if not words or words[0] != "auto":
    return None
  name_at = statement.first + (2 if len(words) > 1 and words[1] == "const" else 1)
  capture = next((i for i in range(name_at, statement.last) if tokens[i].text == "["), None)
  if tokens[name_at].kind != IDENTIFIER or capture is None or capture not in partners or tokens[partners[capture] + 1].text != "(":
    return None
  parameters = partners[capture] + 1
  names = tuple(tokens[part.last - 1].text for part in SplitTopLevel(tokens, Span(parameters + 1, partners[parameters]), ","))
  body = next((i for i in range(partners[parameters], statement.last) if tokens[i].text == "{"), None)
  if not names or body is None or tokens[body + 1].text != "return":
    return None
  expression = Span(body + 2, partners[body] - 1)
  order = _ReadOrder(source, expression, names, context, known)
  if order is None:
    return None
  near = any(t.text in NEAR_MARKS or t.text in known and known[t.text].near for t in tokens[expression.first:expression.last])
  return Accessor(tokens[name_at].text, names, expression, order, near)


def _ReadOrder(source: SourceFile, expression: Span, names: tuple[str, ...], context: WordContext,
               known: dict[str, Accessor]) -> tuple[int, ...] | None:
  """The parameter positions whose words the expression reads, in order; None when it reads no words."""
  tokens, partners = source.tokens, source.partners
  order: list[int] = []
  reads = False
  for index in range(expression.first, expression.last - 1):
    token = tokens[index]
    if token.kind != IDENTIFIER or tokens[index + 1].text != "(" or index + 1 not in partners:
      reads = reads or token.text == context.args
      continue
    arguments = [Compact(tokens, part) for part in Arguments(tokens, partners, index + 1)]
    if token.text in ("Arg", *WIDE_READS) and arguments[:1] == [context.args]:
      reads = True
      order += [names.index(arguments[-1])] if arguments[-1] in names else []
    elif token.text in known:
      reads = True
      inner = known[token.text]
      bound = [names.index(argument) if argument in names else None for argument in arguments]
      order += [bound[position] for position in inner.order if position < len(bound) and bound[position] is not None]
  return tuple(order) if reads else None


def _Member(tokens, index: int) -> bool:
  return index > 0 and tokens[index - 1].text in (".", "->")


def _Value(text: str) -> int:
  return int(text.rstrip("uUlL"), 0)
