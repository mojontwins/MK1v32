#!/usr/bin/env python3
# Nin Nin CPC - one-shot asset generator (not part of the build).
#
# From the NES sources (mojontwins/ninnin64), the 15x10 conversions made by
# yun2mk1.py (raw mode) and the graphics drawn for the CPC, writes:
#
#   map/levelN.map        15x10 screens, 1 byte per tile (level tile 0-15), for rle44map_sp
#   enems/levelN.ene      ponedor .ene (8-byte enemies, 3-byte hotspots), for ene2h
#   gfx/cpc/pal.png       the 4 pens the graphics are drawn with (teal, black, green, white)
#   gfx/cpc/palN.png      the 4 inks of level N (from the mockups)
#   gfx/cpc/font.png      64 chars (NES font, 4 pens)
#   gfx/cpc/work.png      the 48 tiles (CPC tileset as drawn, near-black fixed)
#   gfx/cpc/marco.png     the frame
#   gfx/cpc/title.png     a placeholder title (the frame)
#   gfx/cpc/sprites.png   fixed cells: player (8, drawn) + explosion, coco, punch, empty
#   gfx/cpc/spritesN.png  level cells: 8 enemy cells + 6 swim frames + drop + spare
#   dev/nn_levels.h       per level tables (tile maps, start, palettes, map sizes...)
#
# Levels kept (CPC level: NES level, NES map). The lava levels (NES 6, 9) are out.
#
# usage: nn_assets.py NINNIN64_DIR CONVERTED_DIR [OUT_DIR]

import os, re, sys
from PIL import Image

NES, CONV = sys.argv[1], sys.argv[2]
OUT = sys.argv[3] if len(sys.argv) > 3 else os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__))))

LEVELS = [  # (nes level, nes map, name)
	(0, '0', 'THE BOSQUE'), (1, '1', 'THE DESERT'), (2, '4', 'GOOD MARSH'), (3, '3', 'DE NADAL 1'),
	(4, '2', 'CUEVA CAVE'), (5, '5', 'NOSHE WOOD'), (7, '6', 'DE NADAL 2'), (8, '8', 'MARSHY GOO'),
	(10, 'B', 'ICY DUSTER'), (11, '7', 'EL CASTLER')]

# ---------------------------------------------------------------- NES tables
def c_array(txt, name):
	m = re.search(r'\b' + name + r'\s*\[\]\s*=\s*\{([^}]*)\}', txt)
	return [v.strip() for v in m.group(1).replace('\n', ' ').split(',') if v.strip()]

cfg = open(os.path.join(NES, 'dev/config.h')).read()
lvs = open(os.path.join(NES, 'dev/my/levelset.h')).read()
tsm = open(os.path.join(NES, 'dev/my/tsmap.h')).read()
ssm = open(os.path.join(NES, 'dev/my/enems_ssmap.h')).read()

behs_nes = [int(v) for v in c_array(cfg, 'behs')]
tsmaps = {i: [int(v) for v in c_array(tsm, 'tsmap%d' % i)][:16] for i in range(9)}
l_tsmap = [int(v[-1]) for v in c_array(lvs, 'l_tsmap')]
ssmaps = {i: [int(v) for v in c_array(ssm, 'enems_ssmap%d' % i)] for i in range(6)}
l_ssmap = [int(v[-1]) for v in c_array(lvs, 'l_enems_ssmap')]
l_alt_bg = [int(v) for v in c_array(lvs, 'l_alt_bg')]
l_scr_ini = [int(v) for v in c_array(lvs, 'l_scr_ini')]
l_ini_x = [int(v) for v in c_array(lvs, 'l_ini_x')]
l_ini_y = [int(v) for v in c_array(lvs, 'l_ini_y')]
l_pfacing = [int(v) for v in c_array(lvs, 'l_pfacing')]
l_locks = c_array(lvs, 'l_locks')

# The CPC tileset keeps MK1's hotspot tiles at 16 (refill), 17 (object), 18
# (key): NES tiles 45, 43, 44 swapped with 16, 17, 18.
SWAP = {16: 44, 17: 43, 18: 45, 44: 16, 43: 17, 45: 18}
def cpc_tile(t): return SWAP.get(t, t)
behs = [0] * 48
for t in range(48): behs[cpc_tile(t)] = behs_nes[t]

