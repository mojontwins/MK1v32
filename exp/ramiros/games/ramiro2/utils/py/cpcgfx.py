#!/usr/bin/env python3
# cpcgfx.py - Python port of the CPC modes of mkts_om v0.11 (by The Mojon
# Twins) that MK1 v3.2 uses, for building on Linux.
#
#   cpcgfx.py platform=cpc cpcmode=0|1 pal=pal.png mode=MODE in=in.png out=out.bin
#             [greyordered] [max=N] [metasize=w,h] [mappings=file.h]
#             [pixelperfectm0|pixelperfectm1lut] [prefix=name]
#
#   mode=chars          8x8 patterns, left to right, top to bottom
#   mode=strait2x2      16x16 tiles as 4 patterns (TL TR BL BR)
#   mode=sprites        sprite cells (metasize in 8x8 chars, default 2x2)
#   mode=superbuffer    256x192 screen as 64 bytes per line (12288 bytes)
#   mode=scr            CPC screen memory layout (16384 bytes), mode 0
#   mode=pals           palette as a C array of HW colours
#   mode=palsasassembly palette as an assembly routine (prefix: label)
#
# Colours are matched (after reducing each component to 3 levels, like the
# CPC does) against the first 16 pixels of pal.png.

import sys
from PIL import Image

CPC_HW = [
	(1, 1, 1), (9, 9, 9), (0, 2, 1), (2, 2, 1), (0, 0, 1), (2, 0, 1), (0, 1, 1), (2, 1, 1),
	(9, 9, 9), (9, 9, 9), (2, 2, 0), (2, 2, 2), (2, 0, 0), (2, 0, 2), (2, 1, 0), (2, 1, 2),
	(9, 9, 9), (9, 9, 9), (0, 2, 0), (0, 2, 2), (0, 0, 0), (0, 0, 2), (0, 1, 0), (0, 1, 2),
	(1, 0, 1), (1, 2, 1), (1, 2, 0), (1, 2, 2), (1, 0, 0), (1, 0, 2), (1, 1, 0), (1, 1, 2),
]
GREY_ROW_ORDER = [0, 1, 3, 2, 6, 7, 5, 4]
VERSION = 'cpcgfx (mkts_om v0.11.20250501 port)'


def third(v):
	return 0 if v < 85 else (1 if v < 170 else 2)


def normalize(c):
	return tuple(third(v) for v in c[:3])


def hw_colour(c):
	n = normalize(c)
	for i, h in enumerate(CPC_HW):
		if h == n:
			return i
	return 0


