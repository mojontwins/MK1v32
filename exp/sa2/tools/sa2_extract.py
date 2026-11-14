#!/usr/bin/env python3
# Sir Ababol 2 CPC -> MK1 v3.2: one-shot extraction of the original assets.
# Copyleft 2026 The Mojon Twins
#
#   sa2_extract.py <original source folder> <project folder> <ZX level folder>
#
# The ZX folder has the ZX Spectrum version's levels (levelN.MAP: 15 x 200,
# one byte per tile of its 48-tile sets; sa2-lN.ene), N = 0-4: the CPC levels
# were made from them in a hurry, and some data got lost (see below).
#
# The original folder is the one with "Sir CPC 2 A", "... B", "... C" and
# "Cutscenes". What is taken is what went into the shipped binaries: the
# exomizer blobs (levels, songs, screens), the tile patterns of
# TileMapConf-sa2.asm and the sprite frames of spriteset.h, decoded to pens
# and written with the original palette (palette.txt). Writes:
#
#   gfx/pal.png               the 16 pens, in order (and cpc/inks.txt: the
#                             hardware inks of the original, my_inks)
#   gfx/font.png              patterns 0-63 (the font: ASCII 32-95)
#   gfx/tsA.png .. tsC.png    the 32 metatiles of each part, 8x16 mode 0
#                             pixels each (2x2 patterns), 16 per row
#   gfx/sprites.png           the 22 frames, 8x16 each (pen 12 = transparent)
#   gfx/titleA.png .. C       the title / game frame of each part (128x192
#                             mode 0 pixels: MK1's 64 x 192 screen), and
#   gfx/cut_title.png,        the cutscene screens (256x192 mode 1 pixels);
#   gfx/plate0.png .. 4       cut out of the original's 80 x 200 (see below)
#   map/level0.map .. 5       Mappy raw maps, 15 x 200 tiles (1 x 20 screens), the
#                             tiles of its part (tsA..C: levels 1-3 A, 4-5 B, 6 C)
#   enems/level0.ene .. 5     ponedor .ene, 1 x 20 screens, 3 enemies each
#   ogt/NAME.mus              the songs, for MK1's player (+ the loop table)
#
# The original draws on a grid that starts 4 lines down (the play area is at
# byte 10, line 12 of a standard 80 x 200 screen). The screens are moved up 4
# lines and cut to bytes 8-71, lines 0-191: MK1's 64 x 192 screen, with the
# play area at char 1, row 1 (VIEWPORT_X 1, VIEWPORT_Y 1). The frame fits; only
# the swirls outside the box's sides are lost (what is left of them, outside
# the box's side lines, is cleared).

import os, re, struct, sys
from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from unexo import unexo

src, out = sys.argv[1], sys.argv[2]
PARTS = 'ABC'
def pdir(p): return os.path.join(src, 'Sir CPC 2 %s' % p)

# ----------------------------------------------------------------------------
# Palette

pal = []
for l in open(os.path.join(pdir('A'), 'gfx', 'palette.txt')):
	m = re.match(r'\s*(\d+)\s*->\s*R:\s*(\d+),\s*G:\s*(\d+),\s*B:\s*(\d+)', l)
	if m: pal.append(tuple(int(v) for v in m.group(2, 3, 4)))
assert len(pal) == 16
ms = open(os.path.join(pdir('A'), 'dev', 'mysystem.h'), encoding='latin1').read()
inks = [int(v) for v in re.search(r'my_inks\s*\[\]\s*=\s*\{([^}]*)\}', ms).group(1).replace('\n', '').split(',') if v.strip()]
TPEN = 12

def m0dec(b):
	p0 = ((b >> 7) & 1) | (((b >> 3) & 1) << 1) | (((b >> 5) & 1) << 2) | (((b >> 1) & 1) << 3)
	p1 = ((b >> 6) & 1) | (((b >> 2) & 1) << 1) | (((b >> 4) & 1) << 2) | ((b & 1) << 3)
	return p0, p1

def img(w, h, pens):
	im = Image.new('RGB', (w, h))
	im.putdata([pal[p] for p in pens])
	return im

