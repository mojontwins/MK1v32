#!/usr/bin/env python3
# ZX0 (v2 / "standard") compressor in Python, compatible with the
# dzx0_standard decoder in dev/zx0.h.
#
#   zx0.py input.bin output.bin
#
# It uses a greedy/lazy parser instead of zx0's optimal parser, so the
# output is a few % bigger than zx0.exe's, but it is a valid ZX0 stream.
# Every run decodes the result again with a bit-exact model of the Z80
# decoder and aborts if it does not round-trip.

import sys

MAX_OFFSET = 32640
MIN_MATCH = 2


class BitWriter:
	def __init__(self):
		self.out = bytearray()
		self.bit_mask = 0
		self.bit_index = 0
		self.backtrack = False

	def byte(self, v):
		self.out.append(v & 255)

	def bit(self, v):
		if self.backtrack:
			if v:
				self.out[-1] |= 1
			self.backtrack = False
		else:
			if not self.bit_mask:
				self.bit_mask = 128
				self.bit_index = len(self.out)
				self.byte(0)
			if v:
				self.out[self.bit_index] |= self.bit_mask
			self.bit_mask >>= 1

	def elias(self, value, invert=False):
		# interlaced Elias gamma, backwards_mode = False
		i = 2
		while i <= value:
			i <<= 1
		i >>= 1
		i >>= 1
		while i:
			self.bit(0)
			b = 1 if (value & i) else 0
			self.bit((1 - b) if invert else b)
			i >>= 1
		self.bit(1)


class MatchFinder:
	# Hash chains on byte pairs; positions are inserted as the parser
	# walks past them, and matches are only searched where needed.
	def __init__(self, data):
		self.data = data
		self.n = len(data)
		self.heads = {}
		self.chain = [-1] * self.n
		self.inserted = 0

	def insert_upto(self, p):
		d = self.data
		while self.inserted < p:
			q = self.inserted
			if q + 1 < self.n:
				key = d[q] | (d[q + 1] << 8)
				self.chain[q] = self.heads.get(key, -1)
				self.heads[key] = q
			self.inserted += 1

	def longest(self, p):
		self.insert_upto(p)
		d, n = self.data, self.n
		if p + 1 >= n:
			return 0, 0
		q = self.heads.get(d[p] | (d[p + 1] << 8), -1)
		bo, bl = 0, 0
		tries = 0
		maxl = min(n - p, 4096)
		while q >= 0 and p - q <= MAX_OFFSET and tries < 64:
			if bl < maxl and d[q + bl] == d[p + bl]:
				l = 2
				while l < maxl and d[q + l] == d[p + l]:
					l += 1
				if l > bl:
					bo, bl = p - q, l
					if l == maxl:
						break
			q = self.chain[q]
			tries += 1
		return bo, bl


def compress(data):
	n = len(data)
	mf = MatchFinder(data)
	# Build a list of tokens: ('L', start, length) or ('M', offset, length)
	tokens = []
	p = 0
	lit_start = 0
	last_offset = 1
	while p < n:
		off, ln = mf.longest(p)
		# Repeat-offset match is only legal right after literals.
		rep_len = 0
		if p > lit_start:
			q = p - last_offset
			if q >= 0:
				while p + rep_len < n and rep_len < 4096 and data[q + rep_len] == data[p + rep_len]:
					rep_len += 1
		if p == 0:
			ln = 0; rep_len = 0		# first token must be a literal
		if rep_len >= 1 and rep_len + 1 >= ln:
			if p > lit_start:
				tokens.append(('L', lit_start, p - lit_start))
			tokens.append(('M', last_offset, rep_len))
			p += rep_len
			lit_start = p
		elif ln >= MIN_MATCH and not (ln == 2 and off > 256):
			if p > lit_start:
				tokens.append(('L', lit_start, p - lit_start))
			tokens.append(('M', off, ln))
			last_offset = off
			p += ln
			lit_start = p
		else:
			p += 1
	if p > lit_start:
		tokens.append(('L', lit_start, p - lit_start))

	w = BitWriter()
	last_offset = 1
	prev = None
	for t in tokens:
		if t[0] == 'L':
			if prev is not None:
				w.bit(0)
			w.elias(t[2])
			for i in range(t[2]):
				w.byte(data[t[1] + i])
		else:
			off, ln = t[1], t[2]
			if off == last_offset and prev == 'L':
				w.bit(0)
				w.elias(ln)
			else:
				w.bit(1)
				w.elias((off - 1) // 128 + 1, invert=True)
				w.byte((127 - (off - 1) % 128) << 1)
				w.backtrack = True
				w.elias(ln - 1)
				last_offset = off
		prev = t[0]
	w.bit(1)
	w.elias(256, invert=True)
	return bytes(w.out)


def decompress(src):
	# Bit-exact model of dzx0_standard (dev/zx0.h).
	src = list(src)
	si = 0
	out = bytearray()
	st = {'a': 0x80}

	def nextbit():
		nonlocal si
		a = st['a']
		carry = (a >> 7) & 1
		a = (a << 1) & 255
		if a == 0:
			v = src[si]; si += 1
			c2 = (v >> 7) & 1
			a = ((v << 1) & 255) | carry
			carry = c2
		st['a'] = a
		return carry

	def elias_from(c, b=0):
		# dzx0s_elias_loop with BC preloaded
		while True:
			if nextbit():
				return (b << 8) | c
			bit = nextbit()
			v = ((b << 8) | c) << 1 | bit
			b, c = (v >> 8) & 255, v & 255

	offset = 1
	state = 'lit'
	while True:
		if state == 'lit':
			ln = elias_from(1)
			for i in range(ln):
				out.append(src[si]); si += 1
			state = 'new' if nextbit() else 'rep'
		if state == 'rep':
			ln = elias_from(1)
			for i in range(ln):
				out.append(out[-offset])
			state = 'new' if nextbit() else 'lit'
			continue
		if state == 'new':
			c = elias_from(0xfe) & 255
			c = (c + 1) & 255
			if c == 0:
				return bytes(out)
			b = c
			c = src[si]; si += 1
			# rr b; rr c
			carry = b & 1
			b = (b >> 1) | 0x80		# carry from the elias exit is 1
			c2 = c & 1
			c = (c >> 1) | (carry << 7)
			bc = (b << 8) | c
			offset = (0x10000 - bc) & 0xffff
			if c2:
				ln = 1
			else:
				# elias_backtrack with BC = 1
				ln = 1
				bit = nextbit()
				ln = (ln << 1) | bit
				while not nextbit():
					ln = (ln << 1) | nextbit()
			ln += 1
			for i in range(ln):
				out.append(out[-offset])
			state = 'new' if nextbit() else 'lit'


if __name__ == '__main__':
	data = open(sys.argv[1], 'rb').read()
	c = compress(data)
	d = decompress(c)
	if d != data:
		sys.exit('zx0.py: round-trip FAILED for ' + sys.argv[1])
	open(sys.argv[2], 'wb').write(c)
	print('zx0: %s %d -> %d bytes' % (sys.argv[1], len(data), len(c)))
