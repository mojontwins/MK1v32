# nanako_zxgfx.py - extract the graphics of the original ZX version (2006)
# from its snapshot, as reference for the rework.
#
# usage: python3 nanako_zxgfx.py        (paths are relative to this script's folder)
#
# Reads ../orig/ncjmc-zx.z80 and writes ../gfx/zx_original/:
#   charset.png      the game's charset, codes 32-80 (CHARS = EB34 while it draws
#                    the game): tiles and Nanako. 16 per row.
#   udg.png          the 21 UDGs (A-U, FF58): boxes, karakasa, lives icon...
#   font.png         the font, codes 32-127 (CHARS = FB58 for texts)
#   tiles.png        16x16 metatiles, as the game draws them
#   sprites.png      Nanako (4 frames), karakasa (16x24), and the rest of the UDGs
#   *_x4.png         the same, 4x, with the codes
#   screen_*.png     title, level box and level 1, from orig/zx/*.scr
#
# The original is compiled BASIC: everything is PRINTed with characters. The
# colours are the ones seen in the game (an emulator run of the snapshot);
# the characters never seen on screen are white on black.

import os, struct
from PIL import Image, ImageDraw

H = os.path.dirname (os.path.abspath (__file__))
O = os.path.join (H, '..', 'orig')
D = os.path.join (H, '..', 'gfx', 'zx_original')
os.makedirs (D, exist_ok = True)

def z80_mem (f):
	d = open (f, 'rb').read ()
	hl, p7 = struct.unpack ('<H', d [30:32]) [0], d [35]
	def unrle (b):
		o, i = bytearray (), 0
		while i < len (b):
			if b [i] == 0xed and i + 1 < len (b) and b [i + 1] == 0xed:
				o += bytes ([b [i + 3]]) * b [i + 2]; i += 4
			else:
				o.append (b [i]); i += 1
		return o
	banks, i = {}, 32 + hl
	while i < len (d):
		l, pg = struct.unpack ('<HB', d [i:i + 3])
		n = 16384 if l == 0xffff else l
		banks [pg - 3] = bytes (d [i + 3:i + 3 + n]) if l == 0xffff else bytes (unrle (d [i + 3:i + 3 + n]))
		i += 3 + n
	m = bytearray (65536)
	m [0x4000:0x8000] = banks [5]; m [0x8000:0xc000] = banks [2]; m [0xc000:] = banks [p7 & 7]
	return m

M = z80_mem (os.path.join (O, 'ncjmc-zx.z80'))

def game (c): return M [0xeb34 + c * 8:0xeb34 + c * 8 + 8]
def udg (c): return M [0xff58 + c * 8:0xff58 + c * 8 + 8]
def font (c): return M [0xfb58 + c * 8:0xfb58 + c * 8 + 8]

ZX = [(0, 0, 0), (0, 0, 205), (205, 0, 0), (205, 0, 205), (0, 205, 0), (0, 205, 205), (205, 205, 0), (205, 205, 205)]
def rgb (k, br):
	return tuple (255 if v else 0 for v in ZX [k]) if br else ZX [k]

# Attributes seen in the game
WHITE = 0x47
GAME_ATTR = { 33: 0x45, 34: 0x05, 35: 0x41, 36: 0x01, 37: 0x03, 38: 0x5e, 39: 0x46, 40: 0x03, 41: 0x03, 42: 0x5e, 43: 0x30, 44: 0x70, 45: 0x30, 46: 0x30, 47: 0x47, 48: 0x07, 49: 0x07, 50: 0x45, 51: 0x46, 52: 0x44, 53: 0x44, 54: 0x04 }
for c in range (64, 81): GAME_ATTR [c] = 0x46
UDG_ATTR = { 18: 0x45, 19: 0x05, 8: 0x43, 9: 0x03, 10: 0x43, 11: 0x03, 12: 0x06, 13: 0x06, 14: 0x43, 15: 0x42, 16: 0x42, 17: 0x02, 20: 0x46 }

def char_img (g, attr):
	im = Image.new ('RGB', (8, 8))
	ink, pap = rgb (attr & 7, (attr >> 6) & 1), rgb ((attr >> 3) & 7, (attr >> 6) & 1)
	for y in range (8):
		for x in range (8):
			im.putpixel ((x, y), ink if g [y] & (128 >> x) else pap)
	return im

