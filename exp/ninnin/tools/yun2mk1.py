#!/usr/bin/env python3
"""
yun2mk1.py - Convert Yun (MK2 NES, 16x12 screens) maps + enems to smaller
screens for MTE MK1 (15x10) or a 16x11 variant, deleting interior rows/columns.

Approach
  * Edge rows/columns are never deleted -> each screen keeps its borders.
  * Deleting rows shifts left/right edges; deleting columns shifts top/bottom
    edges. So row choices are optimised along each horizontal strip of screens
    and column choices along each vertical strip (Viterbi), with a penalty for
    misaligned openings between neighbours.
  * Per-screen cost: deleting a line that duplicates a neighbour line is free;
    losing unique geometry, hazards, locks, entities or closing a passage costs.
  * Verification: a small simulation of Yun's real player physics (values from
    v5/dev/definitions.h) builds a reachability graph per screen before and
    after. Screens whose graph lost connections are flagged. Bubbles are NOT
    modelled, so a flag means "check by hand", not "broken".

Usage
  python3 yun2mk1.py yun YUN_V5_DIR MAPNUM [--cols 15 --rows 10] [--out DIR]
  python3 yun2mk1.py raw --map level.map --map-w 4 --map-h 6 --behs 0,0,8,... \
          [--scr-w 16 --scr-h 12] [--ene level.ene] [--tiles ts.png] [--lock 15]
          [--fixmappy] [--phys VY_JUMP=48,...] [--cols 15 --rows 10] [--out DIR]

  The reachability check models MK2 NES-style physics (1/16 px, Yun's values by
  default); pass --phys with your game's constants for other games.
"""

import argparse, itertools, os, re, sys
from functools import lru_cache

try:
    from PIL import Image, ImageDraw
except ImportError:
    Image = None

SCR_W, SCR_H = 16, 12

# ---------------------------------------------------------------------------
# Input

def read_maplist(yun):
    res = {}
    with open(os.path.join(yun, 'map', 'maplist.txt')) as f:
        for line in f:
            p = line.strip().split(',')
            if len(p) < 5: continue
            n = int(re.search(r'(\d+)', p[0]).group(1))
            res[n] = dict(file=p[0], w=int(p[1]), h=int(p[2]),
                          lock=int(p[3]), fixmappy=int(p[4]) != 0)
    return res

def find_file(d, name):
    for f in os.listdir(d):
        if f.lower() == name.lower(): return os.path.join(d, f)
    raise FileNotFoundError(os.path.join(d, name))

def read_behs(yun):
    txt = open(os.path.join(yun, 'dev', 'assets', 'behs.h')).read()
    res = {}
    for m in re.finditer(r'behs(\d+)\s*\[\]\s*=\s*\{([^}]*)\}', txt):
        res[int(m.group(1))] = [int(v) for v in m.group(2).replace('\n', '').split(',') if v.strip()]
    return res

def read_map(path, w, h, fixmappy):
    d = open(path, 'rb').read()
    assert len(d) == w * h * SCR_W * SCR_H, (path, len(d))
    screens = []
    for sy in range(h):
        for sx in range(w):
            g = []
            for y in range(SCR_H):
                row = []
                for x in range(SCR_W):
                    t = d[(sy * SCR_H + y) * w * SCR_W + sx * SCR_W + x]
                    if fixmappy and t: t -= 1
                    row.append(t)
                g.append(row)
            screens.append(g)
    return screens

def read_ene(path):
    d = open(path, 'rb').read()
    hdr = d[:256]
    w, h, sw, sh, ne = d[256:261]
    p = 261
    enems, hots = [], []
    for s in range(w * h):
        lst = []
        for i in range(ne):
            lst.append(list(d[p:p + 8])); p += 8
        enems.append(lst)
    for s in range(w * h):
        hots.append(list(d[p:p + 3])); p += 3
    return dict(hdr=hdr, w=w, h=h, sw=sw, sh=sh, ne=ne, enems=enems, hots=hots)

