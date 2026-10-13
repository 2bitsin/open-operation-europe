"""C-style casts of an inlined expression written as `static_cast`: `(uint8_t)x` -> `static_cast<uint8_t>(x)`,
`(TextHost*)port_textout_host()` -> `static_cast<TextHost*>(port_textout_host())`. Only a cast to a plain type
(an integer type, `void`, or a possibly const pointer to a named type, which a void* converts to) whose operand is a
postfix expression (a name with its calls, subscripts and member accesses, a named cast, a literal, or a parenthesised
expression) is rewritten; any other cast is left as written. A cast to a pointer to a character or integer type
reinterprets the bytes it points at (`(const char*)bytes.data()`), so it becomes a `reinterpret_cast`."""
from __future__ import annotations

import re

_TYPE = re.compile(r"\(\s*((?:const\s+)?(?:std::)?(?:u?int(?:8|16|32|64)_t|short|int|long|unsigned(?:\s+(?:short|int|char|long))?|"
                   r"signed\s+char|char|bool|void|[A-Z]\w*)(?:\s+const)?(?:\s*\*(?:\s*const)?)?)\s*\)")
_NAME = re.compile(r"[A-Za-z_][\w]*(?:\s*::\s*[A-Za-z_]\w*)*")
_LITERAL = re.compile(r"(?:0[xX][0-9a-fA-F]+|\d+)[uUlL]*")
_POINTER_FREE = re.compile(r"[A-Z]\w*")
_BYTE_POINTER = re.compile(r"(?:const\s+)?(?:std::)?(?:u?int(?:8|16|32|64)_t|short|int|long|unsigned(?:\s+\w+)?|signed\s+char|char)(?:\s+const)?\s*\*.*")
_NAMED_CASTS = ("static_cast", "reinterpret_cast", "const_cast")


def ModernCasts(text: str) -> str:
  """text with each convertible C-style cast written as a static_cast (innermost operands first)."""
  out, position = [], 0
  while (match := _NextCast(text, position)) is not None:
    start, type_text, operand_start = match
    end = _OperandEnd(text, operand_start)
    if end is None:
      out.append(text[position:operand_start])
      position = operand_start
      continue
    operand = ModernCasts(text[operand_start:end])
    out.append(text[position:start])
    kind = "reinterpret_cast" if _BYTE_POINTER.fullmatch(type_text.strip()) else "static_cast"
    out.append(f"{kind}<{' '.join(type_text.split())}>({_Unparenthesised(operand)})")
    position = end
  out.append(text[position:])
  return "".join(out)


def _NextCast(text: str, position: int) -> tuple[int, str, int] | None:
  """(start, type, operand start) of the next cast from position: a type in parentheses not preceded by a name or a
  closing bracket (that would be a call's argument list or a declarator) and followed by an operand."""
  for match in _TYPE.finditer(text, position):
    before = text[:match.start()].rstrip()
    if before and (before[-1].isalnum() or before[-1] in "_)]>"):
      continue
    type_text = match.group(1)
    if _POINTER_FREE.fullmatch(type_text.strip()):
      continue  # `(Name)` alone is as likely a parenthesised name as a cast to a class
    operand = match.end()
    while operand < len(text) and text[operand] == " ":
      operand += 1
    return match.start(), type_text, operand
  return None


def _OperandEnd(text: str, start: int) -> int | None:
  """The end of the postfix expression at start, or None when none starts there."""
  if start >= len(text):
    return None
  if text[start] == "(":
    end = _Closing(text, start)
    if end is not None and _TYPE.fullmatch(text, start, end):
      return None  # a cast of a cast: left as written
  elif (name := _NAME.match(text, start)) is not None:
    end = _NamedCastEnd(text, name) if name.group(0) in _NAMED_CASTS else name.end()
  elif (literal := _LITERAL.match(text, start)) is not None:
    return literal.end()
  else:
    return None
  if end is None:
    return None
  while True:
    if end < len(text) and text[end] in "([":
      closing = _Closing(text, end)
      if closing is None:
        return None
      end = closing
    elif (member := re.match(r"\s*(?:\.|->)\s*[A-Za-z_]\w*", text[end:])) is not None:
      end += member.end()
    else:
      return end


def _NamedCastEnd(text: str, name: re.Match) -> int | None:
  """The index just past the `<type>` of a named cast (its parenthesised operand follows), or None."""
  if name.end() >= len(text) or text[name.end()] != "<":
    return None
  depth = 0
  for index in range(name.end(), len(text)):
    depth += {"<": 1, ">": -1}.get(text[index], 0)
    if depth == 0:
      return index + 1 if text[index + 1:index + 2] == "(" else None
  return None


def _Closing(text: str, opening: int) -> int | None:
  """The index just past the bracket closing the one at opening."""
  pairs = {"(": ")", "[": "]"}
  depth = 0
  for index in range(opening, len(text)):
    if text[index] in pairs:
      depth += 1
    elif text[index] in pairs.values():
      depth -= 1
      if depth == 0:
        return index + 1
  return None


def _Unparenthesised(operand: str) -> str:
  if operand.startswith("(") and _Closing(operand, 0) == len(operand):
    return operand[1:-1]
  return operand
