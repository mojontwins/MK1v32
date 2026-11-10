#!/usr/bin/env python3
# One-off generator for apor/greatesc.apor: the camp map, the enemies and the
# tile behaviours, written as ASCII here and packed into an Aportador project.
# After this first generation, edit apor/greatesc.apor with Aportador.
#
#   python3 map/src/make_apor.py
#
# Map: 6 x 5 screens of 15 x 10 tiles. Screen n = y * 6 + x.
#
#   y=0   0- 5  Outside the wire (forest). Entering any of them = escape attempt.
#   y=1   6-11  North compound: shed, Hut 1, Hut 2, Mess hut, Red Cross store, NE corner
#   y=2  12-17  South compound: storeroom yard, parade ground W/E, barracks*, office*, main gate*
#   y=3  18-23  Interiors: Hut 1 (your bunk), Hut 2, Mess hall, Red Cross store, barracks*, office*
#   y=4  24-29  Escape tunnel 24-28, solitary cell 29
#   (* = restricted area)

import base64
import json
import os

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..')

LEGEND = {
	'.': 0, ',': 1, 'T': 2, '-': 3, '|': 4, 'R': 5, '^': 6, 'D': 7,
	'_': 8, 'W': 9, 'b': 10, 't': 11, 'S': 12, 'L': 13, 'x': 14, 'O': 15,
	':': 16, '#': 17, '%': 18, 'H': 19, 'U': 20, 'G': 21, '$': 22, 'd': 23,
	'~': 24, 'X': 25, 'h': 26, 'C': 27, '*': 28, '=': 29, '+': 30, 's': 31,
}

S = {}

# ---------------------------------------------------------------- row 0 --
S[0] = """
TTTTTTTTTTTTTTT
T,,,*,,,T,,,,,,
T,T,,,,,,,,*,,,
T,,,,T,,,,,,,T,
T,,*,,,,,T,,,,,
T,,,,,,,,,,,,,,
TT,,,,*,,,,,,,,
T,,,,,,,,,T,,,,
T,,T,,,,,,,,,,,
T,,,,,,,,,,,,,,
"""
S[1] = """
TTTTTTTTTTTTTTT
,,,T,,,,,*,,,,,
,,,,,,T,,,,,T,,
,*,,,,,,,,,,,,,
,,,,T,,,*,,,,,,
,,,,,,,,,,,T,,,
,,T,,,,,,,,,,,,
,,,,,,*,,,T,,,,
,,,,,,,,,,,,,,,
,,,,,,,,,,,,,,,
"""
S[2] = """
TTTTTTTTTTTTTTT
,,,,,,,T,,,,,,,
,,T,,,,,,,,*,,,
,,,,,*,,,,,,,,,
,,,,,,,,,T,,,,,
,T,,,,,,,,,,,,,
,,,,,,T,,,,,*,,
,,,*,,,,,,,,,,,
,,,,,,,,,,,,,,,
,,,,,,,,,,,,,,,
"""
S[3] = """
TTTTTTTTTTTTTTT
,,T,,,,,,,,T,,,
,,,,,*,,,,,,,,,
,,,,,,,,T,,,,*,
,T,,,,,,,,,,,,,
,,,,,,*,,,,T,,,
,,,,,,,,,,,,,,,
,,,T,,,,,*,,,,,
,,,,,,,,,,,,,,,
,,,,,,,,,,,,,,,
"""
S[4] = """
TTTTTTTTTTTTTTT
,,,,,*,,,,,,,T,
,T,,,,,,,T,,,,,
,,,,,,,,,,,,,,,
,,,,T,,,*,,,,,,
,,,,,,,,,,,,T,,
,,*,,,,,,,,,,,,
,,,,,,,,T,,,,,,
,,,,,,,,,,,,,,,
,,,,,,,,,,,,,,,
"""
S[5] = """
TTTTTTTTTTTTTTT
,,T,,,,,,,,,,,T
,,,,,,*,,,,T,,T
,,,,,,,,,,,,,,T
,T,,,,,,,,*,,,T
,,,,,,,,,,,,,,T
,,,,*,,,,,,,,,T
,,,,,,,,,T,,,,T
,,,,,,,,,,,,,,T
,,,,,,,,,,,,,,T
"""

