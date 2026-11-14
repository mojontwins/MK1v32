#!/usr/bin/env python3
"""mksna.py game.bin ADDR -pc PC -o game.sna

Make a 64K CPC snapshot (v3 header) with a binary loaded at ADDR, like
cpctbin2sna.exe as MK1's comp.bat uses it: ROMs off, mode 1, standard CRTC,
IM 1, interrupts on, SP = 0xBFFA (values copied from a cpctbin2sna snapshot).
"""
import argparse, struct

ap = argparse.ArgumentParser()
ap.add_argument('bin'); ap.add_argument('addr')
ap.add_argument('-pc', required=True); ap.add_argument('-o', required=True)
ap.add_argument('-sp', default='0xbffa')
a = ap.parse_args()
addr, pc, sp = int(a.addr, 0), int(a.pc, 0), int(a.sp, 0)

h = bytearray(256)
h[0:8] = b'MV - SNA'
h[0x10] = 3
struct.pack_into('<HHHH', h, 0x11, 0xff0c, 0xb0ff, 0x0040, 0xabff)    # AF BC DE HL
h[0x19] = 0; h[0x1a] = 0; h[0x1b] = 1; h[0x1c] = 1                   # R I IFF1 IFF2
struct.pack_into('<HHHH', h, 0x1d, 0, 0, sp, pc)                      # IX IY SP PC
h[0x25] = 1                                                           # IM 1
h[0x2e] = 15
h[0x2f:0x40] = bytes([4, 10, 19, 12, 11, 20, 21, 13, 6, 30, 31, 7, 18, 25, 10, 7, 4])
h[0x40] = 0x8d                                                        # mode 1, ROMs off
h[0x42] = 13
h[0x43:0x55] = bytes([63, 40, 46, 142, 38, 0, 25, 30, 0, 7, 0, 0, 48, 0, 192, 0, 63, 40])
h[0x56:0x5a] = bytes([63, 0, 0, 130])
h[0x5a] = 7; h[0x5b + 7] = 63
struct.pack_into('<H', h, 0x6b, 64)                                   # 64K dump
ram = bytearray(65536)
data = open(a.bin, 'rb').read()
ram[addr:addr + len(data)] = data
open(a.o, 'wb').write(bytes(h) + bytes(ram))
print('mksna.py: %s, %d bytes at %04x, PC %04x' % (a.o, len(data), addr, pc))
