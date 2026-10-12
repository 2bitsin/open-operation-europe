"""`python buildutil_wine_link.py <tool> <args>`: the wine-msvc lane's link and archive launcher. Wine refuses a
command line over 32,767 characters and msvc-wine's wrapper never looks inside a response file, so the arguments
go into one, translated as the wrapper translates a command line."""
import os
import re
import subprocess
import sys
from pathlib import Path

# wine-msvc.sh's `case` over each argument, in its order; each yields the path part.
_PATH_PART = (
  (re.compile(r"[-/][A-Za-z]/.*", re.S), lambda argument: argument[2:]),
  (re.compile(r"[-/][A-Za-z]{2}/.*", re.S), lambda argument: argument[3:]),
  (re.compile(r"[-/][A-Za-z]{3}.*:/.*", re.S), lambda argument: argument.split(":", 1)[1]),
  (re.compile(r"/.*", re.S), lambda argument: argument),
)


def dirname(path: str) -> str:
  """POSIX dirname(1), which the wrapper's test runs on."""
  stripped = path.rstrip("/")
  if not stripped:
    return "/" if path else "."
  if "/" not in stripped:
    return "."
  return stripped[:stripped.rindex("/")].rstrip("/") or "/"


def wine_argument(argument: str) -> str:
  """The argument as msvc-wine's wrapper hands it to wine: its path part `z:`-prefixed if its parent is a dir but /."""
  path = next((part(argument) for pattern, part in _PATH_PART if pattern.fullmatch(argument)), "")
  parent = dirname(path)
  if not path or parent == "/" or not os.path.isdir(parent):
    return argument
  return argument[:len(argument) - len(path)] + "z:" + path


def output_of(arguments: list[str]) -> str:
  """The /OUT: value CMake gives every link and archive command."""
  found = [argument[5:] for argument in arguments if argument[:5].lower() in ("/out:", "-out:")]
  if not found:
    raise SystemExit(f"buildutil_wine_link: no /OUT: in {arguments}")
  return found[-1]


def response_line(argument: str) -> str:
  """One argument, quoted by the MSVC runtime's rules where it needs quoting."""
  return subprocess.list2cmdline([argument])


def launch(command: list[str]) -> list[str]:
  """Writes the response file beside the output and returns the command that reads it."""
  tool, *arguments = command
  if any(argument.startswith("@") for argument in arguments):
    raise SystemExit(f"buildutil_wine_link: a response file on the line is not translated: {arguments}")
  response = Path(output_of(arguments) + ".rsp")
  # UTF-16 with a byte-order mark is the one encoding link and lib read without a code page.
  response.write_text("\ufeff" + "".join(response_line(wine_argument(argument)) + "\n" for argument in arguments),
                      encoding="utf-16-le")
  return [tool, "@" + wine_argument(str(response))]


if __name__ == "__main__":
  sys.exit(subprocess.run(launch(sys.argv[1:])).returncode)