# ---------------------------------------------------------------- row 1 --
S[6] = """
O--------------
|..............
|.xxx..........
|.x............
|.xxx..........
|..............
|.......==.....
|..............
|..............
|..............
"""
S[7] = """
---------------
...RRRRRRRRR...
...^^^^^^^^^...
...RRRRRRRRR...
...RRRRRRRRR...
...RRRRDRRRR...
...............
...............
...............
...............
"""
S[8] = """
---------------
...RRRRRRRRR...
...^^^^^^^^^...
...RRRRRRRRR...
...RRRRRRRRR...
...RRRRDRRRR...
...............
...............
...............
...............
"""
S[9] = """
---------------
..RRRRRRRRRRR..
..^^^^^^^^^^^..
..RRRRRRRRRRR..
..RRRRRRRRRRR..
..RRRRRDRRRRR..
...............
...............
...............
...............
"""
S[10] = """
---------------
...............
....RRRRRRR....
....^^^^^^^....
....RRRRRRR....
....RRRDRRR....
...............
..x.........x..
...............
...............
"""
S[11] = """
--------------O
..............|
..............|
..............|
....===.......|
..............|
..............|
.........===..|
..............|
..............|
"""

# ---------------------------------------------------------------- row 2 --
S[12] = """
|..............
|.xxxx.........
|.x..x.........
|.x....x.x.....
|.xxxx.........
|..............
|......xx......
|......xx......
|..............
O--------------
"""
S[13] = """
...............
.~~~~~~~~~~~~~~
.~~~~~~~~~~~~~~
.~~~~~~~~~~~~~~
.~~~~~~~~~~~~~~
.~~~~~~~~~~~~~~
.~~~~~~~~~~~~~~
.~~~~~~~~~~~~~~
...............
---------------
"""
S[14] = """
...............
~~~~~~~~~~~~~~.
~~~~~~~~~~~~~~.
~~~~~~~~~~~~~~.
~~~~~~~X~~~~~~.
~~~~~~~~~~~~~~.
~~~~~~~~~~~~~~.
~~~~~~~~~~~~~~.
...............
---------------
"""
S[15] = """
...............
...RRRRRRRRR...
...^^^^^^^^^...
...RRRRRRRRR...
...RRRRRRRRR...
...RRRRDRRRR...
...............
.=...........=.
...............
---------------
"""
S[16] = """
...............
....RRRRRRR....
....^^^^^^^....
....RRRRRRR....
....RRRRRRR....
....RRRDRRR....
...............
...............
..=.........=..
---------------
"""
S[17] = """
..............|
..............|
..=...........O
...........=..|
..............G
..............G
...........=..|
..............|
..............|
--------------O
"""

# ---------------------------------------------------------------- row 3 --
S[18] = """
WWWWWWWWWWWWWWW
Wb_b_b_b______W
Wb_b_b_b____S_W
W_____________W
W____ttt______W
W____ttt______W
W_____________W
W_____________W
WWWWWWWDWWWWWWW
WWWWWWWWWWWWWWW
"""
S[19] = """
WWWWWWWWWWWWWWW
Wb_b_b_b_____LW
Wb_b_b_b_____LW
W_____________W
W_____________W
W____ttt______W
W____ttt______W
W_____________W
WWWWWWWDWWWWWWW
WWWWWWWWWWWWWWW
"""
S[20] = """
WWWWWWWWWWWWWWW
W_____LLLL__SSW
W_____________W
W_ttt_ttt_ttt_W
W_____________W
W_ttt_ttt_ttt_W
W_____________W
W_____________W
WWWWWWWDWWWWWWW
WWWWWWWWWWWWWWW
"""
S[21] = """
WWWWWWWWWWWWWWW
Wxx_xx___xx_xxW
W_____________W
W_____________W
Wxx_________xxW
Wxx_________xxW
W_____________W
W_____________W
WWWWWWWDWWWWWWW
WWWWWWWWWWWWWWW
"""
S[22] = """
WWWWWWWWWWWWWWW
WLLLL______bbbW
W__________bbbW
W_____________W
Wbb_bb___bb_bbW
W_____________W
W_____________W
W_____________W
WWWWWWWDWWWWWWW
WWWWWWWWWWWWWWW
"""
S[23] = """
WWWWWWWWWWWWWWW
W+++++++++++$+W
W++ddd++++++++W
W+++++++++++++W
W+++++++++++++W
W+++++++++++++W
W+LL+++++++LL+W
W+++++++++++++W
WWWWWWWDWWWWWWW
WWWWWWWWWWWWWWW
"""

# ---------------------------------------------------------------- row 4 --
S[24] = """
###############
###############
###############
#:U::::::::::::
#::::::::::::::
#:::::::::#####
####:::::######
###############
###############
###############
"""
S[25] = """
###############
###############
###############
:::::::::######
:::::::::::::::
######:::::::::
###############
###############
###############
###############
"""
S[26] = """
###############
###############
###############
###::::%:::::::
:::::::%:::::::
:::::::%:::::##
###############
###############
###############
###############
"""
S[27] = """
###############
###############
###############
:::::::########
:::::::::::::::
##:::::::::::::
###############
###############
###############
###############
"""
S[28] = """
###############
###############
###############
###########:U:#
::::::::::::::#
:::::::::::::##
###############
###############
###############
###############
"""
S[29] = """
WWWWWWWWWWWWWWW
WWWWWWWWWWWWWWW
WWWWW+++++WWWWW
WWWWW+++++WWWWW
WWWWW++b++WWWWW
WWWWW+++++WWWWW
WWWWW+++++WWWWW
WWWWWWWCWWWWWWW
WWWWWWWWWWWWWWW
WWWWWWWWWWWWWWW
"""