def write_ene(path, ene, cols, rows, enems, hots):
    out = bytearray(ene['hdr'])
    out += bytes([ene['w'], ene['h'], cols, rows, ene['ne']])
    for s in enems:
        for e in s: out += bytes(e)
    for hs in hots: out += bytes(hs)
    open(path, 'wb').write(out)

def write_map(path, screens, w, h, cols, rows):
    out = bytearray()
    for sy in range(h):
        for y in range(rows):
            for sx in range(w):
                out += bytes(screens[sy * w + sx][y])
    open(path, 'wb').write(out)

# ---------------------------------------------------------------------------
# Tile semantics (MK2 NES behaviours)

class Behs:
    def __init__(self, behs, lock):
        self.b = behs + [0] * (32 - len(behs))
        self.lock = lock
    def blocked(self, t):  return bool(self.b[t] & 8)
    def platform(self, t): return self.b[t] == 4
    def kill(self, t):     return self.b[t] == 1
    def standable(self, t): return self.blocked(t) or self.platform(t)
    def air(self, t):      return not self.blocked(t) and not self.platform(t) and not self.kill(t)
    def weight(self, t):
        if t == self.lock: return 25.0
        if self.kill(t): return 6.0
        if self.blocked(t) or self.platform(t): return 3.0
        return 0.3                                   # decoration / plain air

# ---------------------------------------------------------------------------
# Deleting lines

def apply_deletion(g, drows, dcols):
    return [[t for x, t in enumerate(r) if x not in dcols]
            for y, r in enumerate(g) if y not in drows]

def transpose(g): return [list(r) for r in zip(*g)]

def line_cost(g, dels, B, ent_lines):
    """Cost of deleting rows `dels` from grid g (rows are the lines).
    Columns are handled by transposing first."""
    H, W = len(g), len(g[0])
    kept = [y for y in range(H) if y not in dels]
    cost = 0.0
    for r in dels:
        ra = max(k for k in kept if k < r)
        rb = min(k for k in kept if k > r)
        for c in range(W):
            t = g[r][c]
            if t == g[ra][c] or t == g[rb][c]:
                if t == B.lock: cost += 25          # never lose a lock silently
                continue
            cost += B.weight(t)
        cost += 2.0 * ent_lines.count(r)
    # Passage checks per perpendicular line: runs of non-blocked cells
    for c in range(W):
        col = [g[y][c] for y in range(H)]
        runs, y = [], 0
        while y < H:
            if not B.blocked(col[y]):
                y0 = y
                while y < H and not B.blocked(col[y]): y += 1
                runs.append((y0, y))
            else: y += 1
        for y0, y1 in runs:
            n0 = y1 - y0
            n1 = sum(1 for yy in range(y0, y1) if yy not in dels)
            bounded = y0 > 0 and y1 < H
            if n1 == 0: cost += 30 if bounded else 8   # passage closed
            elif bounded and n0 >= 2 and n1 == 1: cost += 4
            elif bounded and n1 < n0 and n1 < 3: cost += 1
    return cost

def edge_line(g, dels, side):
    """Edge cells (perpendicular to deleted lines) after deletion.
    side 0 = first column, 1 = last column."""
    x = 0 if side == 0 else len(g[0]) - 1
    return [g[y][x] for y in range(len(g)) if y not in dels]

def edge_cost(a, b, B):
    return 10.0 * sum(1 for ta, tb in zip(a, b) if B.blocked(ta) != B.blocked(tb))

