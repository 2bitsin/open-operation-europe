#!/usr/bin/env python3
"""Run a Linux lane of this checkout in a Docker container.

    python3 ci/linux/lane.py port       # lane A: plain CMake port + ctests
    python3 ci/linux/lane.py buildutil  # lane B: buildutil build + test
                                        #   (--buildutil-src <checkout> required)

The checkout is mounted read-only at /src (and the buildutil checkout at
/buildutil for lane B). The ubuntu:24.04 base image has no Python, so the
container's only shell command line installs python3 and hands over to the
lane's Python script; everything else is Python."""
import argparse
import platform
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
LANES = {'port': 'port_cmake.py', 'buildutil': 'buildutil_lane.py'}


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('lane', choices=sorted(LANES))
    ap.add_argument('--image', default=None, help='Docker image (default: ubuntu:24.04 for the host architecture)')
    ap.add_argument('--buildutil-src', type=Path, help='buildutil checkout to install in lane B')
    ap.add_argument('--name', default='opeu-linux-lane', help='container name')
    a = ap.parse_args()
    image = a.image or ('arm64v8/ubuntu:24.04' if platform.machine() in ('arm64', 'aarch64') else 'ubuntu:24.04')
    mounts = ['-v', f'{ROOT}:/src:ro']
    if a.lane == 'buildutil':
        if not a.buildutil_src:
            ap.error('lane buildutil needs --buildutil-src')
        mounts += ['-v', f'{a.buildutil_src.resolve()}:/buildutil:ro']
    bootstrap = ('export DEBIAN_FRONTEND=noninteractive; apt-get update -qq && '
                 'apt-get install -y -qq python3 >/dev/null && exec python3 /src/ci/linux/' + LANES[a.lane])
    cmd = ['docker', 'run', '--rm', '--name', a.name, *mounts, image, 'sh', '-c', bootstrap]
    print('+', ' '.join(cmd), flush=True)
    return subprocess.run(cmd).returncode


if __name__ == '__main__':
    sys.exit(main())
