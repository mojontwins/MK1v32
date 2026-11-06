#!/usr/bin/env python3
"""
nes2load.py - ONE-SHOT: makes a new load (map, enemies, placeholder graphics)
from Yun NES v5 and the map converter's output. Not part of the build.

  python3 nes2load.py YUN_V5_DIR CONVERTER_OUT_DIR GAME_DIR LOAD [--zx-from LOAD1_GAME_DIR]

LOAD 2 = NES levels 4-6 (maps 2, 3, 5), LOAD 3 = NES levels 7-8 (maps 6, 7).
(Load 1 was made with nes2png.py.) Writes into GAME_DIR, never over an
existing file:

  map/levelN.map         the converter's 15x10 map, as is (no padding: the
                         engine takes the width per level, VARIABLE_MAP_W)
  enems/levelN.ene       ponedor, 3-byte hotspots, MK1 types (see below)
  gfx/speccy, gfx/cpc    tilesets, sprites, palettes, as nes2png.py
  nes2load.txt           what goes in the hand-written headers

--zx-from copies the ZX graphics already redrawn for load 1 (font, the first
22 fixed cells, the hotspot tiles, the screens) instead of making placeholders.

MK1 types: 1-3 linear (the 3 commonest NES linear kinds of the level; the
others take their place), 4 platform, 5 fanty, 6 monococo, 7 pezon (zurully),
8 monococo (the one in the wall, NES 0x9B), 9 gyrosaw, 10 cannon.
Hotspots: NES 1 key -> 2, 2 refill -> 3, 4 star -> 1, 3 bulb -> 4.
"""
import argparse, itertools, os, re, shutil, sys
from collections import Counter
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, '..', '..', 'yun2mk1'))
from nes2png import (LIN_BOXES, PLAT_BOXES, BUB_BOXES, FANTY_BOXES, MC_APPEAR, MC_OUT, COCO_BOX, BOOM_BOX,
                     HOT_BOXES, BLACK, cell_to_zx, put_cells, crop16, mono, rank3, half, fw_rgb, cdist,
                     nearest_fw, read_c_array, chr_patterns, pixels, lum, pal_img)

