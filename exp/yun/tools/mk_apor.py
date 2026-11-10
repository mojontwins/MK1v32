#!/usr/bin/env python3
# mk_apor.py - Yun: one Aportador project (.apor) per level, made once from
# the converted levels: map/levelN.map, enems/levelN.ene, the level's
# tileset and its tile behaviours (dev/yun_load.h).
#
#   python3 tools/mk_apor.py           (from the project root)
#
# Writes:
#   map/levelN.apor              the project: exports to levelN.map and
#                                ../enems/levelN.ene, behaviours to levelN_behs.txt
#   gfx/speccy/apor/levelN.png   tsN.png + tsx.png below it (the hotspots are
#   gfx/cpc/apor/levelN.png      drawn with tile 16 + type): the project's
#                                tilesets "ZX" and "CPC" (double width) come
#                                from these, so "Reimportar" works. Run the
#                                script with --tilesets after changing a
#                                tsN.png or tsx.png, then reimport.
#
# It checks every project the way Aportador reads it and exports it: the
# .map comes out identical and the .ene too, but for its 256-byte header
# (paths) and empty hotspots with x, y set (Aportador writes them as 0).
#
# One-shot: once the levels are edited in Aportador, the .apor files are the
# originals and this script must not be run again (it would overwrite them
# with the exported .map / .ene, which is the same thing, but loses nothing
# only if they were exported). It refuses to overwrite an .apor unless
# --force.

import base64, io, json, os, re, sys
from PIL import Image

ROOT = os.path.dirname (os.path.dirname (os.path.abspath (__file__)))

# level: map width, height (screens), as in dev/comp.sh
LEVELS = { 0: (4, 6), 1: (4, 6), 2: (4, 6), 3: (4, 6), 4: (12, 2), 5: (6, 4), 6: (6, 4), 7: (9, 3) }

def p (*a): return os.path.join (ROOT, *a)

# ---- Aportador's .apor ------------------------------------------------------

def write_apor (path, mapw, maph, mapdata, ene, tilesets, behs, exports, cur_ts = 0):
	"""tilesets: [(name, double_width, source (relative to the .apor), png bytes)]"""
	sw, sh, eps = ene [258], ene [259], ene [260]
	assert (ene [256], ene [257]) == (mapw, maph), (path, list (ene [256:261]))
	assert (sw, sh) == (15, 10)
	ns = mapw * maph
	assert len (mapdata) == ns * 150, (path, len (mapdata))
	wt = mapw * sw
	# enemies and hotspots: by screen, row by row (as the .ene)
	screens = []
	base = 261 + ns * eps * 8
	assert len (ene) == base + ns * 3, (path, 'hotspots of 3 bytes', len (ene))
	for s in range (ns):
		en = [list (ene [261 + (s * eps + i) * 8 : 261 + (s * eps + i + 1) * 8]) for i in range (eps)]
		h = list (ene [base + s * 3 : base + s * 3 + 3])
		if h [2] == 0: h = [0, 0, 0]
		screens.append ({ 'hotspot': h, 'enemies': en })
	rows = [list (mapdata [y * wt : (y + 1) * wt]) for y in range (maph * sh)]
	j = [
		'{',
		'  "aportador": 1,',
		'  "tile": {"w": 16, "h": 16},',
		'  "screen": {"w": %d, "h": %d},' % (sw, sh),
		'  "map": {"w": %d, "h": %d, "rows": [' % (mapw, maph),
		',\n'.join ('    ' + json.dumps (r, separators = (', ', ': ')) for r in rows),
		'  ]},',
		'  "enemies_per_screen": %d,' % eps,
		'  "behaviours": %s,' % json.dumps (behs + [0] * (256 - len (behs))),
		'  "screens": [',
		',\n'.join ('    ' + json.dumps (s, separators = (', ', ': ')) for s in screens),
		'  ],',
		'  "tilesets": [',
		',\n'.join ('    ' + json.dumps ({ 'name': n, 'double_width': d, 'source': src,
			'png': base64.b64encode (png).decode () }) for n, d, src, png in tilesets),
		'  ],',
		'  "current_tileset": %d,' % cur_ts,
		'  "exports": %s' % json.dumps (exports),
		'}',
	]
	open (path, 'w', newline = '\n').write ('\n'.join (j) + '\n')

