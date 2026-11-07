#!/usr/bin/env python3
# Punchy (MK1 v3.2 ZX / CPC mode 0) - one-shot asset generator.
#
# Not part of the build: it was run once to make the placeholder resources in
# ../map, ../enems, ../gfx and dev/pn_frames.h from the NES tester
# (MK1_NES/testers/02_tester_punchy). The build only uses the MK1 tools on
# what this wrote. Run again only to regenerate the placeholders:
#
#   python3 pn_assets.py PATH/TO/02_tester_punchy
#
# - Map: NES screens are 16x12, MK1 ones 15x10: column 15 goes, and two rows
#   (0-1, or 0 and 2 in screen 3 to keep its top platform). Map value v is
#   tile v - 1 (Mappy export).
# - ZX: black, white and a 50% checker for midtones (by luminance). The
#   background colour of the tiles (the NES sky) is black.
# - CPC mode 0, split palette (CPC_DIRTY): 4 background inks chosen to fit
#   the tiles best (ink 0 = sky), 3 sprite inks. Art halved horizontally.

import sys, os
from PIL import Image

SRC = sys.argv[1] if len(sys.argv) > 1 else '../../ext/MK1_NES/testers/02_tester_punchy'
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')

CYAN = (0, 235, 219)            # NES transparent / sky
BLACK = (0, 0, 0)
def lum(c): return 0.299 * c[0] + 0.587 * c[1] + 0.114 * c[2]
def P(*p): return os.path.join(OUT, *p)

ss = Image.open(os.path.join(SRC, 'gfx', 'ss.png')).convert('RGB')
ts = Image.open(os.path.join(SRC, 'gfx', 'ts0.png')).convert('RGB')
font = Image.open(os.path.join(SRC, 'gfx', 'font.png')).convert('RGB')
prop = Image.open(os.path.join(SRC, 'gfx', 'propellers.png')).convert('RGB')

# ---------------------------------------------------------------------------
# Map and enemies

NSCR = 4
nes = open(os.path.join(SRC, 'map', 'level0.MAP'), 'rb').read()
NW = 16 * 16
DROP_ROWS = {0: (0, 1), 1: (0, 1), 2: (0, 1), 3: (0, 2)}
def keep_rows(s): return [r for r in range(12) if r not in DROP_ROWS[s]]
def new_row(s, r):
    k = keep_rows(s)
    return max(0, len([q for q in k if q < r]))

mk1 = bytearray(15 * NSCR * 10)
for s in range(NSCR):
    for y, r in enumerate(keep_rows(s)):
        for x in range(15):
            mk1[y * 15 * NSCR + s * 15 + x] = nes[r * NW + s * 16 + x] - 1
os.makedirs(P('map'), exist_ok=True)
open(P('map', 'level0.map'), 'wb').write(mk1)

# .ene (ponedor format): 2 x 128 bytes of names, map w, h, screen w, h,
# enemies per screen, then 8 bytes per enemy (t x y x2 y2 speed s1 s2) and
# 3 bytes per screen of hotspots. MK1 types: 1 punchy, 2 shooty, 3 linear,
# 32 + n warper to screen n (x2, y2: where the player appears; dsall).
ene = open(os.path.join(SRC, 'enems', 'level0.ene'), 'rb').read()
out = bytearray(b'level0.map'.ljust(128, b'\0') + b'ts.png'.ljust(128, b'\0'))
out += bytes([NSCR, 1, 15, 10, 3])
o = 261
for s in range(16):
    for e in range(3):
        t, x, y, xx, yy, mn, s1, s2 = ene[o:o + 8]; o += 8
        if s >= NSCR: continue
        if t == 0: out += bytes(8); continue
        x, xx = min(x, 14), min(xx, 14)
        y, yy = new_row(s, y), new_row(s, yy)
        if t == 0x41: t = 1
        elif t == 0x81: t = 2
        elif t == 0x01: t = 3
        elif t == 0xff:
            t = 32 + (mn & 15)
            xx, yy = min((s1 & 15), 14), new_row(mn & 15, s1 >> 4)
            mn = 0
        out += bytes([t, x, y, xx, yy, mn & 15, 0, 0])