# Enemies: per screen, list of (type, x1, y1, x2, y2, speed) in tiles.
# 1 = guard, 2 = dog, 3 = officer, 4 = guard with torch.
ENEMIES = {
	6: [(2, 1, 8, 13, 8, 2)],
	7: [(1, 0, 7, 14, 7, 1)],
	8: [(1, 13, 1, 13, 8, 1)],
	9: [(1, 1, 7, 13, 7, 1)],
	10: [(4, 2, 8, 12, 8, 1)],
	11: [(2, 2, 1, 2, 8, 2)],
	12: [(1, 8, 1, 8, 8, 1)],
	13: [(3, 2, 4, 12, 4, 1)],
	14: [(1, 1, 8, 13, 8, 1)],
	15: [(1, 0, 7, 14, 7, 2), (4, 12, 1, 12, 6, 1)],
	16: [(3, 1, 7, 13, 7, 1), (1, 1, 1, 1, 8, 1)],
	17: [(1, 12, 1, 12, 8, 1), (2, 2, 6, 10, 6, 2)],
	20: [(1, 1, 6, 13, 6, 1)],
	22: [(1, 2, 6, 12, 6, 1)],
	23: [(3, 2, 5, 10, 5, 1)],
}

# Tile behaviours (comportamiento_tiles): 0 walk, 8 obstacle, 128 special.
BEHS = [
	0, 0, 8, 8, 8, 8, 8, 128, 0, 8, 8, 8, 8, 8, 8, 8,
	0, 8, 8, 128, 128, 8, 8, 8, 0, 0, 0, 128, 8, 8, 0, 8,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 128, 8,
]

MAP_W, MAP_H, SW, SH = 6, 5, 15, 10


def screen_rows(n):
	rows = [r for r in S[n].strip('\n').split('\n')]
	assert len(rows) == SH, (n, len(rows))
	for r in rows:
		assert len(r) == SW, (n, r, len(r))
	return [[LEGEND[c] for c in r] for r in rows]


def main():
	grid = [[0] * (MAP_W * SW) for _ in range(MAP_H * SH)]
	for n in range(MAP_W * MAP_H):
		rows = screen_rows(n)
		ox, oy = (n % MAP_W) * SW, (n // MAP_W) * SH
		for y in range(SH):
			for x in range(SW):
				grid[oy + y][ox + x] = rows[y][x]
	screens = []
	for n in range(MAP_W * MAP_H):
		ens = []
		for e in ENEMIES.get(n, []):
			t, x1, y1, x2, y2, sp = e
			ens.append([t, x1, y1, x2, y2, sp, 0, 0])
		while len(ens) < 3:
			ens.append([0, 0, 0, 0, 0, 0, 0, 0])
		screens.append({'hotspot': [0, 0, 0], 'enemies': ens})
	png = open(os.path.join(ROOT, 'gfx', 'speccy', 'work.png'), 'rb').read()
	proj = {
		'aportador': 1,
		'tile': {'w': 16, 'h': 16},
		'screen': {'w': SW, 'h': SH},
		'map': {'w': MAP_W, 'h': MAP_H, 'rows': grid},
		'enemies_per_screen': 3,
		'behaviours': BEHS + [0] * (256 - len(BEHS)),
		'screens': screens,
		'tilesets': [{'name': 'ZX', 'double_width': False, 'source': '../gfx/speccy/work.png',
			'png': base64.b64encode(png).decode('ascii')}],
		'current_tileset': 0,
		'exports': {'map': '../map/mapa.map', 'ene': '../enems/enems.ene', 'behs': '../map/behs.txt',
			'hotspot_bytes': 2},
	}
	# Same layout Aportador writes: one map row per line.
	out = json.dumps(proj, indent='\t')
	lines = []
	for l in out.split('\n'):
		lines.append(l)
	txt = '\n'.join(lines)
	# compact the numeric arrays (rows, enemies, behaviours) onto single lines
	import re
	txt = re.sub(r'\[\s+((?:-?\d+,\s*)*-?\d+)\s+\]', lambda m: '[' + ', '.join(x.strip() for x in m.group(1).split(',')) + ']', txt)
	os.makedirs(os.path.join(ROOT, 'apor'), exist_ok=True)
	open(os.path.join(ROOT, 'apor', 'greatesc.apor'), 'w').write(txt + '\n')
	print('apor/greatesc.apor written')


if __name__ == '__main__':
	main()
