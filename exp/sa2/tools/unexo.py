#!/usr/bin/env python3
# unexo.py - exomizer 2 raw (forward) decruncher, as cpc_UnExo / deexo.asm.
# usage: unexo.py in.exo out.bin
import sys

def unexo(src):
	pos = 0
	bitbuf = 0x80

	def getbit():
		nonlocal bitbuf, pos
		bitbuf <<= 1
		if bitbuf & 0xff == 0:
			bitbuf = (src[pos] << 1) | 1
			pos += 1
		r = (bitbuf >> 8) & 1
		bitbuf &= 0xff
		return r

	def getbits(n):
		v = 0
		for _ in range(n):
			v = (v << 1) | getbit()
		return v

	bits = []; base = []
	b = 0
	for i in range(52):
		if i % 16 == 0: b = 1
		n = getbits(4)
		bits.append(n); base.append(b)
		b += 1 << n

	out = bytearray()
	out.append(src[pos]); pos += 1		# deexo: first byte is always a literal
	while True:
		if getbit():
			out.append(src[pos]); pos += 1
			continue
		i = 0
		while not getbit():
			i += 1
		if i == 16:
			break
		if i == 17:
			n = getbits(16)
			out += src[pos:pos + n]; pos += n
			continue
		ln = base[i] + getbits(bits[i])
		if ln == 1:   j = 48 + getbits(2)
		elif ln == 2: j = 32 + getbits(4)
		else:         j = 16 + getbits(4)
		off = base[j] + getbits(bits[j])
		for _ in range(ln):
			out.append(out[-off])
	return bytes(out)

if __name__ == '__main__':
	d = unexo(open(sys.argv[1], 'rb').read())
	open(sys.argv[2], 'wb').write(d)
	print(len(d))
