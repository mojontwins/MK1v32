#!/usr/bin/env python3
"""
mkassets.py - Build the MK1 v3.2 resources for the Yun ZX prototype from the
NES v5 sources + yun2mk1 converter output.

  python3 mkassets.py YUN_V5_DIR CONVERTER_OUT_DIR GAME_DIR [--map 4] [--fps 16.7]

Writes into GAME_DIR:
  map/mapa.map               15x10 map (raw, 1 byte/tile)
  enems/enems.ene            MK1 .ene (types 1-4, 2-byte legacy hotspots)
  gfx/speccy/work.png        256x48 tileset (tiles 0-15 + hotspot tiles 16-18)
  gfx/speccy/mappy.png       same, for Mappy
  gfx/speccy/font.png        256x16 font (from Yun's)
  gfx/speccy/sprites.png     256x32, 32 OR cells (sprcnv2 ... 32 nomask)
  gfx/speccy/{title,marco,ending,loading}.png
  dev/yun_level.h            level constants (kills to win, start pos, behaviours)
  dev/yun_luts.h             bubble movement tables resampled to the ZX frame rate

Everything here is placeholder art meant to be painted over.
"""
import argparse, os, sys, re
from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', '..', 'yun2mk1'))

# --------------------------------------------------------------------------
# ZX palette and helpers

ZX = [(0, 0, 0), (0, 0, 192), (192, 0, 0), (192, 0, 192), (0, 192, 0), (0, 192, 192), (192, 192, 0), (192, 192, 192)]
ZXB = [(0, 0, 0), (0, 0, 255), (255, 0, 0), (255, 0, 255), (0, 255, 0), (0, 255, 255), (255, 255, 0), (255, 255, 255)]

def lum(c): return 0.299 * c[0] + 0.587 * c[1] + 0.114 * c[2]

