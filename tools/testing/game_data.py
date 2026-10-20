"""The game's data in the Python suites, as sources/opeu/game-data.test.hpp gives it to the gtests.

The private repository holds the 26 data files, the classic pack and the goldens and fixtures made from them, so
nothing skips there. A clone without them (the public mirror) skips exactly the tests that need them, each with
"no game data: <what>", unless OPEU_DATA_DIR names a directory with the player's own 26 files; such a directory must
hold every one of them, or the tests fail. No test skips any other way (tools/testing/skips.py checks the sources for it).

A suite imports it after putting tools/testing on sys.path:

    sys.path.insert(0, str(ROOT / 'tools' / 'testing'))
    import game_data
"""
from __future__ import annotations

import os
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
NO_GAME_DATA = 'no game data: '
DATA_DIRECTORY_VARIABLE = 'OPEU_DATA_DIR'
IN_TREE_DATA = ROOT / 'sources' / 'opeu' / 'opeu' / 'assets.embed'
IN_TREE_CLASSIC_PACK = ROOT / 'sources' / 'opeu' / 'opeu' / 'packs.embed' / 'classic'
DATA_FILES_HEADER = ROOT / 'sources' / 'opeu' / 'assets' / 'data-files.hpp'


def pytest():
    """pytest, imported when a test needs it: play.py imports this module outside a test run too (the CI driver's
    smokes, with a Python that has no pytest)."""
    import pytest as module  # noqa: PLC0415
    return module


def data_file_names() -> list[str]:
    """The 26 names, from the table the game checks a directory against (assets/data-files.hpp)."""
    return re.findall(r'\.name = "([^"]+)"sv', DATA_FILES_HEADER.read_text())


def missing_files(directory: Path) -> list[str]:
    """The data files `directory` lacks, matched by name in any case."""
    present = {path.name.upper() for path in directory.iterdir() if path.is_file()} if directory.is_dir() else set()
    return [name for name in data_file_names() if name not in present]


def data_directory() -> Path | None:
    """OPEU_DATA_DIR, else the private repository's data files; None when the clone has neither. A named directory
    that lacks a file fails the test."""
    named = os.environ.get(DATA_DIRECTORY_VARIABLE)
    if named:
        missing = missing_files(Path(named))
        if missing:
            pytest().fail(f'{DATA_DIRECTORY_VARIABLE}={named} lacks {", ".join(missing)}')
        return Path(named)
    return IN_TREE_DATA if IN_TREE_DATA.is_dir() else None


def require_data() -> Path:
    """The directory of the 26 data files, or the test skips. A suite that runs `opeu` calls it first: the private
    repository's build embeds the files, and a build without them finds OPEU_DATA_DIR's, which the game inherits."""
    directory = data_directory()
    if directory is None:
        pytest().skip(no_game_data(f'the 26 data files (set {DATA_DIRECTORY_VARIABLE} to a directory holding them)'))
    return directory


def require_file(path: Path, what: str) -> Path:
    """A file of the private repository made from the game's data (a golden, a fixture, the classic pack), or the
    test skips."""
    if not path.exists():
        pytest().skip(no_game_data(what))
    return path


def no_game_data(what: str) -> str:
    """The one skip message."""
    return f'{NO_GAME_DATA}{what}'

