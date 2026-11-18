#!/usr/bin/env python3
"""
gentape.py - .tap builder, a subset of GenTape.exe's command line (Linux),
so comp.sh and comp.bat build the same tape.

  python3 gentape.py out.tap BLOCK...

  basic 'name' line file      BASIC program (tokenised, as GenTape), autostart line
  hdata 'name' address file   CODE with its header (address in hex, as GenTape)
  data file                   headerless block (flag 255)
"""
import struct, sys

def block (flag, data):
	chk = flag
	for b in data: chk ^= b
	return struct.pack ('<H', len (data) + 2) + bytes ([flag]) + data + bytes ([chk])

def header (kind, name, length, p1, p2):
	return block (0, bytes ([kind]) + name.ljust (10) [:10].encode () + struct.pack ('<HHH', length, p1, p2))

def main ():
	a = sys.argv [1:]
	if len (a) < 2: print (__doc__); sys.exit (1)
	out, a, tap = a [0], a [1:], b''
	while a:
		k = a.pop (0)
		if k == 'basic':
			name, line, f = a.pop (0).strip ("'"), int (a.pop (0)), a.pop (0)
			d = open (f, 'rb').read ()
			tap += header (0, name, len (d), line, len (d)) + block (0xff, d)
		elif k == 'hdata':
			name, addr, f = a.pop (0).strip ("'"), int (a.pop (0), 16), a.pop (0)
			d = open (f, 'rb').read ()
			tap += header (3, name, len (d), addr, 32768) + block (0xff, d)
		elif k == 'data':
			tap += block (0xff, open (a.pop (0), 'rb').read ())
		else:
			print ('gentape.py: what is %s?' % k); sys.exit (1)
	open (out, 'wb').write (tap)

if __name__ == '__main__':
	main ()
