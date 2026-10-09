#!/usr/bin/env python3
"""Provenance of the opening in the port (the OPEN.EXE counterpart of
tools/port_lint.py's rule for MAIN):

  1. every recovered src/open C module (core, data, io) is compiled into the
     build (compile_commands.json);
  2. no file under port/ defines a routine that a src/open module defines
     (its #pragma alloc_text names, or the link name the forced include
     port/open/open_names.h gives it);
  3. port/open/open_names.h and port/open/open_image.c are what their
     generators produce from the current src/open (--check).

Usage: check_open_provenance.py <compile_commands.json>
"""
import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
PORT = ROOT/'port'


def main(argv):
    problems = []
    entries = json.loads(Path(argv[1]).read_text())
    compiled = {str(Path(e['file']).resolve()) for e in entries}
    modules = sorted(p for d in ('core', 'data', 'io') for p in (ROOT/'src/open'/d).glob('*.c'))
    missing = [p for p in modules if str(p.resolve()) not in compiled]
    print(f'open provenance: {len(modules) - len(missing)} of {len(modules)} recovered src/open modules compiled')
    for p in missing:
        problems.append(f'{p.relative_to(ROOT)} is not compiled into the port')

    routines = set()
    for p in modules:
        for m in re.finditer(r'#pragma\s+alloc_text\s*\(\s*\w+\s*,([^)]*)\)', p.read_text(errors='replace')):
            routines.update(n.strip() for n in m.group(1).split(',') if n.strip())
    names = routines | {'open_' + n for n in routines}
    definition = re.compile(r'^[A-Za-z_][\w \t\*]*?\b(\w+)\s*\([^;{}]*\)\s*\{', re.M)
    files = [p for p in PORT.rglob('*.c') if 'build' not in p.relative_to(PORT).parts]
    for p in files:
        text = re.sub(r'/\*.*?\*/|//[^\n]*', '', p.read_text(errors='replace'), flags=re.S)
        for m in definition.finditer(text):
            if m.group(1) in names:
                problems.append(f'{p.relative_to(ROOT)} defines {m.group(1)}, a recovered src/open routine')
    print(f'open provenance: {len(routines)} recovered src/open routines; none may be defined in port/ ({len(files)} files)')

    for tool in ('gen_open_names.py', 'gen_open_image.py'):
        r = subprocess.run([sys.executable, str(PORT/'open/tools'/tool), '--check'], capture_output=True, text=True)
        if r.returncode:
            problems.append(f'{tool} --check: {(r.stdout + r.stderr).strip()}')
        else:
            print(f'open provenance: {tool} --check ok')

    for p in problems:
        print('FAIL  ' + p)
    print('open provenance: ' + ('PASS' if not problems else f'{len(problems)} problem(s)'))
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
