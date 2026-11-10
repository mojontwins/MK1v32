#!/usr/bin/env python3
# movetest.py - checks addons/ramiro.h's player_custom_move against a Python
# model of Ramiro 4's move () (MK1v4 games_zx/ramiro4, resolved with its
# config.h), frame by frame, with random input.
#
#   movetest.py game.tap game.map [frames] [seed]
#
# Runs the game in skoolkit's simulator; every time player_custom_move is
# entered it predicts, from the state at that moment, the state at the next
# entry, and compares. Uses screen 7 of Ramiro 1 (after the intro).

import random
import subprocess
import sys

sys.path.insert(0, '/home/claude/src/skoolkit')
from skoolkit.snapshot import Snapshot
from skoolkit.simutils import from_snapshot, PC, T
from skoolkit.csimulator import CSimulator

ROWS = {
	'A': (0xFD, 0), 'Q': (0xFB, 0), 'O': (0xDF, 1), 'P': (0xDF, 0),
	'SPACE': (0x7F, 0), '2': (0xF7, 1),
}

# Ramiro 4 config
VY_INICIAL_SALTO, MAX_VY_SALTANDO, INCR_SALTO = 128, 256, 16
MAX_VY_CAYENDO, G = 512, 32
MAX_VY_CAYENDO_H, G_HOVER = 256, 4
MAX_VX, AX, RX = 256, 64, 48
sp_UP, sp_DOWN, sp_LEFT, sp_RIGHT, sp_FIRE = 0x01, 0x02, 0x04, 0x08, 0x80


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
				if hi & ((~r) & 0xFF) == 0:
					v &= ~(1 << b)
			return v
		if port & 0xFF == 0x1F:
			return 0
		return 0xFF

	def write_port(self, registers, port, value, t=0):
		pass


def s16(v):
	v &= 0xFFFF
	return v - 0x10000 if v & 0x8000 else v


def symbols(fn):
	d = {}
	for l in open(fn):
		if '=' in l and ', G:' in l:
			name, rest = l.split('=', 1)
			d[name.strip()] = int(rest.split(',')[0].strip(), 16)
	return d


class State:
	FIELDS = ('x', 'y', 'vx', 'vy', 'cont_salto', 'saltando', 'facing', 'gotten', 'possee',
		'ceiling', 'hovering', 'just_hovered', 'just_jumped', 'gpx', 'gpy', 'gpyy')

	def __init__(self, m, sym):
		p = sym['_PLAYER']
		w = lambda o: s16(m[p + o] | (m[p + o + 1] << 8))
		self.x, self.y, self.vx, self.vy = w(0), w(2), w(6), w(8)
		self.cont_salto, self.saltando = m[p + 14], m[p + 19]
		self.facing, self.gotten, self.possee = m[p + 22], m[p + 25], m[p + 26]
		self.ceiling, self.hovering = m[p + 37], m[p + 41]
		self.just_hovered, self.just_jumped = m[p + 42], m[p + 43]
		self.gpx, self.gpy = m[sym['_GPX']], m[sym['_GPY']]
		self.gpyy = m[sym['_RM_GPYY']]
		self.pad0 = m[sym['_PAD0']]
		self.ptgmx = s16(m[sym['_PTGMX']] | (m[sym['_PTGMX'] + 1] << 8))
		self.ptgmy = s16(m[sym['_PTGMY']] | (m[sym['_PTGMY'] + 1] << 8))
		self.attr = list(m[23300:23450])
		self.n_pant = m[sym['_N_PANT']]
		self.estado = m[p + 23]

	def key(self):
		return tuple(getattr(self, f) for f in self.FIELDS)


