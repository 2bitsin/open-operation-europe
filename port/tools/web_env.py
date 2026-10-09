#!/usr/bin/env python3
"""Run a command with the environment an unconfigured emscripten install needs.

    python3 port/tools/web_env.py emcmake cmake -S port -B port/build/web -DCMAKE_BUILD_TYPE=Release
    python3 port/tools/web_env.py cmake --build port/build/web -j8
    python3 port/tools/web_env.py            # print the values and exit

It only sets environment variables for the command it runs:
  EMSDK_PYTHON  a Python >= 3.10 (emscripten 6 refuses older ones; macOS'
                /usr/bin/python3 is 3.9 and may come first in PATH)
  EM_CONFIG     port/build/emscripten.config, written here when the install
                has no config of its own (Homebrew's emscripten ships LLVM
                and Binaryen under libexec but no .emscripten)
  EM_CACHE      port/build/emcache, so the system libraries and the SDL3
                port are built inside the checkout, not in the install
Existing values of these variables are kept.
"""
from __future__ import annotations

import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
BUILD = ROOT / "port" / "build"


def find_python() -> str | None:
    for name in ("python3.14", "python3.13", "python3.12", "python3.11", "python3.10", "python3"):
        exe = shutil.which(name)
        if not exe:
            continue
        probe = subprocess.run(
            [exe, "-c", "import sys; sys.exit(sys.version_info < (3, 10))"],
            capture_output=True,
        )
        if probe.returncode == 0:
            return exe
    return None


def emscripten_libexec(emcc: str) -> Path:
    """The real emscripten tree behind a (possibly wrapped) emcc launcher."""
    try:
        text = Path(emcc).read_text(errors="replace")
    except OSError:
        text = ""
    match = re.search(r'exec "([^"]*)/emcc"', text)
    if match:
        return Path(match.group(1))
    return Path(os.path.realpath(emcc)).parent


def find_config() -> str | None:
    emcc = shutil.which("emcc")
    if not emcc:
        return None
    lib = emscripten_libexec(emcc)
    if (lib / ".emscripten").is_file():
        return None
    if not ((lib / "llvm" / "bin").is_dir() and (lib / "binaryen").is_dir()):
        return None
    BUILD.mkdir(parents=True, exist_ok=True)
    config = BUILD / "emscripten.config"
    config.write_text(
        f"LLVM_ROOT = '{lib / 'llvm' / 'bin'}'\n"
        f"BINARYEN_ROOT = '{lib / 'binaryen'}'\n"
        f"NODE_JS = '{shutil.which('node') or 'node'}'\n"
    )
    return str(config)


def web_environment() -> dict[str, str]:
    env = dict(os.environ)
    if not env.get("EMSDK_PYTHON"):
        python = find_python()
        if python:
            env["EMSDK_PYTHON"] = python
    if not env.get("EM_CONFIG"):
        config = find_config()
        if config:
            env["EM_CONFIG"] = config
    env.setdefault("EM_CACHE", str(BUILD / "emcache"))
    return env


def main(argv: list[str]) -> int:
    env = web_environment()
    print(
        "web_env: EMSDK_PYTHON={} EM_CONFIG={} EM_CACHE={}".format(
            env.get("EMSDK_PYTHON", "<unset>"),
            env.get("EM_CONFIG", "<install default>"),
            env["EM_CACHE"],
        ),
        file=sys.stderr,
    )
    if argv and argv[0] == "--":
        argv = argv[1:]
    if not argv:
        for key in ("EMSDK_PYTHON", "EM_CONFIG", "EM_CACHE"):
            if key in env:
                print(f"{key}={env[key]}")
        return 0
    return subprocess.call(argv, env=env)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
