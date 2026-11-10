#!/usr/bin/env python3
# cpc_placeholders.py - makes placeholder CPC (mode 1) graphics for a Ramiro
# game from its ZX graphics, so the CPC version can be built and tried
# while the real CPC art is drawn.
#
#   python3 cpc_placeholders.py game_dir
#
# Reads game_dir/gfx/speccy/*.png, writes game_dir/gfx/cpc/*.png:
#   pal.png          4 pens: 0 black (also "transparent" in sprites),
#                    1 dark blue (black inside sprites), 2 the game's colour
#                    (red or yellow), 3 white.
#   font.png work.png sprites.png title.png ending.png   (mode 1 sizes)
#   loading.png pal_loading.png                          (mode 0, 160x200)
#
# The ZX colours are folded onto those pens: black -> 0, red/yellow -> 2,
# white/grey -> 3. In sprites, the ZX mask (red) -> 0 and black -> 1.

import os
import sys
from PIL import Image

BLACK, BLUE, WHITE = (0, 0, 0), (0, 0, 128), (255, 255, 255)


def classify(c):
	r, g, b = c[:3]
	if r < 60 and g < 60 and b < 60:
		return 'k'
	if r > 150 and g > 150 and b > 150:
		return 'w'
	return 'c'


def fold(im, pal, sprite=False):
	out = Image.new('RGB', im.size)
	src = im.convert('RGB')
	for y in range(im.height):
		for x in range(im.width):
			k = classify(src.getpixel((x, y)))
			out.putpixel((x, y), {'k': pal[0], 'c': pal[2], 'w': pal[3]}[k])
	return out


def main():
	g = sys.argv[1]
	zx = os.path.join(g, 'gfx', 'speccy')
	cpc = os.path.join(g, 'gfx', 'cpc')
	os.makedirs(cpc, exist_ok=True)
	work = Image.open(os.path.join(zx, 'work.png')).convert('RGB')
	# The game's colour: red for Ramiro 1, yellow for Ramiro 2
	reds = yellows = 0
	for y in range(work.height):
		for x in range(work.width):
			r, gg, b = work.getpixel((x, y))
			if r > 150 and b < 60:
				if gg > 150:
					yellows += 1
				elif gg < 60:
					reds += 1
	colour = (255, 255, 0) if yellows > reds else (255, 0, 0)
	pal = [BLACK, BLUE, colour, WHITE]

	p = Image.new('RGB', (16, 1), WHITE)
	for i, c in enumerate(pal):
		p.putpixel((i, 0), c)
	p.save(os.path.join(cpc, 'pal.png'))

	for f in ('font', 'work', 'title', 'ending'):
		fold(Image.open(os.path.join(zx, f + '.png')), pal).save(os.path.join(cpc, f + '.png'))

	# Sprites: ZX sheet is sprite, mask, sprite, mask... (16x16 each, 2 rows)
	s = Image.open(os.path.join(zx, 'sprites.png')).convert('RGB')
	out = Image.new('RGB', (128, 32), BLACK)
	for row in range(2):
		for i in range(8):
			for y in range(16):
				for x in range(16):
					c = s.getpixel((i * 32 + x, row * 16 + y))
					m = s.getpixel((i * 32 + 16 + x, row * 16 + y))
					if m[0] > 150 and m[1] < 60 and m[2] < 60:
						v = BLACK 		# masked out: transparent
					else:
						k = classify(c)
						v = {'k': BLUE, 'c': colour, 'w': WHITE}[k]
					out.putpixel((i * 16 + x, row * 16 + y), v)
	out.save(os.path.join(cpc, 'sprites.png'))

	# Loading screen: mode 0, 160x200, its own 16 colour palette
	lpal = [BLACK, colour, WHITE] + [BLACK] * 13
	lp = Image.new('RGB', (16, 1))
	for i, c in enumerate(lpal):
		lp.putpixel((i, 0), c)
	lp.save(os.path.join(cpc, 'pal_loading.png'))
	ld = Image.open(os.path.join(zx, 'loading.png')).convert('RGB').resize((160, 192), Image.NEAREST)
	l = Image.new('RGB', (160, 200), BLACK)
	l.paste(fold(ld, [BLACK, BLUE, colour, WHITE]), (0, 4))
	l.save(os.path.join(cpc, 'loading.png'))
	print('%s: CPC placeholders written (colour %s)' % (cpc, colour))


if __name__ == '__main__':
	main()
