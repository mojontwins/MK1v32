#!/usr/bin/env python3
"""
nes2png.py - ONE-SHOT placeholder generator. Not part of the build.

Makes the editable assets of the Yun ZX / CPC first load (NES levels 1-3) from
the NES v5 graphics, so there is something to look at until the real art is
drawn. Run it once; from then on the PNGs, maps and .ene files are the sources
and the build (dev/comp.bat, dev/comp_cpc.bat) only uses the MK1 utils.

  python3 nes2png.py YUN_V5_DIR CONVERTER_OUT_DIR GAME_DIR [--overwrite]

It does not overwrite existing files unless --overwrite is given.

Writes (README.md, "Recursos", has the formats):
  gfx/speccy/  font.png ts0-2.png tsx.png sprites.png sprites0-2.png
  gfx/cpc/     font.png ts0-2.png tsx.png sprites.png sprites0-2.png
               pal.png pal_cnvtiles0-2.png pal_cnvspr.png title.png marco.png ending.png
  enems/level0-2.ene   rewritten with 3-byte hotspots (ponedor's current format)
"""
import argparse, itertools, os, re, sys
from collections import Counter
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', '..', 'yun2mk1'))

LEVELS = (0, 1, 2)

# NES metasprite boxes in ss.png (from gfx/cut_patterns.spt), pixels
LIN_BOXES = {0: ((0, 24), (16, 24)), 1: ((32, 24), (48, 24)), 2: ((0, 40), (16, 40)),
             3: ((32, 40), (48, 40)), 4: ((0, 56), (16, 56)), 5: ((32, 56), (48, 56)),
             6: ((64, 56), (80, 56))}
PLAT_BOXES = ((96, 24), (112, 24))
BUB_BOXES = ((96, 72), (112, 72))
FANTY_BOXES = ((64, 40), (80, 40))
MC_APPEAR = ((16, 88), (64, 24))
MC_OUT = (80, 24)
COCO_BOX = (120, 40, 128, 48)
BOOM_BOX = (64, 72, 80, 88)
HOT_BOXES = ((16, 72, 32, 88), (48, 72, 64, 88), (0, 72, 16, 88))   # tiles 16-18: refill, star, key

# ---------------------------------------------------------------------------
# helpers

ZX = [(0, 0, 0), (0, 0, 192), (192, 0, 0), (192, 0, 192), (0, 192, 0), (0, 192, 192), (192, 192, 0), (192, 192, 192)]
ZXB = [(0, 0, 0), (0, 0, 255), (255, 0, 0), (255, 0, 255), (0, 255, 0), (0, 255, 255), (255, 255, 0), (255, 255, 255)]
BLACK = (0, 0, 0)

def pixels(im): return [im.getpixel((x, y)) for y in range(im.size[1]) for x in range(im.size[0])]

def lum(c): return 0.299 * c[0] + 0.587 * c[1] + 0.114 * c[2]

