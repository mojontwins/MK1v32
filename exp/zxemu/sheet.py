#!/usr/bin/env python3
"""sheet.py out.png cols frame.ppm... -> contact sheet (2x), frame names as labels."""
import sys, os
from PIL import Image, ImageDraw
out, cols, files = sys.argv[1], int(sys.argv[2]), sys.argv[3:]
crop = os.environ.get('CROP')          # x0,y0,x1,y1 in Spectrum pixels
ims = []
for f in files:
    im = Image.open(f).convert('RGB')
    if crop: im = im.crop(tuple(int(v) for v in crop.split(',')))
    ims.append((os.path.basename(f).split('.')[0], im))
w, h = ims[0][1].size
rows = (len(ims) + cols - 1) // cols
sheet = Image.new('RGB', (cols * (w + 4), rows * (h + 14)), (70, 70, 70))
d = ImageDraw.Draw(sheet)
for i, (n, im) in enumerate(ims):
    x, y = (i % cols) * (w + 4), (i // cols) * (h + 14)
    sheet.paste(im, (x, y + 12)); d.text((x + 2, y), n, fill=(255, 255, 0))
sheet = sheet.resize((sheet.width * 2, sheet.height * 2), Image.NEAREST)
sheet.save(out)
