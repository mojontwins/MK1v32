#!/usr/bin/env python3
# Generates the original top-down graphics for "Camp Escape" (a study of
# The Great Escape's design in MK1 v3.2):
#
#   gfx/speccy/work.png     256x48 tileset (48 tiles 16x16)
#   gfx/speccy/sprites.png  256x32 spriteset (16 cells + masks)
#   gfx/speccy/font.png     skeleton font with frame glyphs for text boxes
#   gfx/speccy/title.png, marco.png, ending.png, loading.png
#
# All art is drawn here procedurally / from ASCII. Run from the project root:
#   python3 gfx/src/make_gfx.py

import os
import random
from PIL import Image, ImageDraw

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..')
OUT = os.path.join(ROOT, 'gfx', 'speccy')

N, B = 215, 255
PAL = {
	'k': (0, 0, 0), 'b': (0, 0, N), 'r': (N, 0, 0), 'm': (N, 0, N),
	'g': (0, N, 0), 'c': (0, N, N), 'y': (N, N, 0), 'w': (N, N, N),
	'K': (0, 0, 0), 'B': (0, 0, B), 'R': (B, 0, 0), 'M': (B, 0, B),
	'G': (0, B, 0), 'C': (0, B, B), 'Y': (B, B, 0), 'W': (B, B, B),
}


class Tile:
	def __init__(self, bg):
		self.p = [[bg] * 16 for _ in range(16)]

	def px(self, x, y, c):
		if 0 <= x < 16 and 0 <= y < 16:
			self.p[y][x] = c

	def rect(self, x0, y0, x1, y1, c, fill=True):
		for y in range(y0, y1 + 1):
			for x in range(x0, x1 + 1):
				if fill or x in (x0, x1) or y in (y0, y1):
					self.px(x, y, c)

	def hline(self, x0, x1, y, c):
		self.rect(x0, y, x1, y, c)

	def vline(self, x, y0, y1, c):
		self.rect(x, y0, x, y1, c)

	def art(self, rows, cmap):
		for y, row in enumerate(rows):
			for x, ch in enumerate(row):
				if ch in cmap:
					self.px(x, y, cmap[ch])

	def specks(self, c, n, seed):
		rnd = random.Random(seed)
		for i in range(n):
			self.px(rnd.randrange(16), rnd.randrange(16), c)


