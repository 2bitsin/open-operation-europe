#!/usr/bin/env python3
"""Linux lane A: the C port exactly as `make port` builds it (plain CMake,
SDL3 through find_package or the FetchContent fallback), then its ctests.

Runs inside ubuntu:24.04 with the checkout mounted read-only at /src; the
host side starts it through ci/linux/lane.py, which installs python3 into
the fresh container first (the base image has none). Passed 2026-10-09
(docs/buildutil-notes.md, "Linux")."""
import os
import shutil
import subprocess
import sys
from pathlib import Path

SRC = Path('/src')
WORK = Path('/work')
PACKAGES = (
    'build-essential cmake ninja-build git python3 python3-venv pkg-config rsync '
    'libasound2-dev libpulse-dev libx11-dev libxext-dev libxrandr-dev '
    'libxcursor-dev libxfixes-dev libxi-dev libxss-dev libxtst-dev '
    'libxkbcommon-dev libdrm-dev libgbm-dev libgl1-mesa-dev libgles2-mesa-dev '
    'libegl1-mesa-dev libdbus-1-dev libibus-1.0-dev libudev-dev '
    'libpipewire-0.3-dev libwayland-dev libdecor-0-dev').split()
EXCLUDES = ('/_*', '/build/', '/port/build/', '/.venv', '/.git')


def run(*args, **kw):
    print('+', ' '.join(str(a) for a in args), flush=True)
    subprocess.run([str(a) for a in args], check=True, **kw)


def main():
    env = dict(os.environ, DEBIAN_FRONTEND='noninteractive')
    run('apt-get', 'update', '-qq', env=env)
    run('apt-get', 'install', '-y', '-qq', *PACKAGES, env=env, stdout=subprocess.DEVNULL)
    for probe in (('sh', '-c', 'head -2 /etc/os-release'), ('gcc', '--version'), ('cmake', '--version')):
        run(*probe)
    WORK.mkdir(parents=True, exist_ok=True)
    run('rsync', '-a', *(f'--exclude={e}' for e in EXCLUDES), f'{SRC}/', f'{WORK}/')
    os.chdir(WORK)
    # the FM driver comparison needs a Python with unicorn, as on the Mac
    run(sys.executable, '-m', 'venv', '.venv')
    run('.venv/bin/pip', 'install', '-q', 'unicorn')
    run('cmake', '-S', 'port', '-B', 'port/build/release', '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=RelWithDebInfo')
    run('cmake', '--build', 'port/build/release')
    run('ctest', '--test-dir', 'port/build/release', '--output-on-failure')
    return 0


if __name__ == '__main__':
    sys.exit(main())