def crop64(im, ppb):
	# bytes 8-71, lines 0-191; clear what is outside the box's side lines (the
	# first and last columns with something on most lines)
	im = im.crop((8 * ppb, 0, 72 * ppb, 192)).copy()
	w, h = im.size
	px = im.load()
	black = px[0, 191]
	def full(x): return sum(1 for y in range(192) if px[x, y] != black) > 120
	cols = [x for x in range(w) if full(x)]
	if not cols: return im 		# (the cutscene plates: the box fits)
	left, right = cols[0], cols[-1]
	top = max(y for y in range(192) if px[left, y] != black)
	for y in range(top + 1):
		for x in list(range(left)) + list(range(right + 1, w)):
			px[x, y] = black
	return im

def mk(*d):
	p = os.path.join(out, *d)
	os.makedirs(os.path.dirname(p), exist_ok=True)
	return p

# ----------------------------------------------------------------------------
# Patterns (TileMapConf-sa2.asm: 256 x 2 bytes x 8 lines after _tile_0)

def patterns(part):
	s = open(os.path.join(pdir(part), 'dev', 'TileMapConf-sa2.asm'), encoding='latin1').read()
	s = s[s.index('._tile_0'):]
	data = []
	for l in s.splitlines():
		m = re.match(r'\s*defb\s+(.*)', l, re.I)
		if not m: continue
		for v in m.group(1).split(';')[0].split(','):
			v = v.strip()
			if v: data.append(int(v[1:], 16) if v.startswith('$') else int(v, 0))
		if len(data) >= 4096: break
	# (parts B and C define fewer: tiles 0-26 at most)
	assert len(data) % 16 == 0 and len(data) >= 64 * 16 + 27 * 64, (part, len(data))
	data += [0] * (4096 - len(data))
	return [bytes(data[i * 16:i * 16 + 16]) for i in range(256)]

def pat_pens(p):
	# 4 x 8 pixels, row-major
	px = []
	for y in range(8):
		for x in range(2):
			px += m0dec(p[y * 2 + x])
	return px

def pattern_img(p):
	return img(4, 8, pat_pens(p))

pats = {p: patterns(p) for p in PARTS}
for p in 'BC':
	if pats[p][:64] != pats['A'][:64]: print('note: the font of part %s differs from A\'s' % p)

font = Image.new('RGB', (4 * 64, 8))
for i in range(64): font.paste(pattern_img(pats['A'][i]), (i * 4, 0))
font.save(mk('gfx', 'font.png'))

