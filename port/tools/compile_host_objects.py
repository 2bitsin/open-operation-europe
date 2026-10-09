#!/usr/bin/env python3
"""Compile every recovered host branch to an object for the symbol inventory.

    python3 port/tools/compile_host_objects.py [OUT_DIR]

OUT_DIR defaults to port/build/hostobj. Each src/main/<area>/<module>.c is
compiled with the host C compiler as <area>_<module>.o; compiler output of a
failing module is kept next to it as <area>_<module>.err.
"""
from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
AREAS = ("core", "data", "map", "units", "io")


def main(argv: list[str]) -> int:
    out = Path(argv[0]) if argv else ROOT / "port" / "build" / "hostobj"
    out.mkdir(parents=True, exist_ok=True)
    cc = os.environ.get("CC", "cc")
    failures = 0
    for area in AREAS:
        for source in sorted((ROOT / "src" / "main" / area).glob("*.c")):
            name = f"{area}_{source.stem}"
            result = subprocess.run(
                [cc, "-std=c99", "-Wall", "-Wextra", "-Werror",
                 "-I", str(ROOT / "src" / "common"),
                 "-I", str(ROOT / "src" / "main" / "include"),
                 "-c", str(source), "-o", str(out / f"{name}.o")],
                capture_output=True, text=True,
            )
            (out / f"{name}.err").write_text(result.stderr)
            if result.returncode != 0:
                failures += 1
                print(f"FAIL {source}")
    print(f"failures: {failures}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