def sheet (cells, per):
	# cells: [(graphic, attr) or None]
	rows = (len (cells) + per - 1) // per
	im = Image.new ('RGB', (per * 8, rows * 8))
	for i, c in enumerate (cells):
		if c: im.paste (char_img (*c), ((i % per) * 8, (i // per) * 8))
	return im

def x4 (im, labels, per, w, h):
	# 4x with labels under each cell of w x h pixels
	big = im.resize ((im.width * 4, im.height * 4), Image.NEAREST)
	out = Image.new ('RGB', (big.width + (im.width // w) * 4, big.height + (im.height // h) * 12), (64, 64, 64))
	d = ImageDraw.Draw (out)
	for i, lab in enumerate (labels):
		cx, cy = i % per, i // per
		x0, y0 = cx * (w * 4 + 4), cy * (h * 4 + 12)
		out.paste (big.crop ((cx * w * 4, cy * h * 4, (cx + 1) * w * 4, (cy + 1) * h * 4)), (x0, y0))
		d.text ((x0, y0 + h * 4), lab, fill = (255, 255, 0))
	return out

def save (im, name, labels, per, w, h):
	im.save (os.path.join (D, name + '.png'))
	x4 (im, labels, per, w, h).save (os.path.join (D, name + '_x4.png'))

# Charsets
codes = list (range (32, 81))
save (sheet ([(game (c), GAME_ATTR.get (c, WHITE)) for c in codes], 16), 'charset', [str (c) for c in codes], 16, 8, 8)
save (sheet ([(udg (c), UDG_ATTR.get (c, WHITE)) for c in range (21)], 16), 'udg', [chr (65 + c) for c in range (21)], 16, 8, 8)
codes = list (range (32, 128))
save (sheet ([(font (c), WHITE) for c in codes], 16), 'font', [chr (c) for c in codes], 16, 8, 8)

# 16x16 metatiles
def meta (g4, a4):
	im = Image.new ('RGB', (16, 16))
	for i in range (4):
		im.paste (char_img (g4 [i], a4 [i]), ((i & 1) * 8, (i >> 1) * 8))
	return im

T, L = [], []
def metai (codes, attrs, inv = False, src = None):
	src = src or game
	gs = [bytes (255 - v for v in src (c)) if inv else src (c) for c in codes]
	return meta (gs, attrs)

# Platform styles, levels 1-5 (seen with the passwords of levels 2-5):
#   1: 33 34 / 34 33        2: 35 36 / 36 35       3: 43 44 / 45 46 (INVERSE)
#   4: 37 38 39 / 40 41 42 (3 chars wide: three tiles)      5: UDGs S T / S T
T.append (metai ((33, 34, 34, 33), (0x45, 0x05, 0x05, 0x45))); L.append ('lv1 33,34')
T.append (metai ((35, 36, 36, 35), (0x41, 0x01, 0x01, 0x41))); L.append ('lv2 35,36')
T.append (metai ((43, 44, 45, 46), (0x30, 0x70, 0x30, 0x30), inv = True)); L.append ('lv3 43-46 inv')
T.append (metai ((37, 38, 40, 41), (0x03, 0x5e, 0x03, 0x03))); L.append ('lv4 a')
T.append (metai ((39, 37, 42, 40), (0x46, 0x03, 0x5e, 0x03))); L.append ('lv4 b')
T.append (metai ((38, 39, 41, 42), (0x5e, 0x46, 0x03, 0x5e))); L.append ('lv4 c')
T.append (metai ((18, 19, 18, 19), (0x45, 0x05, 0x05, 0x05), src = udg)); L.append ('lv5 S,T')
# Fixed objects (the game picks one at random): skull, vase, blue crate;
# and the box (the crate's UDGs, red)
T.append (metai ((47, 48, 49, 50), (0x47, 0x07, 0x07, 0x45))); L.append ('47-50')
T.append (metai ((51, 52, 53, 54), (0x46, 0x44, 0x44, 0x04))); L.append ('51-54')
T.append (metai ((14, 15, 16, 17), (0x03, 0x41, 0x41, 0x01), src = udg)); L.append ('crate O-R')
T.append (metai ((14, 15, 16, 17), (0x43, 0x42, 0x42, 0x02), src = udg)); L.append ('box O-R')
im = Image.new ('RGB', (len (T) * 16, 16))
for i, t in enumerate (T): im.paste (t, (i * 16, 0))
save (im, 'tiles', L, len (T), 16, 16)

# Sprites: Nanako (left 67 68 / 69 70, 67 68 / 71 72; right 73 74 / 75 76,
# 73 74 / 77 78), karakasa (UDG I-N, 2x3), the other UDGs in 2x2
S, L = [], []
for a, b, c, d in ((67, 68, 69, 70), (67, 68, 71, 72), (73, 74, 75, 76), (73, 74, 77, 78)):
	t = Image.new ('RGB', (16, 24)); t.paste (meta ([game (a), game (b), game (c), game (d)], [0x46] * 4), (0, 8))
	S.append (t); L.append ('%d-%d' % (a, d))
t = Image.new ('RGB', (16, 24))
for i in range (6): t.paste (char_img (udg (8 + i), UDG_ATTR [8 + i]), ((i & 1) * 8, (i >> 1) * 8))
S.append (t); L.append ('karakasa')
for c in (0, 4):
	t = Image.new ('RGB', (16, 24)); t.paste (meta ([udg (c + i) for i in range (4)], [UDG_ATTR.get (c + i, WHITE) for i in range (4)]), (0, 8))
	S.append (t); L.append ('UDG %s-%s' % (chr (65 + c), chr (68 + c)))
im = Image.new ('RGB', (len (S) * 16, 24))
for i, t in enumerate (S): im.paste (t, (i * 16, 0))
save (im, 'sprites', L, len (S), 16, 24)

# Screens taken from the game running (orig/zx/*.scr, see orig/zx/README.md)
def scr_rgb (scr):
	im = Image.new ('RGB', (256, 192))
	for y in range (192):
		a = ((y & 0xc0) << 5) | ((y & 7) << 8) | ((y & 0x38) << 2)
		for cx in range (32):
			b = scr [a + cx]
			at = scr [6144 + (y >> 3) * 32 + cx]
			ink, pap = rgb (at & 7, (at >> 6) & 1), rgb ((at >> 3) & 7, (at >> 6) & 1)
			for p in range (8):
				im.putpixel ((cx * 8 + p, y), ink if b & (128 >> p) else pap)
	return im

for n in ('title', 'levelbox', 'ingame'):
	scr_rgb (open (os.path.join (O, 'zx', n + '.scr'), 'rb').read ()).save (os.path.join (D, 'screen_' + n + '.png'))

print ('gfx/zx_original: charset, udg, font, tiles, sprites, screens')
