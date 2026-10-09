#!/usr/bin/env python3
"""Inventory of the call boundaries of the recovered host branches.

For every src/main module this reports:
  - the host branch (the `#else` side of `#if defined(_MSC_VER) && _MSC_VER < 1000`),
  - the service-callback typedefs it takes,
  - macro pairs: a macro defined in both branches whose host side calls a
    service (event/service/EVENT/host->event) and whose DOS side names the
    real callee, giving `service id -> callee`,
  - service constants written as original routine addresses (0x2a09cUL)
    resolved to the routine names of port/gen/symbols.json,
  - every recovered routine name the module calls directly on the host.

Usage: port/tools/inventory.py [--json out.json] [--md out.md]
"""
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
AREAS = ('core', 'io', 'map', 'units', 'data')
COND = re.compile(r'^#if\s+defined\(_MSC_VER\)\s*&&\s*_MSC_VER\s*<\s*1000', re.M)


def split_branches(text):
    """Return (dos_text, host_text) concatenated over every conditional block."""
    dos, host = [], []
    lines = text.split('\n')
    state = None   # None, 'dos', 'host'
    depth = 0
    for line in lines:
        s = line.strip()
        if state is None:
            if COND.match(s):
                state, depth = 'dos', 0
            continue
        if s.startswith('#if'):
            depth += 1
        elif s.startswith('#endif'):
            if depth == 0:
                state = None
                continue
            depth -= 1
        elif s.startswith('#else') and depth == 0:
            state = 'host'
            continue
        (dos if state == 'dos' else host).append(line)
    return '\n'.join(dos), '\n'.join(host)


def macros(text):
    out = {}
    joined = re.sub(r'\\\n', ' ', text)
    for m in re.finditer(r'^\s*#\s*define\s+(\w+)(\([^)]*\))?\s*(.*)$', joined, re.M):
        out[m.group(1)] = ((m.group(2) or ''), m.group(3).strip())
    return out


SERVICE_CALL = re.compile(r'\b(?:event|service|EVENT|host->event|call\d?|setup_service|svc|ui)\s*\(\s*(?:ds\s*,\s*)?(?:host->memory\s*,\s*)?(?:event\s*,\s*)?(0x[0-9a-fA-F]+|\d+)')


def main(argv):
    symbols = json.loads((ROOT/'port/gen/symbols.json').read_text())
    by_address = {}
    for name, (seg, off) in symbols.items():
        if seg == 18194:
            continue
        by_address.setdefault(seg*16+off, []).append(name.lstrip('_$@'))
    host_funcs = set()
    modules = {}
    for area in AREAS:
        for path in sorted((ROOT/'src/main'/area).glob('*.c')):
            text = path.read_text(errors='replace')
            dos, host = split_branches(text)
            rec = {'path': str(path.relative_to(ROOT))}
            exports = []
            for m in re.finditer(r'#pragma\s+alloc_text\s*\(\s*\w+\s*,([^)]*)\)', text):
                exports += [n.strip() for n in m.group(1).split(',') if n.strip()]
            rec['alloc_text'] = exports
            rec['typedefs'] = [t.strip() for t in re.findall(r'typedef[^;]*\(\s*\*\s*\w+\s*\)[^;]*;', host)]
            dm, hm = macros(dos), macros(host)
            pairs = []
            for name, (params, body) in hm.items():
                sm = SERVICE_CALL.search(body)
                if sm and name in dm:
                    pairs.append({'macro': name + params, 'id': sm.group(1), 'host': body, 'dos': dm[name][1]})
            rec['pairs'] = pairs
            addrs = {}
            for m in re.finditer(r'\b0x([0-9a-fA-F]{4,5})UL\b', host):
                a = int(m.group(1), 16)
                if a in by_address:
                    addrs[hex(a)] = by_address[a]
            rec['address_services'] = addrs
            modules[path.stem if area != 'data' else 'data/' + path.stem] = rec
    if '--json' in argv:
        Path(argv[argv.index('--json')+1]).write_text(json.dumps(modules, indent=1))
    n_pairs = sum(len(r['pairs']) for r in modules.values())
    n_addr = sum(len(r['address_services']) for r in modules.values())
    print(f'{len(modules)} modules, {n_pairs} id->callee macro pairs, {n_addr} address services')
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