class Converter:
	def __init__(self, opts):
		self.opts = opts
		self.mode = int(opts.get('cpcmode', '0'))
		self.brick = 2 if (self.mode == 1 or 'brickinput' in opts) else 1
		self.pw = 4 * self.brick 			# pattern width in pixels
		self.out = bytearray()
		self.pal = []
		if 'pal' in opts:
			p = Image.open(opts['pal']).convert('RGB')
			self.pal = [normalize(p.getpixel((i, 0))) for i in range(16)]
		self.img = None

	def idx(self, x, y):
		n = normalize(self.img.getpixel((x, y)))
		for i, c in enumerate(self.pal):
			if c == n:
				return i
		return 0xff

	def byte(self, x0, y0):
		bit = lambda v, b: (v >> b) & 1
		if self.mode == 0:
			c1 = self.idx(x0, y0)
			c2 = self.idx(x0 + self.brick, y0)
			return (bit(c2, 3) | (bit(c1, 3) << 1) | (bit(c2, 1) << 2) | (bit(c1, 1) << 3) |
				(bit(c2, 2) << 4) | (bit(c1, 2) << 5) | (bit(c2, 0) << 6) | (bit(c1, 0) << 7))
		c = [self.idx(x0 + i, y0) & 3 for i in range(4)]
		return (bit(c[3], 1) | (bit(c[2], 1) << 1) | (bit(c[1], 1) << 2) | (bit(c[0], 1) << 3) |
			(bit(c[3], 0) << 4) | (bit(c[2], 0) << 5) | (bit(c[1], 0) << 6) | (bit(c[0], 0) << 7))

	def pattern(self, x0, y0):
		p = []
		for y in range(8):
			p.append(self.byte(x0, y0 + y))
			p.append(self.byte(x0 + 2 * self.brick, y0 + y))
		return p

	def chars(self, mx):
		w, h = self.img.width // self.pw, self.img.height // 8
		ct = 0
		for y in range(0, h * 8, 8):
			for x in range(0, w * self.pw, self.pw):
				self.out += bytes(self.pattern(x, y))
				ct += 1
				if mx > 0 and ct == mx:
					return ct
		return ct

	def strait2x2(self, mx):
		w, h = self.img.width // (2 * self.pw), self.img.height // 16
		ct = 0
		for y in range(0, h * 16, 16):
			for x in range(0, w * 2 * self.pw, 2 * self.pw):
				for yy in (0, 8):
					for xx in (0, self.pw):
						self.out += bytes(self.pattern(x + xx, y + yy))
				ct += 1
				if mx > 0 and ct == mx:
					return ct
		return ct

	def sprites(self, wm, hm, mx):
		wp, hp = wm * self.pw, hm * 8
		w, h = self.img.width // wp, self.img.height // hp
		ct = 0
		for y in range(0, h * hp, hp):
			for x in range(0, w * wp, wp):
				for yy in range(hp):
					xa = x
					for xx in range(wm):
						self.out.append(self.byte(xa, y + yy))
						self.out.append(self.byte(xa + 2 * self.brick, y + yy))
						xa += self.pw
				ct += 1
				if mx > 0 and ct == mx:
					return ct
		return ct

	def superbuffer(self):
		for y in range(192):
			for x in range(64):
				self.out.append(self.byte(x * 2 * self.brick, y))

	def scr(self):
		for yy in range(8):
			for y in range(0, 193, 8):
				for x in range(80):
					yp = y + yy
					self.out.append(self.byte(x * 2 * self.brick, yp) if yp < self.img.height else 0)
			self.out += bytes(48)

	def write(self, fn, grey=False):
		if grey:
			o = bytearray()
			for i in range(0, len(self.out), 16):
				for j in range(8):
					g = i + GREY_ROW_ORDER[j] * 2
					if j & 1 == 0:
						o += self.out[g:g + 2]
					else:
						o += bytes([self.out[g + 1], self.out[g]])
			data = o
		else:
			data = self.out
		open(fn, 'wb').write(data)
		return len(data)


def mappings(fn, wm, hm, n, pp0, pp1):
	cox, coy = 0, (-8 if hm == 3 else 0)
	if pp1:
		suf = '%dx%d' % (wm * 8, hm * 8)
	elif pp0:
		suf = '%dx%d' % (wm * 4, hm * 8)
	else:
		suf = '%dBx%d' % (wm * 2, hm * 8)
	size = 16 * hm * wm
	L = ['// MTE MK1 (la Churrera) v5.10', '// Copyleft 2010-2014, 2020-2022 by the Mojon Twins', '',
		'// Spriteset mappings. ', '// One entry per sprite face in the spriteset!', '',
		'#define SWsprites_ALL  16', '']

	def table(name, vals, per, ctype='unsigned char'):
		L.append('%s %s [] = {' % (ctype, name))
		row = ''
		for i, v in enumerate(vals):
			if i % per == 0:
				row = '\t'
			row += v + (', ' if i < len(vals) - 1 else '')
			if i % per == per - 1 or i == len(vals) - 1:
				L.append(row.rstrip(' ') if i == len(vals) - 1 else row)
		L.append('};')
		L.append('')

	h2 = lambda v: '0x%02X' % (v & 255)
	table('sm_cox', [h2(cox)] * n, 8)
	table('sm_coy', [h2(coy)] * n, 8)
	inv = 'cpc_PutSpTileMap' + suf + ('Px' if pp0 else '') + ('PxM1' if pp1 else '')
	upd = 'cpc_PutTrSp' + suf + 'TileMap' + ('Px' if pp0 else '') + ('PxM1' if pp1 else '') + ('LUT' if pp1 else '')
	table('sm_invfunc', [inv] * n, 4, 'void *')
	table('sm_updfunc', [upd] * n, 4, 'void *')
	L.append('extern void *sm_sprptr [0];')
	L.append('#asm')
	L.append('\t._sm_sprptr')
	for i in range(0, n, 4):
		L.append('\t\tdefw ' + ', '.join('_sprites + 0x%04X' % (j * size) for j in range(i, min(n, i + 4))))
	L.append('#endasm')
	L.append('')
	L.append('// A list of MK1v4-friendly macros')
	for i in range(n):
		L.append('#define SPRITE_%02X (_sprites + 0x%04X)' % (i, i * size))
	L.append('')
	open(fn, 'w').write('\n'.join(L) + '\n')


