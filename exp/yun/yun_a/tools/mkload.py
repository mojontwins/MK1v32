#!/usr/bin/env python3
"""
mkload.py - Builds a multi-level MK1 v3.2 load for Yun ZX from the NES v5
sources and the yun2mk1 converter output (15x10 maps).

  python3 mkload.py YUN_V5_DIR CONVERTER_OUT_DIR GAME_DIR [--levels 0,1,2]
                    [--tools MK1_TOOLS] [--fps 16.7]

The background graphics are NES CHR bank 0 as is: its 256 patterns become the
ZX charset (dev/tileset.bin, 2048 bytes), and every level only brings a tile
map: 16 tiles x 4 characters + 4 attributes (CHR_MAPPED_TILES in MK1). Tiles
16-31 are shared (hotspots). Patterns no level of the load uses are free and
get the hotspot graphics (NES draws those as sprites).

Writes into GAME_DIR:
  dev/tileset.bin            charset (CHR bank 0 -> 1 bpp)
  dev/mapa.h, dev/enems.h    the level area: _mapa, _malotes, _hotspots,
                             _cerrojos, _comportamiento_tiles, _tile_map,
                             contiguous, overwritten by each level's blob
  dev/yun_level.h            load constants (map size, totals, etc.)
  dev/yun_load.h             level table + blobs (lvlN_area/spr.zx0)
  dev/lvl*.zx0               level blobs
  dev/yun_luts.h             bubble / fanty / monococo tables at the ZX rate
  dev/speccy/sprites.h       sprcnv2 output: cells 0-9 (Yun, bubble) as data, the
                             per-level cells 10-33 as labels at SPR_VAR_BASE
  dev/custom_sprite_cells.h  the cell table (CUSTOM_SPRITE_CELLS)
  gfx/speccy/sprites*.png    the 34 OR cells of each level (sprites.png: level 0)
  gfx/speccy/{title,marco,ending,loading}.png
"""
import argparse, os, sys, re, subprocess, tempfile
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, '..', '..', 'yun2mk1'))
import mkassets as A          # colour/sprite helpers of the one-level prototype
import zx0

MAP_W, MAP_H = 4, 6           # every level is padded to this
NSCR = MAP_W * MAP_H
NCELLS = 34

# sprite cells (sprcnv2 order, label sprite_{cell+1}_a)
# 0-7 Yun and 8-9 the bubble are fixed and the only cells in the binary; from
# C_VAR on, every cell changes per level: they live at a fixed address outside
# the binary and each level decompresses its own set there.
C_BUB, C_LIN, C_PLAT, C_LIN_UD = 8, 10, 16, 18
C_VAR = 10
C_FANTY, C_FANTY_UD = 24, 26
C_MC_APPEAR1, C_MC_APPEAR2, C_MC_OUT, C_MC_UD = 28, 29, 30, 31
C_COCO = 32

# NES metasprite boxes in ss.png (from gfx/cut_patterns.spt), pixels
LIN_BOXES = {0: ((0, 24), (16, 24)), 1: ((32, 24), (48, 24)), 2: ((0, 40), (16, 40)),
             3: ((32, 40), (48, 40)), 4: ((0, 56), (16, 56)), 5: ((32, 56), (48, 56)),
             6: ((64, 56), (80, 56))}
FANTY_BOXES = ((64, 40), (80, 40))
MC_APPEAR = ((16, 88), (64, 24))
MC_OUT = (80, 24)
COCO_BOX = (120, 40, 128, 48)
HOT_BOXES = {16: (16, 72, 32, 88), 17: (48, 72, 64, 88), 18: (0, 72, 16, 88)}  # refill, star, key

def read_c_array(txt, name):
    m = re.search(r'\b' + name + r'\s*\[\]\s*=\s*\{([^}]*)\}', txt)
    body = re.sub(r'//[^\n]*', '', m.group(1))
    return [int(v, 0) for v in body.replace('\n', ' ').split(',') if v.strip()]

def run(cmd, **kw):
    env = dict(os.environ)
    subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL, env=env, **kw)

# --------------------------------------------------------------------------
# CHR

def chr_patterns(path):
    d = open(path, 'rb').read()
    pats = []
    for p in range(256):
        px = []
        for y in range(8):
            lo, hi = d[p * 16 + y], d[p * 16 + 8 + y]
            px.append([((lo >> (7 - x)) & 1) | (((hi >> (7 - x)) & 1) << 1) for x in range(8)])
        pats.append(px)
    return pats

def zx_bits(rows):
    return bytes(sum(128 >> x for x in range(8) if r[x]) for r in rows)

def attr_of(ink_rgb):
    i, b = A.nearest_zx(ink_rgb)
    if i == 0: i = 1
    return i | (64 if b else 0)

# --------------------------------------------------------------------------
# sprcnv2 output -> bytes + label offsets

def parse_sprites_h(path):
    data, labels, on = bytearray(), {}, False
    for line in open(path):
        s = line.strip()
        if s == '#asm': on = True; continue
        if s == '#endasm': break
        if not on: continue
        if s.startswith('._'): labels[s[2:]] = len(data)
        elif s.startswith('defb'):
            data += bytes(int(v) for v in s[4:].split(','))
    return data, labels

