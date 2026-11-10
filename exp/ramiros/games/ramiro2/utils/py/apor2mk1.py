#!/usr/bin/env python3
# Exports an Aportador project (.apor) to the files the MK1 build uses,
# exactly like Aportador's "Exportar" button:
#
#   apor2mk1.py project.apor [preview.png]
#
#   - the Mappy-style .map  (one byte per tile, row by row, whole map)
#   - the ponedor .ene      (enemies + hotspots, 2 or 3 byte hotspots)
#   - behs.txt              (comma separated tile behaviours)
#
# The destinations are the "exports" paths stored in the project, relative
# to the .apor file. With a second argument it also renders the whole map
# (with enemy paths) to a PNG, handy for checking edits.

import base64
import io
import json
import os
import sys


def main():
	fn = sys.argv[1]
	base = os.path.dirname(os.path.abspath(fn))
	p = json.load(open(fn))
	m = p['map']
	sw, sh = p['screen']['w'], p['screen']['h']
	mw, mh = m['w'], m['h']
	rows = m['rows']
	ex = p.get('exports', {})
	rel = lambda f: os.path.normpath(os.path.join(base, f))

	# .map
	data = bytearray()
	for y in range(mh * sh):
		for x in range(mw * sw):
			data.append(rows[y][x] & 255)
	open(rel(ex['map']), 'wb').write(data)

	# .ene
	nE = p.get('enemies_per_screen', 3)
	hb = ex.get('hotspot_bytes', 3)
	ene_fn = rel(ex['ene'])
	mapref = os.path.relpath(rel(ex['map']), os.path.dirname(ene_fn)).replace('/', '\\')
	tsref = ''
	if p.get('tilesets'):
		ts = p['tilesets'][p.get('current_tileset', 0)]
		tsref = os.path.relpath(rel(ts.get('source', '')), os.path.dirname(ene_fn)).replace('/', '\\')
	out = bytearray()
	out += mapref.encode('latin-1')[:127].ljust(127, b'\0')
	out += b'\0'
	out += tsref.encode('latin-1')[:128].ljust(128, b'\0')
	out += bytes([mw, mh, sw, sh, nE])
	scr = p['screens']
	for s in range(mw * mh):
		ens = scr[s]['enemies'] if s < len(scr) else []
		for i in range(nE):
			e = ens[i] if i < len(ens) else [0] * 8
			out += bytes(v & 255 for v in e[:8])
	for s in range(mw * mh):
		h = scr[s].get('hotspot', [0, 0, 0]) if s < len(scr) else [0, 0, 0]
		x, y, t = h
		if not t:
			x = y = 0
		if hb == 2:
			out += bytes([(x * 16 + y) & 255, t])
		else:
			out += bytes([x, y, t])
	open(ene_fn, 'wb').write(out)

	# behs.txt
	if 'behaviours' in p and ex.get('behs'):
		b = p['behaviours']
		n = 48
		open(rel(ex['behs']), 'w').write(','.join(str(v) for v in b[:max(n, 48)]))

	print('apor2mk1: %s -> %s, %s%s' % (os.path.basename(fn), ex['map'], ex['ene'],
		(', ' + ex['behs']) if ex.get('behs') else ''))

	if len(sys.argv) > 2:
		from PIL import Image, ImageDraw
		ts = p['tilesets'][p.get('current_tileset', 0)]
		tim = Image.open(io.BytesIO(base64.b64decode(ts['png']))).convert('RGB')
		tw, th = p['tile']['w'], p['tile']['h']
		per_row = tim.width // tw
		W, H = mw * sw * tw, mh * sh * th
		im = Image.new('RGB', (W + mw - 1, H + mh - 1), (255, 0, 255))
		for y in range(mh * sh):
			for x in range(mw * sw):
				t = rows[y][x]
				tile = tim.crop(((t % per_row) * tw, (t // per_row) * th, (t % per_row) * tw + tw, (t // per_row) * th + th))
				im.paste(tile, (x * tw + x // sw, y * th + y // sh))
		d = ImageDraw.Draw(im)
		for s in range(mw * mh):
			ox = (s % mw) * (sw * tw + 1)
			oy = (s // mw) * (sh * th + 1)
			d.text((ox + 2, oy + 2), str(s), fill=(255, 0, 255))
			for e in scr[s]['enemies']:
				if e[0]:
					x1, y1, x2, y2 = [v * tw + tw // 2 for v in e[1:5]]
					d.line((ox + x1, oy + y1, ox + x2, oy + y2), fill=(255, 0, 0), width=2)
					d.rectangle((ox + x1 - 3, oy + y1 - 3, ox + x1 + 3, oy + y1 + 3), outline=(255, 0, 0))
					d.text((ox + x1 + 4, oy + y1 - 10), str(e[0]), fill=(255, 0, 0))
		im.save(sys.argv[2])
		print('preview: ' + sys.argv[2])


if __name__ == '__main__':
	main()
