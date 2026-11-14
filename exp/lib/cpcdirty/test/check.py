#!/usr/bin/env python3
# cpcdirty - test bench checker. Copyleft 2026 The Mojon Twins
#
#   check.py mode dumpfile [scrw vpx vpy]
#
# mode: n0, m1, p0. dumpfile: cpcheadless output with full 64K dumps
# ("frame F @0: xx xx ..."). For every dump, rebuilds the viewport from the
# map (CD_TMAP), the patterns (CD_PAT) and the sprite list as the library
# should draw it, pixel by pixel, and compares it with the screen.

import sys

VPX, VPY, NSPR = 1, 2, 8
SCRW = 64
if len(sys.argv) > 3: SCRW, VPX, VPY = int(sys.argv[3]), int(sys.argv[4]), int(sys.argv[5])
LIST, EMPTY, STEP = 0x8000, 0x8500, 0x8600
PAT, TMAP = 0x9000, 0xAC00
TPEN = 5

mode = sys.argv[1]
ppb = 4 if mode == 'm1' else 2 		# pixels per byte

def dec(b):
	if mode == 'm1':
		return [((b >> (7 - k)) & 1) | (((b >> (3 - k)) & 1) << 1) for k in range(4)]
	p0 = ((b >> 7) & 1) | (((b >> 3) & 1) << 1) | (((b >> 5) & 1) << 2) | (((b >> 1) & 1) << 3)
	p1 = ((b >> 6) & 1) | (((b >> 2) & 1) << 1) | (((b >> 4) & 1) << 2) | ((b & 1) << 3)
	return [p0, p1]

def enc(px):
	if mode == 'm1':
		b = 0
		for k, p in enumerate(px):
			b |= (p & 1) << (7 - k) | ((p >> 1) & 1) << (3 - k)
		return b
	p0, p1 = px
	return ((p0 & 1) << 7 | ((p0 >> 1) & 1) << 3 | ((p0 >> 2) & 1) << 5 | ((p0 >> 3) & 1) << 1 |
		(p1 & 1) << 6 | ((p1 >> 1) & 1) << 2 | ((p1 >> 2) & 1) << 4 | ((p1 >> 3) & 1))

def scr(c, r, line, side):
	return 0xC000 + (VPY + r) * SCRW + line * 0x800 + (VPX + c) * 2 + side

def check(mem):
	W = 60 * ppb 		# viewport pixels
	pix = [[0] * W for _ in range(160)]
	for r in range(20):
		for c in range(30):
			t = mem[TMAP + r * 30 + c]
			for line in range(8):
				for side in range(2):
					for k, p in enumerate(dec(mem[PAT + t * 16 + line * 2 + side])):
						pix[r * 8 + line][(c * 2 + side) * ppb + k] = p
	for s in range(NSPR):
		e = LIST + s * 16
		fr = mem[e] | mem[e + 1] << 8
		if fr == EMPTY: continue
		x, y = mem[e + 8], mem[e + 9]
		px = x - VPX * 2 * ppb
		py = y - VPY * 8
		if px < 0 or px > W - 4 * ppb or py < 0 or py > 144: continue
		for sy in range(16):
			for sb in range(4):
				for k, p in enumerate(dec(mem[fr + sy * 4 + sb])):
					X, Y = px + sb * ppb + k, py + sy
					if mode == 'p0': pix[Y][X] |= p
					elif mode == 'm1':
						if p != 0: pix[Y][X] = p
					else:
						if p != TPEN: pix[Y][X] = p
	bad = []
	for r in range(20):
		for c in range(30):
			for line in range(8):
				for side in range(2):
					want = enc(pix[r * 8 + line][(c * 2 + side) * ppb:(c * 2 + side + 1) * ppb])
					got = mem[scr(c, r, line, side)]
					if want != got: bad.append((c, r, line, side, want, got))
	return bad

ok = True
n = 0
for l in open(sys.argv[2]):
	if not l.startswith('frame '): continue
	f, rest = l.split(' @0:')
	mem = bytes(int(h, 16) for h in rest.split())
	n += 1
	bad = check(mem)
	print('%s: step %d, %d bytes differ%s' % (f, mem[STEP], len(bad), (': ' + str(bad[:4])) if bad else ''))
	if bad: ok = False
print('%s: %d dumps, %s' % (mode, n, 'OK' if ok else 'FAILED'))
sys.exit(0 if ok else 1)
