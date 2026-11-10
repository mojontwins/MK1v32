#!/usr/bin/env python3
# Tiny test harness: runs a .z80 snapshot in skoolkit's C Z80 simulator,
# with scripted key presses, and saves screenshots.
#   zxrun.py snap.z80 script   where script lines are:
#     run N            run N frames
#     key K[,K] N      hold keys for N frames (names: 1 2 3 Q A O P SPACE M ENTER)
#     shot file.png    save the screen
#     peek ADDR [N]    print memory
import sys
import os
sys.path.insert(0, os.environ.get('SKOOLKIT', os.path.expanduser('~/skoolkit')))
from skoolkit import CSimulator
from skoolkit.snapshot import Snapshot
from skoolkit.simutils import from_snapshot, PC, T
from PIL import Image

ROWS = {  # key: (half-row high byte, bit)
	'CAPS': (0xFE, 0), 'Z': (0xFE, 1), 'X': (0xFE, 2), 'C': (0xFE, 3), 'V': (0xFE, 4),
	'A': (0xFD, 0), 'S': (0xFD, 1), 'D': (0xFD, 2), 'F': (0xFD, 3), 'G': (0xFD, 4),
	'Q': (0xFB, 0), 'W': (0xFB, 1), 'E': (0xFB, 2), 'R': (0xFB, 3), 'T': (0xFB, 4),
	'1': (0xF7, 0), '2': (0xF7, 1), '3': (0xF7, 2), '4': (0xF7, 3), '5': (0xF7, 4),
	'0': (0xEF, 0), '9': (0xEF, 1), '8': (0xEF, 2), '7': (0xEF, 3), '6': (0xEF, 4),
	'P': (0xDF, 0), 'O': (0xDF, 1), 'I': (0xDF, 2), 'U': (0xDF, 3), 'Y': (0xDF, 4),
	'ENTER': (0xBF, 0), 'L': (0xBF, 1), 'K': (0xBF, 2), 'J': (0xBF, 3), 'H': (0xBF, 4),
	'SPACE': (0x7F, 0), 'SYM': (0x7F, 1), 'M': (0x7F, 2), 'N': (0x7F, 3), 'B': (0x7F, 4),
}


class Kbd:
	def __init__(self):
		self.down = set()
		self.border = 0
		self.outfe = 0

	def read_port(self, registers, port):
		if port & 0xFF == 0xFE:
			hi = port >> 8
			v = 0xBF
			for k in self.down:
				r, b = ROWS[k]
				rowmask = (~r) & 0xFF
				if hi & rowmask == 0:
					v &= ~(1 << b)
			return v
		if port & 0xFF == 0x1F:
			return 0
		return 0xFF

	def write_port(self, registers, port, value, t=0):
		if port & 1 == 0:
			self.border = value & 7


PAL = [(0, 0, 0), (0, 0, 215), (215, 0, 0), (215, 0, 215), (0, 215, 0), (0, 215, 215), (215, 215, 0), (215, 215, 215)]
PALB = [(0, 0, 0), (0, 0, 255), (255, 0, 0), (255, 0, 255), (0, 255, 0), (0, 255, 255), (255, 255, 0), (255, 255, 255)]


def shot(mem, fn):
	im = Image.new('RGB', (256, 192))
	px = im.load()
	for y in range(192):
		addr = 16384 + ((y & 0xC0) << 5) + ((y & 7) << 8) + ((y & 0x38) << 2)
		for cx in range(32):
			b = mem[addr + cx]
			a = mem[22528 + (y // 8) * 32 + cx]
			pal = PALB if a & 64 else PAL
			ink, paper = pal[a & 7], pal[(a >> 3) & 7]
			for i in range(8):
				px[cx * 8 + i, y] = ink if b & (128 >> i) else paper
	im = im.resize((512, 384), Image.NEAREST)
	im.save(fn)


def symbols(fn):
	d = {}
	for l in open(fn):
		if '=' in l and ', G:' in l:
			name, rest = l.split('=', 1)
			d[name.strip()] = int(rest.split(',')[0].strip(), 16)
	return d


def main():
	sym = symbols(sys.argv[3]) if len(sys.argv) > 3 else {}
	snap = Snapshot.get(sys.argv[1])
	sim = from_snapshot(CSimulator, snap)
	kb = Kbd()
	sim.set_tracer(kb)
	FRAME = 69888
	for line in open(sys.argv[2]).read().splitlines():
		p = line.split()
		if not p or p[0].startswith('#'):
			continue
		if p[0] == 'run':
			n = int(p[1])
			sim.trace(None, -1, 0, sim.registers[T] + n * FRAME, 1, None, None, None, None, None)
		elif p[0] == 'key':
			kb.down = set(p[1].split(','))
			n = int(p[2])
			sim.trace(None, -1, 0, sim.registers[T] + n * FRAME, 1, None, None, None, None, None)
			kb.down = set()
		elif p[0] == 'shot':
			shot(sim.memory, p[1])
			print('shot', p[1], 'PC=%d' % sim.registers[PC])
		elif p[0] == 'state':
			m = sim.memory
			f = list(m[sym['_FLAGS']:sym['_FLAGS'] + 32])
			print('state %-10s npant=%d gpx=%d gpy=%d slots=%s clock=%d alarm=%d restrict=%d event=%d life=%d' % (
				' '.join(p[1:]), m[sym['_N_PANT']], m[sym['_GPX']], m[sym['_GPY']], f[0:2], f[2], f[3], f[4], f[5],
				m[sym['_PLAYER'] + 29]))
			print('      flags', f)
		elif p[0] == 'tp':
			m = sim.memory
			n, x, y = int(p[1]), int(p[2]) * 16, int(p[3]) * 16
			m[sym['_N_PANT']] = n
			m[sym['_GPX']] = x; m[sym['_GPY']] = y
			pl = sym['_PLAYER']
			X, Y = x << 6, y << 6
			m[pl] = X & 255; m[pl + 1] = X >> 8
			m[pl + 2] = Y & 255; m[pl + 3] = Y >> 8
		elif p[0] == 'flag':
			sim.memory[sym['_FLAGS'] + int(p[1])] = int(p[2])
		elif p[0] == 'peek':
			a = int(p[1]); n = int(p[2]) if len(p) > 2 else 1
			print('peek', a, list(sim.memory[a:a + n]))


main()