def viterbi(chain_grids, ndel, B, ent_lines_per):
    """chain_grids: grids in chain order (lines to delete are rows; the chain
    runs along columns, i.e. neighbour i is to the right of i-1)."""
    H = len(chain_grids[0])
    cands = list(itertools.combinations(range(1, H - 1), ndel))
    if not cands: cands = [()]
    own = [[line_cost(g, set(c), B, el) for c in cands]
           for g, el in zip(chain_grids, ent_lines_per)]
    edges_r = [[edge_line(g, set(c), 1) for c in cands] for g in chain_grids]
    edges_l = [[edge_line(g, set(c), 0) for c in cands] for g in chain_grids]
    n = len(chain_grids)
    acc = [own[0][:]]; back = [[None] * len(cands)]
    for i in range(1, n):
        row, br = [], []
        for j in range(len(cands)):
            best, bk = None, None
            for k in range(len(cands)):
                v = acc[i - 1][k] + edge_cost(edges_r[i - 1][k], edges_l[i][j], B)
                if best is None or v < best: best, bk = v, k
            row.append(best + own[i][j]); br.append(bk)
        acc.append(row); back.append(br)
    j = min(range(len(cands)), key=lambda k: acc[-1][k])
    path = [j]
    for i in range(n - 1, 0, -1):
        j = back[i][j]; path.append(j)
    path.reverse()
    return [set(cands[j]) for j in path], [own[i][path[i]] for i in range(n)]

# ---------------------------------------------------------------------------
# Entities

def remap(v, dels, size, nudge_up=True):
    kept = [i for i in range(size) if i not in dels]
    if v in kept: return kept.index(v), False
    if v >= size: return len(kept) - 1, True
    below = [k for k in kept if k < v]
    above = [k for k in kept if k > v]
    k = below[-1] if (nudge_up and below) or not above else above[0]
    return kept.index(k), True

# ---------------------------------------------------------------------------
# Reachability (Yun v5 player physics, 1/16 px)

VX_MAX, AX, RX = 32, 3, 4
G, G_JUMP, VY_FALL_MAX = 4, 1, 64
VY_JUMP, VY_RELEASE, JUMP_STEPS = 52, 24, 16
HB_W, HB_TOP, HB_BOT = 8, 4, 15           # hitbox: x..x+7, y+4..y+15

def simulate_screen(g, B):
    """Return (nodes, edges). Nodes are (tx, ty) = tile the player stands in
    (feet tile row above the floor). Plus exit nodes 'L','R','U','D'."""
    H, W = len(g), len(g[0])
    PW, PH = W * 16, H * 16

    def solid_at(px, py):
        tx, ty = px >> 4, py >> 4
        if tx < 0 or tx >= W or ty < 0: return False
        if ty >= H: return False
        return B.blocked(g[ty][tx])

    def tile(px, py):
        tx, ty = px >> 4, py >> 4
        if 0 <= tx < W and 0 <= ty < H: return g[ty][tx]
        return None

    nodes = set()
    for y in range(H - 1):
        for x in range(W):
            if not B.blocked(g[y][x]) and not B.kill(g[y][x]) and B.standable(g[y + 1][x]):
                if not B.blocked(g[y][x]):
                    nodes.add((x, y))

    programs = [(-1, 0, 0), (1, 0, 0), (0, 8, 0), (0, 16, 0)]
    for d in (-1, 1):
        for hold in (6, 16):
            for delay in (0, 8):
                programs.append((d, hold, delay))

    edges = {}
    for (nx, ny) in nodes:
        adj = set()
        for (dirn, hold, delay) in programs:
            for off in (0, 4, 8):
                px = (nx * 16 + off) << 4
                py = (ny * 16) << 4
                res = run(px, py, dirn, hold, delay, g, B, W, H, solid_at, tile)
                adj |= res
        adj.discard((nx, ny))
        edges[(nx, ny)] = adj
    return nodes, edges