for p in PARTS:
	ts = Image.new('RGB', (16 * 8, 2 * 16))
	for t in range(32):
		tx, ty = (t % 16) * 8, (t // 16) * 16
		for k in range(4):
			ts.paste(pattern_img(pats[p][64 + t * 4 + k]), (tx + (k % 2) * 4, ty + (k // 2) * 8))
	ts.save(mk('gfx', 'ts%s.png' % p))

pim = img(16, 1, list(range(16)))
pim.save(mk('gfx', 'pal.png'))
with open(mk('gfx', 'inks.txt'), 'w') as f:
	f.write('# hardware inks of pens 0-15 in the original (my_inks)\n' + ', '.join(str(i) for i in inks) + '\n')

# ----------------------------------------------------------------------------
# Sprites (spriteset.h: 22 frames of 4 bytes x 16 lines, after a 2 byte header)

s = open(os.path.join(pdir('A'), 'dev', 'spriteset.h'), encoding='latin1').read()
frames = []
cur = None
for l in s.replace('\r', '').splitlines():
	m = re.match(r'\._spr_(\d+)\s*$', l)
	if m:
		cur = [int(m.group(1)), []]; frames.append(cur); continue
	m = re.match(r'\s*defb\s+(.*)', l, re.I)
	if m and cur is not None:
		for v in m.group(1).split(';')[0].split(','):
			v = v.strip()
			if v: cur[1].append(int(v[1:], 16) if v.startswith('$') else int(v, 0))
for f in frames:
	assert f[1][:2] == [4, 16] and len(f[1]) == 66, (f[0], f[1][:2], len(f[1]))
	f[1] = bytes(f[1][2:])
frames.sort()
assert [n for n, _ in frames] == list(range(22))
spr = Image.new('RGB', (8 * 22, 16))
for n, d in frames:
	px = []
	for y in range(16):
		for x in range(4):
			px += m0dec(d[y * 4 + x])
	spr.paste(img(8, 16, px), (n * 8, 0))
spr.save(mk('gfx', 'sprites.png'))

# ----------------------------------------------------------------------------
# Screens (exomizer'd 16336-byte standard screens), moved up 4 lines

def screen_img(d, shift=4):
	pens = []
	for y in range(200):
		yy = y + shift
		for xb in range(80):
			if yy < 200:
				a = (yy // 8) * 80 + (yy % 8) * 2048 + xb
				b = d[a] if a < len(d) else 0
			else:
				b = 0
			pens += m0dec(b)
	return img(160, 200, pens)

def unexo_file(*p):
	return unexo(open(os.path.join(src, *p), 'rb').read())

for p in PARTS:
	crop64(screen_img(unexo_file('Sir CPC 2 %s' % p, 'dev', 'titlec.exo')), 2).save(mk('gfx', 'title%s.png' % p))

# The cutscenes are mode 1, 4 inks of their own: the hardware inks of its
# mysystem.h (cpc_SetColour takes hardware numbers; its gfx/palette.txt is
# not what the program shows): 320x200 pixels, moved up 4 lines too.
HWRGB = { 0x14: (0, 0, 0), 0x17: (0, 128, 255), 0x1b: (128, 255, 255), 0x0b: (255, 255, 255) }
cms = open(os.path.join(src, 'Cutscenes', 'dev', 'mysystem.h'), encoding='latin1').read()
cinks = [int(v) for v in re.search(r'my_inks\s*\[\]\s*=\s*\{([^}]*)\}', cms).group(1).replace('\n', '').split(',') if v.strip()]
cpal = [HWRGB [i] for i in cinks]
assert len(cpal) == 4

def m1dec(b):
	return [((b >> (7 - k)) & 1) | (((b >> (3 - k)) & 1) << 1) for k in range(4)]

def screen_img_m1(d, shift=4):
	pens = []
	for y in range(200):
		yy = y + shift
		for xb in range(80):
			b = 0
			if yy < 200:
				a = (yy // 8) * 80 + (yy % 8) * 2048 + xb
				if a < len(d): b = d[a]
			pens += m1dec(b)
	im = Image.new('RGB', (320, 200))
	im.putdata([cpal[p] for p in pens])
	return im

crop64(screen_img_m1(unexo_file('Cutscenes', 'dev', 'titlec.exo')), 4).save(mk('gfx', 'cut_title.png'))
for n in range(5):
	crop64(screen_img_m1(unexo_file('Cutscenes', 'dev', 'plate%dc.exo' % n)), 4).save(mk('gfx', 'plate%d.png' % n))
cp = Image.new('RGB', (4, 1)); cp.putdata(cpal); cp.save(mk('gfx', 'cut_pal.png'))

# Every plate is the same frame with a picture window: cut_frame.png is the
# frame (plate 1 with the window empty), cut_winN.png the window of plate N
# (128 x 72 pixels: bytes 16-47, char rows 2-10 of the 64 x 192 screen;
# mkts_om mode=chars cuts it in 16 x 9 cells). Plate 0 is not used by the
# original.
WX, WY, WW, WH = 64, 16, 128, 72
fr = Image.open(mk('gfx', 'plate1.png')).copy()
fr.paste(Image.new('RGB', (WW, WH), cpal[0]), (WX, WY))
fr.save(mk('gfx', 'cut_frame.png'))
for n in range(5):
	Image.open(mk('gfx', 'plate%d.png' % n)).crop((WX, WY, WX + WW, WY + WH)).save(mk('gfx', 'cut_win%d.png' % n))
with open(mk('gfx', 'inks.txt'), 'a') as f:
	f.write('# hardware inks of the cutscenes (mode 1, pens 0-3)\n' + ', '.join(str(i) for i in cinks) + '\n')

# the cutscene font: mode 1 patterns (2 bytes = 8 pixels x 8 lines) of its
# TileMapConf, ASCII 32-95
cs = open(os.path.join(src, 'Cutscenes', 'dev', 'TileMapConf-sa2.asm'), encoding='latin1').read()
cs = cs[cs.index('._tile_0'):]
cdata = []
for l in cs.splitlines():
	m = re.match(r'\s*defb\s+(.*)', l, re.I)
	if not m: continue
	for v in m.group(1).split(';')[0].split(','):
		v = v.strip()
		if v: cdata.append(int(v[1:], 16) if v.startswith('$') else int(v, 0))
cf = Image.new('RGB', (8 * 64, 8))
pens = []
for y in range(8):
	for c in range(64):
		for x in range(2):
			pens += m1dec(cdata[c * 16 + y * 2 + x] if c * 16 + y * 2 + x < len(cdata) else 0)
cf.putdata([cpal[p] for p in pens])
cf.save(mk('gfx', 'cut_font.png'))

# ----------------------------------------------------------------------------
# Levels: 20 screens x 75 bytes (packed nibbles), then 60 enemies x 11 bytes
# (x, y words; x1 y1 x2 y2; mx my t), then 20 hotspots x 3 (unused)

# The levels are rebuilt from both versions. The CPC ones are the ZX ones with
# their 48 tiles brought down to 16 (each part has its tileset), except:
#   - CPC level 2 (ZX 1) lost the enemies of its screens 0-9, the boss with
#     them: the ZX ones are used;
#   - ZX level 2 is not in the CPC version: CPC level 4 is ZX level 4 again
#     (with part C's tiles). Here level 3 is ZX 2, converted to part A's tiles
#     (the ZX ones of levels 0-2 are the same set) with the tile mapping of ZX
#     levels 0-1 against CPC levels 1-2 (98.5% and 99.3% the same; the rest
#     are small hand edits of the CPC maps);
#   - CPC level 6 is not in the ZX version: kept as it is.
# So: level 1-2 CPC maps (part A), 3 ZX 2 (part A), 4-5 CPC (part B: ZX 3, 4),
# 6 CPC (part C); enemies from the ZX .ene files, but level 6's.

zxdir = sys.argv[3]
def cpc_level(p, n):
	d = unexo_file('Sir CPC 2 %s' % p, 'dev', 'level%dc.exo' % n)
	assert len(d) == 2220
	return d

def cpc_map(d):
	# 15 x 200, row by row: screen by screen (1 x 20)
	m = bytearray()
	for s_ in range(20):
		rows = [[0] * 15 for _ in range(10)]
		for i in range(150):
			b = d[s_ * 75 + i // 2]
			rows[i // 15][i % 15] = (b >> 4) if i % 2 == 0 else (b & 15)
		for r_ in rows: m += bytes(r_)
	return m

def cpc_enems(d, n):
	# 3 per screen, 8 bytes each for the .ene: t x y xx yy mn s1 s2 (tiles)
	e = bytearray()
	for k in range(60):
		x, y, x1, y1, x2, y2, mx, my, t = struct.unpack('<hhBBBBbbb', d[1500 + k * 11:1500 + k * 11 + 11])
		if t == 0:
			e += bytes(8); continue
		# start where the original starts it (always one corner of its box, as
		# in the ZX .ene files), end at the opposite corner
		if x in (x1, x2) and y in (y1, y2):
			sx, sy, ex, ey = x, y, x1 + x2 - x, y1 + y2 - y
		else:
			# (one does not: the corner it moves away from, on each axis)
			sx = x2 if mx < 0 else x1
			sy = y2 if my < 0 else y1
			ex, ey = x1 + x2 - sx, y1 + y2 - sy
			print('level %d screen %d: enemy starts at %d,%d, between its ends (moved to %d,%d)' % (n, k // 3, x, y, sx, sy))
		e += bytes([t, sx // 16, sy // 16, ex // 16, ey // 16, max(abs(mx), abs(my)), 0, 0])
	return e

def zx_enems(n):
	d = open(os.path.join(zxdir, 'sa2-l%d.ene' % n), 'rb').read()
	assert d[256:261] == bytes([1, 20, 15, 10, 3]), n
	return d[261:261 + 480]

def write_level(n, m, e, what):
	open(mk('map', 'level%d.map' % n), 'wb').write(bytes(m))
	hdr = bytearray(256)
	tag = ('SA2 level %d: %s' % (n + 1, what)).encode()[:127]
	hdr[:len(tag)] = tag
	# ponedor .ene: header, w h sw sh ne, 8 bytes per enemy, 3 per hotspot (none)
	open(mk('enems', 'level%d.ene' % n), 'wb').write(bytes(hdr) + bytes([1, 20, 15, 10, 3]) + bytes(e) + bytes(60))
	types = [e[k * 8] for k in range(60)]
	assert 6 in types, 'level %d has no boss' % (n + 1)

A0, A1 = cpc_level('A', 0), cpc_level('A', 1)
B2, B3 = cpc_level('B', 2), cpc_level('B', 3)
C5 = cpc_level('C', 5)

# ZX (set.bmp) tile -> part A tile: the most common pair in ZX 0-1 / CPC 1-2
import collections
pairs = collections.defaultdict(collections.Counter)
for zn, d in ((0, A0), (1, A1)):
	z = open(os.path.join(zxdir, 'level%d.MAP' % zn), 'rb').read()
	for a, b in zip(z, cpc_map(d)): pairs[a][b] += 1
zx2a = {t: c.most_common(1)[0][0] for t, c in pairs.items()}
z2 = open(os.path.join(zxdir, 'level2.MAP'), 'rb').read()
unseen = sorted(set(z2) - set(zx2a))
assert not unseen, 'ZX level 2 uses tiles never seen in levels 0-1: %s' % unseen

write_level(0, cpc_map(A0), zx_enems(0), 'CPC map, ZX 0 enemies')
write_level(1, cpc_map(A1), zx_enems(1), 'CPC map, ZX 1 enemies (the CPC ones lost screens 0-9)')
write_level(2, bytes(zx2a[t] for t in z2), zx_enems(2), 'ZX 2 converted to part A (missing in the CPC version)')
write_level(3, cpc_map(B2), zx_enems(3), 'CPC map, ZX 3 enemies')
write_level(4, cpc_map(B3), zx_enems(4), 'CPC map, ZX 4 enemies')
write_level(5, cpc_map(C5), cpc_enems(C5, 5), 'CPC only')
# level -> tileset (part)
LEVEL_PARTS = 'AAABBC'

# ----------------------------------------------------------------------------
# Songs

# The original's songs are for the old cpcwyzlib player; MK1's (WYZ 0.47c)
# reads them differently. Checked against the original's PSG registers (the
# title and the level jingle come out the same frame by frame):
#  * no loop table: tempo, flags, 2 reserved bytes and the channels. MK1's
#    player reads 4 loop offsets (words) after the header: 0 = each channel
#    loops back to its start, as the old one did;
#  * the instrument command (3F nn) has no relative volume: MK1's player
#    reads one more byte (3F nn vv) and takes tempo changes from it: vv = 0;
#  * 5 sections: A, B, C, a 2 byte one and the percussion one. The old
#    player never sounds the percussion (no noise, channel B untouched in the
#    original): MK1's channel P is the percussion one with every sound turned
#    into a silence of its length (a shorter channel P restarts the song);
#  * the notes are the old player's note table (wyz_player.h DATOS_NOTAS).
def old_mus(d):
	secs, cur, p = [], bytearray(), 4
	while p < len(d):
		b = d[p]; p += 1; cur.append(b)
		if b == 0:
			secs.append(cur); cur = bytearray(); continue
		if b & 0x3f == 0x3f:
			if b >> 6 == 0: cur.append(d[p]); p += 1; cur.append(0) 	# instrument
			elif b >> 6 in (1, 2): cur.append(d[p]); p += 1 			# effect, envelope
	P, q, i = secs[4], bytearray(), 0
	while i < len(P):
		b = P[i]
		if b & 0x3f == 0x3f and b >> 6 == 1: q.append((P[i + 1] & 0xc0) | 1); i += 2
		elif b & 0x3f == 0x3f and b >> 6 == 0: q += P[i:i + 3]; i += 3
		elif b & 0x3f == 0x3f and b >> 6 == 2: q += P[i:i + 2]; i += 2
		else: q.append(b); i += 1
	return bytes(d[:4]) + bytes(8) + b''.join(bytes(x) for x in secs[:3] + [q]) + bytes(cur)

songs = [('A', 'm_title', 'title'), ('A', 'm_level', 'level'), ('A', 'm_gover', 'gover')]
songs += [(p, 'm_level_%d' % (n + 1), 'level%d' % n) for p, n in (('A', 0), ('A', 1), ('B', 2), ('B', 3), ('C', 4), ('C', 5))]
songs += [('Cutscenes', 'm_intro', 'intro'), ('Cutscenes', 'm_ending', 'ending'), ('Cutscenes', 'm_title', 'cut_title')]
for p, f, name in songs:
	d = unexo_file(p if p == 'Cutscenes' else 'Sir CPC 2 %s' % p, 'dev', f + '.exo')
	open(mk('ogt', name + '.mus'), 'wb').write(old_mus(d))


# ----------------------------------------------------------------------------
# Instruments, drums and effects (sound.h of part A, the same in every part)
# -> dev/wyz/instrumentos.h and dev/wyz/efectos.h in MK1's player format:
# the old player's effects are 2 bytes per step, MK1's 3 (+ noise, 0 here).

snd = open(os.path.join(pdir('A'), 'dev', 'sound.h'), encoding='latin1').read()
def blocks(prefix):
	# ._prefixN followed by defb lines, up to the next label
	res = {}
	for m in re.finditer(r'\._%s(\d+)\s+((?:defb[^\n]*\n\s*)+)' % prefix, snd):
		vals = []
		for l in m.group(2).splitlines():
			if 'defb' not in l: continue
			for v in l.split('defb')[1].split(';')[0].split(','):
				v = v.strip()
				if not v: continue
				vals.append(int(v[1:], 16) if v.startswith('$') else int(v, 0))
		res[int(m.group(1))] = vals
	return res
sounds, pautas, efectos = blocks('sound'), blocks('pattern'), blocks('efecto')
tab_p = re.search(r'\._wyz_pattern_table((?:\s*defw[^\n]*)+)', snd).group(1)
pnums = [int(x) for x in re.findall(r'_pattern(\d+)', tab_p)]

def defbs(vals):
	# 16 values a line: zcpp 1.10 crashes on long lines
	return ''.join('\t\tdefb %s\n' % ', '.join(str(v) for v in vals[i:i + 16]) for i in range(0, len(vals), 16))

with open(mk('dev', 'wyz', 'instrumentos.h'), 'w') as f:
	f.write('// Sir Ababol 2 - instruments and drums of the original (sound.h)\n')
	f.write('// Generated by tools/sa2_extract.py\n\n#asm\n')
	# pattern table: the original's, by position (it has no _pattern6: the
	# songs' instrument 6 is _pattern7, 7 is _pattern8...)
	ids = sorted(pautas)
	names = ['PAUTA_%d' % n for n in pnums]
	f.write('\t.TABLA_PAUTAS\n' + ''.join('\t\tdefw %s\n' % ', '.join(names[i:i + 8]) for i in range(0, len(names), 8)))
	f.write('\t.TABLA_SONIDOS\n\t\tdefw ' + ', '.join('SONIDO%d' % n for n in sorted(sounds)) + '\n')
	for n in ids:
		f.write('\t.PAUTA_%d\n' % n + defbs(pautas[n]))
	for n in sorted(sounds):
		f.write('\t.SONIDO%d\n' % n + defbs(sounds[n]))
	f.write('#endasm\n')

with open(mk('dev', 'wyz', 'efectos.h'), 'w') as f:
	f.write('// Sir Ababol 2 - sound effects of the original (sound.h), 3 bytes per\n')
	f.write('// step for MK1\'s player (the original\'s 2 + noise 0)\n')
	f.write('// Generated by tools/sa2_extract.py\n\n#asm\n')
	names = ['EFECTO%d' % n for n in sorted(efectos)]
	f.write('\t.TABLA_EFECTOS\n' + ''.join('\t\tdefw %s\n' % ', '.join(names[i:i + 8]) for i in range(0, len(names), 8)))
	for n in sorted(efectos):
		v = efectos[n]
		assert v[-1] == 0xFF and (len(v) - 1) % 2 == 0, (n, len(v))
		steps = []
		for i in range(0, len(v) - 1, 2): steps += [v[i], v[i + 1], 0]
		f.write('\t.EFECTO%d\n' % n + defbs(steps + [255]))
	f.write('#endasm\n')
print('sound tables: %d instruments, %d drums, %d effects' % (len(pautas), len(sounds), len(efectos)))

print('done')
