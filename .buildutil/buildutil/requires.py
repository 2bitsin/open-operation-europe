"""The Require() lines of sources/CMakeLists.txt, for buildutil and conanfile.py."""

import hashlib
import re
from pathlib import Path

REQUIRE_RE = re.compile(
  # Package name permits letters, digits, underscores, and hyphens
  # (yaml-cpp et al. aren't \w-only).
  r'^\s*Require\s*\(\s*([\w-]+)\s+VERSION\s+"([^"]+)"((?:"[^"]*"|[^")])*)\)',
  re.MULTILINE | re.DOTALL,
)
# One cmake argument; a quoted one keeps its spaces and parentheses.
ARGUMENT_RE = re.compile(r'"([^"]*)"|([^\s"]+)')
QUOTED_OR_COMMENT_RE = re.compile(r'("[^"]*")|#[^\n]*')
REQUIRE_START_RE = re.compile(r'^[ \t]*(Require)\s*\(', re.MULTILINE)


FLAGS = {"TEST": "test", "BENCH": "bench", "TOOL": "tool", "SYSTEM": "system",
         "FORCE": "force", "PUBLIC": "public"}
LISTS = {"CONAN": "conan", "COMPONENTS": "components", "PLATFORM": "platform",
         "OPTIONS": "options"}
KEYWORDS = FLAGS.keys() | LISTS.keys()


def _coerce_option(value: str):
  """A conan option value as its natural type: bool, int, else the text."""
  if value in ("True", "False"):
    return value == "True"
  try:
    return int(value)
  except ValueError:
    return value


def _keyword_values(tokens: list[str]) -> dict[str, list[str]]:
  """Each keyword's value tokens, in exact case as cmake_parse_arguments reads them."""
  values, current = {}, None
  for token in tokens:
    if token in KEYWORDS:
      current = token
      values.setdefault(token, [])
    elif current in LISTS:
      values[current].append(token)
  return values


def _options(tokens: list[str]) -> dict:
  """OPTIONS key=value tokens as a dict."""
  options = {}
  for token in tokens:
    key, eq, value = token.partition("=")
    if not eq or not key:
      # a dropped typo would build the package with the wrong defaults
      raise ValueError(f"Require OPTIONS token {token!r} is not key=value")
    options[key] = _coerce_option(value)
  return options


def arguments(extra: str) -> list[str]:
  """The cmake arguments of a Require call's text, quotes removed."""
  return [quoted or bare for quoted, bare in ARGUMENT_RE.findall(extra)]


def parse_extra(extra: str) -> dict:
  """The keywords after Require(NAME VERSION "v") as one record."""
  values = _keyword_values(arguments(extra))
  out = {field: keyword in values for keyword, field in FLAGS.items()}
  components = values.get("COMPONENTS", [])
  out["conan"] = next(iter(values.get("CONAN", [])), None)
  out["components"] = [token for token in components if token[:1] not in "+-"]
  out["platform"] = values.get("PLATFORM", [])
  out["options"] = _options(values.get("OPTIONS", []))
  _apply_sugar(out, [token for token in components if token[:1] in "+-"])
  if out["force"] and not out["system"]:
    raise ValueError(
      "Require FORCE without SYSTEM: FORCE takes the host's package over "
      "the conan pins it replaces, and only a SYSTEM dep is the host's")
  return out


def _apply_sugar(out: dict, sugar: list) -> None:
  """COMPONENTS +name / -name tokens as the with_name / without_name options they mean."""
  signs = {}
  for token in sugar:
    sign, name = token[0], token[1:]
    if not name:
      raise ValueError(
        f"Require COMPONENTS token {token!r} names no option")
    if signs.setdefault(name, sign) != sign:
      raise ValueError(
        f"Require COMPONENTS has both '+{name}' and '-{name}'")
    for spelling in (f"with_{name}", f"without_{name}"):
      if spelling in out["options"]:
        raise ValueError(
          f"Require COMPONENTS {token!r} and OPTIONS "
          f"{spelling}={out['options'][spelling]!r} both set an option "
          f"for {name!r}")
    # whether the recipe has the option is conan's check, by name
    out["options"]["with_" + name if sign == "+" else "without_" + name] = True
  if sugar and out["system"]:
    raise ValueError(
      "Require COMPONENTS option shorthand with SYSTEM is meaningless — "
      "a SYSTEM dep is the host's package and conan never builds it")


def to_conan_version(v: str) -> str:
  v = v.strip()
  if v == "*":
    return "[*]"
  ops = (">=", "<=", ">", "<", "~", "^")
  if not any(v.startswith(op) for op in ops):
    return v
  # the major cap keeps "cci.20210126"-style versions, which sort above releases, out of range
  m = re.match(r"^[><=~^]+\s*(\d+)\.", v)
  if m:
    major = int(m.group(1))
    return f"[{v} <{major + 1}]"
  return f"[{v}]"


def strip_comments(text: str) -> str:
  """cmake's text with each # line comment outside quotes removed."""
  return QUOTED_OR_COMMENT_RE.sub(lambda match: match.group(1) or "", text)


def _calls(text: str, path: str) -> list[tuple[str, str, str]]:
  """(name, version, extra) of every Require call."""
  calls = []
  for start in REQUIRE_START_RE.finditer(text):
    call = REQUIRE_RE.match(text, start.start())
    if call is None:
      # a skipped call would drop its package unnoticed
      line = text.count("\n", 0, start.start(1)) + 1
      raise ValueError(
        f"{path}:{line}: this Require call does not read as "
        'Require(NAME VERSION "v" ...) with balanced quotes')
    calls.append(call.groups())
  return calls


def requires(text: str, target_os: str | None = None,
             path: str = "sources/CMakeLists.txt") -> list[dict]:
  """The Require() calls active on target_os, or on any platform if None."""
  entries = []
  for name, version, extra in _calls(strip_comments(text), path):
    info = parse_extra(extra)
    if target_os and info["platform"] and target_os not in info["platform"]:
      continue
    conan = info["conan"] or name.lower()
    entries.append({
      "cmake_name": name,
      "conan_name": conan,
      "version": to_conan_version(version),
      "floor": version,
      "ref": (f"{conan}/system@host" if info["system"]
              else f"{conan}/{to_conan_version(version)}"),
      **info,
    })
  return entries


def wrapper_recipe(root: Path, conan: str) -> Path:
  """Where the driver renders conan's system@host recipe for a project."""
  return root / "_bdudata" / "host" / conan / "conanfile.py"


def recipe_revision(recipe: bytes) -> str:
  """conan's hash revision of an export holding only this conanfile.py."""
  summary = f"conanfile.py: {hashlib.md5(recipe).hexdigest()}\n"
  return hashlib.md5(summary.encode()).hexdigest()