def run(px, py, dirn, hold, delay, g, B, W, H, solid_at, tile):
    """One trajectory. Returns set of nodes landed on / walked over / exits."""
    out = set()
    vx = vy = 0
    jumping = hold > 0
    pj = False
    airborne = False
    if jumping:
        vy = -VY_JUMP; pj = True; airborne = True
    ctj = 0
    for f in range(110):
        d = dirn if f >= delay else 0
        # horizontal
        if d: vx = max(-VX_MAX, min(VX_MAX, vx + d * AX))
        else:
            if vx > 0: vx = max(0, vx - RX)
            elif vx < 0: vx = min(0, vx + RX)
        nx = px + vx
        x0, y0 = nx >> 4, py >> 4
        if vx < 0:
            if solid_at(x0, y0 + HB_TOP) or solid_at(x0, y0 + HB_BOT):
                nx = (((x0 >> 4) + 1) << 4) << 4; vx = 0
        elif vx > 0:
            if solid_at(x0 + HB_W - 1, y0 + HB_TOP) or solid_at(x0 + HB_W - 1, y0 + HB_BOT):
                nx = ((((x0 + HB_W - 1) >> 4) << 4) - HB_W) << 4; vx = 0
        px = nx
        # vertical
        vy += G_JUMP if pj else G
        if vy > VY_FALL_MAX: vy = VY_FALL_MAX
        if pj:
            ctj += 1
            if ctj >= hold:
                pj = False
                if hold < JUMP_STEPS and vy < -VY_RELEASE: vy = -VY_RELEASE
        ny = py + vy
        xa, ya = px >> 4, ny >> 4
        oldbot = (py >> 4) + HB_BOT
        grounded = False
        if vy < 0:
            if solid_at(xa, ya + HB_TOP) or solid_at(xa + HB_W - 1, ya + HB_TOP):
                ny = (((((ya + HB_TOP) >> 4) + 1) << 4) - HB_TOP) << 4; vy = 0; pj = False
        elif vy > 0:
            bot = ya + HB_BOT
            def stands(tx_px):
                t = tile(tx_px, bot)
                if t is None: return False
                if B.blocked(t): return True
                return B.platform(t) and (oldbot >> 4) < (bot >> 4)
            if stands(xa) or stands(xa + HB_W - 1):
                ny = ((((bot >> 4) << 4) - HB_BOT - 1)) << 4; vy = 0; grounded = True
        py = ny
        # grounded: feet exactly on top of a standable tile (sub-pixel sink ignored)
        if vy >= 0 and not grounded:
            below = (py >> 4) + HB_BOT + 1
            if below % 16 == 0:
                for tx_px in (px >> 4, (px >> 4) + HB_W - 1):
                    t = tile(tx_px, below)
                    if t is not None and B.standable(t): grounded = True
            if grounded:
                vy = 0; py = (((py >> 4) << 4))
        # exits
        cx, cy = (px >> 4) + HB_W // 2, (py >> 4) + 8
        if (px >> 4) < 0: out.add('L'); return out
        if (px >> 4) + HB_W > W * 16: out.add('R'); return out
        if (py >> 4) + HB_TOP < 0: out.add('U'); return out
        if (py >> 4) + HB_BOT >= H * 16: out.add('D'); return out
        # hazards
        for tx_px in ((px >> 4), (px >> 4) + HB_W - 1):
            for ty_px in ((py >> 4) + HB_TOP, (py >> 4) + HB_BOT):
                t = tile(tx_px, ty_px)
                if t is not None and B.kill(t): return out
        if grounded:
            node = (cx >> 4, ((py >> 4) + HB_BOT) >> 4)
            out.add(node)
            if airborne: return out
            if dirn == 0 and not jumping: return out
        else:
            airborne = True
        if grounded and vx == 0 and f > delay + 2: return out
    return out

def closure(nodes, edges):
    reach = {}
    for n in nodes:
        seen, st = set(), [n]
        while st:
            a = st.pop()
            for b in edges.get(a, ()):
                if b not in seen:
                    seen.add(b)
                    if b in edges: st.append(b)
        reach[n] = seen
    return reach

_sim_cache = {}
def screen_reach(g, B):
    key = tuple(map(tuple, g))
    if key not in _sim_cache:
        nodes, edges = simulate_screen(g, B)
        _sim_cache[key] = (nodes, closure(nodes, edges))
    return _sim_cache[key]