def tiles():
	T = [None] * 48

	# 0 DIRT (camp ground)
	t = Tile('y'); t.specks('k', 7, 1); T[0] = t
	# 1 GRASS (outside the wire)
	t = Tile('g')
	for (x, y) in ((2, 3), (10, 1), (6, 8), (13, 10), (3, 13), (9, 14)):
		t.px(x, y, 'k'); t.px(x + 1, y - 1, 'k'); t.px(x + 2, y, 'k')
	T[1] = t
	# 2 TREE (canopy seen from above)
	t = Tile('k')
	t.art([
		".....GGGGGG.....",
		"...GGGGGGGGGG...",
		"..GGGGkGGGGGGG..",
		".GGGkGGGGGkGGGG.",
		".GGGGGGGGGGGGGG.",
		"GGGGGGGkGGGGGGGG",
		"GGkGGGGGGGGGkGGG",
		"GGGGGGGGGGGGGGGG",
		"GGGGGkGGGGGGGGGG",
		"GGGGGGGGGGkGGGGG",
		".GGGGGGGGGGGGGG.",
		".GGkGGGGGGGGkGG.",
		"..GGGGGkGGGGGG..",
		"...GGGGGGGGGG...",
		".....GGGGGG.....",
		"................"], {'G': 'G', 'k': 'k'})
	T[2] = t
	# 3 FENCE, horizontal (barbed wire with posts)
	t = Tile('y')
	t.rect(0, 3, 1, 12, 'k'); t.rect(14, 3, 15, 12, 'k')
	for y in (5, 10):
		t.hline(0, 15, y, 'k')
		for x in (4, 8, 11):
			t.px(x, y - 1, 'k'); t.px(x + 1, y + 1, 'k')
	T[3] = t
	# 4 FENCE, vertical
	t = Tile('y')
	t.rect(3, 0, 12, 1, 'k'); t.rect(3, 14, 12, 15, 'k')
	for x in (5, 10):
		t.vline(x, 0, 15, 'k')
		for y in (4, 8, 11):
			t.px(x - 1, y, 'k'); t.px(x + 1, y + 1, 'k')
	T[4] = t
	# 5 HUT ROOF (felt strips)
	t = Tile('r')
	for y in (3, 7, 11, 15):
		t.hline(0, 15, y, 'k')
	for y, off in ((0, 2), (4, 9), (8, 5), (12, 12)):
		t.vline(off, y, y + 2, 'k')
	T[5] = t
	# 6 HUT ROOF RIDGE
	t = Tile('r')
	t.hline(0, 15, 6, 'k'); t.hline(0, 15, 9, 'k')
	for x in range(0, 16, 4):
		t.px(x, 7, 'k'); t.px(x + 2, 8, 'k')
	T[6] = t
	# 7 DOOR (doorway at the bottom of a hut / building)
	t = Tile('y')
	t.rect(1, 0, 14, 15, 'k')
	t.rect(3, 2, 12, 15, 'y', False)
	t.rect(4, 3, 11, 15, 'k')
	t.px(10, 9, 'y')
	T[7] = t
	# 8 WOOD FLOOR
	t = Tile('y')
	for y in (3, 7, 11, 15):
		t.hline(0, 15, y, 'k')
	for y, x in ((0, 5), (4, 12), (8, 2), (12, 9)):
		t.vline(x, y, y + 2, 'k')
	T[8] = t
	# 9 INTERIOR WALL (whitewashed bricks)
	t = Tile('w')
	for y in (3, 7, 11, 15):
		t.hline(0, 15, y, 'k')
	for row, y in enumerate((0, 4, 8, 12)):
		for x in ((3, 11) if row % 2 == 0 else (7, 15)):
			t.vline(x, y, y + 2, 'k')
	T[9] = t
	# 10 BUNK BED (blanket + pillow)
	t = Tile('b')
	t.rect(1, 0, 14, 15, 'w', False)
	t.rect(3, 1, 12, 4, 'w')
	for y in (7, 10, 13):
		t.hline(2, 13, y, 'w')
	T[10] = t
	# 11 TABLE
	t = Tile('k')
	t.rect(1, 1, 14, 14, 'r')
	t.rect(1, 1, 14, 14, 'k', False)
	t.hline(3, 12, 5, 'k'); t.hline(3, 12, 10, 'k')
	T[11] = t
	# 12 STOVE
	t = Tile('k')
	t.rect(2, 2, 13, 13, 'R', False)
	t.rect(5, 5, 10, 10, 'R')
	t.rect(7, 7, 8, 8, 'k')
	T[12] = t
	# 13 LOCKER
	t = Tile('c')
	t.rect(0, 0, 15, 15, 'k', False)
	t.vline(7, 0, 15, 'k'); t.vline(8, 0, 15, 'k')
	for y in (3, 4, 5):
		t.hline(2, 5, y, 'k'); t.hline(10, 13, y, 'k')
	t.px(5, 9, 'k'); t.px(10, 9, 'k')
	T[13] = t
	# 14 CRATE
	t = Tile('y')
	t.rect(0, 0, 15, 15, 'k', False); t.rect(1, 1, 14, 14, 'k', False)
	for i in range(2, 14):
		t.px(i, i, 'k'); t.px(15 - i, i, 'k')
	T[14] = t
	# 15 WATCHTOWER (roof seen from above)
	t = Tile('k')
	t.rect(0, 0, 15, 15, 'w', False)
	for i in range(1, 15):
		t.px(i, i, 'w'); t.px(15 - i, i, 'w')
	t.rect(6, 6, 9, 9, 'w')
	T[15] = t
	# 16 TUNNEL FLOOR
	t = Tile('r'); t.specks('k', 7, 16); T[16] = t
	# 17 TUNNEL WALL (shoring planks)
	t = Tile('k')
	for y in (1, 5, 9, 13):
		t.rect(0, y, 15, y + 2, 'y')
	t.vline(4, 0, 15, 'k'); t.vline(12, 0, 15, 'k')
	T[17] = t
	# 18 RUBBLE (collapsed tunnel)
	t = Tile('r')
	for (x, y, w, h) in ((1, 1, 5, 4), (8, 0, 6, 5), (0, 7, 4, 5), (5, 6, 6, 5), (12, 7, 3, 4), (2, 12, 6, 3), (10, 12, 5, 3)):
		t.rect(x, y, x + w - 1, y + h - 1, 'k', False)
		t.px(x + 1, y + 1, 'k')
	T[18] = t
	# 19 HATCH (trap door, on wood floor colours)
	t = Tile('y')
	t.rect(1, 1, 14, 14, 'k')
	t.rect(3, 3, 12, 12, 'y')
	for y in (5, 8, 11):
		t.hline(3, 12, y, 'k')
	t.rect(6, 6, 9, 7, 'k')
	T[19] = t
	# 20 LADDER (tunnel exit, going up)
	t = Tile('r')
	t.vline(3, 0, 15, 'k'); t.vline(12, 0, 15, 'k')
	for y in (2, 6, 10, 14):
		t.hline(3, 12, y, 'k')
	T[20] = t
	# 21 GATE (closed, bars)
	t = Tile('k')
	for x in (1, 5, 9, 13):
		t.rect(x, 0, x + 1, 15, 'w')
	t.hline(0, 15, 4, 'w'); t.hline(0, 15, 11, 'w')
	T[21] = t
	# 22 SAFE
	t = Tile('k')
	t.rect(1, 1, 14, 14, 'c')
	t.rect(1, 1, 14, 14, 'k', False)
	t.rect(5, 5, 10, 10, 'k', False)
	t.rect(7, 4, 8, 6, 'k')
	t.px(12, 8, 'k'); t.px(12, 9, 'k')
	T[22] = t
	# 23 DESK
	t = Tile('k')
	t.rect(0, 2, 15, 13, 'r')
	t.rect(3, 4, 9, 9, 'k', False)
	t.hline(4, 8, 6, 'k'); t.hline(11, 13, 5, 'k')
	T[23] = t
	# 24 GRAVEL (parade ground)
	t = Tile('w'); t.specks('k', 9, 24); T[24] = t
	# 25 ROLL CALL SPOT
	t = Tile('w')
	for i in range(3, 13):
		t.px(i, i, 'k'); t.px(i + 1, i, 'k'); t.px(15 - i, i, 'k'); t.px(14 - i, i, 'k')
	T[25] = t
	# 26 FENCE HOLE (cut wire)
	t = Tile('y')
	t.rect(0, 3, 1, 12, 'k'); t.rect(14, 3, 15, 12, 'k')
	for y in (5, 10):
		t.hline(0, 3, y, 'k'); t.hline(12, 15, y, 'k')
		t.px(4, y + 1, 'k'); t.px(11, y - 1, 'k')
	T[26] = t
	# 27 CELL DOOR (solitary)
	t = Tile('k')
	t.rect(2, 0, 13, 15, 'c')
	t.rect(2, 0, 13, 15, 'k', False)
	t.rect(5, 3, 10, 7, 'k')
	for x in (6, 8):
		t.vline(x, 3, 7, 'c')
	t.px(11, 10, 'k')
	T[27] = t
	# 28 BUSH
	t = Tile('g')
	t.art([
		"................",
		"....kkk..kkk....",
		"..kkGGGkkGGGk...",
		".kGGGGGGGGGGGk..",
		".kGGkGGGGGkGGk..",
		"kGGGGGGGGGGGGGk.",
		"kGGGGGkGGGGGGGk.",
		".kGGGGGGGGkGGk..",
		".kGkGGGGGGGGGGk.",
		"kGGGGGGGkGGGGGk.",
		"kGGGGGGGGGGGGk..",
		".kGGGkGGGGGGk...",
		"..kkGGGGGkkk....",
		"....kkkkk.......",
		"................",
		"................"], {'G': 'g', 'k': 'k'})
	T[28] = t
	# 29 SANDBAGS
	t = Tile('y')
	for (x, y) in ((0, 0), (8, 0), (4, 5), (12, 5), (-4, 5), (0, 10), (8, 10)):
		t.rect(x, y, x + 7, y + 4, 'k', False)
	T[29] = t
	# 30 STONE FLOOR (offices)
	t = Tile('c')
	t.hline(0, 15, 7, 'k'); t.hline(0, 15, 15, 'k')
	t.vline(7, 0, 6, 'k'); t.vline(15, 8, 14, 'k')
	T[30] = t
	# 31 STOVE PUSHED ASIDE (hatch visible next to it is tile 19)
	t = Tile('k')
	t.rect(0, 2, 7, 13, 'R', False)
	t.rect(2, 6, 5, 9, 'R')
	t.hline(9, 15, 3, 'R'); t.hline(9, 15, 12, 'R')
	T[31] = t

	# ---- Items (yellow background, black icons). Used in the map by the
	# script and in the HUD inventory.
	def item(rows):
		t = Tile('y')
		t.art(rows, {'#': 'k'})
		return t

	T[32] = item([  # KEY
		"................",
		"................",
		"...####.........",
		"..#....#........",
		"..#....##########",
		"..#....#.....#.#.",
		"...####......#.#.",
		"................",
		"................",
		"................",
		"................",
		"................",
		"................",
		"................",
		"................",
		"................"][:16])
	T[33] = item([  # LOCKPICK (bent wire tools)
		"................",
		"................",
		"..##............",
		"..###...........",
		"...###..........",
		"....###.........",
		".....###........",
		"......###.......",
		".......###......",
		"........###.#...",
		".........#####..",
		"..........###...",
		"...........#....",
		"................",
		"................",
		"................"])
	T[34] = item([  # WIRE CUTTERS
		"................",
		"...#.......#....",
		"...##.....##....",
		"....##...##.....",
		".....##.##......",
		"......###.......",
		"......#.#.......",
		".....##.##......",
		"....##...##.....",
		"...##.....##....",
		"..###.....###...",
		"..###.....###...",
		"................",
		"................",
		"................",
		"................"])
	T[35] = item([  # SHOVEL
		"................",
		"..........###...",
		"..........#.#...",
		"...........#....",
		"..........#.....",
		".........#......",
		"........#.......",
		".......#........",
		"....####........",
		"...#####........",
		"...#####........",
		"...####.........",
		"....##..........",
		"................",
		"................",
		"................"])
	T[36] = item([  # TORCH
		"................",
		"................",
		"................",
		"...........#....",
		"..######..##....",
		"..#....####.....",
		"..#....#####....",
		"..#....####.....",
		"..######..##....",
		"...........#....",
		"................",
		"................",
		"................",
		"................",
		"................",
		"................"])
	T[37] = item([  # UNIFORM (tunic)
		"................",
		".....##..##.....",
		"...####..####...",
		"..##.#####..##..",
		"..##.##.##..##..",
		"..##.#####..##..",
		"..##.##.##..##..",
		"..##.#####..##..",
		".....##.##......",
		".....#####......",
		".....##.##......",
		".....#####......",
		"................",
		"................",
		"................",
		"................"])
	T[38] = item([  # PAPERS
		"................",
		"..#########.....",
		"..#.......##....",
		"..#.#####.#.#...",
		"..#.......####..",
		"..#.######...#..",
		"..#..........#..",
		"..#.######...#..",
		"..#..........#..",
		"..#.###..##..#..",
		"..#.###..##..#..",
		"..############..",
		"................",
		"................",
		"................",
		"................"])
	T[39] = item([  # COMPASS
		"................",
		".....######.....",
		"....#......#....",
		"...#...##...#...",
		"..#....##....#..",
		"..#...####...#..",
		"..#...#..#...#..",
		"..#....##....#..",
		"..#....##....#..",
		"...#........#...",
		"....#......#....",
		".....######.....",
		"................",
		"................",
		"................",
		"................"])
	T[40] = item([  # FOOD (bread loaf)
		"................",
		"................",
		"................",
		"....########....",
		"..###.#.#.####..",
		".##.#.#.#.#..##.",
		".#..#.#.#.#...#.",
		".#............#.",
		".##############.",
		"................",
		"................",
		"................",
		"................",
		"................",
		"................",
		"................"])
	T[41] = item([  # BRIBE (cigarette packet)
		"................",
		"................",
		"....#.#.#.......",
		"....#.#.#.......",
		"...#########....",
		"...#.......#....",
		"...#.#####.#....",
		"...#.#...#.#....",
		"...#.#####.#....",
		"...#.......#....",
		"...#.......#....",
		"...#########....",
		"................",
		"................",
		"................",
		"................"])
	T[42] = item([  # PARCEL (with a cross)
		"................",
		"..############..",
		"..#....##....#..",
		"..#....##....#..",
		"..#..######..#..",
		"..#..######..#..",
		"..#....##....#..",
		"..#....##....#..",
		"..############..",
		"..#..........#..",
		"..############..",
		"................",
		"................",
		"................",
		"................",
		"................"])
	T[43] = Tile('k')
	# 44 EMPTY SLOT (HUD)
	t = Tile('k')
	for (x, y, dx, dy) in ((0, 0, 1, 1), (15, 0, -1, 1), (0, 15, 1, -1), (15, 15, -1, -1)):
		t.px(x, y, 'y'); t.px(x + dx, y, 'y'); t.px(x, y + dy, 'y')
	T[44] = t
	# 45 SAFE OPEN
	t = Tile('k')
	t.rect(1, 1, 14, 14, 'c')
	t.rect(1, 1, 14, 14, 'k', False)
	t.rect(3, 3, 12, 12, 'k')
	t.hline(3, 12, 8, 'c')
	T[45] = t
	# 46 GATE OPEN (walkable, takes you out)
	t = Tile('y')
	t.rect(0, 0, 1, 15, 'k'); t.rect(14, 0, 15, 15, 'k')
	t.specks('k', 3, 46)
	T[46] = t
	# 47 DARK
	T[47] = Tile('k')
	return T