# ---------------------------------------------------------------- palettes
TEAL, BLACK, GREEN, WHITE = (0, 128, 128), (0, 0, 0), (0, 255, 0), (255, 255, 255)
PENS = [TEAL, BLACK, GREEN, WHITE]
P = {	# (pen 0, pen 1, pen 2, pen 3), from the mockups
	'forest': [TEAL, BLACK, GREEN, WHITE],
	'desert': [TEAL, BLACK, (255, 128, 0), (255, 255, 128)],
	'marsh': [(0, 128, 0), BLACK, (128, 255, 0), (255, 255, 128)],
	'ice': [(0, 0, 128), BLACK, (128, 128, 255), WHITE],
	'cave': [(128, 0, 128), BLACK, (128, 128, 128), (0, 255, 255)],
	'water': [TEAL, (0, 0, 128), GREEN, (255, 255, 128)]}
LPAL = ['forest', 'desert', 'marsh', 'water', 'cave', 'ice', 'water', 'marsh', 'ice', 'cave']

def pal_png(path, cols):
	im = Image.new('RGB', (len(cols), 1))
	for i, c in enumerate(cols): im.putpixel((i, 0), c)
	im.save(path)

def nearest(c, cols):
	return min(range(len(cols)), key=lambda i: sum((a - b) ** 2 for a, b in zip(c, cols[i])))

def repen(im, cols=PENS):
	# every pixel to the nearest of the 4 pens, drawn with the pen colours
	im = im.convert('RGB'); out = Image.new('RGB', im.size)
	for y in range(im.height):
		for x in range(im.width):
			out.putpixel((x, y), PENS[nearest(im.getpixel((x, y)), cols)])
	return out

# ---------------------------------------------------------------- NES sprites
ss = Image.open(os.path.join(NES, 'fiddle/ss.png')).convert('RGB')
NES_BG = [(44, 44, 44), (255, 0, 255)]

def lum(c): return 0.299 * c[0] + 0.587 * c[1] + 0.114 * c[2]