LOADS = {2: (3, 4, 5), 3: (6, 7)}
BULB_BOX = (32, 72, 48, 88)
ALT_MC = ((96, 88), (112, 88))          # monococo in the wall: hidden, peeking
PEZON = (32, 88)
GYRO = ((64, 88), (80, 88))
CANNON = ((0, 128), (16, 128), (32, 128), (48, 128))     # left, up, right, down
NCELLS = 32
HMAP = {1: 2, 2: 3, 4: 1, 3: 4}

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('yun'); ap.add_argument('conv'); ap.add_argument('game'); ap.add_argument('load', type=int)
    ap.add_argument('--zx-from')
    ap.add_argument('--screens', action='store_true', help='remake title.png and ending.png')
    a = ap.parse_args()
    import yun2mk1 as Y

    G = a.game
    ZD, CD = os.path.join(G, 'gfx', 'speccy'), os.path.join(G, 'gfx', 'cpc')
    for d in (ZD, CD, os.path.join(G, 'enems'), os.path.join(G, 'map')): os.makedirs(d, exist_ok=True)
    def save(img, *path):
        p = os.path.join(*path)
        if os.path.exists(p): print('kept', p); return
        img.save(p)
    def write(data, *path):
        p = os.path.join(*path)
        if os.path.exists(p): print('kept', p); return
        open(p, 'wb').write(data)

    lvl = open(os.path.join(a.yun, 'dev', 'assets', 'levelset.h')).read()
    order = [int(x) for x in re.findall(r'map_(\d+)_scr_offsets', lvl)]
    tsorder = [int(x) for x in re.findall(r'ts(\d+)_tmaps', lvl.split('l_ts_tmaps')[1].split(';')[0])]
    scr_ini = read_c_array(lvl, 'l_scr_ini')
    pix, piy = read_c_array(lvl, 'player_ini_x'), read_c_array(lvl, 'player_ini_y')
    tiledata = open(os.path.join(a.yun, 'dev', 'assets', 'tiledata.h')).read()
    pats = chr_patterns(os.path.join(a.yun, 'dev', 'tileset0.chr'))
    all_behs = Y.read_behs(a.yun)
    mls = Y.read_maplist(a.yun)
    ss = Image.open(os.path.join(a.yun, 'gfx', 'ss.png')).convert('RGB')
    has_bulbs = False
    notes = []

    # ======================================================================
    # Levels: map, enemies, start
    L = []
    for k, li in enumerate(LOADS[a.load]):
        m = order[li]
        tag = 'level%d_15x10' % m
        W, H = mls[m]['w'], mls[m]['h']
        mapbin = open(os.path.join(a.conv, tag + '.map'), 'rb').read()
        assert len(mapbin) == W * H * 150 and max(mapbin) < 16
        write(mapbin, G, 'map', 'level%d.map' % k)
        behs = all_behs[m][:16]
        lock = mls[m]['lock']
        if lock > 15 and 15 in mapbin: print('note: map %d has tile 15 and no locks' % m)

        # start position through the converter's deletions
        rep = open(os.path.join(a.conv, tag + '_report.txt')).read()
        dels = {}
        for mm in re.finditer(r'screen\s+(\d+)\s+rows-\[([^\]]*)\] cols-\[([^\]]*)\]', rep):
            dels[int(mm.group(1))] = ({int(v) for v in mm.group(2).split(',') if v.strip()},
                                      {int(v) for v in mm.group(3).split(',') if v.strip()})
        dr, dc = dels[scr_ini[li]]
        sx, _ = Y.remap(pix[li], dc, 16, False)
        sy, _ = Y.remap(piy[li], dr, 12, True)

        ene = Y.read_ene(os.path.join(a.conv, tag + '.ene'))
        lin = Counter(e[0] & 15 for s in ene['enems'] for e in s if e[0] >> 4 == 1)
        kinds = [n for n, _ in lin.most_common(3)]
        while len(kinds) < 3: kinds.append(kinds[-1] if kinds else 0)
        tmap = {n: i + 1 for i, n in enumerate(kinds) if n not in list(kinds)[:i]}
        merged = {}
        for n in lin:
            if n not in tmap:
                tmap[n] = 1 + len(merged) % 3     # spread the extra kinds over 1-3
                merged[n] = tmap[n]
        kills, dropped = 0, []
        out = bytearray(256)
        for off, s in ((0, '..\\map\\level%d.map' % k), (128, '..\\gfx\\speccy\\ts%d.png' % k)):
            out[off:off + len(s)] = s.encode()
        out += bytes([W, H, 15, 10, 3])
        hots = []
        for s in range(W * H):
            for e in ene['enems'][s]:
                t0, x1, y1, x2, y2, at = e[0], e[1], e[2], e[3], e[4], e[5]
                beh, nib = t0 >> 4, t0 & 15
                t, sp = 0, 0
                if beh == 1:
                    t = tmap[nib]
                    sp = at * 4                       # NES speed at the ZX rate (as load 1)
                    sp = 1 if sp < 2 else 2 if sp < 4 else 4 if sp < 8 else 8
                elif beh == 2:
                    t = 4
                    sp = at * 4
                    sp = 1 if sp < 2 else 2 if sp < 4 else 4 if sp < 8 else 8
                elif beh == 3: t, x2, y2 = 5, x1, y1
                elif beh == 9: t, x2, y2 = (8 if nib == 0xb else 6), x1, y1
                elif beh == 0xa: t, x2, y2 = 7, x1, y1
                elif beh == 0xb:                      # gyrosaw: clockwise if the path goes somewhere
                    t, y2 = 9, y1
                    x2, sp = (x1 + 1 if x1 < 14 else x1 - 1, 1) if at else (x1, 0)
                elif beh == 8: t, sp = 10, max(1, at)    # cannon: path = where it shoots, speed = seconds
                elif t0: dropped.append('screen %d %02x' % (s, t0))
                if t in (1, 2, 3, 5, 6, 7, 8): kills += 1
                out += bytes([t, x1, y1, x2, y2, sp, 0, 0]) if t else bytes(8)
            for _ in range(3 - len(ene['enems'][s])): out += bytes(8)
        for s in range(W * H):
            x, y, t = ene['hots'][s]
            t = HMAP.get(t, 0)
            if t == 4: has_bulbs = True
            out += bytes([x, y, t] if t else [0, 0, 0])
        write(bytes(out), G, 'enems', 'level%d.ene' % k)

        # tiles: 4 cells (CHR pattern, 8x8 RGB) per tile
        tm = read_c_array(tiledata, 'ts%d_tmaps' % tsorder[li])
        tsimg = Image.open(os.path.join(a.yun, 'gfx', 'ts%d.png' % m)).convert('RGB')
        tiles = []
        for t in range(16):
            cells = []
            for q in range(4):
                bx, by = t * 16 + (q & 1) * 8, (q >> 1) * 8
                cells.append((tm[t * 4 + q], [[tsimg.getpixel((bx + x, by + y)) for x in range(8)] for y in range(8)]))
            tiles.append(cells)
        L.append(dict(k=k, li=li, m=m, W=W, H=H, behs=behs, tiles=tiles, kinds=kinds, merged=merged,
                      scr=scr_ini[li], x=sx, y=sy, kills=kills, lock=lock, dropped=dropped))
        notes.append('level %d (NES level %d, map %d): %dx%d screens, start screen %d at (%d, %d), %d kills, '
                     'lock tile %s, linear kinds %s%s%s\n  behs %s'
                     % (k, li + 1, m, W, H, scr_ini[li], sx, sy, kills, lock if lock < 16 else 'none',
                        ['%x' % n for n in kinds],
                        (', merged %s' % {'%x' % n: v for n, v in merged.items()}) if merged else '',
                        (', dropped %s' % dropped) if dropped else '', behs))

    # ======================================================================
    # ZX
    P = lambda n: (n * 16, 0, n * 16 + 16, 24)
    B = lambda xy: xy + (xy[0] + 16, xy[1] + 16)
    def fixed_cells(enc):
        c = [None] * NCELLS
        for i, f in enumerate((1, 2, 3)):
            c[i] = enc(crop16(ss, P(f), squash=True)); c[4 + i] = enc(crop16(ss, P(f), squash=True, flipx=True))
        c[3] = enc(crop16(ss, P(5), squash=True)); c[7] = enc(crop16(ss, P(5), squash=True, flipx=True))
        for f in range(2):
            c[8 + f] = enc(crop16(ss, B(BUB_BOXES[f])))
            c[10 + f] = enc(crop16(ss, B(PLAT_BOXES[f])))
            c[12 + f] = enc(crop16(ss, B(FANTY_BOXES[f])))
            c[14 + f] = enc(crop16(ss, B(FANTY_BOXES[f]), flipy=True))
            c[22 + f] = enc(crop16(ss, B(ALT_MC[f])))
            c[26 + f] = enc(crop16(ss, B(GYRO[f])))
        c[16] = enc(crop16(ss, B(MC_APPEAR[0])))
        c[17] = enc(crop16(ss, B(MC_APPEAR[1])))
        c[18] = enc(crop16(ss, B(MC_OUT)))
        c[19] = enc(crop16(ss, B(MC_OUT), flipy=True))
        c[20] = enc(crop16(ss, COCO_BOX))
        c[21] = enc(crop16(ss, BOOM_BOX))
        c[24] = enc(crop16(ss, B(PEZON)))
        c[25] = enc(crop16(ss, B(PEZON), flipy=True))
        for d in range(4): c[28 + d] = enc(crop16(ss, B(CANNON[d])))
        return c
    def level_cells(enc, kinds):
        c = [None] * 12
        for t in range(3):
            for f, xy in enumerate(LIN_BOXES.get(kinds[t], FANTY_BOXES)):     # kind 7 looks like a fanty
                c[2 * t + f] = enc(crop16(ss, B(xy)))
                c[6 + 2 * t + f] = enc(crop16(ss, B(xy), flipy=True))
        return c
    def sheet(cells, cw, rows):
        im = Image.new('RGB', (16 * cw, 16 * rows))
        for n, c in enumerate(cells): im.paste(c, ((n % 16) * cw, (n // 16) * 16))
        return im
    hot_boxes = list(HOT_BOXES) + ([None, BULB_BOX] if has_bulbs else [])
    ntiles_x = len(hot_boxes)

    for Lv in L:
        im = Image.new('RGB', (256, 16))
        for t, cells in enumerate(Lv['tiles']):
            for q, (p, rgb) in enumerate(cells):
                x0, y0 = (q & 1) * 8, (q >> 1) * 8
                put_cells(im, t * 16 + x0, y0, cell_to_zx(rgb, Lv['behs'][t] == 0, x0, y0))
        save(im, ZD, 'ts%d.png' % Lv['k'])
        save(sheet(level_cells(mono, Lv['kinds']), 16, 1), ZD, 'sprites%d.png' % Lv['k'])

    zx_fixed = fixed_cells(mono)
    src = a.zx_from and os.path.join(a.zx_from, 'gfx', 'speccy')
    if src:
        old = Image.open(os.path.join(src, 'sprites.png')).convert('RGB')
        for n in range(22): zx_fixed[n] = old.crop(((n % 16) * 16, (n // 16) * 16, (n % 16) * 16 + 16, (n // 16) * 16 + 16))
        for f in ('font.png', 'title.png', 'marco.png', 'ending.png', 'loading.png'):
            if not os.path.exists(os.path.join(ZD, f)): shutil.copy(os.path.join(src, f), os.path.join(ZD, f))
    save(sheet(zx_fixed, 16, 2), ZD, 'sprites.png')
    im = Image.new('RGB', (16 * ntiles_x, 16))
    if src: im.paste(Image.open(os.path.join(src, 'tsx.png')).convert('RGB'), (0, 0))
    for i, box in enumerate(hot_boxes):
        if box is None or (src and i < 3): continue
        tile = ss.crop(box)
        for q in range(4):
            x0, y0 = (q & 1) * 8, (q >> 1) * 8
            put_cells(im, i * 16 + x0, y0, cell_to_zx([[tile.getpixel((x0 + x, y0 + y)) for x in range(8)] for y in range(8)]))
    save(im, ZD, 'tsx.png')

    # ======================================================================
    # CPC
    ycols = sorted([c for c, _ in Counter(pixels(ss.crop((0, 0, 128, 24)))).most_common() if c != BLACK][:3], key=lum)
    spr_rgb = [BLACK] + [fw_rgb(nearest_fw(c)) for c in ycols]
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
            for q, (p, rgb) in enumerate(cells):
                cmap = {pats[p][y][x]: rgb[y][x] for y in range(8) for x in range(8)}
                m4 = {v: min(range(4), key=lambda j: cdist(c, inks[j])) for v, c in cmap.items()}
                put_cells(im, t * 8 + (q & 1) * 4, (q >> 1) * 8,
                          [[inks[j] for j in half([m4[v] for v in row])] for row in pats[p]])
        save(im, CD, 'ts%d.png' % Lv['k'])
        save(pal_img([inks[i >> 2] for i in range(16)]), CD, 'pal_cnvtiles%d.png' % Lv['k'])
        Lv['inks'] = [nearest_fw(c) for c in inks]
    save(pal_img([spr_rgb[i & 3] for i in range(16)]), CD, 'pal_cnvspr.png')
    save(pal_img([spr_rgb[i & 3] if i & 3 else lvl_rgb[0][i >> 2] for i in range(16)]), CD, 'pal.png')
    if has_bulbs:
        # dark rooms: background inks black / dark blue; lightning: bright
        dark = [BLACK, fw_rgb(1), fw_rgb(1), fw_rgb(2)]
        save(pal_img([dark[i >> 2] for i in range(16)]), CD, 'pal_dark.png')
        light = [fw_rgb(13), fw_rgb(26), fw_rgb(26), fw_rgb(26)]
        save(pal_img([light[i >> 2] for i in range(16)]), CD, 'pal_light.png')

    def enc_cpc(im):
        r = rank3(pixels(im))
        out = Image.new('RGB', (8, 16))
        put_cells(out, 0, 0, [[spr_rgb[v] for v in half([r.get(im.getpixel((x, y)), 0) for x in range(16)])] for y in range(16)])
        return out
    im = Image.new('RGB', (8 * ntiles_x, 16))
    for i, box in enumerate(hot_boxes):
        if box: im.paste(enc_cpc(ss.crop(box)), (i * 8, 0))
    save(im, CD, 'tsx.png')
    save(sheet(fixed_cells(enc_cpc), 8, 2), CD, 'sprites.png')
    for Lv in L: save(sheet(level_cells(enc_cpc, Lv['kinds']), 8, 1), CD, 'sprites%d.png' % Lv['k'])
    l1 = a.zx_from and os.path.join(a.zx_from, 'gfx', 'cpc')
    if l1:
        # font, title, frame and ending: load 1's (recoloured to this load's level 0)
        pal_old = Image.open(os.path.join(l1, 'pal.png')).convert('RGB')
        pal_new = Image.open(os.path.join(CD, 'pal.png')).convert('RGB')
        old_cols = [pal_old.getpixel((i, 0)) for i in range(16)]
        new_cols = [pal_new.getpixel((i, 0)) for i in range(16)]
        f_old = Image.open(os.path.join(l1, 'pal_cnvtiles0.png')).convert('RGB').getpixel((12, 0))
        for f in ('font.png', 'title.png', 'marco.png', 'ending.png'):
            im = Image.open(os.path.join(l1, f)).convert('RGB')
            for y in range(im.size[1]):
                for x in range(im.size[0]):
                    c = im.getpixel((x, y))
                    if f == 'font.png': im.putpixel((x, y), lvl_rgb[0][3] if c == f_old else BLACK)
                    elif c in old_cols: im.putpixel((x, y), new_cols[old_cols.index(c)])
            save(im, CD, f)

    # title and ending with this load's levels (Yun's NES font)
    fnt = Image.open(os.path.join(a.yun, 'gfx', 'font.png')).convert('RGB')
    first, last = LOADS[a.load][0] + 1, LOADS[a.load][-1] + 1
    names = {2: 'SECOND', 3: 'THIRD'}
    def screen(lines, w, colour, bg):
        im = Image.new('RGB', (w, 192), bg)
        cw = w // 32
        for cy, txt, c in lines:
            cx = (32 - len(txt)) // 2
            for i, ch in enumerate(txt.upper()):
                n = ord(ch) - 32
                if not 0 <= n < 64: continue
                for y in range(8):
                    for x in range(cw):
                        px = [fnt.getpixel(((n % 32) * 8 + x * 8 // cw + d, (n // 32) * 8 + y)) for d in range(8 // cw)]
                        if any(lum(q) > 60 for q in px): im.putpixel(((cx + i) * cw + x, cy * 8 + y), colour(c))
        return im
    zxc = lambda c: [(255, 255, 0), (255, 255, 255), (0, 255, 255), (255, 0, 255)][c]
    cpc = lambda c: spr_rgb[(3, 2, 2, 1)[c]]
    title = [(6, 'YUN', 0), (9, '%s - LEVELS %d-%d', 1), (14, 'O P Q   SPACE: BUBBLE', 2),
             (17, '1 KEYS  2 KEMPSTON', 1), (19, '3 SINCLAIR', 1), (22, 'THE MOJON TWINS', 3)]
    ending = [(10, 'WELL DONE!', 0), (13, 'END OF THE %s LOAD' % names[a.load], 1)]
    for d, w, col, bg, sysname in ((ZD, 256, zxc, BLACK, 'ZX SPECTRUM'), (CD, 128, cpc, lvl_rgb[0][0], 'AMSTRAD CPC')):
        tl = [(y, (t % (sysname, first, last)) if '%' in t else t, c) for y, t, c in title]
        if d == CD: tl = [l for l in tl if not l[1].startswith('3 ')]
        for name, lines in (('title.png', tl), ('ending.png', ending)):
            pth = os.path.join(d, name)
            if os.path.exists(pth) and not getattr(a, 'screens', False): print('kept', pth); continue
            screen(lines, w, col, bg).save(pth)

    with open(os.path.join(G, 'nes2load.txt'), 'w') as f:
        f.write('Load %d (nes2load.py)\n\n' % a.load + '\n'.join(notes) + '\n')
        f.write('\nhotspot tiles: %d (bulbs: %s)\n' % (16 + ntiles_x, has_bulbs))
        f.write('CPC inks: levels %s, sprites %s\n' % ([Lv['inks'] for Lv in L], [nearest_fw(c) for c in spr_rgb[1:]]))
    print(open(os.path.join(G, 'nes2load.txt')).read())

if __name__ == '__main__':
    main()