def check_cells(t, i):
	for cy in (0, 8):
		for cx in (0, 8):
			cols = set()
			for y in range(8):
				for x in range(8):
					cols.add(PAL[t.p[cy + y][cx + x]])
			if len(cols) > 2:
				raise SystemExit('tile %d cell %d,%d has %d colours' % (i, cx, cy, len(cols)))


def save_tiles(T):
	im = Image.new('RGB', (256, 48))
	px = im.load()
	for i, t in enumerate(T):
		check_cells(t, i)
		ox, oy = (i % 16) * 16, (i // 16) * 16
		for y in range(16):
			for x in range(16):
				px[ox + x, oy + y] = PAL[t.p[y][x]]
	im.save(os.path.join(OUT, 'work.png'))
	return im


# ---------------------------------------------------------------- sprites --
# Each sprite: 16 rows, '#' = dark body (drawn as background PAPER colour),
# 'o' = light detail (drawn as INK, i.e. the tile's light colour),
# '.' = transparent. The mask is the silhouette ('#' or 'o') grown by 1px,
# so every figure gets a dark outline on any background.

PRISONER_DOWN = [
	"................",
	"......####......",
	".....######.....",
	".....#oooo#.....",
	".....#o#o##.....",
	".....#oooo#.....",
	"......#oo#......",
	"....########....",
	"...##########...",
	"...#o######o#...",
	"...#o##o###o#...",
	"....o######o....",
	".....######.....",
	".....##..##.....",
	".....##..##.....",
	".....oo..oo....."]


def flip_h(rows):
	return [r[::-1] for r in rows]


def walk(rows, a):
	# second walking frame: swap the legs
	r = list(rows)
	if a:
		r[13] = ".....##...##...."[:16]
		r[14] = "....##.....##..."
		r[15] = "....oo.....oo..."
	return r


PRISONER_UP = [
	"................",
	"......####......",
	".....######.....",
	".....######.....",
	".....######.....",
	".....######.....",
	"......####......",
	"....########....",
	"...##########...",
	"...#o######o#...",
	"...#o######o#...",
	"....o######o....",
	".....######.....",
	".....##..##.....",
	".....##..##.....",
	".....oo..oo....."]

PRISONER_RIGHT = [
	"................",
	"......####......",
	".....######.....",
	".....###ooo.....",
	".....###o#o.....",
	".....###ooo.....",
	"......##oo......",
	".....######.....",
	"....########....",
	"....###o####....",
	"....###o###o....",
	".....##o##......",
	".....######.....",
	"......####......",
	"......##.##.....",
	"......oo.oo....."]

PRISONER_RIGHT2 = PRISONER_RIGHT[:13] + [
	".....##..##.....",
	"....##....##....",
	"....oo....oo...."]

GUARD = [
	"......####......",
	".....######.....",
	"....########....",
	"....#oooooo#....",
	".....o#oo#o.....",
	".....oooooo.....",
	"......#oo#......",
	"...##########...",
	"..###o####o###..",
	"..##o######o##..",
	"..##o##oo##o##..",
	"...o#oo##oo#o...",
	".....######.....",
	".....##..##.....",
	".....##..##.....",
	"....###..###...."]
GUARD2 = GUARD[:13] + [
	".....##...##....",
	"....##.....##...",
	"...###.....###.."]

DOG = [
	"................",
	"................",
	"................",
	"..........##....",
	".........####...",
	".........#o#o#..",
	".........######.",
	"..#......####o#.",
	"...#######.##...",
	"...########.....",
	"...########.....",
	"...##o##o##.....",
	"...#.#...#.#....",
	"...#.#...#.#....",
	"..##.##.##.##...",
	"................"]
DOG2 = DOG[:12] + [
	"...#..#.#..#....",
	"..#...#.#...#...",
	".##..##.##..##..",
	"................"]

OFFICER = [
	"....########....",
	"...##########...",
	"....#oooooo#....",
	".....o#oo#o.....",
	".....oooooo.....",
	"......#oo#......",
	"....########....",
	"...###o##o###...",
	"..####o##o####..",
	"..###o####o###..",
	"..##.o####o.##..",
	"..oo.######.oo..",
	".....######.....",
	".....##..##.....",
	".....##..##.....",
	"....###..###...."]
OFFICER2 = OFFICER[:13] + [
	".....##...##....",
	"....##.....##...",
	"...###.....###.."]

SEARCHER = [  # guard with a torch beam (type 4 enemies)
	"................",
	".....######.....",
	"....########....",
	"....#oooooo#....",
	".....o#oo#o.....",
	".....oooooo.....",
	"...##########...",
	"..###o####o###..",
	"..##o######o##..",
	"..##o######o##oo",
	"...o#oo##oo#oooo",
	".....######..ooo",
	".....######.....",
	".....##..##.....",
	".....##..##.....",
	"....###..###...."]
SEARCHER2 = SEARCHER[:9] + [
	"..##o######o##..",
	"...o#oo##oo#o...",
	".....######.....",
	".....######.....",
	".....##..##.....",
	".....##..##.....",
	"....###..###...."]


def sprite_cells():
	cells = [
		walk(PRISONER_RIGHT, 0), PRISONER_RIGHT2,
		flip_h(walk(PRISONER_RIGHT, 0)), flip_h(PRISONER_RIGHT2),
		PRISONER_UP, walk(PRISONER_UP, 1),
		PRISONER_DOWN, walk(PRISONER_DOWN, 1),
		GUARD, GUARD2,
		DOG, flip_h(DOG2),
		OFFICER, OFFICER2,
		SEARCHER, SEARCHER2,
	]
	return cells


def save_sprites():
	im = Image.new('RGB', (256, 32))
	px = im.load()
	for i, rows in enumerate(sprite_cells()):
		assert len(rows) == 16 and all(len(r) == 16 for r in rows), i
		ox, oy = (i % 8) * 32, (i // 8) * 16
		solid = [[rows[y][x] in '#o' for x in range(16)] for y in range(16)]
		for y in range(16):
			for x in range(16):
				if rows[y][x] == 'o':
					px[ox + x, oy + y] = (255, 255, 255)
				# mask: 1 = transparent (keep background), 0 = sprite area
				grown = any(solid[yy][xx] for yy in range(max(0, y - 1), min(16, y + 2))
							for xx in range(max(0, x - 1), min(16, x + 2)))
				px[ox + 16 + x, oy + y] = (0, 0, 0) if grown else (255, 255, 255)
	im.save(os.path.join(OUT, 'sprites.png'))
	return im


# ------------------------------------------------------------------ font --

def make_font():
	src = Image.open(os.path.join(OUT, 'font.png')).convert('RGB')
	im = Image.new('RGB', (256, 16))
	im.paste(src.crop((0, 0, 256, 16)), (0, 0))
	d = ImageDraw.Draw(im)
	W = (255, 255, 255)

	def glyph(ch, rows):
		i = ord(ch) - 32
		ox, oy = (i % 32) * 8, (i // 32) * 8
		d.rectangle((ox, oy, ox + 7, oy + 7), fill=(0, 0, 0))
		for y, r in enumerate(rows):
			for x, c in enumerate(r):
				if c == '#':
					im.putpixel((ox + x, oy + y), W)
	# Text box frame (same character roles as Papaya's extern.h)
	glyph('<', ["........", "........", "........", "...#####", "..##....", "..#.....", "..#.....", "..#....."])
	glyph('=', ["........", "........", "........", "########", "........", "........", "........", "........"])
	glyph('>', ["........", "........", "........", "#####...", "....##..", ".....#..", ".....#..", ".....#.."])
	glyph('#', ["..#.....", "..#.....", "..#.....", "..#.....", "..#.....", "..#.....", "..#.....", "..#....."])
	glyph('$', [".....#..", ".....#..", ".....#..", ".....#..", ".....#..", ".....#..", ".....#..", ".....#.."])
	glyph("'", ["..#.....", "..#.....", "..##....", "...#####", "........", "........", "........", "........"])
	glyph('(', ["........", "........", "........", "########", "........", "........", "........", "........"])
	glyph(')', [".....#..", ".....#..", "....##..", "#####...", "........", "........", "........", "........"])
	# The 5-bit text decoder (msc4i.h) outputs , . ? ! and space as the
	# characters 91..95 ([ \\ ] ^ _): copy those glyphs into these slots.
	def copy_glyph(src, dst):
		i, j = ord(src) - 32, ord(dst) - 32
		g = im.crop(((i % 32) * 8, (i // 32) * 8, (i % 32) * 8 + 8, (i // 32) * 8 + 8))
		im.paste(g, ((j % 32) * 8, (j // 32) * 8))
	for src, dst in ((',', '['), ('.', '\\'), ('?', ']'), ('!', '^'), (' ', '_')):
		copy_glyph(src, dst)
	im.save(os.path.join(OUT, 'font.png'))
	return im


# --------------------------------------------------------------- screens --

def text(img, font, x, y, s, ink, paper=(0, 0, 0)):
	fp = font.load()
	ip = img.load()
	for n, ch in enumerate(s.upper()):
		i = ord(ch) - 32
		if i < 0 or i > 63:
			i = 0
		ox, oy = (i % 32) * 8, (i // 32) * 8
		for yy in range(8):
			for xx in range(8):
				ip[(x + n) * 8 + xx, y * 8 + yy] = ink if fp[ox + xx, oy + yy] != (0, 0, 0) else paper


def tile_at(img, work, cx, cy, t):
	img.paste(work.crop(((t % 16) * 16, (t // 16) * 16, (t % 16) * 16 + 16, (t // 16) * 16 + 16)), (cx * 8, cy * 8))


def big_text(img, x, y, s, colour, scale=2):
	# Chunky title lettering built from the font, scaled up.
	font = Image.open(os.path.join(OUT, 'font.png')).convert('RGB')
	fp = font.load()
	d = ImageDraw.Draw(img)
	for n, ch in enumerate(s):
		i = ord(ch) - 32
		ox, oy = (i % 32) * 8, (i // 32) * 8
		for yy in range(8):
			for xx in range(8):
				if fp[ox + xx, oy + yy] != (0, 0, 0):
					X = x + (n * 8 + xx) * scale
					Y = y + yy * scale
					d.rectangle((X, Y, X + scale - 1, Y + scale - 1), fill=colour)


def frame_screen(work, font):
	# Game frame: viewport at char (1,1)-(30,20), HUD in rows 21-23.
	im = Image.new('RGB', (256, 192))
	d = ImageDraw.Draw(im)
	Y = (255, 255, 0)
	R = (215, 0, 0)
	# barbed wire border around the viewport
	for x in range(0, 32):
		for (yy) in (0, 21):
			text(im, font, x, yy, '=', R)
	for y in range(0, 22):
		text(im, font, 0, y, '#', R)
		text(im, font, 31, y, '$', R)
	text(im, font, 0, 0, '<', R); text(im, font, 31, 0, '>', R)
	text(im, font, 0, 21, "'", R); text(im, font, 31, 21, ')', R)
	text(im, font, 1, 22, 'MORALE', (255, 255, 255))
	text(im, font, 11, 22, 'TIME', (255, 255, 255))
	text(im, font, 19, 22, 'ITEMS', (255, 255, 255))
	return im


def title_screen(work, font):
	im = Image.new('RGB', (256, 192))
	# landscape strip: wire fence across the middle, watchtowers, trees
	for cx in range(0, 32, 2):
		tile_at(im, work, cx, 10, 3)
		tile_at(im, work, cx, 12, 0)
		tile_at(im, work, cx, 8, 1)
	for cx in (2, 26):
		tile_at(im, work, cx, 6, 15)
	for cx in (6, 10, 14, 18, 22):
		tile_at(im, work, cx, 6, 2)
	big_text(im, 32, 8, 'CAMP', (255, 255, 0), 3)
	big_text(im, 128, 8, 'ESCAPE', (255, 255, 255), 2)
	text(im, font, 16, 4, 'MK1 V3.2', (0, 215, 215))
	text(im, font, 4, 16, '1 KEYBOARD  OPQA SPC', (255, 255, 255))
	text(im, font, 4, 18, '2 KEMPSTON', (255, 255, 255))
	text(im, font, 4, 19, '3 SINCLAIR', (255, 255, 255))
	text(im, font, 6, 22, 'AN MK1 STUDY - 2026', (215, 215, 215))
	return im


def ending_screen(work, font):
	im = Image.new('RGB', (256, 192))
	for cx in range(0, 32, 2):
		for cy in range(12, 24, 2):
			tile_at(im, work, cx, cy, 1)
	for cx in (0, 4, 8, 20, 24, 28):
		tile_at(im, work, cx, 12, 2)
	text(im, font, 6, 4, 'YOU MADE IT ACROSS', (255, 255, 255))
	text(im, font, 6, 6, 'THE BORDER. FREEDOM!', (255, 255, 0))
	text(im, font, 4, 9, 'WELL DONE, PRISONER.', (0, 215, 215))
	return im


def loading_screen(work, font):
	im = title_screen(work, font)
	d = ImageDraw.Draw(im)
	d.rectangle((0, 128, 255, 191), fill=(0, 0, 0))
	text(im, font, 10, 18, 'LOADING...', (255, 255, 255))
	return im


def main():
	T = tiles()
	work = save_tiles(T)
	save_sprites()
	font = make_font()
	# marco.png has been edited by hand (room name bar in row 21): not regenerated.
	# frame_screen(work, font).save(os.path.join(OUT, 'marco.png'))
	title_screen(work, font).save(os.path.join(OUT, 'title.png'))
	ending_screen(work, font).save(os.path.join(OUT, 'ending.png'))
	loading_screen(work, font).save(os.path.join(OUT, 'loading.png'))
	# A copy of the tileset for aportador / previews
	work.save(os.path.join(OUT, 'mappy.png'))
	print('gfx done')


if __name__ == '__main__':
	main()
