"""The skip rule: a skip is a defect, except "no game data: ..." in a clone without the game data (the public mirror);
in the private repository nothing skips. GitLab's jobs run it with --none, the public mirror's with --game-data.

    python3 tools/testing/skips.py --log LOG --none        any skip in LOG (./buildutil test's log) fails
    python3 tools/testing/skips.py --log LOG --game-data   skips are allowed, and every skip in the sources goes through
                                                      the game-data helpers (so its message is "no game data: ...")

The game-data helpers (sources/opeu/game-data.test.hpp, tools/testing/game_data.py) are the only places a test may
skip from; a GTEST_SKIP elsewhere must stream NoGameData(...), and no Python test calls pytest.skip or a skip mark
itself.
"""
from __future__ import annotations

import argparse
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
HELPERS = {'sources/opeu/game-data.test.hpp', 'tools/testing/game_data.py', 'tools/testing/skips.py'}
CTEST_SKIPPED = re.compile(r'^\s*\d+ - (\S+) \(Skipped\)', re.M)
PYTEST_SKIPPED = re.compile(r'^pytest (\S+): .*?\b(\d+) skipped', re.M)
GTEST_SKIP = re.compile(r'GTEST_SKIP\(\)(?!.*NoGameData\()')
PYTEST_SKIP = re.compile(r'pytest\.skip\(|pytest\.mark\.skip|skipif\(|unittest\.skip')


def skipped(log: str) -> list[str]:
    """The skipped ctest entries and the pytest suites with skipped cases, as named in the log."""
    return [f'ctest {name}' for name in CTEST_SKIPPED.findall(log)] + [f'pytest {suite}: {count} skipped' for suite, count in PYTEST_SKIPPED.findall(log)]


def stray_skips(files: dict[str, str]) -> list[str]:
    """Skips in the given sources (path -> text) that do not go through the game-data helpers."""
    return [f'{path}:{number}: {line.strip()}' for path, text in sorted(files.items()) if path not in HELPERS
            for number, line in enumerate(text.splitlines(), 1)
            if (path.endswith(('.cpp', '.hpp')) and GTEST_SKIP.search(line)) or (path.endswith('.py') and PYTEST_SKIP.search(line))]


def tracked_sources() -> dict[str, str]:
    names = subprocess.run(['git', 'ls-files', 'sources', 'tools'], cwd=ROOT, capture_output=True, text=True, check=True).stdout.split()
    return {name: (ROOT / name).read_text(errors='replace') for name in names
            if name.endswith(('.cpp', '.hpp', '.py')) and (ROOT / name).is_file()}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--log', type=Path, required=True)
    rule = parser.add_mutually_exclusive_group(required=True)
    rule.add_argument('--none', action='store_true', help='the private repository: nothing may skip')
    rule.add_argument('--game-data', action='store_true', help='a clone without the game data: only its skips')
    args = parser.parse_args()
    found = skipped(args.log.read_text(errors='replace'))
    stray = stray_skips(tracked_sources())
    for line in stray:
        print(f'skips: a skip outside the game-data helpers: {line}')
    if args.none:
        for line in found:
            print(f'skips: skipped in a clone that holds the game data: {line}')
        print(f'skips: {len(found)} skipped (none allowed)')
        return 1 if found or stray else 0
    print(f'skips: {len(found)} skipped, every one through the game-data helpers' if not stray else 'skips: stray skips')
    return 1 if stray else 0


if __name__ == '__main__':
    sys.exit(main())
