#!/usr/bin/env python3
"""Write port/gen/index.txt: for every recovered host function, its module,
original routine identifier, the link callback(s) of its module and its host
prototype. A lookup aid for writing port/link files.

Usage: port/tools/gen_index.py
"""
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def main():
    protos = (ROOT/'port/gen/recovered.h').read_text().splitlines()
    symbols = json.loads((ROOT/'port/gen/symbols.json').read_text())
    address = {}
    for raw, (seg, off) in symbols.items():
        if seg != 18194:
            address[re.sub(r'^(\$@|@|_)', '', raw)] = f'{seg:04x}:{off:04x}'
    callbacks = {}
    for entry in json.loads((ROOT/'port/gen/callback_types.json').read_text()):
        types = entry['types']
        many = len(types) > 1
        callbacks[entry['module']] = [
            (f'link_{entry["module"]}_{name}' if many else f'link_{entry["module"]}') + f' = {spelled}'
            for spelled, name in types]
    out = []
    module = None
    for line in protos:
        m = re.match(r'/\* (\w+)/(\w+)\.c \*/', line)
        if m:
            module = m.group(2)
            out.append(f'\n## {m.group(1)}/{module}.c')
            for cb in callbacks.get(module, []):
                out.append(f'   callback {cb}')
            continue
        m = re.match(r'.*?\b(opeu_\w+|koei_\w+|\w+)\(', line)
        if m and module and line.endswith(';') and not line.startswith('/*'):
            name = m.group(1)
            out.append(f'{address.get(name, "----:----")} {line}')
    (ROOT/'port/gen/index.txt').write_text('\n'.join(out).lstrip() + '\n')


if __name__ == '__main__':
    main()
