#!/usr/bin/env python3
"""check_tiles.py - TEST ONLY, not part of the build: checks that mkts_om's
charset + tile maps (gfx/*/tiles.spt) give back the tileset PNGs pixel for pixel.

  python3 check_tiles.py GAME_DIR/dev zx|cpc

Run it after comp.sh / comp_cpc.sh. (mkts_om has misread images in some
sequences of PNG sizes; keeping every tileset PNG the same width avoids it.)
"""
import os, re, sys
from PIL import Image
dev, plat = sys.argv[1], sys.argv[2]
ZX = [(0,0,0),(0,0,192),(192,0,0),(192,0,192),(0,192,0),(0,192,192),(192,192,0),(192,192,192)]
ZXB = [(0,0,0),(0,0,255),(255,0,0),(255,0,255),(0,255,0),(0,255,255),(255,255,0),(255,255,255)]
gfx = os.path.join(dev, '..', 'gfx', 'speccy' if plat == 'zx' else 'cpc')
spt = open(os.path.join(gfx, 'tiles.spt')).read()
jobs, pal, img = [], None, None
for line in spt.splitlines():
    t = line.split('#')[0].split()
    if not t: continue
    if t[0] == 'pal': pal = t[1]
    if t[0] == 'open': img = t[1]
    if t[0] == 'metatileset': n = int(t[3])
    if t[0] == 'output' and t[1] == 'tmaps': jobs.append((t[2], img, n, pal))
pats = open(os.path.join(dev, 'tileset.bin' if plat == 'zx' else 'cpc/patterns.bin'), 'rb').read()
def lev(v): return 0 if v < 85 else 128 if v < 170 else 255
def zxn(c):
    # as mkts_om reads a colour: each channel on or off, bright if any is over 224
    on = tuple(1 if v >= 192 else 0 for v in c)
    return tuple((255 if max(c) > 224 else 192) * k for k in on)
bad_total = 0
for tmf, png, n, palf in jobs:
    tm = open(os.path.join(dev, tmf), 'rb').read()
    im = Image.open(os.path.join(dev, png)).convert('RGB')
    bad = 0
    if plat == 'zx':
        for t in range(n):
            for q in range(4):
                a, c = tm[t * 8 + q * 2], tm[t * 8 + q * 2 + 1]
                P = ZXB if a & 64 else ZX
                for y in range(8):
                    b = pats[c * 8 + y] if c * 8 + y < len(pats) else 0
                    for x in range(8):
                        v = P[a & 7] if b & (128 >> x) else P[(a >> 3) & 7]
                        if v != zxn(im.getpixel((t * 16 + (q & 1) * 8 + x, (q >> 1) * 8 + y))): bad += 1
    else:
        pi = Image.open(os.path.join(dev, palf)).convert('RGB')
        cols = [tuple(lev(v) for v in pi.getpixel((i, 0))) for i in range(16)]
        for t in range(n):
            for q in range(4):
                c = tm[t * 4 + q]
                for y in range(8):
                    for bx in range(2):
                        b = pats[c * 16 + y * 2 + bx]
                        for k in range(2):
                            s = b << k
                            pen = ((s >> 7) & 1) | (((s >> 3) & 1) << 1) | (((s >> 5) & 1) << 2) | (((s >> 1) & 1) << 3)
                            o = tuple(lev(v) for v in im.getpixel((t * 8 + (q & 1) * 4 + bx * 2 + k, (q >> 1) * 8 + y)))
                            if cols.index(o) != pen if o in cols else True: bad += 1
    print('%-22s %-28s %s' % (tmf, png, 'ok' if not bad else '%d wrong pixels' % bad))
    bad_total += bad
sys.exit(1 if bad_total else 0)