def model(s):
	"""Ramiro 4 move () vertical + horizontal + evil tile. Returns new State-like dict."""
	o = dict((f, getattr(s, f)) for f in State.FIELDS)
	pad0 = s.pad0
	attrmap = s.attr

	def attr(x, y):
		x &= 255; y &= 255
		if y >= 10:
			if y >= 11:
				y = 0
			else:
				return 0
		if x >= 15:
			return 0
		return attrmap[y * 15 + x]

	gpcx, gpcy = o['x'], o['y']
	gpx = o['gpx']
	gpyy = o['gpyy']
	button_jump = (pad0 & (sp_UP | sp_FIRE)) != (sp_UP | sp_FIRE)
	rdi = (attr((gpx + 4) >> 4, gpyy + 1) & 12) or (attr((gpx + 11) >> 4, gpyy + 1) & 12)
	if button_jump:
		if o['saltando'] == 0 and o['just_hovered'] == 0:
			if rdi or o['gotten']:
				o['saltando'] = 1
				o['just_jumped'] = 1
				o['cont_salto'] = 0
		if o['saltando']:
			o['vy'] -= (VY_INICIAL_SALTO + INCR_SALTO - (o['cont_salto'] >> 1))
			if o['vy'] < -MAX_VY_SALTANDO:
				o['vy'] = -MAX_VY_SALTANDO
			o['cont_salto'] += 1
			if o['cont_salto'] == 8:
				o['saltando'] = 0
	else:
		o['saltando'] = 0
		o['just_jumped'] = 0

	o['hovering'] = 0
	if (pad0 & sp_DOWN) == 0:
		o['just_hovered'] = 1
		if o['vy'] > 0:
			o['hovering'] = 1
			pad0 |= sp_DOWN
			if o['vy'] - (MAX_VY_CAYENDO_H - G_HOVER) >= 0:   # unsigned-ish compare as in asm (vy > 0 here)
				o['vy'] = MAX_VY_CAYENDO_H
			else:
				o['vy'] += G_HOVER
	else:
		o['just_hovered'] = 0
	if o['hovering'] == 0:
		if o['vy'] < 0:
			o['vy'] += G
		elif o['vy'] >= MAX_VY_CAYENDO - G:
			o['vy'] = MAX_VY_CAYENDO
		else:
			o['vy'] += G
	if o['gotten']:
		o['vy'] = 0

	o['y'] += o['vy']
	if o['y'] < -512:
		o['y'] = -512
	if o['y'] > 9216:
		o['y'] = 9216
	gpx = (o['x'] >> 6) & 255
	gpxx = gpx >> 4
	gpy = (o['y'] >> 6) & 255
	gpyy = gpy >> 4

	o['possee'] = 0
	o['ceiling'] = 0
	rdj = o['vy'] + s.ptgmy
	if rdj:
		if rdj < 0:
			if (gpy & 15) < 12:
				if ((gpx & 15) < 12 and attr(gpxx, gpyy) & 8) or ((gpx & 15) > 4 and attr(gpxx + 1, gpyy) & 8):
					o['vy'] = 0
					gpy = ((gpyy << 4) + 12) & 255
					o['y'] = gpy << 6
					o['ceiling'] = 1
		elif ((o['vy'] >> 6) & 255) >= (gpy & 15):
			if ((gpx & 15) < 12 and attr(gpxx, gpyy + 1) & 12) or ((gpx & 15) > 4 and attr(gpxx + 1, gpyy + 1) & 12):
				o['vy'] = 0
				gpy = (gpyy << 4) & 255
				o['y'] = gpy << 6
				o['possee'] = 1

	if (pad0 & sp_LEFT) != 0 and (pad0 & sp_RIGHT) != 0:
		if o['vx'] > 0:
			o['vx'] -= RX
			if o['vx'] < 0:
				o['vx'] = 0
		elif o['vx'] < 0:
			o['vx'] += RX
			if o['vx'] > 0:
				o['vx'] = 0
	if (pad0 & sp_LEFT) == 0:
		if o['vx'] > -MAX_VX:
			o['facing'] = 4
			o['vx'] -= AX
	if (pad0 & sp_RIGHT) == 0:
		if o['vx'] < MAX_VX:
			o['vx'] += AX
			o['facing'] = 0
	o['x'] += o['vx'] + s.ptgmx
	if o['x'] < 0:
		o['x'] = 0
	if o['x'] > 14336:
		o['x'] = 14336
	gpx = (o['x'] >> 6) & 255
	gpxx = gpx >> 4

	rdj = o['vx'] + s.ptgmx
	if rdj:
		if rdj < 0:
			if (gpx & 15) < 12:
				if ((gpy & 15) < 12 and attr(gpxx, gpyy) & 8) or ((gpy & 15) and attr(gpxx, gpyy + 1) & 8):
					o['vx'] = 0
					gpx = (gpxx << 4) + 12
					o['x'] = gpx << 6
		else:
			if (gpx & 15) >= 4:
				if ((gpy & 15) < 12 and attr(gpxx + 1, gpyy) & 8) or ((gpy & 15) and attr(gpxx + 1, gpyy + 1) & 8):
					o['vx'] = 0
					gpx = (gpxx << 4) + 4
					o['x'] = gpx << 6

	# Evil tile
	if attr((gpx + 8) >> 4, ((gpy + 14) & 255) >> 4) == 1:
		o['x'], o['y'] = gpcx, gpcy
		o['vy'] = -abs(o['vy'])
		if o['vy'] > -(G * 9):
			o['vy'] = -(G * 9)
	o['gpx'], o['gpy'], o['gpyy'] = gpx, gpy, gpyy
	return o