def compare_reach(g0, g1, drows, dcols, B):
    n0, r0 = screen_reach(g0, B)
    n1, r1 = screen_reach(g1, B)
    def m(n):
        if isinstance(n, str): return n
        x, y = n
        if x in dcols or y in drows: return None
        return (x - sum(1 for c in dcols if c < x), y - sum(1 for r in drows if r < y))
    lost, gained, lost_exits = 0, 0, set()
    for a in n0:
        ma = m(a)
        if ma is None or ma not in r1: continue
        before = {m(b) for b in r0[a]} - {None}
        after = r1[ma]
        for b in before - after:
            if not isinstance(b, str) and b not in n1: continue   # lip landings, not real spots
            lost += 1
            if isinstance(b, str): lost_exits.add(b)
        gained += len({b for b in after if not isinstance(b, str)} - before)
    ex0 = set().union(*[{b for b in r0[a] if isinstance(b, str)} for a in n0]) if n0 else set()
    ex1 = set().union(*[{b for b in r1[a] if isinstance(b, str)} for a in n1]) if n1 else set()
    return dict(lost=lost, gained=gained, lost_exits=sorted(lost_exits),
                exits_gone=sorted(ex0 - ex1))

# ---------------------------------------------------------------------------
# Previews

def load_tiles(path):
    im = Image.open(path).convert('RGB')
    return [im.crop((x * 16, y * 16, x * 16 + 16, y * 16 + 16))
            for y in range(im.height // 16) for x in range(im.width // 16)]

def render(screens, w, h, cols, rows, tiles, marks=None, flagged=None):
    im = Image.new('RGB', (w * cols * 16 + (w - 1) * 4, h * rows * 16 + (h - 1) * 4), (40, 40, 40))
    dr = ImageDraw.Draw(im, 'RGBA')
    for s, g in enumerate(screens):
        ox = (s % w) * (cols * 16 + 4); oy = (s // w) * (rows * 16 + 4)
        for y, row in enumerate(g):
            for x, t in enumerate(row):
                if t < len(tiles): im.paste(tiles[t], (ox + x * 16, oy + y * 16))
        if marks:
            drows, dcols = marks[s]
            for r in drows: dr.rectangle([ox, oy + r * 16, ox + cols * 16 - 1, oy + r * 16 + 15], fill=(255, 0, 0, 110))
            for c in dcols: dr.rectangle([ox + c * 16, oy, ox + c * 16 + 15, oy + rows * 16 - 1], fill=(255, 0, 0, 110))
        if flagged and s in flagged:
            col = (255, 60, 60) if flagged[s] == 'bad' else (255, 200, 0)
            for k in range(3):
                dr.rectangle([ox + k, oy + k, ox + cols * 16 - 1 - k, oy + rows * 16 - 1 - k], outline=col)
        dr.text((ox + 3, oy + 2), '%d' % s, fill=(255, 255, 255, 255))
    return im

# ---------------------------------------------------------------------------

def parse_args():
    ap = argparse.ArgumentParser(description='Shrink NES flip-screen maps (+ enemies) to smaller screens.')
    sub = ap.add_subparsers(dest='mode', required=True)
    y = sub.add_parser('yun', help='Yun v5 preset: reads maplist.txt, behs.h, level*.ene, ts*.png')
    y.add_argument('yun'); y.add_argument('mapnum', type=int)
    r = sub.add_parser('raw', help='any game: give the files and sizes explicitly')
    r.add_argument('--map', required=True, help='raw map, 1 byte/tile, whole map row-major (Mappy .map)')
    r.add_argument('--map-w', type=int, required=True); r.add_argument('--map-h', type=int, required=True)
    r.add_argument('--scr-w', type=int, default=16); r.add_argument('--scr-h', type=int, default=12)
    r.add_argument('--behs', required=True, help='comma-separated behaviour per tile (MK1/MK2: 1 kill, 4 platform, 8 solid)')
    r.add_argument('--lock', type=int, default=-1, help='lock tile number (never deleted silently)')
    r.add_argument('--fixmappy', action='store_true', help='tiles are 1-based (Mappy): subtract 1')
    r.add_argument('--ene', help='ponedor .ene (8 bytes/enemy, 3-byte hotspots)')
    r.add_argument('--tiles', help='tileset png of 16x16 tiles, for previews')
    r.add_argument('--name', default='map')
    for p_ in (y, r):
        p_.add_argument('--cols', type=int, default=15)
        p_.add_argument('--rows', type=int, default=10)
        p_.add_argument('--out', default='out')
        p_.add_argument('--noverify', action='store_true')
        p_.add_argument('--phys', default='', help='override player physics, e.g. "VY_JUMP=48,JUMP_STEPS=12" '
                        '(1/16 px units, NES frames; names as in this file)')
    return ap.parse_args()

def main():
    global SCR_W, SCR_H
    a = parse_args()
    for kv in filter(None, a.phys.split(',')):
        k, v = kv.split('='); k = k.strip()
        if k not in ('VX_MAX', 'AX', 'RX', 'G', 'G_JUMP', 'VY_FALL_MAX', 'VY_JUMP', 'VY_RELEASE', 'JUMP_STEPS',
                     'HB_W', 'HB_TOP', 'HB_BOT'):
            sys.exit('unknown physics constant ' + k)
        globals()[k] = int(v)

    if a.mode == 'yun':
        ml = read_maplist(a.yun)[a.mapnum]
        W, Hm = ml['w'], ml['h']
        B = Behs(read_behs(a.yun)[a.mapnum], ml['lock'] if ml['lock'] < 99 else -1)
        screens = read_map(find_file(os.path.join(a.yun, 'map'), ml['file']), W, Hm, ml['fixmappy'])
        ene = read_ene(find_file(os.path.join(a.yun, 'enems'), 'level%d.ene' % a.mapnum))
        tiles_png = os.path.join(a.yun, 'gfx', 'ts%d.png' % a.mapnum)
        tag = 'level%d_%dx%d' % (a.mapnum, a.cols, a.rows)
        title = 'Yun map %d' % a.mapnum
    else:
        SCR_W, SCR_H = a.scr_w, a.scr_h
        W, Hm = a.map_w, a.map_h
        B = Behs([int(v) for v in a.behs.split(',')], a.lock)
        screens = read_map(a.map, W, Hm, a.fixmappy)
        ene = read_ene(a.ene) if a.ene else None
        tiles_png = a.tiles
        tag = '%s_%dx%d' % (a.name, a.cols, a.rows)
        title = a.map
    if ene is None:
        ene = dict(hdr=bytes(256), w=W, h=Hm, sw=SCR_W, sh=SCR_H, ne=0,
                   enems=[[] for _ in range(W * Hm)], hots=[[0, 0, 0] for _ in range(W * Hm)], none=True)
    assert (ene['w'], ene['h']) == (W, Hm), 'enems file is for a %dx%d map' % (ene['w'], ene['h'])
    if a.cols > SCR_W or a.rows > SCR_H or a.cols < 3 or a.rows < 3:
        sys.exit('target size must be between 3x3 and the source screen size')
    os.makedirs(a.out, exist_ok=True)
    ndr, ndc = SCR_H - a.rows, SCR_W - a.cols

    # entity lines per screen (rows / cols occupied by enemy endpoints and hotspots)
    ent_rows, ent_cols = [], []
    for s in range(W * Hm):
        rr, cc = [], []
        for e in ene['enems'][s]:
            if e[0]: rr += [e[2], e[4]]; cc += [e[1], e[3]]
        hs = ene['hots'][s]
        if hs[2]: rr.append(hs[1]); cc.append(hs[0])
        ent_rows.append(rr); ent_cols.append(cc)

    drows = [set() for _ in screens]; dcols = [set() for _ in screens]
    rcost = [0.0] * len(screens); ccost = [0.0] * len(screens)
    if ndr:
        for sy in range(Hm):
            idx = [sy * W + sx for sx in range(W)]
            sel, cs = viterbi([screens[i] for i in idx], ndr, B, [ent_rows[i] for i in idx])
            for i, d, c in zip(idx, sel, cs): drows[i] = d; rcost[i] = c
    if ndc:
        for sx in range(W):
            idx = [sy * W + sx for sy in range(Hm)]
            sel, cs = viterbi([transpose(screens[i]) for i in idx], ndc, B, [ent_cols[i] for i in idx])
            for i, d, c in zip(idx, sel, cs): dcols[i] = d; ccost[i] = c

    # Repair pass: re-choose lines for screens whose reachability suffers,
    # keeping neighbours fixed and scoring with the physics sim.
    def reach_pen(s, dr, dc):
        g1 = apply_deletion(screens[s], dr, dc)
        r = compare_reach(screens[s], g1, dr, dc, B)
        return 40 * len(r['exits_gone']) + 15 * len(r['lost_exits']) + 0.2 * r['lost']
    def local_score(s, dr, dc):
        sx, sy = s % W, s // W
        g = screens[s]
        sc = line_cost(g, dr, B, ent_rows[s]) + line_cost(transpose(g), dc, B, ent_cols[s])
        if sx > 0:
            sc += edge_cost(edge_line(screens[s - 1], drows[s - 1], 1), edge_line(g, dr, 0), B)
        if sx < W - 1:
            sc += edge_cost(edge_line(g, dr, 1), edge_line(screens[s + 1], drows[s + 1], 0), B)
        gt = transpose(g)
        if sy > 0:
            sc += edge_cost(edge_line(transpose(screens[s - W]), dcols[s - W], 1), edge_line(gt, dc, 0), B)
        if sy < Hm - 1:
            sc += edge_cost(edge_line(gt, dc, 1), edge_line(transpose(screens[s + W]), dcols[s + W], 0), B)
        return sc + reach_pen(s, dr, dc)
    repaired = []
    if not a.noverify:
        rc = [set(c) for c in itertools.combinations(range(1, SCR_H - 1), ndr)] or [set()]
        cc = [set(c) for c in itertools.combinations(range(1, SCR_W - 1), ndc)] or [set()]
        for _pass in range(2):
            changed = False
            for s in range(len(screens)):
                if reach_pen(s, drows[s], dcols[s]) == 0: continue
                best = (local_score(s, drows[s], dcols[s]), drows[s], dcols[s])
                for dr in rc:
                    v = local_score(s, dr, dcols[s])
                    if v < best[0] - 0.01: best = (v, dr, dcols[s])
                for dc in cc:
                    v = local_score(s, best[1], dc)
                    if v < best[0] - 0.01: best = (v, best[1], dc)
                if (best[1], best[2]) != (drows[s], dcols[s]):
                    drows[s], dcols[s] = best[1], best[2]; changed = True
                    if s not in repaired: repaired.append(s)
            if not changed: break
        for s in range(len(screens)):
            rcost[s] = line_cost(screens[s], drows[s], B, ent_rows[s])
            ccost[s] = line_cost(transpose(screens[s]), dcols[s], B, ent_cols[s])

    new = [apply_deletion(g, drows[i], dcols[i]) for i, g in enumerate(screens)]

    # entities
    enems_out, hots_out, moved = [], [], []
    for s in range(W * Hm):
        lst = []
        for k, e in enumerate(ene['enems'][s]):
            e = e[:]
            if e[0]:
                e[1], m1 = remap(e[1], dcols[s], SCR_W, False)
                e[3], m2 = remap(e[3], dcols[s], SCR_W, False)
                e[2], m3 = remap(e[2], drows[s], SCR_H, True)
                e[4], m4 = remap(e[4], drows[s], SCR_H, True)
                if m1 or m2 or m3 or m4: moved.append('screen %d enemy %d (type %02x) nudged' % (s, k, e[0]))
            lst.append(e)
        enems_out.append(lst)
        hs = ene['hots'][s][:]
        if hs[2]:
            hs[0], m1 = remap(hs[0], dcols[s], SCR_W, False)
            hs[1], m2 = remap(hs[1], drows[s], SCR_H, True)
            if m1 or m2: moved.append('screen %d hotspot (type %d) nudged' % (s, hs[2]))
            if B.blocked(new[s][hs[1]][hs[0]]):
                o = ene['hots'][s]
                was = o[1] < SCR_H and B.blocked(screens[s][o[1]][o[0]])
                moved.append('screen %d hotspot (type %d) sits in a solid tile%s'
                             % (s, hs[2], ' (same in the NES original)' if was else ' - CAUSED BY CONVERSION'))
        hots_out.append(hs)

    write_map(os.path.join(a.out, tag + '.map'), new, W, Hm, a.cols, a.rows)
    if not ene.get('none'):
        write_ene(os.path.join(a.out, tag + '.ene'), ene, a.cols, a.rows, enems_out, hots_out)

    # verify
    flagged, lines = {}, []
    for s in range(W * Hm):
        info = ''
        if not a.noverify:
            r = compare_reach(screens[s], new[s], drows[s], dcols[s], B)
            if r['exits_gone'] or r['lost_exits']: flagged[s] = 'bad'
            elif r['lost']: flagged[s] = 'warn'
            info = ' lost=%d gained=%d%s%s' % (r['lost'], r['gained'],
                    (' LOST-EXITS-FROM-SOME=' + ''.join(r['lost_exits'])) if r['lost_exits'] else '',
                    (' EXITS-GONE=' + ''.join(r['exits_gone'])) if r['exits_gone'] else '')
        lines.append('screen %2d  rows-%s cols-%s  cost r=%.1f c=%.1f%s%s' % (
            s, sorted(drows[s]), sorted(dcols[s]), rcost[s], ccost[s], info,
            '  <-- ' + flagged[s].upper() if s in flagged else ''))

    rep = ['%s -> %dx%d  (%dx%d screens of %dx%d)' % (title, a.cols, a.rows, W, Hm, SCR_W, SCR_H), '']
    rep += lines + ['']
    rep.append('Flagged: %d bad (an exit became unreachable), %d warn (internal connections lost), of %d screens'
               % (sum(1 for v in flagged.values() if v == 'bad'), sum(1 for v in flagged.values() if v == 'warn'), len(screens)))
    def edge_mis(scrs):
        m = 0
        for s, g in enumerate(scrs):
            if s % W < W - 1:
                m += sum(B.blocked(r[-1]) != B.blocked(q[0]) for r, q in zip(g, scrs[s + 1]))
            if s // W < Hm - 1:
                m += sum(B.blocked(t) != B.blocked(u) for t, u in zip(g[-1], scrs[s + W][0]))
        return m
    rep.append('Edge cells where solid meets passable across neighbouring screens: %d in the NES map, %d after'
               % (edge_mis(screens), edge_mis(new)))
    rep.append('Re-chosen by the repair pass: %s' % (', '.join(map(str, repaired)) or 'none'))
    rep += [''] + (moved or ['No entities moved.'])
    open(os.path.join(a.out, tag + '_report.txt'), 'w').write('\n'.join(rep) + '\n')
    print('\n'.join(rep))

    if Image and tiles_png:
        tiles = load_tiles(tiles_png)
        before = render(screens, W, Hm, SCR_W, SCR_H, tiles, marks=list(zip(drows, dcols)))
        after = render(new, W, Hm, a.cols, a.rows, tiles, flagged=flagged)
        before.save(os.path.join(a.out, tag + '_before.png'))
        after.save(os.path.join(a.out, tag + '_after.png'))

if __name__ == '__main__':
    main()
