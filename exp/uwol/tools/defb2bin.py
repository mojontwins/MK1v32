#!/usr/bin/env python3
"""defb2bin.py in.asm out.bin - assemble a data-only source (defb/db/defw/dw
lines, ; comments) like system/cpc_TrPixLutM*.asm. Replaces pasmo.exe for
those files; anything else is an error."""
import re, sys

def num(t):
    t = t.strip()
    if t.startswith('$'): return int(t[1:], 16)
    if t.startswith('#'): return int(t[1:], 16)
    if t.lower().startswith('0x'): return int(t, 16)
    if t.lower().endswith('h'): return int(t[:-1], 16)
    if t.startswith('%'): return int(t[1:], 2)
    return int(t)

out = bytearray()
for n, line in enumerate(open(sys.argv[1]), 1):
    line = line.split(';')[0].strip()
    if not line: continue
    m = re.match(r'(?i)(defb|db|defw|dw)\s+(.*)', line)
    if not m: sys.exit('%s:%d: only defb/defw data is supported: %s' % (sys.argv[1], n, line))
    for v in m.group(2).split(','):
        v = num(v)
        if m.group(1).lower() in ('defb', 'db'): out.append(v & 0xff)
        else: out += bytes([v & 0xff, (v >> 8) & 0xff])
open(sys.argv[2], 'wb').write(out)
print('defb2bin.py: %d bytes' % len(out))
