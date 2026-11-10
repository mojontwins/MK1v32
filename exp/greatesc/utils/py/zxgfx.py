#!/usr/bin/env python3
# Python ports of the MK1 v3.2 ZX Spectrum graphics converters.
#
#   zxgfx.py ts2bin font.png work.png tileset.bin defaultink   (= ts2bin.exe)
#   zxgfx.py sprcnv sprites.png sprites.h                       (= sprcnv.exe)
#   zxgfx.py png2scr img.png img.scr                            (= png2scr.exe)
#
# The colour detection and the bit/attribute layout follow the FreeBASIC
# originals in utils/*.bas line by line, so the output is byte-identical
# for the same PNG.

import sys
from PIL import Image


def load(fn):
	return Image.open(fn).convert('RGB')


# ---------------------------------------------------------------- ts2bin ---

def ts_colour(rgb):
	# speccyColour () from ts2bin.bas: bit 6 = BRIGHT, bit 7 = "flash"
	# (used by the engine as "sprites go behind this cell").
	res = 0
	for v, bit in zip(rgb, (2, 4, 1)):
		if v >= 128:
			res |= bit
			if v >= 240:
				res |= 64
			elif v < 192:
				res |= 128
				if v >= 160:
					res |= 64
	return res


def udg_into_charset(img, x0, y0, ts, idx, default_ink, inverted):
	px = img.load()
	c1 = ts_colour(px[x0, y0])
	c2 = c1
	for y in range(8):
		for x in range(8):
			c = ts_colour(px[x0 + x, y0 + y])
			if c != c1:
				c2 = c
	f = 0
	if c1 & 128: f = 128; c1 &= 127
	if c2 & 128: f = 128; c2 &= 127
	b = 0
	if c1 & 64: b = 64; c1 &= 63
	if c2 & 64: b = 64; c2 &= 63
	if c1 == c2:
		if default_ink != -1:
			c1 = default_ink
		else:
			c1 = 7 if c2 < 4 else 0
	if inverted:
		if c1 > c2: c1, c2 = c2, c1
	else:
		if c2 > c1: c1, c2 = c2, c1
	ts[2048 + idx] = (f + b + 8 * c2 + c1) & 255
	for y in range(8):
		bb = 0
		for x in range(8):
			c = ts_colour(px[x0 + x, y0 + y]) & 63
			if c == c1:
				bb += 1 << (7 - x)
		ts[8 * idx + y] = bb


def ts2bin(font_fn, work_fn, out_fn, default_ink):
	ts = bytearray(2304)
	font = load(font_fn)
	idx = 0
	for y in range(2):
		for x in range(32):
			udg_into_charset(font, x * 8, y * 8, ts, idx, -1, False)
			idx += 1
	work = load(work_fn)
	inverted = False
	if default_ink.startswith('inverted:'):
		inverted = True
		default_ink = default_ink[-1]
	dink = int(default_ink)
	x = y = 0
	idx = 64
	for i in range(48):
		for dx, dy in ((0, 0), (8, 0), (0, 8), (8, 8)):
			udg_into_charset(work, x + dx, y + dy, ts, idx, dink, inverted)
			idx += 1
		x += 16
		if x == 256:
			x = 0; y += 16
	open(out_fn, 'wb').write(ts)
	print('ts2bin: %d bytes written' % len(ts))


# ---------------------------------------------------------------- sprcnv ---

def sprcnv(png_fn, out_fn):
	img = load(png_fn)
	px = img.load()

	def rbv(x, y):
		ac = 0
		for xx in range(8):
			if px[x + xx, y] != (0, 0, 0):
				ac += 1 << (7 - xx)
		return ac

	o = []
	o.append('// MTE MK1 (la Churrera) v5.0')
	o.append('// Copyleft 2010-2014, 2020 by the Mojon Twins')
	o.append('')
	o.append('// Sprites.h')
	o.append(' ')
	for s in range(1, 17):
		for c in 'abc':
			o.append('extern unsigned char sprite_%d_%s []; ' % (s, c))
	o.append(' ')
	o.append('#asm')
	for i in range(8):
		o.append('        defb 0, 255')
	o.append(' ')
	x = y = 0
	for s in range(1, 17):
		for col, xo in (('a', 0), ('b', 8)):
			o.append('    ._sprite_%d_%s' % (s, col))
			for yy in range(16):
				o.append('        defb %d, %d' % (rbv(x + xo, y + yy), rbv(x + xo + 16, y + yy)))
			for yy in range(8):
				o.append('        defb 0, 255')
			o.append(' ')
		o.append('    ._sprite_%d_c' % s)
		for yy in range(24):
			o.append('        defb 0, 255')
		o.append(' ')
		x += 32
		if x == 256:
			y += 16; x = 0
	o.append('#endasm')
	o.append(' ')
	open(out_fn, 'w').write('\n'.join(o) + '\n')
	print('sprcnv: done')


# ---------------------------------------------------------------- png2scr --

def scr_colour(rgb):
	res = 0
	for v, bit in zip(rgb, (2, 4, 1)):
		if v >= 128:
			res |= bit
			if v >= 220:
				res |= 64
	return res


def png2scr(png_fn, out_fn):
	img = load(png_fn)
	px = img.load()
	cp = [[(0, 0)] * 32 for _ in range(24)]
	pc1 = pc2 = 99
	for y in range(24):
		for x in range(32):
			c1 = scr_colour(px[x * 8, y * 8])
			c2 = c1
			done = False
			for i in range(8):
				for j in range(8):
					c2 = scr_colour(px[x * 8 + i, y * 8 + j])
					if c2 != c1:
						done = True
						break
				if done:
					break
			if c1 == c2:
				if pc2 == c2:
					c1 = pc1
				else:
					if (c1 & 63) < 4:
						c1 = 7 + (64 if c2 & 64 else 0)
					else:
						c1 = 0 + (64 if c2 & 64 else 0)
			if (c1 & 63) < (c2 & 63):
				c1, c2 = c2, c1
			cp[y][x] = (c1, c2)
			pc1, pc2 = c1, c2
	out = bytearray()
	for third in range(3):
		for lic in range(8):
			for cl in range(8):
				for column in range(32):
					x = 8 * column
					y = third * 64 + lic + cl * 8
					c1 = cp[y // 8][x // 8][0]
					c = 0
					for i in range(8):
						if scr_colour(px[x + i, y]) == c1:
							c += 1 << (7 - i)
					out.append(c)
	for y in range(24):
		for x in range(32):
			c1, c2 = cp[y][x]
			b = 1 if (c1 & 64) or (c2 & 64) else 0
			out.append(((c1 & 63) + 8 * (c2 & 63) + 64 * b) & 255)
	open(out_fn, 'wb').write(out)
	print('png2scr: %s -> %s' % (png_fn, out_fn))


if __name__ == '__main__':
	cmd = sys.argv[1]
	a = sys.argv[2:]
	if cmd == 'ts2bin':
		ts2bin(a[0], a[1], a[2], a[3] if len(a) > 3 else '-1')
	elif cmd == 'sprcnv':
		sprcnv(a[0], a[1])
	elif cmd == 'png2scr':
		png2scr(a[0], a[1])
	else:
		sys.exit('unknown command ' + cmd)
