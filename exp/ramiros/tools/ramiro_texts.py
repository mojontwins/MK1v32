#!/usr/bin/env python3
# ramiro_texts.py - converts a 2016 Ramiro texts.txt to msc4 TEXT BOX strings
#
#   python3 ramiro_texts.py texts.txt
#
# Prints, per line n (EXTERN n in the 2016 script), the TEXT BOX string to
# use, and the speakers table for extern.h.
#   lines 1-30: room names -> "0NAME"
#   others "SPEAKER%TEXT" -> "kTEXT", k = 1-9 speaker index
# Font slots: ] = ¡, [ = ¿, \ = Ñ; msc4 can't encode those, so they are
# written as < > = (addons/ramiro.h translates them back).
import sys

def conv(t):
	return t.replace(']', '<').replace('[', '>').replace('\\', '=')

lines = open(sys.argv[1], encoding='latin-1').read().splitlines()
speakers = []
for n, l in enumerate(lines, 1):
	l = l.strip()
	if not l:
		continue
	if n <= 30:
		print('%d\t"0%s"' % (n, conv(l)))
	else:
		sp, tx = l.split('%', 1)
		if sp not in speakers:
			speakers.append(sp)
		print('%d\t"%d%s"' % (n, speakers.index(sp) + 1, conv(tx)))
print('SPEAKERS', speakers)