def nearest_zx(c):
    """Match hue against the 8 ZX colours (colour normalised to full intensity),
    brightness from the colour's own intensity."""
    m = max(c)
    if m == 0: return 0, 0
    n = tuple(v * 255 // m for v in c)
    i = min(range(1, 8), key=lambda k: sum((a - b) ** 2 for a, b in zip(n, ZXB[k])))
    return i, 1 if m >= 200 else 0

def cell_to_zx(px, sparse=False, sparse_keep=0.25, x0=0, y0=0):
    """px: 8x8 list of RGB. Returns 8x8 RGB in ZX colours: black paper, one ink
    (the dominant non-black colour, brightness of that colour)."""
    cols = {}
    for row in px:
        for c in row:
            if c != (0, 0, 0): cols[c] = cols.get(c, 0) + 1
    if not cols:
        return [[(0, 0, 0)] * 8 for _ in range(8)]
    # ink = most frequent non-black colour, weighted to brighter ones
    inkc = max(cols, key=lambda c: cols[c] * (0.5 + lum(c) / 255))
    i, b = nearest_zx(inkc)
    if i == 0: i = 1
    ink = (ZXB if b else ZX)[i]
    thr = sorted(lum(c) for c in cols)[0] if len(cols) > 1 else 1
    out = []
    for y, row in enumerate(px):
        o = []
        for x, c in enumerate(row):
            on = c != (0, 0, 0) and (len(cols) == 1 or lum(c) > thr or cols[c] < 8)
            if sparse and on:
                # keep a sparse, regular subset so OR sprites stay readable
                xx, yy = x0 + x, y0 + y
                on = (xx % 4 == 0 and yy % 4 == 0) or (xx % 4 == 2 and yy % 4 == 2)
            o.append(ink if on else (0, 0, 0))
        out.append(o)
    return out

def tile_to_zx(img, sparse=False):
    px = [[img.getpixel((x, y)) for x in range(16)] for y in range(16)]
    out = Image.new('RGB', (16, 16))
    for cy in range(2):
        for cx in range(2):
            cell = [r[cx * 8:cx * 8 + 8] for r in px[cy * 8:cy * 8 + 8]]
            z = cell_to_zx(cell, sparse, x0=cx * 8, y0=cy * 8)
            for y in range(8):
                for x in range(8):
                    out.putpixel((cx * 8 + x, cy * 8 + y), z[y][x])
    return out

def mono(img, box, size=(16, 16), squash=False, flipx=False, flipy=False):
    """NES sprite -> 1bpp white-on-black. Lit: outline (darkest colour) + highlights (brightest)."""
    im = img.crop(box).convert('RGB')
    if squash and im.size != size:
        # nearest-neighbour row sampling keeps the silhouette
        w, h = im.size
        sq = Image.new('RGB', size)
        for y in range(size[1]):
            sy = int(y * h / size[1] + 0.5 * h / size[1])
            for x in range(size[0]):
                sq.putpixel((x, y), im.getpixel((x * w // size[0], min(h - 1, sy))))
        im = sq
    elif im.size != size:
        im = im.crop((0, 0) + size)
    cols = sorted({c for c in im.getdata() if c != (0, 0, 0)}, key=lum)
    lit = set()
    if cols:
        lit.add(cols[0])
        lit.add(cols[-1])
    out = Image.new('RGB', size)
    for y in range(size[1]):
        for x in range(size[0]):
            c = im.getpixel((x, y))
            # outline: any coloured pixel touching transparency is lit too
            edge = c != (0, 0, 0) and any(
                not (0 <= x + dx < size[0] and 0 <= y + dy < size[1]) or im.getpixel((x + dx, y + dy)) == (0, 0, 0)
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
            if c in lit or edge: out.putpixel((x, y), (255, 255, 255))
    if flipx: out = out.transpose(Image.FLIP_LEFT_RIGHT)
    if flipy: out = out.transpose(Image.FLIP_TOP_BOTTOM)
    return out

# --------------------------------------------------------------------------
# Text on placeholder screens, using the converted font

class Font:
    def __init__(self, img): self.img = img
    def text(self, dst, cx, cy, s, colour=(255, 255, 255)):
        for i, ch in enumerate(s.upper()):
            n = ord(ch) - 32
            if not 0 <= n < 64: continue
            g = self.img.crop(((n % 32) * 8, (n // 32) * 8, (n % 32) * 8 + 8, (n // 32) * 8 + 8))
            for y in range(8):
                for x in range(8):
                    if g.getpixel((x, y)) != (0, 0, 0):
                        dst.putpixel(((cx + i) * 8 + x, cy * 8 + y), colour)

# --------------------------------------------------------------------------

def resample(incr, ratio, nframes):
    """incr: per-NES-frame increments (last one repeats). Returns per-ZX-frame
    integer increments whose running sum tracks the NES running sum."""
    cum = [0]
    for i in range(int(nframes * ratio) + 2):
        cum.append(cum[-1] + incr[min(i, len(incr) - 1)])
    out, prev = [], 0
    for k in range(1, nframes + 1):
        t = k * ratio
        i = int(t)
        v = cum[i] + (cum[i + 1] - cum[i]) * (t - i)
        v = int(round(v))
        out.append(v - prev); prev = v
    return out

def read_c_array(txt, name):
    m = re.search(name + r'\s*\[\]\s*=\s*\{([^}]*)\}', txt)
    body = re.sub(r'//[^\n]*', '', m.group(1))
    return [int(v, 0) for v in body.replace('\n', ' ').split(',') if v.strip()]

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('yun'); ap.add_argument('conv'); ap.add_argument('game')
    ap.add_argument('--map', type=int, default=4)
    ap.add_argument('--fps', type=float, default=50 / 3)
    ap.add_argument('--nes-fps', type=float, default=60)
    a = ap.parse_args()
    import yun2mk1 as Y

    G = a.game
    for d in ('map', 'enems', 'gfx/speccy', 'dev'):
        os.makedirs(os.path.join(G, d), exist_ok=True)
    ratio = a.nes_fps / a.fps
    mapnum = a.map
    tag = 'level%d_15x10' % mapnum

    # ---- map
    ml = Y.read_maplist(a.yun)[mapnum]
    W, H = ml['w'], ml['h']
    mapbin = open(os.path.join(a.conv, tag + '.map'), 'rb').read()
    assert len(mapbin) == W * H * 150 and max(mapbin) < 16
    open(os.path.join(G, 'map', 'mapa.map'), 'wb').write(mapbin)
    behs = Y.read_behs(a.yun)[mapnum][:16]

    # ---- start position (NES level index whose map is mapnum)
    lvl = open(os.path.join(a.yun, 'dev', 'assets', 'levelset.h')).read()
    order = [int(x) for x in re.findall(r'map_(\d+)_scr_offsets', lvl)]
    li = order.index(mapnum)
    scr_ini = read_c_array(lvl, 'l_scr_ini')[li]
    pix = read_c_array(lvl, 'player_ini_x')[li]
    piy = read_c_array(lvl, 'player_ini_y')[li]
    rep = open(os.path.join(a.conv, tag + '_report.txt')).read()
    dels = {}
    for m in re.finditer(r'screen\s+(\d+)\s+rows-\[([^\]]*)\] cols-\[([^\]]*)\]', rep):
        dels[int(m.group(1))] = ({int(v) for v in m.group(2).split(',') if v.strip()},
                                 {int(v) for v in m.group(3).split(',') if v.strip()})
    dr, dc = dels[scr_ini]
    sx, _ = Y.remap(pix, dc, 16, False)
    sy, _ = Y.remap(piy, dr, 12, True)

    # ---- enems: Yun types -> MK1 types
    ene = Y.read_ene(os.path.join(a.conv, tag + '.ene'))
    tmap, kills = {}, 0
    def mk1_type(t):
        beh, spr = t >> 4, t & 15
        if beh == 2: return 4                       # moving platform
        if beh == 1:
            if spr not in tmap: tmap[spr] = len(tmap) + 1
            return tmap[spr] if tmap[spr] <= 3 else 3
        return 0                                    # not supported yet -> dropped
    speed_mul = max(1, int(round(ratio)))
    out = bytearray(open(os.path.join(HERE, '..', 'enems', 'enems.ene'), 'rb').read()[:256])
    out += bytes([W, H, 15, 10, 3])
    dropped = []
    for s in range(W * H):
        for e in ene['enems'][s]:
            e = e[:]
            if e[0]:
                t = mk1_type(e[0])
                if not t: dropped.append('screen %d type %02x' % (s, e[0]))
                if 1 <= t <= 3: kills += 1
                sp = e[5] * speed_mul
                sp = 1 if sp < 2 else 2 if sp < 4 else 4 if sp < 8 else 8
                # (ene2h swaps the limits itself, keeping start point and direction)
                e = [t, e[1], e[2], e[3], e[4], sp if t else 0, 0, 0]
            out += bytes(e)
    hmap = {1: 2, 2: 3, 4: 1}                       # key, refill, star -> MK1 key, refill, object
    for s in range(W * H):
        x, y, t = ene['hots'][s]
        t = hmap.get(t, 0)
        out += bytes([((x << 4) | y) if t else 0, t])
    open(os.path.join(G, 'enems', 'enems.ene'), 'wb').write(out)

    # ---- tileset
    ts = Image.open(os.path.join(a.yun, 'gfx', 'ts%d.png' % mapnum)).convert('RGB')
    ss = Image.open(os.path.join(a.yun, 'gfx', 'ss.png')).convert('RGB')
    work = Image.new('RGB', (256, 48))
    for i in range(16):
        t = ts.crop((i * 16, 0, i * 16 + 16, 16))
        sparse = behs[i] == 0                        # background / decoration -> sparse
        work.paste(tile_to_zx(t, sparse), (i * 16, 0))
    # hotspot tiles: 16 refill (heart), 17 object (star), 18 key
    for n, box in ((16, (16, 72, 32, 88)), (17, (48, 72, 64, 88)), (18, (0, 72, 16, 88))):
        work.paste(tile_to_zx(ss.crop(box)), ((n % 16) * 16, (n // 16) * 16))
    work.save(os.path.join(G, 'gfx', 'speccy', 'work.png'))
    work.save(os.path.join(G, 'gfx', 'speccy', 'mappy.png'))

    # ---- font: white on black
    fn = Image.open(os.path.join(a.yun, 'gfx', 'font.png')).convert('RGB')
    font = Image.new('RGB', (256, 16))
    for y in range(16):
        for x in range(256):
            if fn.getpixel((x, y)) != (0, 0, 0) and lum(fn.getpixel((x, y))) > 60:
                font.putpixel((x, y), (255, 255, 255))
    font.save(os.path.join(G, 'gfx', 'speccy', 'font.png'))
    F = Font(font)

    # ---- sprites: 32 OR cells, 16 per row (sprcnv2 nomask layout)
    spr = Image.new('RGB', (256, 32))
    def put(cell, im): spr.paste(im, ((cell % 16) * 16, (cell // 16) * 16))
    P = lambda n: (n * 16, 0, n * 16 + 16, 24)       # Yun 16x24 frames
    # 0-2 walk right, 3 air right, 4-6 walk left, 7 air left (Yun faces right in the NES sheet)
    for i, f in enumerate((1, 2, 3)):
        put(i, mono(ss, P(f), squash=True)); put(4 + i, mono(ss, P(f), squash=True, flipx=True))
    put(3, mono(ss, P(5), squash=True)); put(7, mono(ss, P(5), squash=True, flipx=True))
    # enemies: MK1 type n uses cells 8+2(n-1), +1
    enem_boxes = {4: ((0, 56), (16, 56)), 5: ((32, 56), (48, 56)), 6: ((64, 56), (80, 56)),
                  0: ((0, 24), (16, 24)), 1: ((32, 24), (48, 24)), 2: ((0, 40), (16, 40)),
                  3: ((32, 40), (48, 40))}
    inv = {v: k for k, v in tmap.items()}
    for t in (1, 2, 3):
        nes = inv.get(t, 4)
        for f, (x, y) in enumerate(enem_boxes.get(nes, enem_boxes[4])):
            put(8 + 2 * (t - 1) + f, mono(ss, (x, y, x + 16, y + 16)))
            put(18 + 2 * (t - 1) + f, mono(ss, (x, y, x + 16, y + 16), flipy=True))   # captured
    for f, (x, y) in enumerate(((96, 24), (112, 24))):
        put(14 + f, mono(ss, (x, y, x + 16, y + 16)))                               # platform
    # 16-17 bubble frames
    put(16, mono(ss, (96, 72, 112, 88))); put(17, mono(ss, (112, 72, 128, 88)))
    spr.save(os.path.join(G, 'gfx', 'speccy', 'sprites.png'))

    # ---- fixed screens
    def screen(lines):
        im = Image.new('RGB', (256, 192))
        for cy, s, col in lines: F.text(im, (32 - len(s)) // 2, cy, s, col)
        return im
    title = screen([(6, 'YUN', (255, 255, 0)), (9, 'ZX SPECTRUM PROTOTYPE', (255, 255, 255)),
                    (14, 'O P Q   SPACE: BUBBLE', (0, 255, 255)), (17, '1 KEYS  2 KEMPSTON', (255, 255, 255)),
                    (19, '3 SINCLAIR', (255, 255, 255)),
                    (22, 'THE MOJON TWINS', (255, 0, 255))])
    title.save(os.path.join(G, 'gfx', 'speccy', 'title.png'))
    title.save(os.path.join(G, 'gfx', 'speccy', 'loading.png'))
    marco = Image.new('RGB', (256, 192))
    F.text(marco, 1, 0, 'LIFE', (255, 255, 0)); F.text(marco, 9, 0, 'STARS', (255, 255, 0))
    F.text(marco, 18, 0, 'KILLED', (255, 255, 0)); F.text(marco, 28, 0, '/%02d' % kills, (255, 255, 0))
    marco.save(os.path.join(G, 'gfx', 'speccy', 'marco.png'))
    screen([(10, 'LEVEL CLEAR!', (255, 255, 0)), (13, 'MORE SOON', (255, 255, 255))]).save(
        os.path.join(G, 'gfx', 'speccy', 'ending.png'))

    # ---- level header
    with open(os.path.join(G, 'dev', 'yun_level.h'), 'w') as f:
        f.write('// Generated by tools/mkassets.py - do not edit\n')
        f.write('// Yun v5 map %d (NES level %d), %dx%d screens\n\n' % (mapnum, li, W, H))
        f.write('#define YUN_MAP_W\t\t\t%d\n#define YUN_MAP_H\t\t\t%d\n' % (W, H))
        f.write('#define YUN_SCR_INI\t\t\t%d\n#define YUN_INI_X\t\t\t%d\n#define YUN_INI_Y\t\t\t%d\n' % (scr_ini, sx, sy))
        f.write('#define LEVEL_KILLS\t\t\t%d\t\t// killable enemies in the level\n' % kills)
        f.write('#define YUN_BEHS\t\t\t%s\n' % ', '.join(map(str, behs)))

    # ---- bubble LUTs
    pre = open(os.path.join(a.yun, 'dev', 'assets', 'precalcs.h')).read()
    ix = read_c_array(pre, 'fbl_incr_x'); iy = read_c_array(pre, 'fbl_incr_y')
    life = int(round(200 / ratio))
    nx = int(round(len(ix) / ratio)) + 1
    dx = resample(ix, ratio, nx)
    ny = int(round(len(iy) / ratio)) + 1
    dy = resample(iy, ratio, ny)
    bob = resample(list(reversed(iy[:32])), ratio, int(round(32 / ratio)))
    with open(os.path.join(G, 'dev', 'yun_luts.h'), 'w') as f:
        f.write('// Generated by tools/mkassets.py - do not edit\n')
        f.write('// Yun v5 fabolee tables resampled from %.0f Hz to %.2f Hz (x%.2f)\n\n' % (a.nes_fps, a.fps, ratio))
        f.write('#define BUB_LIFE\t\t\t%d\n#define BUB_BLINK\t\t\t%d\n' % (life, int(round(50 / ratio))))
        f.write('#define BUB_DX_MAX\t\t\t%d\n#define BUB_DY_MAX\t\t\t%d\n#define BUB_BOB_MAX\t\t\t%d\n\n' % (nx - 1, ny - 1, len(bob)))
        for name, arr in (('bub_dx', dx), ('bub_dy', dy), ('bub_bob', bob)):
            f.write('unsigned char %s [] = { %s };\n' % (name, ', '.join(map(str, arr))))
    print('map %d: %dx%d screens, start screen %d at (%d,%d), %d killable enemies, types %s'
          % (mapnum, W, H, scr_ini, sx, sy, kills, {('%x' % (k | 0x10)): v for k, v in tmap.items()}))
    print('bubble life %d frames, dx %s, dy %s, bob %s' % (life, dx, dy, bob))
    if dropped: print('dropped:', dropped)

if __name__ == '__main__':
    main()