def nearest_zx(c):
    m = max(c)
    if m == 0: return 0, 0
    n = tuple(v * 255 // m for v in c)
    i = min(range(1, 8), key=lambda k: sum((a - b) ** 2 for a, b in zip(n, ZXB[k])))
    return i, 1 if m >= 200 else 0

def cell_to_zx(px, sparse=False, x0=0, y0=0):
    """8x8 RGB -> 8x8 in ZX colours: black paper, one ink. sparse: stippled,
    so OR sprites read over background tiles."""
    cols = Counter(c for row in px for c in row if c != BLACK)
    if not cols: return [[BLACK] * 8 for _ in range(8)]
    inkc = max(cols, key=lambda c: cols[c] * (0.5 + lum(c) / 255))
    i, b = nearest_zx(inkc)
    ink = (ZXB if b else ZX)[i or 1]
    thr = min(lum(c) for c in cols) if len(cols) > 1 else 1
    out = []
    for y, row in enumerate(px):
        o = []
        for x, c in enumerate(row):
            on = c != BLACK and (len(cols) == 1 or lum(c) > thr or cols[c] < 8)
            if sparse and on:
                xx, yy = x0 + x, y0 + y
                on = (xx % 4 == 0 and yy % 4 == 0) or (xx % 4 == 2 and yy % 4 == 2)
            o.append(ink if on else BLACK)
        out.append(o)
    return out

def put_cells(img, x, y, cells):
    for yy, row in enumerate(cells):
        for xx, c in enumerate(row): img.putpixel((x + xx, y + yy), c)

def crop16(ss, box, squash=False, flipx=False, flipy=False, size=(16, 16)):
    im = ss.crop(box)
    if squash and im.size != size:                 # nearest-neighbour rows: keeps the silhouette
        w, h = im.size
        sq = Image.new('RGB', size)
        for y in range(size[1]):
            sy = int(y * h / size[1] + 0.5 * h / size[1])
            for x in range(size[0]):
                sq.putpixel((x, y), im.getpixel((x * w // size[0], min(h - 1, sy))))
        im = sq
    elif im.size != size:
        full = Image.new('RGB', size)
        full.paste(im, ((size[0] - im.size[0]) // 2, (size[1] - im.size[1]) // 2))
        im = full
    if flipx: im = im.transpose(Image.FLIP_LEFT_RIGHT)
    if flipy: im = im.transpose(Image.FLIP_TOP_BOTTOM)
    return im

def mono(im):
    """NES sprite -> 1bpp white on black: outline (darkest colour + edges) and highlights."""
    cols = sorted({c for c in pixels(im) if c != BLACK}, key=lum)
    lit = {cols[0], cols[-1]} if cols else set()
    w, h = im.size
    out = Image.new('RGB', im.size)
    for y in range(h):
        for x in range(w):
            c = im.getpixel((x, y))
            edge = c != BLACK and any(not (0 <= x + dx < w and 0 <= y + dy < h) or im.getpixel((x + dx, y + dy)) == BLACK
                                      for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
            if c in lit or edge: out.putpixel((x, y), (255, 255, 255))
    return out

def rank3(colours):
    """non-black colours by luminance into 1..3"""
    cols = sorted(set(colours) - {BLACK}, key=lum)
    return {c: 1 + min(2, i * 3 // max(1, len(cols))) for i, c in enumerate(cols)}

def half(row):                                     # 8 px -> 4 mode 0 px (left unless 0)
    return [row[2 * i] or row[2 * i + 1] for i in range(len(row) // 2)]

# CPC firmware inks: n = 9 G + 3 R + B, levels 0 / 128 / 255
LEV = (0, 128, 255)
def fw_rgb(n): return (LEV[(n // 3) % 3], LEV[n // 9], LEV[n % 3])
def cdist(a, b): return sum((x - y) ** 2 for x, y in zip(a, b))
def nearest_fw(c): return min(range(27), key=lambda n: cdist(c, fw_rgb(n)))

def read_c_array(txt, name):
    m = re.search(r'\b' + name + r'\s*\[\]\s*=\s*\{([^}]*)\}', txt)
    body = re.sub(r'//[^\n]*', '', m.group(1))
    return [int(v, 0) for v in body.replace('\n', ' ').split(',') if v.strip()]

def chr_patterns(path):
    d = open(path, 'rb').read()
    pats = []
    for p in range(256):
        lo, hi = d[p * 16:p * 16 + 8], d[p * 16 + 8:p * 16 + 16]
        pats.append([[((lo[y] >> (7 - x)) & 1) | (((hi[y] >> (7 - x)) & 1) << 1) for x in range(8)] for y in range(8)])
    return pats

# ---------------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('yun'); ap.add_argument('conv'); ap.add_argument('game')
    ap.add_argument('--overwrite', action='store_true')
    a = ap.parse_args()
    import yun2mk1 as Y

    G = a.game
    ZD, CD = os.path.join(G, 'gfx', 'speccy'), os.path.join(G, 'gfx', 'cpc')
    for d in (ZD, CD, os.path.join(G, 'enems')): os.makedirs(d, exist_ok=True)
    def save(img, *path):
        p = os.path.join(*path)
        if os.path.exists(p) and not a.overwrite: print('kept', p); return
        img.save(p)

    lvl = open(os.path.join(a.yun, 'dev', 'assets', 'levelset.h')).read()
    order = [int(x) for x in re.findall(r'map_(\d+)_scr_offsets', lvl)]
    tsorder = [int(x) for x in re.findall(r'ts(\d+)_tmaps', lvl.split('l_ts_tmaps')[1].split(';')[0])]
    tiledata = open(os.path.join(a.yun, 'dev', 'assets', 'tiledata.h')).read()
    pats = chr_patterns(os.path.join(a.yun, 'dev', 'tileset0.chr'))
    all_behs = Y.read_behs(a.yun)
    ss = Image.open(os.path.join(a.yun, 'gfx', 'ss.png')).convert('RGB')

    # ---- level tiles: per tile, its 4 cells as (CHR pattern, 8x8 RGB)
    L = []
    for li in LEVELS:
        m = order[li]
        tm = read_c_array(tiledata, 'ts%d_tmaps' % tsorder[li])
        tsimg = Image.open(os.path.join(a.yun, 'gfx', 'ts%d.png' % m)).convert('RGB')
        tiles = []
        for t in range(16):
            cells = []
            for k in range(4):
                bx, by = t * 16 + (k & 1) * 8, (k >> 1) * 8
                cells.append((tm[t * 4 + k], [[tsimg.getpixel((bx + x, by + y)) for x in range(8)] for y in range(8)]))
            tiles.append(cells)
        # NES enemy sprite -> MK1 linear type 1-3, in order of appearance (as the .ene files have them)
        ene = Y.read_ene(os.path.join(a.conv, 'level%d_15x10.ene' % m))
        tmap = {}
        for s in ene['enems']:
            for e in s:
                if e[0] >> 4 == 1 and (e[0] & 15) not in tmap: tmap[e[0] & 15] = len(tmap) + 1
        L.append(dict(li=li, m=m, behs=all_behs[m][:16], tiles=tiles, tmap=tmap))

    # ======================================================================
    # ZX

    # tiles: background tiles (behaviour 0) stippled so OR sprites read over them
    for Lv in L:
        im = Image.new('RGB', (256, 16))
        for t, cells in enumerate(Lv['tiles']):
            for k, (p, rgb) in enumerate(cells):
                x0, y0 = (k & 1) * 8, (k >> 1) * 8
                put_cells(im, t * 16 + x0, y0, cell_to_zx(rgb, Lv['behs'][t] == 0, x0, y0))
        save(im, ZD, 'ts%d.png' % Lv['li'])
    im = Image.new('RGB', (48, 16))
    for i, box in enumerate(HOT_BOXES):
        tile = ss.crop(box)
        for k in range(4):
            x0, y0 = (k & 1) * 8, (k >> 1) * 8
            put_cells(im, i * 16 + x0, y0, cell_to_zx([[tile.getpixel((x0 + x, y0 + y)) for x in range(8)] for y in range(8)]))
    save(im, ZD, 'tsx.png')

    # font: blank, 0-9, A E G M O R V. CHR letters/digits (colour 3); G and V
    # are not in the CHR, drawn here in the same slanted style
    extra = {'G': ('..33331.', '.331....', '.31.....', '.31.3331', '.31...31', '.331.331', '..3331..', '........'),
             'V': ('.31...31', '.31...31', '.31..331', '.31..31.', '.331331.', '..3331..', '...31...', '........')}
    chr_of = {'A': 0x2e, 'E': 0x2b, 'M': 0x35, 'O': 0x31, 'R': 0x28}
    font = []
    for ch in ' 0123456789AEGMORV':
        if ch == ' ': font.append([[0] * 8 for _ in range(8)])
        elif ch.isdigit(): font.append(pats[0x53 + int(ch)])
        elif ch in chr_of: font.append(pats[chr_of[ch]])
        else: font.append([['.123'.index(c) for c in r] for r in extra[ch]])
    im = Image.new('RGB', (8 * len(font), 8))
    for i, f in enumerate(font):
        put_cells(im, i * 8, 0, [[(255, 255, 255) if v == 3 else BLACK for v in row] for row in f])
    save(im, ZD, 'font.png')

    # sprites: fixed sheet (22 cells) + one sheet of 12 enemy cells per level
    P = lambda n: (n * 16, 0, n * 16 + 16, 24)
    B = lambda xy: xy + (xy[0] + 16, xy[1] + 16)
    def fixed_cells(enc):
        c = [None] * 22
        for i, f in enumerate((1, 2, 3)):
            c[i] = enc(crop16(ss, P(f), squash=True)); c[4 + i] = enc(crop16(ss, P(f), squash=True, flipx=True))
        c[3] = enc(crop16(ss, P(5), squash=True)); c[7] = enc(crop16(ss, P(5), squash=True, flipx=True))
        for f in range(2):
            c[8 + f] = enc(crop16(ss, B(BUB_BOXES[f])))
            c[10 + f] = enc(crop16(ss, B(PLAT_BOXES[f])))
            c[12 + f] = enc(crop16(ss, B(FANTY_BOXES[f])))
            c[14 + f] = enc(crop16(ss, B(FANTY_BOXES[f]), flipy=True))
        c[16] = enc(crop16(ss, B(MC_APPEAR[0])))
        c[17] = enc(crop16(ss, B(MC_APPEAR[1])))
        c[18] = enc(crop16(ss, B(MC_OUT)))
        c[19] = enc(crop16(ss, B(MC_OUT), flipy=True))
        c[20] = enc(crop16(ss, COCO_BOX))
        c[21] = enc(crop16(ss, BOOM_BOX))
        return c
    def level_cells(enc, tmap):
        inv = {v: k for k, v in tmap.items()}
        c = [None] * 12
        for t in (1, 2, 3):
            nes = inv.get(t, inv.get(1, 0))
            for f, xy in enumerate(LIN_BOXES[nes]):
                c[2 * (t - 1) + f] = enc(crop16(ss, B(xy)))
                c[6 + 2 * (t - 1) + f] = enc(crop16(ss, B(xy), flipy=True))
        return c
    def sheet(cells, cw, rows):
        im = Image.new('RGB', (16 * cw, 16 * rows))
        for n, c in enumerate(cells): im.paste(c, ((n % 16) * cw, (n // 16) * 16))
        return im
    save(sheet(fixed_cells(mono), 16, 2), ZD, 'sprites.png')
    for Lv in L: save(sheet(level_cells(mono, Lv['tmap']), 16, 1), ZD, 'sprites%d.png' % Lv['li'])

    # ======================================================================
    # CPC, mode 0 at its own resolution (a 16x16 tile is 8x16 pixels).
    # Split palette: background pens 0, 4, 8, 12, sprite pens 1-3 (pen = bg * 4 + spr)

    ycols = sorted([c for c, _ in Counter(pixels(ss.crop((0, 0, 128, 24)))).most_common() if c != BLACK][:3], key=lum)
    spr_rgb = [BLACK] + [fw_rgb(nearest_fw(c)) for c in ycols]

    # background inks per level: the 4 that best cover the level tiles' colours
    # (NES colour 0 first); every cell maps its colours to the nearest of them
    lvl_rgb = []
    for Lv in L:
        hist = Counter(c for cells in Lv['tiles'] for _, rgb in cells for row in rgb for c in row)
        bg0 = max((rgb[y][x] for cells in Lv['tiles'] for p, rgb in cells for y in range(8) for x in range(8)
                   if pats[p][y][x] == 0), key=lambda c: hist[c])
        f0 = nearest_fw(bg0)
        best = min(((f0,) + combo for combo in itertools.combinations([n for n in range(27) if n != f0], 3)),
                   key=lambda inks: sum(n * min(cdist(c, fw_rgb(fw)) for fw in inks) for c, n in hist.items()))
        inks = [fw_rgb(n) for n in best]
        lvl_rgb.append(inks)
        im = Image.new('RGB', (128, 16))
        for t, cells in enumerate(Lv['tiles']):
            for k, (p, rgb) in enumerate(cells):
                cmap = {pats[p][y][x]: rgb[y][x] for y in range(8) for x in range(8)}
                m4 = {v: min(range(4), key=lambda j: cdist(c, inks[j])) for v, c in cmap.items()}
                put_cells(im, t * 8 + (k & 1) * 4, (k >> 1) * 8,
                          [[inks[j] for j in half([m4[v] for v in row])] for row in pats[p]])
        save(im, CD, 'ts%d.png' % Lv['li'])
        save(pal_img([inks[i >> 2] for i in range(16)]), CD, 'pal_cnvtiles%d.png' % Lv['li'])
    save(pal_img([spr_rgb[i & 3] for i in range(16)]), CD, 'pal_cnvspr.png')
    # pal.png: title, frame and ending; pens 1-3 must stay the sprite inks (the frame is
    # drawn with them and stays on screen during the game)
    save(pal_img([spr_rgb[i & 3] if i & 3 else lvl_rgb[0][i >> 2] for i in range(16)]), CD, 'pal.png')

    def enc_cpc(im):                            # 16x16 RGB -> 8x16 in sprite inks
        r = rank3(pixels(im))
        out = Image.new('RGB', (8, 16))
        put_cells(out, 0, 0, [[spr_rgb[v] for v in half([r.get(im.getpixel((x, y)), 0) for x in range(16)])] for y in range(16)])
        return out
    # hotspot tiles: sprite inks, so they look the same in every level
    im = Image.new('RGB', (24, 16))
    for i, box in enumerate(HOT_BOXES): im.paste(enc_cpc(ss.crop(box)), (i * 8, 0))
    save(im, CD, 'tsx.png')
    # font: background colour 3 of level 0 (the engine prints those pixels in sprite pen 3)
    im = Image.new('RGB', (4 * len(font), 8))
    for i, f in enumerate(font):
        put_cells(im, i * 4, 0, [[lvl_rgb[0][3] if v else BLACK for v in half([v == 3 for v in row])] for row in f])
    save(im, CD, 'font.png')
    save(sheet(fixed_cells(enc_cpc), 8, 2), CD, 'sprites.png')
    for Lv in L: save(sheet(level_cells(enc_cpc, Lv['tmap']), 8, 1), CD, 'sprites%d.png' % Lv['li'])

    # screens: text with Yun's font at half width, in the sprite inks
    fnt = Image.open(os.path.join(a.yun, 'gfx', 'font.png')).convert('RGB')
    def cscreen(lines):
        im = Image.new('RGB', (128, 192), lvl_rgb[0][0])
        for cy, cx, txt, pen in lines:
            if cx is None: cx = (32 - len(txt)) // 2
            for i, ch in enumerate(txt.upper()):
                n = ord(ch) - 32
                if not 0 <= n < 64: continue
                for y in range(8):
                    for x in range(4):
                        px = [fnt.getpixel(((n % 32) * 8 + 2 * x + d, (n // 32) * 8 + y)) for d in (0, 1)]
                        if any(lum(c) > 60 for c in px): im.putpixel(((cx + i) * 4 + x, cy * 8 + y), spr_rgb[pen])
        return im
    save(cscreen([(6, None, 'YUN', 3), (9, None, 'AMSTRAD CPC - LEVELS 1-3', 2),
                  (14, None, 'O P Q   SPACE: BUBBLE', 3), (17, None, '1 KEYS  2 JOYSTICK', 2),
                  (22, None, 'THE MOJON TWINS', 1)]), CD, 'title.png')
    save(cscreen([(0, 0, 'LIFE', 3), (0, 9, 'KEY', 3), (0, 17, 'KO', 3), (0, 22, '/', 3)]), CD, 'marco.png')
    save(cscreen([(10, None, 'WELL DONE!', 3), (13, None, 'END OF THE FIRST LOAD', 2)]), CD, 'ending.png')

    # ======================================================================
    # .ene: 3-byte hotspots (x, y, type), the level's map and tileset in the header
    for Lv in L:
        path = os.path.join(G, 'enems', 'level%d.ene' % Lv['li'])
        d = open(path, 'rb').read()
        w, h, sw, sh, ne = d[256:261]
        p = 261 + w * h * ne * 8
        if len(d) == p + 3 * w * h: print('kept', path, '(already 3-byte)'); continue
        assert len(d) == p + 2 * w * h, path
        hdr = bytearray(256)
        for off, s in ((0, '..\\map\\level%d.map' % Lv['li']), (128, '..\\gfx\\speccy\\ts%d.png' % Lv['li'])):
            hdr[off:off + len(s)] = s.encode()
        out = hdr + d[256:p]
        for s in range(w * h):
            xy, t = d[p + 2 * s], d[p + 2 * s + 1]
            out += bytes([xy >> 4, xy & 15, t])
        open(path, 'wb').write(out)
        print('wrote', path)

    print('CPC inks: levels %s, sprites %s' % ([[nearest_fw(c) for c in l] for l in lvl_rgb], [nearest_fw(c) for c in spr_rgb[1:]]))

def pal_img(cols):
    im = Image.new('RGB', (16, 1))
    for i, c in enumerate(cols): im.putpixel((i, 0), c)
    return im

if __name__ == '__main__':
    main()
