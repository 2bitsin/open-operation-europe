#!/usr/bin/env python3
"""The port's opening music (--opl-log) against DOSBox-X's raw OPL capture
(DRO 2.0) of the original OPEN.EXE (port/open/tests/dosbox_open_capture.py).

DOSBox-X starts the DRO at the first note played and records every write
that changes a register, with millisecond delays; the port's writes are
filtered the same way. Both streams are
aligned at their first sounding note (a melodic key-on or a rhythm hit) and
compared as (register, value) sequences over a common window; times are
compared per write with a tolerance (DRO resolution is 1 ms, the port's
driver ticks are 3.43 ms).

Usage: compare_open_dro.py <file.dro> <port intro.opl> [--seconds=120] [--json=FILE]
"""
import json
import struct
import sys

RATE = 49716


def read_dro(path):
    data = open(path, 'rb').read()
    if data[:8] != b'DBRAWOPL' or struct.unpack_from('<HH', data, 8) != (2, 0):
        raise SystemExit(f'{path}: not a DRO 2.0 file')
    pairs, ms, hw, fmt, comp, short, long_, nmap = struct.unpack_from('<IIBBBBBB', data, 12)
    codemap = data[26:26 + nmap]
    pos, t, out = 26 + nmap, 0, []
    for _ in range(pairs):
        code, val = data[pos], data[pos + 1]
        pos += 2
        if code == short:
            t += val + 1
        elif code == long_:
            t += (val + 1) << 8
        else:
            reg = codemap[code & 0x7f] | (0x100 if code & 0x80 else 0)
            out.append((t, reg, val))
    return out


def read_port(path):
    """The port's writes, without those that store a register's current
    value (DOSBox-X's raw OPL capture drops them)."""
    out, regs = [], {}
    for line in open(path):
        if line.startswith('#'):
            continue
        frame, reg, val = line.split()
        reg, val = int(reg, 16), int(val, 16)
        if regs.get(reg) == val:
            continue
        regs[reg] = val
        out.append((int(frame) * 1000.0 / RATE, reg, val))
    return out


def note(reg, val):
    return (0xb0 <= reg <= 0xb8 and val & 0x20) or (reg == 0xbd and val & 0x20 and val & 0x1f)


def from_first_note(writes):
    i = next(i for i, (_, r, v) in enumerate(writes) if note(r, v))
    t0 = writes[i][0]
    return [(t - t0, r, v) for t, r, v in writes[i:]]


def main(argv):
    args = [a for a in argv[1:] if not a.startswith('--')]
    opts = dict(a[2:].split('=', 1) for a in argv[1:] if a.startswith('--') and '=' in a)
    seconds = float(opts.get('seconds', 120))
    dro = from_first_note(read_dro(args[0]))
    port = from_first_note(read_port(args[1]))
    dro = [w for w in dro if w[0] < seconds * 1000]
    port = [w for w in port if w[0] < seconds * 1000]
    n = min(len(dro), len(port))
    first = next((i for i in range(n) if dro[i][1:] != port[i][1:]), None)
    drift = [port[i][0] - dro[i][0] for i in range(first if first is not None else n)]
    report = {'window_s': seconds, 'dro_writes': len(dro), 'port_writes': len(port),
              'sequence_identical': first is None and len(dro) == len(port),
              'first_value_mismatch': first,
              'time_offset_ms': {'min': round(min(drift), 2), 'max': round(max(drift), 2)} if drift else None}
    # Per register: the value sequence each register receives, and the
    # largest time offset between corresponding writes. Robust to the
    # interleaving of writes from the driver's two clocks (music ticks and
    # the 62.1 Hz fixed tick, whose phase depends on how long the driver has
    # been resident).
    per = {}
    for name, stream in (('dro', dro), ('port', port)):
        for t, r, v in stream:
            per.setdefault(r, {}).setdefault(name, []).append((t, v))
    regs_same, regs_diff, worst = 0, [], 0.0
    for r, both in sorted(per.items()):
        a, b = both.get('dro', []), both.get('port', [])
        k = min(len(a), len(b))
        if [v for _, v in a[:k]] == [v for _, v in b[:k]] and abs(len(a) - len(b)) <= 1:
            regs_same += 1
            if k:
                worst = max(worst, max(abs(a[i][0] - b[i][0]) for i in range(k)))
        else:
            i = next((i for i in range(k) if a[i][1] != b[i][1]), k)
            regs_diff.append({'reg': f'{r:02x}', 'dro': len(a), 'port': len(b), 'first_difference': i})
    report['registers_identical'] = regs_same
    report['registers_different'] = regs_diff
    report['per_register_max_time_offset_ms'] = round(worst, 2)
    if first is not None:
        report['context_dro'] = [f'{t}:{r:02x}={v:02x}' for t, r, v in dro[first:first + 8]]
        report['context_port'] = [f'{t:.1f}:{r:02x}={v:02x}' for t, r, v in port[first:first + 8]]
    if 'json' in opts:
        open(opts['json'], 'w').write(json.dumps(report, indent=1))
    print(json.dumps(report, indent=1))
    return 0 if not regs_diff else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv))
