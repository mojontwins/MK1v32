#!/usr/bin/env python3
"""
mktap.py - .tap builder for the MK1 v3.2 toolchain on Linux
(replaces bas2tap.exe + bin2tap.exe for the one-line MK1 loader).

  python3 mktap.py out.tap NAME loader.bas screen.bin game.bin ORG

Tokenises loader.bas (only the keywords MK1 loaders use), autostart line 10,
then appends the SCREEN$ block (16384) and the CODE block at ORG.
"""
import re, struct, sys

KEYWORDS = {  # ZX Spectrum 48K tokens
    'RANDOMIZE': 0xF9, 'BORDER': 0xE7, 'SCREEN$': 0xAA, 'PAPER': 0xDA, 'CLEAR': 0xFD,
    'POKE': 0xF4, 'CODE': 0xAF, 'LOAD': 0xEF, 'NOT': 0xC3, 'INK': 0xD9, 'VAL': 0xB0,
    'CLS': 0xFB, 'USR': 0xC0, 'PI': 0xA7, 'BRIGHT': 0xDC, 'PRINT': 0xF5, 'AT': 0xAC,
}
ORDER = sorted(KEYWORDS, key=len, reverse=True)

def tokenise_line(text):
    out, i, instr = bytearray(), 0, False
    while i < len(text):
        ch = text[i]
        if ch == '"':
            instr = not instr; out.append(ord(ch)); i += 1; continue
        if not instr:
            if ch == ' ': i += 1; continue
            for k in ORDER:
                if text.upper().startswith(k, i):
                    out.append(KEYWORDS[k]); i += len(k); break
            else:
                out.append(ord(ch)); i += 1
        else:
            out.append(ord(ch)); i += 1
    return bytes(out)

def basic(src):
    prog = bytearray()
    for line in src.splitlines():
        m = re.match(r'\s*(\d+)\s*(.*)', line)
        if not m: continue
        body = tokenise_line(m.group(2)) + b'\x0d'
        prog += struct.pack('>H', int(m.group(1))) + struct.pack('<H', len(body)) + body
    return bytes(prog)

def block(flag, data):
    chk = flag
    for b in data: chk ^= b
    return struct.pack('<H', len(data) + 2) + bytes([flag]) + data + bytes([chk])

def header(kind, name, length, p1, p2):
    return block(0, bytes([kind]) + name.ljust(10)[:10].encode() + struct.pack('<HHH', length, p1, p2))

def main():
    if len(sys.argv) != 7:
        print(__doc__); sys.exit(1)
    out, name, bas, scr, game, org = sys.argv[1:]
    org = int(org, 0)
    prog = basic(open(bas).read())
    tap = header(0, name, len(prog), 10, len(prog)) + block(0xff, prog)
    s = open(scr, 'rb').read()
    tap += header(3, 'loading', len(s), 16384, 32768) + block(0xff, s)
    g = open(game, 'rb').read()
    tap += header(3, name, len(g), org, 32768) + block(0xff, g)
    open(out, 'wb').write(tap)
    print('mktap.py: %s, %d bytes of code at %d' % (out, len(g), org))

if __name__ == '__main__':
    main()
