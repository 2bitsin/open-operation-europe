#!/usr/bin/env python3
"""Linux lane B: the buildutil build of this checkout (conan SDL3, the C port
as a subdirectory, the C++ modules), then `buildutil test`.

Runs inside arm64v8/ubuntu:24.04 with the checkout at /src and the buildutil
checkout at /buildutil, both read-only; everything is built in a copy under
/work. Started by ci/linux/lane.py. Passed 2026-10-09 (docs/buildutil-notes.md,
"Linux")."""
import os
import shutil
import subprocess
import sys
from pathlib import Path

SRC = Path('/src')
BUILDUTIL_SRC = Path('/buildutil')
WORK = Path('/work')
VENV = Path('/opt/buildutil')
# toolchain: g++-14 for <print> (libstdc++ 13 lacks it)
TOOLCHAIN = 'build-essential g++-14 gcc-14 cmake ninja-build git python3 python3-venv python3-pip pkg-config rsync'.split()
# SDL3's system-side requirements through conan's */system recipes
# (xorg/system, opengl/system, egl/system, libudev/system)
SYSTEM_DEV = (
    'libx11-dev libx11-xcb-dev libfontenc-dev libice-dev libsm-dev libxau-dev '
    'libxaw7-dev libxcomposite-dev libxcursor-dev libxdamage-dev libxdmcp-dev '
    'libxext-dev libxfixes-dev libxi-dev libxinerama-dev libxkbfile-dev libxmu-dev '
    'libxmuu-dev libxpm-dev libxrandr-dev libxrender-dev libxres-dev libxss-dev '
    'libxt-dev libxtst-dev libxv-dev libxxf86vm-dev libxcb-glx0-dev '
    'libxcb-render0-dev libxcb-render-util0-dev libxcb-xkb-dev libxcb-icccm4-dev '
    'libxcb-image0-dev libxcb-keysyms1-dev libxcb-randr0-dev libxcb-shape0-dev '
    'libxcb-sync-dev libxcb-xfixes0-dev libxcb-xinerama0-dev libxcb-dri3-dev '
    'uuid-dev libxcb-cursor-dev libxcb-dri2-0-dev libxcb-present-dev '
    'libxcb-composite0-dev libxcb-ewmh-dev libxcb-res0-dev libxcb-util-dev '
    'libgl-dev libegl-dev libudev-dev xkb-data').split()
EXCLUDES = ('/_*', '/build/', '/port/build/', '/.venv', '/.git', '/.vscode', '/.clangd')


def run(*args, **kw):
    print('+', ' '.join(str(a) for a in args), flush=True)
    subprocess.run([str(a) for a in args], check=True, **kw)


def main():
    env = dict(os.environ, DEBIAN_FRONTEND='noninteractive')
    run('apt-get', 'update', '-qq', env=env)
    run('apt-get', 'install', '-y', '-qq', *TOOLCHAIN, env=env, stdout=subprocess.DEVNULL)
    run('apt-get', 'install', '-y', '-qq', *SYSTEM_DEV, env=env, stdout=subprocess.DEVNULL)
    run('update-alternatives', '--install', '/usr/bin/gcc', 'gcc', '/usr/bin/gcc-14', '100',
        '--slave', '/usr/bin/g++', 'g++', '/usr/bin/g++-14')
    for probe in (('sh', '-c', 'head -2 /etc/os-release'), ('gcc', '--version'), ('uname', '-m')):
        run(*probe)

    run(sys.executable, '-m', 'venv', VENV)
    shutil.copytree(BUILDUTIL_SRC, '/tmp/buildutil-src', symlinks=True)
    run(VENV/'bin/pip', 'install', '-q', '/tmp/buildutil-src', 'cmake')
    # Ubuntu 24.04 cmake (3.28) has no GNU flags for the profile cppstd=26
    # that buildutil writes; the venv cmake (4.x) does, and it comes first.
    env = dict(os.environ, PATH=f'{VENV}/bin:{os.environ["PATH"]}')
    run('buildutil', '--version', env=env)

    WORK.mkdir(parents=True, exist_ok=True)
    run('rsync', '-a', *(f'--exclude={e}' for e in EXCLUDES), f'{SRC}/', f'{WORK}/')
    os.chdir(WORK)
    # a commit for buildutil's build identity (the stamp reads git describe)
    run('git', 'init', '-q')
    run('git', 'add', '-A')
    run('git', '-c', 'user.name=lane', '-c', 'user.email=lane@local', 'commit', '-q', '-m', 'linux lane snapshot')
    run(sys.executable, '-m', 'venv', '.venv')
    run('.venv/bin/pip', 'install', '-q', 'unicorn')

    run('./buildutil', '--write-log-to=/tmp/build.log', 'build', '--watchdog-budget', '5400', env=env)
    run('./buildutil', '--write-log-to=/tmp/test.log', 'test', '--watchdog-budget', '1800', env=env)
    run('ls', '-la', '_install')
    return 0


if __name__ == '__main__':
    sys.exit(main())
