#!/usr/bin/env python3
# profsum.py prof.txt game.map [n] - sum a cpcheadless/zxheadless -profout
# histogram by the symbols of a z88dk .map (each address goes to the closest
# symbol at or below it).
import bisect, re, sys
syms = {}
for l in open(sys.argv[2]):
	m = re.match(r'(\S+)\s+= ([0-9A-F]{4}),', l)
	if m: syms[int(m.group(2), 16)] = m.group(1)
addrs = sorted(syms)
tot = {}
for l in open(sys.argv[1]):
	a, c = l.split(); a = int(a, 16); c = int(c)
	i = bisect.bisect_right(addrs, a) - 1
	n = syms[addrs[i]] if i >= 0 else '?'
	tot[n] = tot.get(n, 0) + c
T = sum(tot.values())
print('total %d NOPs' % T)
for n, c in sorted(tot.items(), key=lambda x: -x[1])[:int(sys.argv[3]) if len(sys.argv) > 3 else 30]:
	print('%-32s %8d %6.2f%%' % (n, c, 100.0 * c / T))