def check_apor (path, mapdata, ene):
	"""As Project::load + writeMap + writeEne (project.cpp)."""
	j = json.load (open (path))
	assert j ['aportador'] == 1
	mw, mh = j ['map'] ['w'], j ['map'] ['h']
	sw, sh = j ['screen'] ['w'], j ['screen'] ['h']
	m = bytes (v for r in j ['map'] ['rows'] for v in r)
	assert len (j ['map'] ['rows']) == mh * sh and all (len (r) == mw * sw for r in j ['map'] ['rows'])
	assert m == mapdata, path + ': the map'
	out = bytearray ([mw, mh, sw, sh, j ['enemies_per_screen']])
	for s in j ['screens']:
		for e in s ['enemies']: out += bytes (e)
	for s in j ['screens']:
		x, y, t = s ['hotspot']
		out += bytes ([x if t else 0, y if t else 0, t])
	# the original, with empty hotspots at 0, 0 as Aportador writes them
	orig = bytearray (ene [256:])
	ns = mw * mh; base = 5 + ns * j ['enemies_per_screen'] * 8
	for s in range (ns):
		if orig [base + s * 3 + 2] == 0: orig [base + s * 3] = orig [base + s * 3 + 1] = 0
	assert bytes (out) == bytes (orig), path + ': the enemies / hotspots'
	for ts in j ['tilesets']:
		im = Image.open (io.BytesIO (base64.b64decode (ts ['png'])))
		tw = 8 if ts ['double_width'] else 16
		n = (im.width // tw) * (im.height // 16)
		assert max (m) < n, path + ': tiles beyond the tileset ' + ts ['name']
	return sum (1 for s in j ['screens'] for e in s ['enemies'] if e [0]), sum (1 for s in j ['screens'] if s ['hotspot'] [2])

def png_bytes (im):
	b = io.BytesIO (); im.save (b, 'PNG'); return b.getvalue ()

# ---- Yun ----------------------------------------------------------------------

def stack (a, b):
	im = Image.new ('RGB', (max (a.width, b.width), a.height + b.height))
	im.paste (a.convert ('RGB'), (0, 0)); im.paste (b.convert ('RGB'), (0, a.height))
	return im

def yun_behs ():
	t = open (p ('dev', 'yun_load.h')).read ()
	tabs = re.findall (r'unsigned char lvl_behs \[\] = \{([^}]*)\}', t)
	assert len (tabs) == 2
	b = {}
	for load, tab in enumerate (tabs):
		v = [int (x) for x in re.findall (r'\d+', tab)]
		for i in range (4): b [load * 4 + i] = v [i * 16 : i * 16 + 16]
	return b

def main ():
	force = '--force' in sys.argv
	only_ts = '--tilesets' in sys.argv
	behs = yun_behs ()
	for d in ('speccy', 'cpc'): os.makedirs (p ('gfx', d, 'apor'), exist_ok = True)
	for n, (mw, mh) in LEVELS.items ():
		ts = {}
		for d, name, dw in (('speccy', 'ZX', False), ('cpc', 'CPC', True)):
			im = stack (Image.open (p ('gfx', d, 'ts%d.png' % n)), Image.open (p ('gfx', d, 'tsx.png')))
			im.save (p ('gfx', d, 'apor', 'level%d.png' % n))
			ts [name] = (name, dw, '../gfx/%s/apor/level%d.png' % (d, n), png_bytes (im))
		if only_ts: continue
		apor = p ('map', 'level%d.apor' % n)
		if os.path.exists (apor) and not force:
			print ('%s exists: not touched (--force to overwrite)' % apor); continue
		mapdata = open (p ('map', 'level%d.map' % n), 'rb').read ()
		ene = open (p ('enems', 'level%d.ene' % n), 'rb').read ()
		# hotspot tiles 16-31 (tsx) behave as nothing
		write_apor (apor, mw, mh, mapdata, ene, [ts ['ZX'], ts ['CPC']], behs [n] + [0] * 16,
			{ 'map': 'level%d.map' % n, 'ene': '../enems/level%d.ene' % n, 'behs': 'level%d_behs.txt' % n, 'hotspot_bytes': 3 })
		e, h = check_apor (apor, mapdata, ene)
		print ('%s: %dx%d screens, %d enemies, %d hotspots: OK' % (apor, mw, mh, e, h))

if __name__ == '__main__':
	main ()