out += bytes(3 * NSCR)
os.makedirs(P('enems'), exist_ok=True)
open(P('enems', 'level0.ene'), 'wb').write(out)

# ---------------------------------------------------------------------------
# Tiles: 16 from ts0 (tile 13, unused in the map, becomes the second frame
# of the propeller), then 16 object, 17 key, 18 refill (MK1 hotspots).

def tile(n):
    return ts.crop(((n % 16) * 16, (n // 16) * 16, (n % 16) * 16 + 16, (n // 16) * 16 + 16))

tiles = [tile(n) for n in range(16)]
t13 = tile(14).copy()
pf = prop.crop((16, 0, 32, 8))                 # propeller, second frame
t13.paste(pf, (0, 0))
tiles[14] = tile(14).copy(); tiles[14].paste(prop.crop((0, 0, 16, 8)), (0, 0))
tiles[13] = t13
def sprite_cell(cx, cy, w, h):
    return ss.crop((cx * 8, cy * 8, cx * 8 + w * 8, cy * 8 + h * 8))
coin = sprite_cell(1, 14, 2, 2); heart = sprite_cell(3, 14, 2, 2)
tiles += [coin, heart, heart, tile(31)]	# 16-18 hotspots, 19 breaking

# ---------------------------------------------------------------------------
# Sprites. Every frame is cut from ss.png (8x8 char grid). Frames face right.

TALL = [  # name, (cx, cy), width in chars, body column in the frame
    ('P_IDLE', (1, 1), 2, 0), ('P_PREWALK', (3, 1), 2, 0),
    ('P_WALK1', (5, 1), 2, 0), ('P_WALK2', (7, 1), 2, 0), ('P_WALK3', (9, 1), 2, 0), ('P_WALK4', (11, 1), 2, 0),
    ('P_UP', (13, 1), 2, 0), ('P_DOWN', (15, 1), 2, 0),
    ('P_PUNCH', (17, 1), 3, 0), ('P_KICK', (20, 1), 3, 0),
    ('P_CLIMB1', (23, 1), 2, 0), ('P_CLIMB2', (25, 1), 2, 0), ('P_CLIMB3', (27, 1), 2, 0),
    ('PU_WALK1', (1, 6), 2, 0), ('PU_WALK2', (3, 6), 2, 0), ('PU_HIT', (5, 6), 2, 0), ('PU_PUNCH', (7, 6), 3, 0),
    ('SH_WALK1', (10, 6), 2, 0), ('SH_WALK2', (12, 6), 2, 0), ('SH_HIT', (14, 6), 2, 0), ('SH_SHOOT', (16, 6), 2, 0),
    ('LI_WALK1', (1, 10), 2, 0), ('LI_WALK2', (3, 10), 2, 0),
]
PIECES = [('PC_PUNCH', 'P_PUNCH'), ('PC_KICK', 'P_KICK'), ('PC_PUPUNCH', 'PU_PUNCH')]

def transparent_frame(w, h): return Image.new('RGB', (w, h), CYAN)

tall_imgs, piece_imgs, piece_dy = [], [], {}
for name, (cx, cy), w, _ in TALL:
    im = sprite_cell(cx, cy, w, 4)
    tall_imgs.append((name, im.crop((0, 0, 16, 32))))
    if w == 3:
        p = im.crop((16, 0, 24, 32))
        cnt = [sum(p.getpixel((x, y)) != CYAN for x in range(8)) for y in range(32)]
        r0 = max(range(17), key=lambda r: (sum(cnt[r:r + 16]), -r))
        piece_dy[name] = r0
        piece_imgs.append((name, p.crop((0, r0, 8, r0 + 16))))
# warper (portal, 16x16) and explosion at the bottom of tall frames
for i in range(4):
    f = transparent_frame(16, 32); f.paste(sprite_cell(1 + 2 * i, 16, 2, 2), (0, 16))
    tall_imgs.append(('WA_%d' % i, f))
boom = transparent_frame(16, 32)
for y in range(16):
    for x in range(16):
        dx, dy = x - 7.5, y - 7.5
        r = (dx * dx + dy * dy) ** 0.5
        if r < 7.5 and ((x * 3 + y * 5) % 4 != 0 or r < 3): boom.putpixel((x, 16 + y), (255, 255, 255) if r < 4 else (248, 120, 88))
empty = transparent_frame(16, 32)
coco = sprite_cell(1, 22, 1, 1)

# player frame cell offsets (world px, facing right) from the NES metasprites:
# normal frames at prx - 4 = gpx, punch at prx - 2 = gpx + 2, kick at
# prx - 6 = gpx - 2; pieces right of the body.
BODY_DX = {'P_PUNCH': 2, 'P_KICK': -2}

# ---------------------------------------------------------------------------
# ZX: monochrome

def zx_level(c, sky_black=False):
    if c == CYAN: return 0 if sky_black else None
    l = lum(c)
    return 0 if l < 75 else (2 if l >= 175 else 1)

def zx_px(level, x, y):
    return 1 if level == 2 or (level == 1 and (x + y) & 1) else 0

W, K = (255, 255, 255), BLACK
def zx_tile(im):
    o = Image.new('RGB', im.size)
    for y in range(im.size[1]):
        for x in range(im.size[0]):
            o.putpixel((x, y), W if zx_px(zx_level(im.getpixel((x, y)), True), x, y) else K)
    return o

def zx_sprite_cell(im):
    """gfx + mask side by side (mkts_om mode=sprites): mask white = see through.
    Sprites go in negative (dark = white) so their outline shows on the black
    background."""
    w, h = im.size
    o = Image.new('RGB', (w * 2, h))
    for y in range(h):
        for x in range(w):
            c = im.getpixel((x, y))
            lv = zx_level(c)
            if lv is not None: lv = 2 - lv
            if lv is None:
                o.putpixel((x, y), K); o.putpixel((x + w, y), W)
            else:
                o.putpixel((x, y), W if zx_px(lv, x, y) else K); o.putpixel((x + w, y), K)
    return o

def sheet(cells, per_row):
    cw, ch = cells[0].size
    rows = (len(cells) + per_row - 1) // per_row
    o = Image.new('RGB', (cw * per_row, ch * rows))
    for i, c in enumerate(cells): o.paste(c, ((i % per_row) * cw, (i // per_row) * ch))
    return o, rows

G = P('gfx', 'speccy'); os.makedirs(G, exist_ok=True)
ts_zx = Image.new('RGB', (16 * len(tiles), 16))
for i, t in enumerate(tiles): ts_zx.paste(zx_tile(t), (i * 16, 0))
ts_zx.save(os.path.join(G, 'ts.png'))

def font_glyph(n):
    return font.crop(((n % 32) * 8, (n // 32) * 8, (n % 32) * 8 + 8, (n // 32) * 8 + 8))
fz = Image.new('RGB', (256, 16))
for n in range(64):
    g = font_glyph(n)
    for y in range(8):
        for x in range(8):
            if g.getpixel((x, y)) != CYAN: fz.putpixel(((n % 32) * 8 + x, (n // 32) * 8 + y), W)
fz.save(os.path.join(G, 'font.png'))

tall_zx, rows_tall = sheet([zx_sprite_cell(im) for _, im in tall_imgs], 8)
tall_zx.save(os.path.join(G, 'spr_tall.png'))
pc_zx, rows_pc = sheet([zx_sprite_cell(im) for _, im in piece_imgs], 8)
pc_zx.save(os.path.join(G, 'spr_piece.png'))
zx_sprite_cell(coco).save(os.path.join(G, 'spr_coco.png'))
zx_sprite_cell(boom).save(os.path.join(G, 'spr_boom.png'))
zx_sprite_cell(empty).save(os.path.join(G, 'spr_empty.png'))
zx_sprite_cell(transparent_frame(8, 16)).save(os.path.join(G, 'spr_piece_empty.png'))

# screens: text on black with the font (2x for the title)
def text(img, x, y, s, colour, scale=1, glyph_w=8):
    for i, ch in enumerate(s.upper()):
        n = ord(ch) - 32
        if n < 0 or n > 63: continue
        g = font_glyph(n)
        for yy in range(8):
            for xx in range(8):
                if g.getpixel((xx, yy)) != CYAN:
                    for a in range(scale):
                        for b in range(scale):
                            px = x + (i * 8 + xx) * scale + a
                            if glyph_w == 4: px = x + (i * 8 + xx) * scale // 2 + a // 2
                            img.putpixel((px, y + yy * scale + b), colour)

def screens(path, w, colour, frame_colour, glyph_w):
    sx = w / 256
    t = Image.new('RGB', (w, 192))
    text(t, int(56 * sx), 40, 'PUNCHY', colour, 3, glyph_w)
    text(t, int(40 * sx), 96, 'MK1 V3.2 PROTOTYPE', colour, 1, glyph_w)
    text(t, int(48 * sx), 112, 'PLACEHOLDER GRAPHICS', colour, 1, glyph_w)
    text(t, int(56 * sx), 152, 'PRESS ANY KEY', colour, 1, glyph_w)
    t.save(os.path.join(path, 'title.png'))
    m = Image.new('RGB', (w, 192))
    for x in range(w):
        for y in (12, 13, 178, 179): m.putpixel((x, y), frame_colour)
    text(m, int(8 * sx), 0, 'LIFE', colour, 1, glyph_w)
    text(m, int(104 * sx), 0, 'KILLS', colour, 1, glyph_w)
    text(m, int(8 * sx), 184, 'O P Q A SPACE  PUNCH: FIRE', colour, 1, glyph_w)
    m.save(os.path.join(path, 'marco.png'))
    e = Image.new('RGB', (w, 192))
    text(e, int(88 * sx), 88, 'THE END', colour, 1, glyph_w)
    e.save(os.path.join(path, 'ending.png'))
    return t

screens(G, 256, W, W, 8).save(os.path.join(G, 'loading.png'))

# ---------------------------------------------------------------------------
# CPC mode 0, split palette

LEV = (0, 128, 255)
CPC27 = [(r, g, b) for r in LEV for g in LEV for b in LEV]
def d2(a, b): return sum((x - y) ** 2 for x, y in zip(a, b))
def nearest(c, inks): return min(range(len(inks)), key=lambda i: d2(c, inks[i]))

# background: ink 0 = sky; best 3 more for the tiles
hist = {}
for t in tiles[:16]:
    for y in range(16):
        for x in range(16):
            c = t.getpixel((x, y)); hist[c] = hist.get(c, 0) + 1
sky = CPC27[nearest(CYAN, CPC27)]
best = None
cands = [c for c in CPC27 if c != sky]
import itertools
for trio in itertools.combinations(cands, 3):
    inks = [sky] + list(trio)
    err = sum(n * min(d2(c, k) for k in inks) for c, n in hist.items() if c != CYAN)
    if best is None or err < best[0]: best = (err, inks)
bg = sorted(best[1][1:], key=lum)
bg_inks = [sky] + bg                    # ink 3 = the brightest (the font)
# sprite ink 1 can't be black: black is pen 0 (see through) in the PNGs, and
# mkts_om takes the first pen of a colour. Dark blue does the outlines.
spr_inks = [(0, 0, 128), (255, 128, 128), (255, 255, 255)]

G = P('gfx', 'cpc'); os.makedirs(G, exist_ok=True)
def pal_png(cols, name):
    im = Image.new('RGB', (16, 1))
    for i, c in enumerate(cols): im.putpixel((i, 0), c)
    im.save(os.path.join(G, name))
pal_png([bg_inks[i >> 2] for i in range(16)], 'pal_cnvtiles.png')
pal_png([(0, 0, 0) if (i & 3) == 0 else spr_inks[(i & 3) - 1] for i in range(16)], 'pal_cnvspr.png')
combined = [spr_inks[(i & 3) - 1] if i & 3 else bg_inks[i >> 2] for i in range(16)]
pal_png(combined, 'pal.png')

def half(im):
    """16 px -> 8 mode 0 px: left pixel, or the right one if the left is the key"""
    w, h = im.size
    o = Image.new('RGB', (w // 2, h))
    for y in range(h):
        for x in range(w // 2):
            a, b = im.getpixel((2 * x, y)), im.getpixel((2 * x + 1, y))
            o.putpixel((x, y), b if a == CYAN else a)
    return o

def cpc_tile(im):
    o = half(im)
    for y in range(o.size[1]):
        for x in range(o.size[0]):
            o.putpixel((x, y), bg_inks[nearest(o.getpixel((x, y)), bg_inks)] if o.getpixel((x, y)) != CYAN else sky)
    return o

def spr_rank(c):
    if c == CYAN: return None
    if c == BLACK or lum(c) < 60: return 0
    if lum(c) > 200: return 2
    return 1

def cpc_sprite(im):
    o = half(im)
    for y in range(o.size[1]):
        for x in range(o.size[0]):
            r = spr_rank(o.getpixel((x, y)))
            o.putpixel((x, y), (0, 0, 0) if r is None else spr_inks[r])
    return o


tsc = Image.new('RGB', (8 * len(tiles), 16))
for i, t in enumerate(tiles):
    tsc.paste(cpc_tile(t), (i * 8, 0))
# hotspot tiles (16-18) are drawn like the rest: background inks
tsc.save(os.path.join(G, 'ts.png'))

fc = Image.new('RGB', (128, 16), sky)
for n in range(64):
    g = half(font_glyph(n))
    for y in range(8):
        for x in range(4):
            if g.getpixel((x, y)) != CYAN: fc.putpixel(((n % 32) * 4 + x, (n // 32) * 8 + y), bg_inks[3])
fc.save(os.path.join(G, 'font.png'))

sheet([cpc_sprite(im) for _, im in tall_imgs], 8)[0].save(os.path.join(G, 'spr_tall.png'))
sheet([cpc_sprite(im) for _, im in piece_imgs], 8)[0].save(os.path.join(G, 'spr_piece.png'))
cpc_sprite(coco).save(os.path.join(G, 'spr_coco.png'))
cpc_sprite(boom).save(os.path.join(G, 'spr_boom.png'))
t = screens(G, 128, (255, 255, 255), bg_inks[2], 4)

# ---------------------------------------------------------------------------
# dev/pn_frames.h: frame numbers and offsets

with open(P('dev', 'pn_frames.h'), 'w') as f:
    f.write('// Punchy frame numbers, generated once by tools/pn_assets.py\n\n')
    f.write('// 16x32 frames (spr_tall.png), facing right\n')
    for i, (n, _) in enumerate(tall_imgs): f.write('#define F_%s %d\n' % (n, i))
    f.write('#define PN_TALL_FRAMES %d\n\n' % len(tall_imgs))
    f.write('// 8x16 pieces (spr_piece.png): the part of a 24 px wide NES frame right\n// of its 16 px body; PD_ = rows from the top of the 32 px frame\n')
    for i, (n, _) in enumerate(piece_imgs): f.write('#define PC_%s %d\n#define PD_%s %d\n' % (n, i, n, piece_dy[n]))
    f.write('#define PN_PIECE_FRAMES %d\n\n' % len(piece_imgs))
    f.write('// world x of the body of a frame relative to gpx, facing right\n')
    for n, dx in BODY_DX.items(): f.write('#define DX_%s %d\n' % (n, dx))
    f.write('\n// CPC inks (RGB, for reference): background %s, sprites %s\n' % (bg_inks, spr_inks))

print('tiles', len(tiles), 'tall', len(tall_imgs), 'pieces', len(piece_imgs), 'rows', rows_tall)
print('bg inks', bg_inks, 'spr inks', spr_inks)
