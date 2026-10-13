"""C++ tokens of a clang-formatted source: identifiers, numbers, strings, characters and punctuators, with their offsets.

Comments and preprocessor lines produce no tokens; every token keeps its byte range, so a rewrite can splice the text.
"""
from __future__ import annotations

import re
from dataclasses import dataclass

IDENTIFIER = "identifier"
NUMBER = "number"
STRING = "string"
CHARACTER = "character"
PUNCTUATOR = "punctuator"

_PUNCTUATORS = sorted(["<<=", ">>=", "...", "->*", "<=>", "::", "->", "++", "--", "<<", ">>", "<=", ">=", "==", "!=", "&&", "||", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", ".*"],
                      key=len, reverse=True)
_SCANNER = re.compile(
  r"(?P<space>\s+)"
  r"|(?P<line_comment>//[^\n]*)"
  r"|(?P<block_comment>/\*.*?\*/)"
  r"|(?P<raw_string>(?:u8|u|U|L)?R\"(?P<delimiter>[^(\s]*)\(.*?\)(?P=delimiter)\")"
  r"|(?P<string>(?:u8|u|U|L)?\"(?:\\.|[^\"\\\n])*\")"
  r"|(?P<character>(?:u8|u|U|L)?'(?:\\.|[^'\\\n])+')"
  r"|(?P<number>\.?[0-9](?:[eEpP][+-]|['.0-9A-Za-z_])*)"
  r"|(?P<identifier>[A-Za-z_]\w*)"
  r"|(?P<punctuator>" + "|".join(re.escape(p) for p in _PUNCTUATORS) + r"|[^\s])",
  re.S)
_DIRECTIVE = re.compile(r"[ \t]*#(?:[^\n\\]|\\.)*", re.S)


@dataclass(frozen=True)
class Token:
  kind:  str
  text:  str
  start: int
  end:   int
  line:  int

  def Is(self, text: str) -> bool:
    return self.text == text


def Tokenize(text: str) -> list[Token]:
  """The tokens of a translation unit's text (preprocessor directives skipped)."""
  tokens: list[Token] = []
  position, line, at_line_start = 0, 1, True
  while position < len(text):
    if at_line_start:
      directive = _DIRECTIVE.match(text, position)
      if directive and directive.group().strip().startswith("#"):
        line += directive.group().count("\n")
        position = directive.end()
        continue
    match = _SCANNER.match(text, position)
    kind = match.lastgroup if match.lastgroup != "delimiter" else "raw_string"
    if kind in ("raw_string", "string"):
      kind = STRING
    if kind in (IDENTIFIER, NUMBER, STRING, CHARACTER, PUNCTUATOR):
      tokens.append(Token(kind, match.group(), match.start(), match.end(), line))
    newlines = match.group().count("\n")
    line += newlines
    at_line_start = newlines > 0 if kind in ("space", "line_comment", "block_comment") else False
    position = match.end()
  return tokens
