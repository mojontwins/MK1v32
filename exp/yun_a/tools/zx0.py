#!/usr/bin/env python3
"""
zx0.py - ZX0 (v2 format) compressor for the MK1 v3.2 toolchain on Linux.

  python3 zx0.py in.bin out.bin

Greedy, not optimal like Einar Saukas' zx0.exe: output is a little bigger but
decodes with the same dzx0_standard routine (dev/zx0.h). `decompress` below is
a direct model of that routine and is used to self-check every file.
"""
import sys

class BitWriter:
    def __init__(self):
        self.out = bytearray(); self.bitpos = -1; self.mask = 0
    def bit(self, b):
        if self.mask == 0:
            self.out.append(0); self.bitpos = len(self.out) - 1; self.mask = 0x80
        if b: self.out[self.bitpos] |= self.mask
        self.mask >>= 1
    def byte(self, v): self.out.append(v & 0xff)
    def elias(self, n, invert=False, skip_first_ctrl=False):
        """Interlaced Elias gamma: for each bit after the leading 1: 0, bit; then 1."""
        bits = bin(n)[3:]
        first = True
        for b in bits:
            if not (first and skip_first_ctrl): self.bit(0)
            first = False
            v = int(b)
            self.bit(v ^ 1 if invert else v)
        if not (first and skip_first_ctrl): self.bit(1)

def compress(data, window=32640):
    n = len(data)
    if n == 0: raise ValueError('empty input')
    # positions of each 2-byte prefix, for match finding
    heads = {}
    tokens = [['lit', bytearray(data[0:1])]]
    i, last_off = 1, 1
    def match_len(i, off):
        l = 0
        while i + l < n and data[i + l] == data[i + l - off]: l += 1
        return l
    def note(p):
        if p + 1 < n: heads.setdefault(data[p:p + 2], []).append(p)
    note(0)
    while i < n:
        best_len, best_off = 0, 0
        for p in reversed(heads.get(data[i:i + 2], [])):
            off = i - p
            if off > window: break
            l = match_len(i, off)
            if l > best_len: best_len, best_off = l, off
            if l > 256: break
        rep = match_len(i, last_off) if tokens[-1][0] == 'lit' and last_off <= i else 0
        if rep and rep >= best_len - 1:
            tokens.append(['rep', rep]); step = rep
        elif best_len >= 2 and not (best_off > 128 and best_len == 2):
            tokens.append(['copy', best_off, best_len]); last_off = best_off; step = best_len
        else:
            if tokens[-1][0] == 'lit': tokens[-1][1].append(data[i])
            else: tokens.append(['lit', bytearray(data[i:i + 1])])
            step = 1
        for k in range(step): note(i + k)
        i += step

    w = BitWriter()
    prev = None
    for t in tokens:
        if t[0] == 'lit':
            if prev is not None: w.bit(0)          # after a copy: 0 = literals
            w.elias(len(t[1]))
            for b in t[1]: w.byte(b)
        elif t[0] == 'rep':
            w.bit(0)                                # after literals: 0 = last offset
            w.elias(t[1])
        else:
            _, off, ln = t
            w.bit(1)                                # new offset
            w.elias(((off - 1) >> 7) + 1, invert=True)
            lsb = (127 - ((off - 1) & 127)) << 1
            # first control bit of elias(ln - 1) lives in the LSB's bit 0
            rest = bin(ln - 1)[3:]
            w.byte(lsb | (1 if not rest else 0))
            for k, b in enumerate(rest):
                if k: w.bit(0)
                w.bit(int(b))
            if rest: w.bit(1)
        prev = t[0]
    w.bit(1)                                        # new offset ...
    w.elias(256, invert=True)                       # ... = end marker
    return bytes(w.out)

def decompress(src):
    """Bit-exact model of dzx0_standard in dev/zx0.h."""
    src = bytes(src); hl = 0; out = bytearray()
    st = {'a': 0x80, 'hl': 0}
    def getbit():
        a = st['a'] << 1; c = a >> 8; a &= 0xff
        if a == 0:
            byte = src[st['hl']]; st['hl'] += 1
            a = (byte << 1) | c; c = a >> 8; a &= 0xff
        st['a'] = a
        return c
    def elias_loop(bc):
        while True:
            if getbit(): return bc
            bc = ((bc << 1) | getbit()) & 0xffff
    def elias_backtrack(bc):
        bc = ((bc << 1) | getbit()) & 0xffff
        return elias_loop(bc)
    offset = 0xffff
    state = 'lit'
    while True:
        if state == 'lit':
            ln = elias_loop(1)
            for _ in range(ln): out.append(src[st['hl']]); st['hl'] += 1
            if getbit(): state = 'new'
            else:
                ln = elias_loop(1)
                state = 'copy'
        if state == 'new':
            v = elias_loop(0x00fe)
            c = ((v & 0xff) + 1) & 0xff
            if c == 0: return bytes(out)
            b = c; c = src[st['hl']]; st['hl'] += 1
            carry = 1
            nb = (carry << 7) | (b >> 1); carry = b & 1
            nc = (carry << 7) | (c >> 1); carry = c & 1
            offset = (nb << 8) | nc
            ln = 1 if carry else elias_backtrack(1)
            ln += 1
            state = 'copy'
        if state == 'copy':
            d = 0x10000 - offset
            for _ in range(ln): out.append(out[len(out) - d])
            state = 'new' if getbit() else 'lit'

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print('usage: zx0.py in.bin out.bin'); sys.exit(1)
    data = open(sys.argv[1], 'rb').read()
    comp = compress(data)
    assert decompress(comp) == data, 'zx0.py: self-check failed'
    open(sys.argv[2], 'wb').write(comp)
    print('zx0.py: %d -> %d bytes' % (len(data), len(comp)))