def nes_cell(n):
	# 16x16 cell n of ss.png to the 4 pens: background -> 0 (transparent),
	# black -> 1, the others by brightness -> 2 (darker), 3 (lighter)
	cx, cy = (n % 16) * 16, (n // 16) * 16
	cell = ss.crop((cx, cy, cx + 16, cy + 16))
	cols = sorted({c for c in cell.get_flattened_data() if c not in NES_BG and c != BLACK}, key=lum)
	m = {c: 0 for c in NES_BG}; m[BLACK] = 1
	if len(cols) == 1: m[cols[0]] = 3 if lum(cols[0]) > 150 else 2
	elif len(cols) == 2: m[cols[0]], m[cols[1]] = 2, 3
	else:
		mid = (lum(cols[0]) + lum(cols[-1])) / 2
		for c in cols: m[c] = 3 if lum(c) >= mid else 2
	out = Image.new('RGB', (16, 16))
	for y in range(16):
		for x in range(16):
			out.putpixel((x, y), PENS[m[cell.getpixel((x, y))]])
	return out

usr = Image.open(os.path.join(OUT, 'gfx/cpc/src_sprites.png')).convert('RGB')
def usr_cell(cx, cy):
	c = usr.crop((cx * 16, cy * 16, cx * 16 + 16, cy * 16 + 16))
	return repen(c)
USER_CELLS = {20: usr_cell(2, 1), 21: usr_cell(3, 1)}	# the skeleton replaces the NES bee (fanty)

def blank(): return Image.new('RGB', (16, 16), TEAL)

def sheet(cells, path, per_row=8):
	rows = (len(cells) + per_row - 1) // per_row
	im = Image.new('RGB', (per_row * 16, rows * 16), TEAL)
	for i, c in enumerate(cells): im.paste(c, ((i % per_row) * 16, (i // per_row) * 16))
	im.save(path)

def small(art):
	# a 16x16 cell from a list of strings ('.' 0, '#' 1, '+' 2, 'o' 3), top-left aligned
	im = blank()
	for y, r in enumerate(art):
		for x, ch in enumerate(r):
			im.putpixel((x, y), PENS['.#+o'.index(ch)])
	return im

COCO = small([
	'.####.', '#oo++#', '#o+++#', '#++++#', '#++++#', '.####.'])
PUNCH = small([
	'.#####.', '#ooo+o#', '#ooo+o#', '#++++o#', '.#####.'])

# ---------------------------------------------------------------- write gfx
G = os.path.join(OUT, 'gfx/cpc')
pal_png(os.path.join(G, 'pal.png'), PENS)
for i, k in enumerate(LPAL): pal_png(os.path.join(G, 'pal%d.png' % i), P[k])

font = Image.open(os.path.join(NES, 'fiddle/font.png')).convert('RGB')
repen(font, [(0, 67, 88), BLACK, (127, 127, 127), (191, 191, 191)]).save(os.path.join(G, 'font.png'))
repen(Image.open(os.path.join(G, 'src_tiles.png'))).save(os.path.join(G, 'work.png'))
marco = repen(Image.open(os.path.join(G, 'src_marco.png')))
marco.save(os.path.join(G, 'marco.png'))
fontp = Image.open(os.path.join(G, 'font.png'))

def text(im, cx, cy, s):
	for i, ch in enumerate(s):
		c = ord(ch) - 32
		im.paste(fontp.crop(((c % 32) * 8, (c // 32) * 8, (c % 32) * 8 + 8, (c // 32) * 8 + 8)), ((cx + i) * 8, cy * 8))

def centred(im, cy, s): text(im, 16 - len(s) // 2, cy, s)

title = marco.copy()
for cy, s in [(6, 'A NINJAJAR! ADVENTURE'), (10, '1  KEYBOARD'), (12, '2  JOYSTICK'),
              (16, 'O P  WALK   Q  JUMP'), (18, 'A  DIVE   SPACE  PUNCH')]:
	centred(title, cy, s)
title.save(os.path.join(G, 'title.png'))
for n, lines in [(1, ['WELL DONE!', '', 'NOW LOAD PART 2']), (2, ['WELL DONE!', '', 'NOW LOAD PART 3']),
                 (3, ['YOU DID IT!', '', 'NIN NIN IS HOME', 'WITH ALL THE ONIGIRI'])]:
	end = marco.copy()
	for k, s in enumerate(lines): centred(end, 9 + k * 2, s)
	end.save(os.path.join(G, 'ending%d.png' % n))

player = [usr_cell(i, 0) for i in range(8)]
sheet(player + [nes_cell(47), COCO, PUNCH, blank()], os.path.join(G, 'sprites.png'))

SWIM = [8, 9, 10, 12, 13, 14]	# player_frames + 8: frames 4 5 6 / 0 1 2
for i, (nl, nm, name) in enumerate(LEVELS):
	ids = ssmaps[l_ssmap[nl]]
	cells = [USER_CELLS.get(n) or nes_cell(n) for n in ids]
	swim = nl in (3, 7)
	cells += [nes_cell(n) for n in SWIM] if swim else [blank()] * 6
	cells += [nes_cell(46), blank()]
	sheet(cells, os.path.join(G, 'sprites%d.png' % i))

# ---------------------------------------------------------------- maps, enemies
import yun2mk1 as Y

def report(path):
	r = open(path).read()
	w, h = map(int, re.search(r'\((\d+)x(\d+) screens', r).groups())
	dels = {}
	for m in re.finditer(r'screen\s+(\d+)\s+rows-\[([\d, ]*)\] cols-\[([\d, ]*)\]', r):
		dels[int(m.group(1))] = ([int(v) for v in m.group(2).split(',') if v.strip()],
		                          [int(v) for v in m.group(3).split(',') if v.strip()])
	bad = [int(m.group(1)) for m in re.finditer(r'screen\s+(\d+).*<-- BAD', r)]
	return w, h, dels, bad

# Fixes by hand to screens the 15x10 reduction left worse (the report flags
# them BAD): (CPC level, screen): [(x, y, level tile), ...]
FIXES = {
	# GOOD MARSH 4: the second rock of the ledge in the middle (NES x 8-9) was in
	# the deleted column; without it the jump to the right ledge is too long
	(2, 4): [(9, 4, 5)],
}

def shrink(v, d):
	n = v - sum(1 for k in d if k < v)
	return n - 1 if v in d and n > 0 else n

info = []
for i, (nl, nm, name) in enumerate(LEVELS):
	stem = os.path.join(CONV, 'nn_l%02d_m%s_15x10' % (nl, nm))
	w, h, dels, bad = report(stem + '_report.txt')
	raw = bytearray(open(stem + '.map', 'rb').read())
	assert len(raw) == w * h * 150, (stem, len(raw))
	for (fl, fs), fx in FIXES.items():
		if fl == i:
			for x, y, t in fx: raw[(fs // w * 10 + y) * w * 15 + (fs % w) * 15 + x] = t
			bad = [b for b in bad if b != fs]
	open(os.path.join(OUT, 'map/level%d.map' % i), 'wb').write(raw)
	open(os.path.join(OUT, 'enems/level%d.ene' % i), 'wb').write(open(stem + '.ene', 'rb').read())
	# empty screens would get index 0 in rle44map_sp: check there are none
	for s in range(w * h):
		sx, sy = s % w, s // w
		if not any(raw[(sy * 10 + y) * w * 15 + sx * 15 + x] for y in range(10) for x in range(15)):
			pass  # all-0 screens: rle44map_sp keepempty encodes them
	scr = l_scr_ini[nl]
	dr, dc = dels[scr]
	tsmap = [cpc_tile(t) for t in tsmaps[l_tsmap[nl]]]
	has_locks = l_locks[nl] != '0' and tsmap[15] == 15
	info.append(dict(name=name, nl=nl, w=w, h=h, scr=scr, x=shrink(l_ini_x[nl], dc), y=shrink(l_ini_y[nl], dr),
		facing=l_pfacing[nl], tsmap=tsmap, alt=cpc_tile(l_alt_bg[nl]), lock=15 if has_locks else 99,
		bad=bad, swim=nl in (3, 7), pal=LPAL[i],
		stalac=int(l_tsmap[nl] in (2, 8))))	# NES deco_flags: tsmap2, tsmap8 (MARSHY GOO: per screen)

# ---------------------------------------------------------------- nn_levels.h
def arr(v): return ', '.join(str(x) for x in v)
o = ['// Nin Nin CPC - per level tables. Generated by tools/nn_assets.py, do not edit.', '']
o.append('// CPC level: NES level, name, map size, palette, start, bad screens (fix by hand)')
for i, d in enumerate(info):
	o.append('//  %d: NES %2d %-11s %2dx%-2d %-6s start %2d (%2d,%d)%s' % (i, d['nl'], d['name'], d['w'], d['h'],
		d['pal'], d['scr'], d['x'], d['y'], ('  BAD: ' + arr(d['bad'])) if d['bad'] else ''))
o.append('')
o.append('#define NN_LEVELS_ALL %d' % len(info))
o.append('')
o.append('// global tile behaviours (48 tiles, CPC numbering: 16 refill, 17 object, 18 key)')
o.append('unsigned char comportamiento_tiles [] = {')
for r in range(3): o.append('\t' + arr(behs[r * 16:(r + 1) * 16]) + (',' if r < 2 else ''))
o.append('};')
o.append('')
o.append('// level tile -> global tile (MAPPED_TILESETS), 16 per level')
o.append('const unsigned char nn_tsmaps [] = {')
for i, d in enumerate(info): o.append('\t' + arr(d['tsmap']) + (',' if i < len(info) - 1 else '') + '\t// %d' % i)
o.append('};')
for k, f in [('w', 'map_w'), ('h', 'map_h'), ('scr', 'scr_ini'), ('x', 'ini_x'), ('y', 'ini_y'),
             ('facing', 'facing'), ('alt', 'alt_bg_l'), ('lock', 'lock_tile'), ('stalac', 'stalac')]:
	o.append('const unsigned char nn_%s [] = { %s };' % (f, arr(d[k] for d in info)))
o.append('const unsigned char nn_swim [] = { %s };' % arr(int(d['swim']) for d in info))
o.append('')
o.append('// level names, for the level screen')
for i, d in enumerate(info): o.append('char nn_name%d [] = "%s";' % (i, d['name']))
o.append('char *nn_names [] = { %s };' % ', '.join('nn_name%d' % i for i in range(len(info))))
open(os.path.join(OUT, 'dev/nn_levels.h'), 'w').write('\n'.join(o) + '\n')

# a .txt the build scripts read: number width height lock
with open(os.path.join(OUT, 'map/levels.txt'), 'w') as f:
	for i, d in enumerate(info): f.write('%d %d %d %d\n' % (i, d['w'], d['h'], d['lock']))
print('\n'.join(o[:len(info) + 3]))
