#!/usr/bin/env python3
# cpctools.py - small CPC build helpers for Linux.
#
#   cpctools.py defb2bin in.asm out.bin
#       Assembles a file made only of "defb" lines (like the sprite LUTs in
#       dev/system/cpc_TrPixLutM*.asm), instead of pasmo.
#
#   cpctools.py bin2sna game.bin 0x400 game.sna [pc]
#       Writes a 64K CPC snapshot (SNA v2) with the binary loaded at the
#       given address, ROMs off, mode 1, PC = load address (or pc), like
#       cpctbin2sna does for testing in emulators. The game initialises the
#       hardware itself (MK1's CPC code doesn't use the firmware).

import re
import sys


def num(s):
	s = s.strip()
	if s.startswith('$'):
		return int(s[1:], 16)
	if s.lower().startswith('0x'):
		return int(s, 16)
	if s.lower().endswith('h'):
		return int(s[:-1], 16)
	return int(s)


def defb2bin(fin, fout):
	out = bytearray()
	for l in open(fin):
		l = l.split(';')[0].strip()
		m = re.match(r'(?i)defb\s+(.*)', l)
		if m:
			out += bytes(num(v) & 255 for v in m.group(1).split(','))
	open(fout, 'wb').write(out)
	print('defb2bin: %s -> %s (%d bytes)' % (fin, fout, len(out)))


def bin2sna(fin, addr, fout, pc=None):
	data = open(fin, 'rb').read()
	addr = num(addr)
	pc = addr if pc is None else num(pc)
	h = bytearray(256)
	h[0:8] = b'MV - SNA'
	h[0x10] = 2 							# version
	h[0x21], h[0x22] = 0xff, 0xbf 			# SP
	h[0x23], h[0x24] = pc & 255, pc >> 8 	# PC
	h[0x25] = 1 							# IM 1
	h[0x2e] = 0 							# GA selected pen
	pal = [0x14] * 17 						# all black
	h[0x2f:0x40] = bytes(pal)
	h[0x40] = 0x8d 							# mode 1, both ROMs off
	h[0x41] = 0 							# RAM config
	crtc = [63, 40, 46, 0x8e, 38, 0, 25, 30, 0, 7, 0, 0, 0x30, 0, 0, 0, 0, 0]
	h[0x42] = 0
	h[0x43:0x55] = bytes(crtc)
	h[0x55] = 0
	h[0x56:0x5a] = bytes([0, 0, 0, 0x82]) 	# PPI
	h[0x6b], h[0x6c] = 64, 0 				# 64K dump
	h[0x6d] = 2 							# CPC 6128 (v2 field)
	mem = bytearray(65536)
	mem[addr:addr + len(data)] = data
	open(fout, 'wb').write(bytes(h) + bytes(mem))
	print('bin2sna: %s @ 0x%04X -> %s' % (fin, addr, fout))


if __name__ == '__main__':
	if sys.argv[1] == 'defb2bin':
		defb2bin(sys.argv[2], sys.argv[3])
	elif sys.argv[1] == 'bin2sna':
		bin2sna(sys.argv[2], sys.argv[3], sys.argv[4], sys.argv[5] if len(sys.argv) > 5 else None)
	else:
		sys.exit('Unknown command')