# --------------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('yun'); ap.add_argument('conv'); ap.add_argument('game')
    ap.add_argument('--levels', default='0,1,2')
    ap.add_argument('--tools', default=os.environ.get('MK1_TOOLS', ''))
    ap.add_argument('--fps', type=float, default=50 / 3)
    ap.add_argument('--nes-fps', type=float, default=60)
    ap.add_argument('--cpc', action='store_true', help='also write the CPC (CPC_DIRTY) resources')
    ap.add_argument('--zx-top', type=int, default=61440 - 80 * 15,
                    help='(ZX) the per-level sprite cells end here: AD_FREE = 61440 - NUMBLOCKS * 15')
    ap.add_argument('--cpc-var', type=lambda v: int(v, 0), default=0xB680,
                    help='(CPC) address of the per-level sprite cells (free in the CPC_DIRTY map)')
    a = ap.parse_args()
    import yun2mk1 as Y
    G, D = a.game, os.path.join(a.game, 'dev')
    for d in ('map', 'enems', 'gfx/speccy', 'dev'):
        os.makedirs(os.path.join(G, d), exist_ok=True)
    tool = lambda n: os.path.join(a.tools, n) if a.tools else n
    ratio = a.nes_fps / a.fps
    levels = [int(x) for x in a.levels.split(',')]

    lvl = open(os.path.join(a.yun, 'dev', 'assets', 'levelset.h')).read()
    order = [int(x) for x in re.findall(r'map_(\d+)_scr_offsets', lvl)]
    tsorder = [int(x) for x in re.findall(r'ts(\d+)_tmaps', lvl.split('l_ts_tmaps')[1].split(';')[0])]
    tiledata = open(os.path.join(a.yun, 'dev', 'assets', 'tiledata.h')).read()
    pats = chr_patterns(os.path.join(a.yun, 'dev', 'tileset0.chr'))
    # GAME OVER: the CHR has A M E R (and 0 for O); G and V go into two of the
    # six blank patterns at the end, in the same slanted style (3 = stroke, 1 = shade)
    for p, rows in ((0xfa, ('..33331.', '.331....', '.31.....', '.31.3331', '.31...31', '.331.331', '..3331..', '........')),
                    (0xfb, ('.31...31', '.31...31', '.31..331', '.31..31.', '.331331.', '..3331..', '...31...', '........'))):
        assert all(v == 0 for row in pats[p] for v in row)
        pats[p] = [['.123'.index(c) for c in r] for r in rows]
    GAME_OVER = [0xfa, 0x2e, 0x35, 0x2b, None, 0x31, 0xfb, 0x2b, 0x28]     # G A M E _ O V E R
    mls = Y.read_maplist(a.yun)
    all_behs = Y.read_behs(a.yun)
    ss = Image.open(os.path.join(a.yun, 'gfx', 'ss.png')).convert('RGB')

    # ---- pass 1: which patterns each level tile uses, in which RGB
    usage = {}                                   # pattern -> list of (rgb 8x8, background?)
    L = []
    for li in levels:
        m = order[li]
        tmaps = read_c_array(tiledata, 'ts%d_tmaps' % tsorder[li])
        behs = all_behs[m][:16]
        tsimg = Image.open(os.path.join(a.yun, 'gfx', 'ts%d.png' % m)).convert('RGB')
        cells = []
        for t in range(16):
            for k in range(4):
                p = tmaps[t * 4 + k]
                box = (t * 16 + (k & 1) * 8, (k >> 1) * 8)
                rgb = [[tsimg.getpixel((box[0] + x, box[1] + y)) for x in range(8)] for y in range(8)]
                # sanity: the RGB cell must be a recolouring of the CHR pattern
                cmap = {}
                for y in range(8):
                    for x in range(8):
                        cmap.setdefault(pats[p][y][x], rgb[y][x])
                        assert cmap[pats[p][y][x]] == rgb[y][x], 'ts%d tile %d does not match CHR pattern %02x' % (m, t, p)
                usage.setdefault(p, []).append((rgb, behs[t] == 0))
                cells.append((p, rgb))
        L.append(dict(li=li, m=m, behs=behs, cells=cells))

    # ---- charset
    # A pattern used only by background tiles is stippled (so OR sprites read
    # over it); one used by both kinds keeps its full version for the solid
    # tiles and gets a stippled copy in a free pattern for the background ones.
    DIGIT_BASE = 0x53
    for n in range(10): assert DIGIT_BASE + n not in usage
    TEXT_RESERVED = range(0xfa, 0x100)                   # 6 blank patterns at the end: text
    free = [p for p in range(1, 0xfa) if p not in usage and not DIGIT_BASE <= p < DIGIT_BASE + 10
            and not 0x28 <= p <= 0x39]                   # keep the CHR letters and digits
    charset = [bytes(8)] * 256
    sparse_copy = {}
    for p in range(256):
        if p in usage:
            rgb, _ = usage[p][0]
            kinds = {bg for _, bg in usage[p]}
            z = A.cell_to_zx(rgb, kinds == {True})
            charset[p] = zx_bits([[c != (0, 0, 0) for c in row] for row in z])
            if kinds == {True, False}:
                q = free.pop(0)
                z = A.cell_to_zx(rgb, True)
                charset[q] = zx_bits([[c != (0, 0, 0) for c in row] for row in z])
                sparse_copy[p] = q
        else:
            charset[p] = zx_bits([[v == 3 for v in row] for row in pats[p]])
    for Lv in L:
        Lv['cells'] = [(sparse_copy.get(p, p) if Lv['behs'][i // 4] == 0 else p, rgb)
                       for i, (p, rgb) in enumerate(Lv['cells'])]
    # empty character: a pattern that is all colour 0
    blank = next(p for p in range(256) if all(v == 0 for row in pats[p] for v in row))
    charset[blank] = bytes(8)

    # shared tiles 16-31: hotspots on free patterns, the rest blank
    hot_slots = []                               # (pattern, 8x8 RGB image) for the CPC set
    tile_extra_chars, tile_extra_attrs = [blank] * 64, [0] * 64
    for t, box in HOT_BOXES.items():
        im = A.tile_to_zx(ss.crop(box).convert('RGB'))
        for k in range(4):
            p = free.pop(0)
            cell = im.crop(((k & 1) * 8, (k >> 1) * 8, (k & 1) * 8 + 8, (k >> 1) * 8 + 8))
            rows = [[cell.getpixel((x, y)) for x in range(8)] for y in range(8)]
            charset[p] = zx_bits([[c != (0, 0, 0) for c in row] for row in rows])
            cols = [c for row in rows for c in row if c != (0, 0, 0)]
            tile_extra_chars[(t - 16) * 4 + k] = p
            hot_slots.append((p, cell))
            tile_extra_attrs[(t - 16) * 4 + k] = attr_of(max(set(cols), key=cols.count)) if cols else 0
    open(os.path.join(D, 'tileset.bin'), 'wb').write(b''.join(charset))

    # ---- per level data
    hmap = {1: 2, 2: 3, 4: 1}                    # NES key, refill, star -> MK1 key, refill, object
    max_locks, out_levels = 0, []
    tmpd = tempfile.mkdtemp()
    for Lv in L:
        li, m = Lv['li'], Lv['m']
        ml = mls[m]
        W, H = ml['w'], ml['h']
        tag = 'level%d_15x10' % m
        mapbin = open(os.path.join(a.conv, tag + '.map'), 'rb').read()
        assert len(mapbin) == W * H * 150 and max(mapbin) < 16
        assert W <= MAP_W and H <= MAP_H
        # padded map: screens outside the NES map are solid (never entered)
        solid = next((t for t in range(16) if Lv['behs'][t] == 8), 0)
        full = bytearray([solid]) * (MAP_W * MAP_H * 150)
        for sy in range(H):
            for y in range(10):
                for sx in range(W):
                    src = ((sy * 10 + y) * W + sx) * 15
                    dst = ((sy * 10 + y) * MAP_W + sx) * 15
                    full[dst:dst + 15] = mapbin[src:src + 15]
        remap_scr = lambda n: (n % W) + (n // W) * MAP_W
        mappath = os.path.join(G, 'map', 'level%d.map' % li)
        open(mappath, 'wb').write(full)
        hdr = os.path.join(tmpd, 'mapa%d.h' % li)
        run([tool('mapcnv'), mappath, hdr, str(MAP_W), str(MAP_H), '15', '10', '15', 'packed'])
        txt = open(hdr).read()
        mapa = bytes(int(v, 16) for v in re.findall(r'0x([0-9A-Fa-f]{2})', txt.split('mapa []')[1].split('};')[0]))
        assert len(mapa) == NSCR * 75
        locks = [tuple(int(v) for v in g) for g in re.findall(r'\{(\d+), (\d+), (\d+), (\d+)\}', txt.split('cerrojos')[-1])]
        max_locks = max(max_locks, len(locks))

        # start position (screen + tile, through the converter's deletions)
        scr_ini = read_c_array(lvl, 'l_scr_ini')[li]
        pix = read_c_array(lvl, 'player_ini_x')[li]
        piy = read_c_array(lvl, 'player_ini_y')[li]
        rep = open(os.path.join(a.conv, tag + '_report.txt')).read()
        dels = {}
        for mm in re.finditer(r'screen\s+(\d+)\s+rows-\[([^\]]*)\] cols-\[([^\]]*)\]', rep):
            dels[int(mm.group(1))] = ({int(v) for v in mm.group(2).split(',') if v.strip()},
                                      {int(v) for v in mm.group(3).split(',') if v.strip()})
        dr, dc = dels[scr_ini]
        sx, _ = Y.remap(pix, dc, 16, False)
        sy, _ = Y.remap(piy, dr, 12, True)

        # enemies: Yun behaviour nibble -> MK1 type
        ene = Y.read_ene(os.path.join(a.conv, tag + '.ene'))
        tmap, kills, dropped = {}, 0, []
        def mk1_type(t):
            beh, spr = t >> 4, t & 15
            if beh == 2: return 4                   # moving platform
            if beh == 3: return 5                   # fanty
            if beh == 9: return 6                   # monococo
            if beh == 1:
                if spr not in tmap: tmap[spr] = len(tmap) + 1
                return min(tmap[spr], 3)
            return 0
        speed_mul = max(1, int(round(ratio)))
        out = bytearray(open(os.path.join(HERE, '..', 'enems', 'enems.ene'), 'rb').read()[:256])
        out += bytes([MAP_W, MAP_H, 15, 10, 3])
        per_scr = [[[0] * 8] * 3 for _ in range(NSCR)]
        hots = [[0, 0, 0] for _ in range(NSCR)]
        for s in range(W * H):
            lst = []
            for e in ene['enems'][s]:
                e = e[:]
                if e[0]:
                    t = mk1_type(e[0])
                    if not t: dropped.append('screen %d type %02x' % (s, e[0]))
                    if t in (1, 2, 3, 5, 6): kills += 1
                    if t in (1, 2, 3, 4):
                        sp = e[5] * speed_mul
                        sp = 1 if sp < 2 else 2 if sp < 4 else 4 if sp < 8 else 8
                    else:
                        sp = 0
                    if t in (5, 6): e[3], e[4] = e[1], e[2]          # static "path"
                    e = [t, e[1], e[2], e[3], e[4], sp if t else 0, 0, 0]
                lst.append(e)
            per_scr[remap_scr(s)] = lst
            hots[remap_scr(s)] = ene['hots'][s]
        for s in range(NSCR):
            for e in per_scr[s]: out += bytes(e)
        for s in range(NSCR):
            x, y, t = hots[s]
            t = hmap.get(t, 0)
            out += bytes([((x << 4) | y) if t else 0, t])
        enepath = os.path.join(G, 'enems', 'level%d.ene' % li)
        open(enepath, 'wb').write(out)
        eh = os.path.join(tmpd, 'enems%d.h' % li)
        run([tool('ene2h'), enepath, eh, '2bytes'])
        etxt = open(eh).read()
        mal = bytearray()
        for g in re.findall(r'\{(-?\d+), (-?\d+), (-?\d+), (-?\d+), (-?\d+), (-?\d+), (-?\d+), (-?\d+), (-?\d+)\}',
                            etxt.split('malotes []')[1].split('};')[0]):
            mal += bytes(int(v) & 255 for v in g)
        assert len(mal) == NSCR * 3 * 9, len(mal)
        hot = bytearray()
        for g in re.findall(r'\{(\d+), (\d+), (\d+)\}', etxt.split('hotspots []')[1].split('};')[0]):
            hot += bytes(int(v) for v in g)
        assert len(hot) == NSCR * 3

        # tile map: chars 0-63 level + 64-127 shared, attrs the same
        chars = [p for p, _ in Lv['cells']] + tile_extra_chars
        attrs = []
        for p, rgb in Lv['cells']:
            cols = [c for row in rgb for c in row if c != (0, 0, 0)]
            attrs.append(attr_of(max(set(cols), key=lambda c: cols.count(c) * (0.5 + A.lum(c) / 255))) if cols else 0)
        attrs += tile_extra_attrs
        behs48 = list(Lv['behs']) + [0] * 32
        out_levels.append(dict(li=li, m=m, W=W, H=H, mapa=mapa, mal=mal, hot=hot, locks=locks,
                               behs=behs48, chars=chars, attrs=attrs, scr_ini=remap_scr(scr_ini),
                               x=sx, y=sy, kills=kills, tmap=tmap, dropped=dropped))

    # ---- sprites: one sheet per level (enemy cells change), level 0 compiled in
    def sheet(tmapl):
        spr = Image.new('RGB', (256, 16 * ((NCELLS + 15) // 16)))
        def put(cell, im): spr.paste(im, ((cell % 16) * 16, (cell // 16) * 16))
        P = lambda n: (n * 16, 0, n * 16 + 16, 24)
        for i, f in enumerate((1, 2, 3)):
            put(i, A.mono(ss, P(f), squash=True)); put(4 + i, A.mono(ss, P(f), squash=True, flipx=True))
        put(3, A.mono(ss, P(5), squash=True)); put(7, A.mono(ss, P(5), squash=True, flipx=True))
        inv = {v: k for k, v in tmapl.items()}
        for t in (1, 2, 3):
            nes = inv.get(t, inv.get(1, 0))
            for f, (x, y) in enumerate(LIN_BOXES[nes]):
                put(C_LIN + 2 * (t - 1) + f, A.mono(ss, (x, y, x + 16, y + 16)))
                put(C_LIN_UD + 2 * (t - 1) + f, A.mono(ss, (x, y, x + 16, y + 16), flipy=True))
        for f, (x, y) in enumerate(((96, 24), (112, 24))):
            put(C_PLAT + f, A.mono(ss, (x, y, x + 16, y + 16)))
        put(C_BUB, A.mono(ss, (96, 72, 112, 88))); put(C_BUB + 1, A.mono(ss, (112, 72, 128, 88)))
        for f, (x, y) in enumerate(FANTY_BOXES):
            put(C_FANTY + f, A.mono(ss, (x, y, x + 16, y + 16)))
            put(C_FANTY_UD + f, A.mono(ss, (x, y, x + 16, y + 16), flipy=True))
        put(C_MC_APPEAR1, A.mono(ss, MC_APPEAR[0] + (MC_APPEAR[0][0] + 16, MC_APPEAR[0][1] + 16)))
        put(C_MC_APPEAR2, A.mono(ss, MC_APPEAR[1] + (MC_APPEAR[1][0] + 16, MC_APPEAR[1][1] + 16)))
        put(C_MC_OUT, A.mono(ss, MC_OUT + (MC_OUT[0] + 16, MC_OUT[1] + 16)))
        put(C_MC_UD, A.mono(ss, MC_OUT + (MC_OUT[0] + 16, MC_OUT[1] + 16), flipy=True))
        coco = Image.new('RGB', (16, 16)); coco.paste(A.mono(ss, COCO_BOX, size=(8, 8)), (4, 4))
        put(C_COCO, coco)
        return spr
    fixed = None
    for Lv in out_levels:
        png = os.path.join(G, 'gfx', 'speccy', 'sprites%d.png' % Lv['li'])
        sheet(Lv['tmap']).save(png)
        sh = os.path.join(tmpd, 'sprites%d.h' % Lv['li'])
        run([tool('sprcnv2'), png, sh, str(NCELLS), 'nomask'])
        data, lab = parse_sprites_h(sh)
        cut = lab['sprite_%d_a' % (C_VAR + 1)]
        if fixed is None: fixed, zx_lab, zx_sh = bytes(data[:cut]), lab, sh
        assert bytes(data[:cut]) == fixed, 'fixed cells differ between levels'
        Lv['spr'] = bytes(data[cut:])
    sheet(out_levels[0]['tmap']).save(os.path.join(G, 'gfx', 'speccy', 'sprites.png'))
    # speccy/sprites.h: the C part as sprcnv2 wrote it, then the fixed cells
    # as data and the per-level ones as labels at SPR_VAR_BASE
    zx_cut = zx_lab['sprite_%d_a' % (C_VAR + 1)]
    zx_var = len(out_levels[0]['spr'])
    spr_var_base = a.zx_top - zx_var
    lines = open(zx_sh).read().split('\n')
    ia = lines.index('#asm')
    with open(os.path.join(D, 'speccy', 'sprites.h'), 'w') as f:
        f.write('// Generated by tools/mkload.py from sprcnv2 output - do not edit\n')
        f.write('// Cells 0-%d in the binary; %d-%d (%d bytes) at SPR_VAR_BASE, per level\n' % (C_VAR - 1, C_VAR, NCELLS - 1, zx_var))
        varnames = {n for n, o in zx_lab.items() if o >= zx_cut}
        cpart = [l for l in lines[:ia] if not any(re.search(r'\b%s\b' % v, l) for v in varnames)]
        f.write('\n'.join(cpart) + '\n')
        # per-level cells: constant pointers for C, defc labels for assembly
        for n in sorted(varnames, key=lambda n: zx_lab[n]):
            f.write('#define %s ((unsigned char *) %d)\n' % (n, spr_var_base + zx_lab[n] - zx_cut))
        f.write('#asm\n')
        pos, byoff = 0, sorted((o, n) for n, o in zx_lab.items())
        for o, n in byoff:
            if o >= zx_cut: break
            while pos < o:
                k = min(o, pos + 16); f.write('\tdefb %s\n' % ', '.join(map(str, fixed[pos:k]))); pos = k
            f.write('._%s\n' % n)
        while pos < zx_cut:
            k = min(zx_cut, pos + 16); f.write('\tdefb %s\n' % ', '.join(map(str, fixed[pos:k]))); pos = k
        for o, n in byoff:
            if o >= zx_cut: f.write('\tdefc _%s = %d\n' % (n, spr_var_base + o - zx_cut))
        f.write('#endasm\n')

    # ---- level blobs
    area_size = NSCR * 75 + NSCR * 27 + NSCR * 3 + max_locks * 4 + 48 + 256
    for Lv in out_levels:
        locks = Lv['locks'] + [(0xff, 0, 0, 0)] * (max_locks - len(Lv['locks']))
        area = bytearray(Lv['mapa']) + Lv['mal'] + Lv['hot']
        for l in locks: area += bytes(l)
        area += bytes(Lv['behs']) + bytes(Lv['chars']) + bytes(Lv['attrs'])
        assert len(area) == area_size
        for name, blob in (('area', area), ('spr', Lv['spr'])):
            open(os.path.join(D, 'lvl%d_%s.zx0' % (Lv['li'], name)), 'wb').write(zx0.compress(bytes(blob)))

    # ---- headers
    with open(os.path.join(D, 'mapa.h'), 'w') as f:
        f.write('// Generated by tools/mkload.py - do not edit\n')
        f.write('// The level area: each level blob decompresses over all of it, in this order.\n\n')
        f.write('#define MAX_CERROJOS %d\n\n' % max_locks)
        f.write('typedef struct {\n\tunsigned char np, x, y, st;\n} CERROJOS;\n\n')
        f.write('extern unsigned char mapa [];\nextern CERROJOS cerrojos [];\n')
        f.write('extern unsigned char comportamiento_tiles [];\nextern unsigned char tile_map [];\n\n')
    with open(os.path.join(D, 'enems.h'), 'w') as f:
        f.write('// Generated by tools/mkload.py - do not edit\n\n')
        f.write('typedef struct {\n\tunsigned char x, y;\n\tunsigned char x1, y1, x2, y2;\n'
                '\tsigned char mx, my;\n\tsigned char t;\n} MALOTE;\n\n')
        f.write('typedef struct {\n\tunsigned char xy, tipo, act;\n} HOTSPOT;\n\n')
        f.write('#define TOTAL_EXISTING_ENEMS %d\n\n' % (NSCR * 3))
        f.write('extern MALOTE malotes [];\nextern HOTSPOT hotspots [];\n\n')
        f.write('#asm\n\t._level_area\n')
        f.write('\t._mapa\n\t\tdefs %d\n' % (NSCR * 75))
        f.write('\t._malotes\n\t\tdefs %d\n' % (NSCR * 27))
        f.write('\t._hotspots\n\t\tdefs %d\n' % (NSCR * 3))
        f.write('\t._cerrojos\n\t\tdefs %d\n' % max(1, max_locks * 4))
        f.write('\t._comportamiento_tiles\n\t\tdefs 48\n')
        f.write('\t._tile_map\n\t\tdefs 256\n')
        f.write('#endasm\n')
    with open(os.path.join(D, 'yun_level.h'), 'w') as f:
        f.write('// Generated by tools/mkload.py - do not edit\n')
        f.write('// Load: NES levels %s (maps %s), padded to %dx%d screens\n\n'
                % (','.join(str(l['li']) for l in out_levels), ','.join(str(l['m']) for l in out_levels), MAP_W, MAP_H))
        f.write('#define YUN_LEVELS\t\t\t%d\n' % len(out_levels))
        f.write('#define YUN_MAP_W\t\t\t%d\n#define YUN_MAP_H\t\t\t%d\n' % (MAP_W, MAP_H))
        f.write('#define YUN_SCR_INI\t\t\tlevel_scr_ini\n#define YUN_INI_X\t\t\tlevel_ini_x\n#define YUN_INI_Y\t\t\tlevel_ini_y\n')
        f.write('#define LEVEL_KILLS\t\t\tlevel_kills\n')
        f.write('#define DIGIT_BASE\t\t\t%d\t\t// CHR bank 0 digits\n' % DIGIT_BASE)
        f.write('#define GAME_OVER_TEXT\t\tgame_over_text\t// yun_load.h\n')
        f.write('#define HUD_TOTAL_X\t\t\t23\t\t// kills to win, after the / of the frame\n')
        f.write('#define SPR_VAR_BASE\t\t%d\t// (ZX) per-level sprite cells, %d bytes, up to %d\n' % (spr_var_base, zx_var, a.zx_top))
        f.write('#define LEVEL_AREA_SIZE\t\t%d\n' % area_size)
    with open(os.path.join(D, 'yun_load.h'), 'w') as f:
        f.write('// Generated by tools/mkload.py - do not edit\n\n')
        # draw_text prints character c - 32
        codes = [((blank if c is None else c) + 32) & 255 for c in GAME_OVER]
        assert 0 not in codes and 0x25 not in codes
        f.write('unsigned char game_over_text [] = { %s, 0 };\t// GAME OVER in CHR characters\n\n' % ', '.join(map(str, codes)))
        for k, name in (('scr_ini', 'lvl_scr_ini'), ('x', 'lvl_ini_x'), ('y', 'lvl_ini_y'), ('kills', 'lvl_kills')):
            f.write('unsigned char %s [] = { %s };\n' % (name, ', '.join(str(l[k]) for l in out_levels)))
        f.write('\n#ifndef CPC\t\t// the CPC blobs are in cpc/yun_cpc.h\n')
        for l in out_levels:
            for name in ('area', 'spr'):
                f.write('extern unsigned char lvl%d_%s [];\n' % (l['li'], name))
        f.write('#asm\n')
        for l in out_levels:
            for name in ('area', 'spr'):
                f.write('\t._lvl%d_%s\n\t\tBINARY "lvl%d_%s.zx0"\n' % (l['li'], name, l['li'], name))
        f.write('#endasm\n\n')
        for name in ('area', 'spr'):
            f.write('unsigned char *lvl_%s [] = { %s };\n' % (name, ', '.join('lvl%d_%s' % (l['li'], name) for l in out_levels)))
        f.write('#endif\n')
    # sprite cell table: player 0-7, enemies (linear + platform) C_LIN..C_LIN+7
    with open(os.path.join(D, 'custom_sprite_cells.h'), 'w') as f:
        f.write('// Generated by tools/mkload.py - do not edit (CUSTOM_SPRITE_CELLS)\n')
        f.write('// player: cells 0-7, enemies: cells %d-%d\n\n' % (C_LIN, C_LIN + 7))
        cells = ['sprite_%d_a' % (n + 1) for n in list(range(8)) + list(range(C_LIN, C_LIN + 8))]
        f.write('#ifdef CPC\n\textern unsigned char *sprite_cells [0];\n\t#asm\n\t\t._sprite_cells\n')
        for i in range(0, 16, 4): f.write('\t\t\tdefw %s\n' % ', '.join('_' + c for c in cells[i:i + 4]))
        f.write('\t#endasm\n#else\n\tunsigned char *sprite_cells [] = {\n')
        for i in range(0, 16, 4): f.write('\t\t%s%s\n' % (', '.join(cells[i:i + 4]), ',' if i < 12 else ''))
        f.write('\t};\n#endif\n')

    # ---- LUTs at the ZX rate (bubbles as in the prototype + fanty + monococo)
    pre = open(os.path.join(a.yun, 'dev', 'assets', 'precalcs.h')).read()
    ix = read_c_array(pre, 'fbl_incr_x'); iy = read_c_array(pre, 'fbl_incr_y')
    life = int(round(200 / ratio))
    nx = int(round(len(ix) / ratio)) + 1; dx = A.resample(ix, ratio, nx)
    ny = int(round(len(iy) / ratio)) + 1; dy = A.resample(iy, ratio, ny)
    bob = A.resample(list(reversed(iy[:32])), ratio, int(round(32 / ratio)))
    fincs = read_c_array(pre, 'fanty_incs')
    # fanty at the ZX rate: the NES speed index goes up 1 per NES frame, so here
    # it goes up 1 per ZX frame over a table ~ratio times shorter, and each entry is
    # the displacement of ratio NES frames at that speed, times 7/8 (NES skips the
    # move when rand () & 7 == 0)
    fz = []
    k = 0
    while True:
        v0 = k * ratio
        steps = 64
        disp = sum(fincs[min(len(fincs) - 1, int(v0 + ratio * j / steps))] for j in range(steps)) * ratio / steps * 7 / 8
        fz.append(int(round(disp)))
        if v0 >= len(fincs) - 1: break
        k += 1
    mc_times = [int(round(t / ratio)) for t in (150, 50, 50, 50)]
    with open(os.path.join(D, 'yun_luts.h'), 'w') as f:
        f.write('// Generated by tools/mkload.py - do not edit\n')
        f.write('// Yun v5 tables, %.0f Hz -> %.2f Hz (x%.2f)\n\n' % (a.nes_fps, a.fps, ratio))
        f.write('#define BUB_LIFE\t\t\t%d\n#define BUB_BLINK\t\t\t%d\n' % (life, int(round(50 / ratio))))
        f.write('#define BUB_DX_MAX\t\t\t%d\n#define BUB_DY_MAX\t\t\t%d\n#define BUB_BOB_MAX\t\t\t%d\n' % (nx - 1, ny - 1, len(bob)))
        for name, arr in (('bub_dx', dx), ('bub_dy', dy), ('bub_bob', bob)):
            f.write('unsigned char %s [] = { %s };\n' % (name, ', '.join(map(str, arr))))
        f.write('\n// fanty at the ZX rate (NES fanty_incs { %s })\n' % ', '.join(map(str, fincs)))
        f.write('#define FZ_MAX\t\t\t\t%d\n' % (len(fz) - 1))
        f.write('unsigned char fz_incs [] = { %s };\n' % ', '.join(map(str, fz)))
        f.write('\n// monococo: hidden, appearing, out, hiding (frames)\n')
        f.write('unsigned char mc_times [] = { %s };\n' % ', '.join(map(str, mc_times)))
        f.write('#define MC_FIRE_AT\t\t\t%d\n' % (mc_times[2] // 2))
        f.write('#define COCO_V\t\t\t\t%d\t\t// 1/16 px per frame\n' % int(round(24 * ratio)))

    # ---- fixed screens (text only)
    fn = Image.open(os.path.join(a.yun, 'gfx', 'font.png')).convert('RGB')
    font = Image.new('RGB', (256, 16))
    for y in range(16):
        for x in range(256):
            if fn.getpixel((x, y)) != (0, 0, 0) and A.lum(fn.getpixel((x, y))) > 60:
                font.putpixel((x, y), (255, 255, 255))
    F = A.Font(font)
    def screen(lines):
        im = Image.new('RGB', (256, 192))
        for cy, s, col in lines: F.text(im, (32 - len(s)) // 2, cy, s, col)
        return im
    title = screen([(6, 'YUN', (255, 255, 0)), (9, 'ZX SPECTRUM - LEVELS 1-3', (255, 255, 255)),
                    (14, 'O P Q   SPACE: BUBBLE', (0, 255, 255)), (17, '1 KEYS  2 KEMPSTON', (255, 255, 255)),
                    (19, '3 SINCLAIR', (255, 255, 255)), (22, 'THE MOJON TWINS', (255, 0, 255))])
    title.save(os.path.join(G, 'gfx', 'speccy', 'title.png'))
    title.save(os.path.join(G, 'gfx', 'speccy', 'loading.png'))
    marco = Image.new('RGB', (256, 192))
    for cx, s in ((0, 'LIFE'), (9, 'KEY'), (17, 'KO'), (22, '/')):
        F.text(marco, cx, 0, s, (255, 255, 0))
    marco.save(os.path.join(G, 'gfx', 'speccy', 'marco.png'))
    screen([(10, 'WELL DONE!', (255, 255, 0)), (13, 'END OF THE FIRST LOAD', (255, 255, 255))]).save(
        os.path.join(G, 'gfx', 'speccy', 'ending.png'))

    # ======================================================================
    # CPC (mode 0, split palette: pen = bg * 4 + sprite; CPC_DIRTY backend)
    if a.cpc:
        C = os.path.join(D, 'cpc')
        os.makedirs(C, exist_ok=True)
        def m0(p0, p1):
            b = 0
            for pen, bits in ((p0, (7, 3, 5, 1)), (p1, (6, 2, 4, 0))):
                for i, bit in enumerate(bits):
                    if pen & (1 << i): b |= 1 << bit
            return b
        def half(row):                            # 8 px -> 4 mode 0 px (left unless 0)
            return [row[2 * i] or row[2 * i + 1] for i in range(len(row) // 2)]
        # patterns: CHR colour index -> background pen bits 2-3
        cpat = []
        for p in range(256):
            src = pats[p]
            q = next((o for o, c in sparse_copy.items() if c == p), None)
            if q is not None: src = pats[q]              # ZX stippled copy: same picture here
            cpat.append(src)
        for p, cell in hot_slots:                         # hotspots, by luminance into 1..3
            cols = sorted({c for row in [[cell.getpixel((x, y)) for x in range(8)] for y in range(8)] for c in row if c != (0, 0, 0)}, key=A.lum)
            rank = {c: 1 + min(2, i * 3 // max(1, len(cols))) for i, c in enumerate(cols)}
            cpat[p] = [[rank.get(cell.getpixel((x, y)), 0) for x in range(8)] for y in range(8)]
        pb = bytearray()
        for p in range(256):
            for y in range(8):
                h = half(cpat[p][y])
                pb += bytes([m0(h[0] << 2, h[1] << 2), m0(h[2] << 2, h[3] << 2)])
        with open(os.path.join(C, 'tileset.h'), 'w') as f:
            f.write('// Generated by tools/mkload.py - do not edit\n// CPC_DIRTY: the patterns are cpc/patterns.zx0, included by cpc_dirty.h\n')

        # CPC hardware inks: firmware n = 9 G + 3 R + B (levels 0, 128, 255)
        HWI = [0x14, 0x04, 0x15, 0x1C, 0x18, 0x1D, 0x0C, 0x05, 0x0D, 0x16, 0x06, 0x17, 0x1E, 0x00,
               0x1F, 0x0E, 0x07, 0x0F, 0x12, 0x02, 0x13, 0x1A, 0x19, 0x1B, 0x0A, 0x03, 0x0B]
        LEV = (0, 128, 255)
        def fw_rgb(n): return (LEV[(n // 3) % 3], LEV[n // 9], LEV[n % 3])
        def nearest_fw(c): return min(range(27), key=lambda n: sum((x - y) ** 2 for x, y in zip(c, fw_rgb(n))))
        # background inks per level: the 4 CPC inks that best cover every pixel
        # of its tiles (colour 0 first, the NES background). Each tile cell then
        # maps its colours to those 4; a pattern that needs a different mapping
        # gets a recoloured copy in a free pattern (shared between levels).
        import itertools
        cfree = list(free)
        recol = {}                                       # pixels -> pattern
        for p in range(256): recol.setdefault(tuple(tuple(r) for r in cpat[p]), p)
        lvl_inks, cpc_chars = [], []
        for Lv0, Lv in zip(L, out_levels):
            hist = {}
            for p, rgb in Lv0['cells']:
                for row in rgb:
                    for c in row: hist[c] = hist.get(c, 0) + 1
            bg0 = max(((rgb[y][x], 1) for p, rgb in Lv0['cells'] for y in range(8) for x in range(8) if pats[p][y][x] == 0),
                      key=lambda t: hist[t[0]])[0] if Lv0['cells'] else (0, 0, 0)
            f0 = nearest_fw(bg0)
            def err(c, fw): return sum((x - y) ** 2 for x, y in zip(c, fw_rgb(fw)))
            best = None
            for combo in itertools.combinations([n for n in range(27) if n != f0], 3):
                inks = (f0,) + combo
                e = sum(n * min(err(c, fw) for fw in inks) for c, n in hist.items())
                if best is None or e < best[0]: best = (e, inks)
            inks = list(best[1])
            lvl_inks.append(inks)
            chars = []
            for p, rgb in Lv0['cells']:
                cmap = {}
                for y in range(8):
                    for x in range(8): cmap[pats[p][y][x]] = rgb[y][x]
                m4 = {v: min(range(4), key=lambda j: err(c, inks[j])) for v, c in cmap.items()}
                pix = tuple(tuple(m4.get(v, 0) for v in row) for row in pats[p])
                q = recol.get(pix)
                if q is None:
                    q = cfree.pop(0) if cfree else p
                    if q != p:
                        cpat[q] = [list(r) for r in pix]
                        recol[pix] = q
                chars.append(q)
            cpc_chars.append(chars)
        pb = bytearray()
        for p in range(256):
            for y in range(8):
                h = half(cpat[p][y])
                pb += bytes([m0(h[0] << 2, h[1] << 2), m0(h[2] << 2, h[3] << 2)])
        open(os.path.join(C, 'patterns.zx0'), 'wb').write(zx0.compress(bytes(pb)))
        # CPC level areas: the same as the ZX ones but the tile map characters
        for Lv, chars in zip(out_levels, cpc_chars):
            area = bytearray(zx0.decompress(open(os.path.join(D, 'lvl%d_area.zx0' % Lv['li']), 'rb').read()))
            tm = len(area) - 256
            area[tm:tm + 64] = bytes(chars)
            open(os.path.join(D, 'lvl%d_carea.zx0' % Lv['li']), 'wb').write(zx0.compress(bytes(area)))
        # sprite inks: Yun's three colours, darkest first
        from collections import Counter
        ycols = [c for c, _ in Counter(ss.crop((0, 0, 128, 24)).getdata()).most_common() if c != (0, 0, 0)][:3]
        ycols.sort(key=A.lum)
        spr_fw = [nearest_fw(c) for c in ycols]

        def rgb16(box, squash=False, flipx=False, flipy=False, size=(16, 16)):
            im = ss.crop(box).convert('RGB')
            if squash and im.size != size:
                w, h = im.size
                sq = Image.new('RGB', size)
                for y in range(size[1]):
                    sy = int(y * h / size[1] + 0.5 * h / size[1])
                    for x in range(size[0]):
                        sq.putpixel((x, y), im.getpixel((x * w // size[0], min(h - 1, sy))))
                im = sq
            elif im.size != size:
                full = Image.new('RGB', size); full.paste(im, ((size[0] - im.size[0]) // 2, (size[1] - im.size[1]) // 2)); im = full
            if flipx: im = im.transpose(Image.FLIP_LEFT_RIGHT)
            if flipy: im = im.transpose(Image.FLIP_TOP_BOTTOM)
            return im
        def enc_sprite(im):
            cols = sorted({c for c in im.getdata() if c != (0, 0, 0)}, key=A.lum)
            # by luminance: darkest third -> pen 1, middle -> 2, brightest -> 3
            rank = {c: 1 + min(2, i * 3 // max(1, len(cols))) for i, c in enumerate(cols)}
            out = bytearray()
            for y in range(16):
                row = [rank.get(im.getpixel((x, y)), 0) for x in range(16)]
                h = half(row)
                out += bytes(m0(h[2 * i], h[2 * i + 1]) for i in range(4))
            return bytes(out)
        def cpc_cells(tmapl):
            cells = [bytes(64)] * NCELLS
            P = lambda n: (n * 16, 0, n * 16 + 16, 24)
            for i, f in enumerate((1, 2, 3)):
                cells[i] = enc_sprite(rgb16(P(f), squash=True)); cells[4 + i] = enc_sprite(rgb16(P(f), squash=True, flipx=True))
            cells[3] = enc_sprite(rgb16(P(5), squash=True)); cells[7] = enc_sprite(rgb16(P(5), squash=True, flipx=True))
            inv = {v: k for k, v in tmapl.items()}
            for t in (1, 2, 3):
                nes = inv.get(t, inv.get(1, 0))
                for f, (x, y) in enumerate(LIN_BOXES[nes]):
                    cells[C_LIN + 2 * (t - 1) + f] = enc_sprite(rgb16((x, y, x + 16, y + 16)))
                    cells[C_LIN_UD + 2 * (t - 1) + f] = enc_sprite(rgb16((x, y, x + 16, y + 16), flipy=True))
            for f, (x, y) in enumerate(((96, 24), (112, 24))):
                cells[C_PLAT + f] = enc_sprite(rgb16((x, y, x + 16, y + 16)))
            cells[C_BUB] = enc_sprite(rgb16((96, 72, 112, 88))); cells[C_BUB + 1] = enc_sprite(rgb16((112, 72, 128, 88)))
            for f, (x, y) in enumerate(FANTY_BOXES):
                cells[C_FANTY + f] = enc_sprite(rgb16((x, y, x + 16, y + 16)))
                cells[C_FANTY_UD + f] = enc_sprite(rgb16((x, y, x + 16, y + 16), flipy=True))
            for c, b in ((C_MC_APPEAR1, MC_APPEAR[0]), (C_MC_APPEAR2, MC_APPEAR[1]), (C_MC_OUT, MC_OUT)):
                cells[c] = enc_sprite(rgb16(b + (b[0] + 16, b[1] + 16)))
            cells[C_MC_UD] = enc_sprite(rgb16(MC_OUT + (MC_OUT[0] + 16, MC_OUT[1] + 16), flipy=True))
            cells[C_COCO] = enc_sprite(rgb16(COCO_BOX, size=(16, 16)))
            return cells
        with open(os.path.join(C, 'sprites.h'), 'w') as f:
            f.write('// Generated by tools/mkload.py - do not edit\n')
            f.write('// CPC_DIRTY sprites: %d cells, 4 bytes x 16 lines, sprite pens only (mask 0xCC)\n' % NCELLS)
            f.write('// Cells 0-%d in the binary; %d-%d at 0x%04X, decompressed per level\n\n' % (C_VAR - 1, C_VAR, NCELLS - 1, a.cpc_var))
            for n in range(C_VAR): f.write('extern unsigned char sprite_%d_a [];\n' % (n + 1))
            for n in range(C_VAR, NCELLS):
                f.write('#define sprite_%d_a ((unsigned char *) 0x%04X)\n' % (n + 1, a.cpc_var + (n - C_VAR) * 64))
            f.write('\n#asm\n')
            for n, cell in enumerate(cpc_cells(out_levels[0]['tmap'])[:C_VAR]):
                f.write('\t._sprite_%d_a\n' % (n + 1))
                for i in range(0, 64, 16): f.write('\t\tdefb %s\n' % ', '.join(map(str, cell[i:i + 16])))
            for n in range(C_VAR, NCELLS):
                f.write('\tdefc _sprite_%d_a = 0x%04X\n' % (n + 1, a.cpc_var + (n - C_VAR) * 64))
            f.write('#endasm\n')
        with open(os.path.join(C, 'extrasprites.h'), 'w') as f:
            f.write('// Generated by tools/mkload.py - do not edit\n\n')
            f.write('extern unsigned char spr_boom [];\nextern unsigned char spr_empty [];\n\n#asm\n\t._spr_boom\n')
            boom = enc_sprite(rgb16((64, 72, 80, 88)))
            for i in range(0, 64, 16): f.write('\t\tdefb %s\n' % ', '.join(map(str, boom[i:i + 16])))
            f.write('\t._spr_empty\n\t\tdefs 64, 0\n#endasm\n')
        assert a.cpc_var + (NCELLS - C_VAR) * 64 <= 0xC000
        for Lv in out_levels:
            cells = cpc_cells(Lv['tmap'])
            open(os.path.join(D, 'lvl%d_cspr.zx0' % Lv['li']), 'wb').write(zx0.compress(b''.join(cells[C_VAR:])))
        with open(os.path.join(C, 'yun_cpc.h'), 'w') as f:
            f.write('// Generated by tools/mkload.py - do not edit\n\n')
            f.write('// hardware inks: background pens per level (CHR colour 0-3), sprite pens 1-3\n')
            f.write('unsigned char lvl_inks [] = { %s };\n' % ', '.join(str(HWI[i]) for l in lvl_inks for i in l))
            f.write('unsigned char spr_inks [] = { %s };\n\n' % ', '.join(str(HWI[i]) for i in spr_fw))
            for l in out_levels:
                f.write('extern unsigned char lvl%d_cspr [];\n' % l['li'])
            f.write('#asm\n')
            for l in out_levels:
                f.write('\t._lvl%d_cspr\n\t\tBINARY "lvl%d_cspr.zx0"\n' % (l['li'], l['li']))
            f.write('#endasm\n')
            for l in out_levels: f.write('extern unsigned char lvl%d_carea [];\n' % l['li'])
            f.write('#asm\n')
            for l in out_levels: f.write('\t._lvl%d_carea\n\t\tBINARY "lvl%d_carea.zx0"\n' % (l['li'], l['li']))
            f.write('#endasm\n')
            for name in ('cspr', 'carea'):
                f.write('unsigned char *lvl_%s [] = { %s };\n' % (name, ', '.join('lvl%d_%s' % (l['li'], name) for l in out_levels)))
        # screens: mode 0, 128 x 192, text with Yun's font halved, sprite pens;
        # 8 zx0 streams, one per pixel line of the 24 char rows (64 x 24 bytes
        # each), after a table of their 8 offsets
        fnt = Image.open(os.path.join(a.yun, 'gfx', 'font.png')).convert('RGB')
        def cscreen(lines, path):
            px = [[0] * 128 for _ in range(192)]
            for cy, cx, txt, pen in lines:
                if cx is None: cx = (32 - len(txt)) // 2
                for i, ch in enumerate(txt.upper()):
                    n = ord(ch) - 32
                    if not 0 <= n < 64: continue
                    for y in range(8):
                        for x in range(4):
                            a0 = fnt.getpixel(((n % 32) * 8 + 2 * x, (n // 32) * 8 + y))
                            a1 = fnt.getpixel(((n % 32) * 8 + 2 * x + 1, (n // 32) * 8 + y))
                            if A.lum(a0) > 60 or A.lum(a1) > 60: px[cy * 8 + y][(cx + i) * 4 + x] = pen
            streams = []
            for line in range(8):
                blk = bytearray()
                for row in range(24):
                    r = px[row * 8 + line]
                    blk += bytes(m0(r[2 * i], r[2 * i + 1]) for i in range(64))
                streams.append(zx0.compress(bytes(blk)))
            # 8 offsets (words, from the start of the file) then the 8 streams
            out, off = bytearray(), 16
            for st in streams: out += bytes([off & 255, off >> 8]); off += len(st)
            for st in streams: out += st
            open(path, 'wb').write(out)
        cscreen([(6, None, 'YUN', 3), (9, None, 'AMSTRAD CPC - LEVELS 1-3', 2),
                 (14, None, 'O P Q   SPACE: BUBBLE', 3), (17, None, '1 KEYS  2 JOYSTICK', 2),
                 (22, None, 'THE MOJON TWINS', 1)], os.path.join(C, 'titlec.bin'))
        cscreen([(0, 0, 'LIFE', 3), (0, 9, 'KEY', 3), (0, 17, 'KO', 3), (0, 22, '/', 3)], os.path.join(C, 'marcoc.bin'))
        cscreen([(10, None, 'WELL DONE!', 3), (13, None, 'END OF THE FIRST LOAD', 2)], os.path.join(C, 'endingc.bin'))
        print('cpc: inks per level %s, sprites %s, %d recoloured patterns, %d free left, sprite blobs %s' % (
            lvl_inks, spr_fw, len(free) - len(cfree), len(cfree), ', '.join('%d' % os.path.getsize(os.path.join(D, 'lvl%d_cspr.zx0' % l['li'])) for l in out_levels)))

    for Lv in out_levels:
        print('level %d (map %d, %dx%d): start screen %d at (%d,%d), %d kills, %d locks, types %s%s'
              % (Lv['li'], Lv['m'], Lv['W'], Lv['H'], Lv['scr_ini'], Lv['x'], Lv['y'], Lv['kills'], len(Lv['locks']),
                 {('%x' % (k | 0x10)): v for k, v in Lv['tmap'].items()},
                 ('  dropped: %s' % Lv['dropped']) if Lv['dropped'] else ''))
    print('charset: %d patterns used, %d stippled copies, %d free left; area %d bytes; blobs: %s' % (
        len(usage), len(sparse_copy), len(free), area_size,
        ', '.join('%d+%d' % tuple(os.path.getsize(os.path.join(D, 'lvl%d_%s.zx0' % (l['li'], n))) for n in ('area', 'spr')) for l in out_levels)))

if __name__ == '__main__':
    main()