def main():
	tap, mapfn = sys.argv[1], sys.argv[2]
	frames = int(sys.argv[3]) if len(sys.argv) > 3 else 2000
	seed = int(sys.argv[4]) if len(sys.argv) > 4 else 1
	rnd = random.Random(seed)
	sym = symbols(mapfn)
	subprocess.run(['python3', '/home/claude/src/skoolkit/tap2sna.py', '--sim-load-config', 'finish-tape=1',
		'-c', 'accelerator=auto', tap, '/tmp/movetest.z80'], check=True, capture_output=True)
	snap = Snapshot.get('/tmp/movetest.z80')
	sim = from_snapshot(CSimulator, snap)
	kb = Kbd()
	sim.set_tracer(kb)
	FRAME = 69888

	def run(n):
		sim.trace(None, -1, 0, sim.registers[T] + n * FRAME, 1, None, None, None, None, None)

	run(50)
	kb.down = {'2'}; run(3); kb.down = set()
	run(300)
	for i in range(4):
		kb.down = {'SPACE'}; run(5); kb.down = set(); run(250)

	entry = sym['_PLAYER_CUSTOM_MOVE']
	keysets = [set(), {'P'}, {'O'}, {'Q'}, {'P', 'Q'}, {'O', 'Q'}, {'A'}, {'P', 'A'}, {'O', 'A'},
		{'Q', 'A'}, {'SPACE'}, {'P', 'SPACE'}, {'O', 'P'}]
	prev = None
	bad = 0
	hold = 0
	checked = 0
	for it in range(frames):
		if hold == 0:
			kb.down = set(rnd.choice(keysets))
			hold = rnd.randint(1, 25)
		hold -= 1
		# run to the next entry of player_custom_move
		sim.trace(None, entry, 0, sim.registers[T] + 10 * FRAME, 1, None, None, None, None, None)
		if sim.registers[PC] != entry:
			print('timeout waiting for player_custom_move (text box?)')
			kb.down = {'SPACE'}; run(5); kb.down = set()
			prev = None
			continue
		# step one instruction so the next trace doesn't stop at once
		cur = State(sim.memory, sym)
		if prev is not None and prev.n_pant == cur.n_pant and prev.estado == 0 and cur.estado == 0:
			exp = model(prev)
			got = dict((f, getattr(cur, f)) for f in State.FIELDS)
			diff = [(f, exp[f], got[f]) for f in State.FIELDS if exp[f] != got[f]]
			checked += 1
			if diff:
				bad += 1
				if bad <= 10:
					print('frame %d pad0=%02x: %s' % (it, prev.pad0, diff))
					print('   prev', prev.key())
		prev = cur
		sim.trace(None, -1, 1, sim.registers[T] + FRAME, 1, None, None, None, None, None)
	print('checked %d frames, %d mismatches' % (checked, bad))


main()