def pals(c, fn, prefix, asm):
	img = Image.open(c.opts['in']).convert('RGB')
	cols = [img.getpixel((i, 0)) for i in range(16)]
	mc = 16 if c.mode == 0 else 4
	L = ['// CPC Palette with HW values', '// Generated by ' + VERSION, '// Copyleft 2017-2025 by The Mojon Twins', '']
	if asm:
		L += ['#asm', '\t.' + prefix, '\t\tld  b, 0x7f']
		for i in range(mc):
			h = hw_colour(cols[i])
			L += ['\t\tld  a, %d' % i, '\t\tout (c), a', '\t\tld  a, 0x%02X\t// 0b0100000 OR %X' % ((h & 0x1f) | 64, h),
				'\t\tout (c), a']
		L += ['\t\tret', '#endasm']
	else:
		L.append('const unsigned char %s [] = {' % prefix)
		for i in range(16):
			L.append('\t0x%02X%s// %02X%02X%02X' % (hw_colour(cols[i]), ', ' if i < 15 else '  ', *cols[i]))
		L.append('};')
	L.append('')
	open(fn, 'w').write('\n'.join(L))


def main():
	opts = {}
	for a in sys.argv[1:]:
		if '=' in a:
			k, v = a.split('=', 1)
			opts[k.lower()] = v
		else:
			opts[a.lower()] = True
	c = Converter(opts)
	mode = opts['mode'].lower()
	out = opts.get('out', '')
	base = out[:-4] if out.endswith('.bin') else (out[:-2] if out.endswith('.h') else out)
	if mode in ('pals', 'pal', 'palsasassembly'):
		pals(c, base + '.h', opts.get('prefix', 'my_inks'), mode == 'palsasassembly')
		print('%s: palette -> %s.h' % (VERSION, base))
		return
	c.img = Image.open(opts['in']).convert('RGB')
	mx = int(opts.get('max', '0'))
	wm, hm = 2, 2
	if 'metasize' in opts:
		wm, hm = [int(v) for v in opts['metasize'].split(',')]
	grey = 'greyordered' in opts
	if mode == 'chars':
		n = c.chars(mx)
		msg = '%d patterns' % n
	elif mode == 'strait2x2':
		n = c.strait2x2(mx)
		msg = '%d tiles' % n
	elif mode == 'sprites':
		n = c.sprites(wm, hm, mx)
		msg = '%d sprite cells' % n
		grey = False
		if 'mappings' in opts:
			mappings(opts['mappings'], wm, hm, n, 'pixelperfectm0' in opts,
				'pixelperfectm1lut' in opts or 'pixelperfectm1' in opts)
	elif mode == 'superbuffer':
		c.superbuffer()
		msg = 'superbuffer'
		grey = False
	elif mode == 'scr':
		c.scr()
		msg = 'scr'
		grey = False
	else:
		sys.exit('Unknown mode ' + mode)
	b = c.write(base + '.bin', grey)
	print('%s: %s -> %s.bin, %s (%d bytes)' % (VERSION, opts['in'], base, msg, b))


if __name__ == '__main__':
	main()
