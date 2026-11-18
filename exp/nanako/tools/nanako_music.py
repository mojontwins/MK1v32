# nanako_music.py - one-shot conversion of the original Nanako songs and sound
# effects (CPC, 2009, cpcwyzlib's old WYZ player) to MK1 v3.2's WYZ player,
# for the CPC and for the ZX 128K (RAM1, MODE_128K_DUAL).
#
# usage: python3 nanako_music.py        (paths are relative to this script's folder)
#
# Reads ../orig/mus/wyz/*.mus, ../orig/dev/sound.h (instruments, drums,
# effects), ../sfx/sfx.txt (the effect numbers) and writes:
#   ../ogt/nanako_*.mus          the songs in MK1's format (comp_cpc packs them)
#   ../dev/wyz/instrumentos.h    CPC: instruments (PAUTAs) and drums (SONIDOs)
#   ../dev/wyz/efectos.h         CPC: EFECTO0-9
#   ../dev/wyz/efectos_ids.h     SFX_* (the same numbers as sfx/sfx.txt)
#   ../dev/wyz/songs.h           CPC: the song table
#   ../ogt/ram1/ram1.asm         ZX 128K: player, instruments, effects, songs
#
# The old format vs MK1's (WYZ 0.47c), the same as Uwol's:
#  * header: old = tempo + the instruments of channels A, B, C. MK1 = tempo,
#    flags (bit 0: loop), 2 reserved bytes, 4 loop offsets (words). Each channel
#    starts with its instrument command and loops right after it.
#  * the instrument command has one more byte in MK1 (relative volume, 0).
#  * drum 0 can't be 0 in MK1 (it looks for 0 to find where channels start):
#    the drums move up one (SONIDO0 is a dummy).
#  * instruments: old = 1 byte per frame [loop|oct-1|oct+1|-|vol], MK1 = 2
#    [loop|oct-1|oct+1|ornament|vol] + pitch (0 here).
#  * notes: the same numbers (MK1's CPC table and the 128K table are the same
#    notes for each machine's AY clock).
#  * the original was made for the CPC (1 MHz AY): drums and effects are
#    used as they are on the CPC, and scaled to the 128K's clock (1.7734 MHz).
#  * the original played the drums over channel C (cpc_WyzConfigurePlayer (0))
#    and the effects over C too; MK1 plays the drums over B: channels B and C
#    are swapped, and the effects go over B (WYZ_FX_CHANNEL 1, MK1's default),
#    so both mask the same line as in the original.

import os, re, struct

H = os.path.dirname (os.path.abspath (__file__))
O = os.path.join (H, '..', 'orig')
D = os.path.join (H, '..')

CLOCK_128K = 1773400 / 1000000

# Songs (MK1 numbers): name, original file, loops?
SONGS = [
	('title', 	'ncjmc-title', 		1),
	('ingame', 	'ncjmc-ingame', 	1),
	('jingle', 	'ncjmc-jingle', 	0),
]

def parse_old (d):
	tempo, ins, p, chans = d [0], list (d [1:4]), 4, []
	for c in range (4):
		cur = []
		while True:
			b = d [p]; p += 1
			if b == 0: break
			if b & 0x3f == 0x3f:
				l = b >> 6
				if l == 0: cur.append ((b, d [p])); p += 1 			# instrument
				elif l == 1: cur.append ((b, d [p])); p += 1 		# drum
				elif l == 2: cur.append ((b,)) 						# envelope (unused here)
				else: raise Exception ('unknown command %02x' % b)
			else: cur.append ((b,))
		chans.append (cur)
	return tempo, ins, chans

def convert (d, loop):
	tempo, ins, chans = parse_old (d)
	chans [1], chans [2] = chans [2], chans [1] 		# Drums over C -> B
	ins [1], ins [2] = ins [2], ins [1]
	out = bytearray ([tempo, 1 if loop else 0, 0, 0])
	body, offs = bytearray (), []
	for c in range (4):
		ch = bytearray ()
		if c < 3:
			ch += bytes ([0x3f, ins [c], 0])
			offs.append (3)
		else:
			offs.append (0)
		for g in chans [c]:
			if g [0] == 0x3f: ch += bytes ([0x3f, g [1], 0])
			elif g [0] == 0x7f:
				n = g [1]
				ch += bytes ([0x7f, (n & 0xc0) | ((n & 0x3f) + 1)])
			elif g [0] == 0xbf: raise Exception ('envelopes are not converted')
			else: ch += bytes (g)
		body += ch + b'\x00'
	for o in offs: out += bytes ([o & 255, o >> 8])
	return bytes (out + body)

# ----------------------------------------------------------------------------
# Instruments, drums and effects from sound.h

src = open (os.path.join (O, 'dev', 'sound.h'), 'rb').read ().decode ('latin-1').replace ('\r', '')

def blk (label):
	# defb values after ._label, until the next label
	i = src.index ('._' + label)
	j = src.find ('._', i + 2)
	body = src [i:j if j >= 0 else len (src)]
	vals = []
	for m in re.findall (r'defb\s+([^\n;]+)', body):
		for x in m.split (','):
			x = x.strip ()
			vals.append (int (x [1:], 16) if x.startswith ('$') else int (x))
	return vals

def table (label):
	m = re.search (r'\._' + label + r'\s+defw\s+([^\n;]+)', src)
	return [x.strip ().lstrip ('_') for x in m.group (1).split (',') if x.strip ()]

pautas = []
for name in table ('wyz_pattern_table'):
	old, new = blk (name), []
	for b in old:
		if b & 0x80:
			n = b & 0x0f
			if n == 0: new += [0, 0, 0x81, 0] 			# stays silent
			else: new += [0x80 | n, 0]
		else: new += [b, 0]
	pautas.append ((name, new))

def scale (v):
	return int (round (v * CLOCK_128K))

def sound_bytes (old, k):
	new = []
	assert old [-1] == 255 and (len (old) - 1) % 3 == 0
	for i in range (0, len (old) - 1, 3):
		per = ((old [i + 1] >> 4) << 8) | old [i]
		noise = old [i + 2]
		if k != 1:
			per = min (int (round (per * k)), 0xfff)
			noise = min (31, max (1, int (round (noise * k)))) if noise else 0
		lo = per & 255
		if lo == 255: lo = 254 						# 255 ends the drum
		new += [lo, ((per >> 8) << 4) | (old [i + 1] & 15), noise]
	return new + [255]

sonidos = [(name, blk (name)) for name in table ('wyz_sound_table')]

def effect_bytes (old, k):
	out = []
	assert old [-1] == 255 and (len (old) - 1) % 2 == 0
	for i in range (0, len (old) - 1, 2):
		lo, hv = old [i], old [i + 1]
		per = ((hv >> 4) << 8) | lo
		if k != 1: per = min (int (round (per * k)), 0xfff)
		lo = per & 255
		if lo == 255: lo = 254 					# 255 ends the effect
		out += [lo, ((per >> 8) << 4) | (hv & 15), 0]
	return out + [255]

effects = [blk (name) for name in table ('wyz_effects_table')] 	# jump, take, drop, death

# sfx.txt numbers -> original effect (None = silent)
sfx_names = re.findall (r'^sfx\s+(\w+)', open (os.path.join (D, 'sfx', 'sfx.txt')).read (), re.M)
assert len (sfx_names) == 10, 'the CPC player has 10 effects'
ORIG_FX = { 'jump': 0, 'take': 1, 'drop': 2, 'player_hit': 3 }

def defbs (vals):
	return ''.join ('\t\tdefb %s\n' % ', '.join (str (v) for v in vals [i:i + 16]) for i in range (0, len (vals), 16))

# ----------------------------------------------------------------------------
# CPC

os.makedirs (os.path.join (D, 'dev', 'wyz'), exist_ok = True)
with open (os.path.join (D, 'dev', 'wyz', 'instrumentos.h'), 'w') as f:
	f.write ('// Nanako - instruments and drums of the original (orig/dev/sound.h)\n')
	f.write ('// Generated by tools/nanako_music.py\n\n#asm\n')
	f.write ('\t.TABLA_PAUTAS\n\t\tdefw ' + ', '.join ('PAUTA_%d' % i for i in range (len (pautas))) + '\n')
	f.write ('\t.TABLA_SONIDOS\n\t\tdefw SONIDO0, ' + ', '.join ('SONIDO%d' % (i + 1) for i in range (len (sonidos))) + '\n')
	for i, (name, new) in enumerate (pautas):
		f.write ('\t// %s\n\t.PAUTA_%d\n' % (name, i) + defbs (new))
	f.write ('\t// Dummy: drum 0 can\'t be used (see tools/nanako_music.py)\n\t.SONIDO0\n\t\tdefb 0, 0, 0, 255\n')
	for i, (name, old) in enumerate (sonidos):
		f.write ('\t// %s\n\t.SONIDO%d\n' % (name, i + 1) + defbs (sound_bytes (old, 1)))
	f.write ('#endasm\n')

with open (os.path.join (D, 'dev', 'wyz', 'efectos_ids.h'), 'w') as f:
	f.write ('// Nanako - sound effect numbers (sfx/sfx.txt). Generated by tools/nanako_music.py\n\n')
	for i, n in enumerate (sfx_names):
		f.write ('#define SFX_%s %d\n' % (n.upper (), i))

with open (os.path.join (D, 'dev', 'wyz', 'efectos.h'), 'w') as f:
	f.write ('// Nanako - the original\'s sound effects (orig/dev/sound.h) in sfx/sfx.txt\'s numbers\n')
	f.write ('// Generated by tools/nanako_music.py\n\n#asm\n')
	for i, n in enumerate (sfx_names):
		o = ORIG_FX.get (n)
		f.write ('\t// %d: %s -> %s\n\t.EFECTO%d\n' % (i, n, 'original %d' % o if o is not None else 'silent', i))
		f.write (defbs (effect_bytes (effects [o], 1) if o is not None else [255]))
	f.write ('#endasm\n')

os.makedirs (os.path.join (D, 'ogt'), exist_ok = True)
print ('%-8s %-14s %6s %6s' % ('song', 'original', 'old', 'mk1'))
for name, orig, loop in SONGS:
	d = open (os.path.join (O, 'mus', 'wyz', orig + '.mus'), 'rb').read ()
	new = convert (d, loop)
	open (os.path.join (D, 'ogt', 'nanako_%s.mus' % name), 'wb').write (new)
	print ('%-8s %-14s %6d %6d' % (name, orig, len (d), len (new)))

with open (os.path.join (D, 'dev', 'wyz', 'songs.h'), 'w') as f:
	f.write ('// Nanako - songs (ogt/nanako_*.mus, packed by comp_cpc)\n')
	f.write ('// Generated by tools/nanako_music.py\n\n')
	f.write ('extern unsigned char *wyz_songs [0];\n\n#asm\n')
	for name, orig, loop in SONGS:
		f.write ('\t._nanako_%s_mus_bin\n\t\tBINARY "../ogt/nanako_%s.mus.bin"\n' % (name, name))
	f.write ('\n\t._wyz_songs\n\t\tdefw %s\n' % ', '.join ('_nanako_%s_mus_bin' % s [0] for s in SONGS))
	f.write ('#endasm\n')

# ----------------------------------------------------------------------------
# ZX 128K: RAM1

src_p = open (os.path.join (D, 'dev', 'wyz', 'wyz_player.h'), 'rb').read ().decode ('latin-1')
player = src_p [src_p.index ('\n#asm', src_p.index ('void wyz_stop_sound')) + 5:src_p.rindex ('#endasm')]

def sub (a, b):
	global player
	assert a in player, a
	player = player.replace (a, b)

# AY ports: ZX
i0 = player.index ('\t.WRITEPSGHL')
i1 = player.index ('RET', i0) + 3
player = player [:i0] + '''	.WRITEPSGHL
		; ZX: register A, value (HL), HL ++
		LD BC,0xFFFD
		OUT (C),A
		LD B,0xC0
		OUTI
		RET''' + player [i1:]
sub ('_wyz_beat_ct', 'WYZ_BEAT_CT')
sub ('(_wyz_signal)', '(WYZ_SIGNAL)')
# The 128K note table
i0 = player.index ('\t.DATOS_NOTAS')
i1 = player.index ('\t.TABLA_SONG')
notes = struct.unpack ('<62H', open (os.path.join (D, 'ogt', 'notas_128k.dat'), 'rb').read () [:124])
player = player [:i0] + '\t.DATOS_NOTAS\n' + ''.join ('\t\tdefw %s\n' % ', '.join (str (v) for v in notes [i:i + 10]) for i in range (0, 62, 10)) + '\n' + player [i1:]
sub ('\t\tdefw \tWYZ_SONG_BUFFER', '\t\tdefw ' + ', '.join ('SONG_%d' % i for i in range (len (SONGS))))
i0 = player.index ('\t.TABLA_EFECTOS')
i1 = player.index ('EFECTO9', i0) + 7
player = player [:i0] + player [i1:]

os.makedirs (os.path.join (D, 'ogt', 'ram1'), exist_ok = True)
with open (os.path.join (D, 'ogt', 'ram1', 'ram1.asm'), 'w') as f:
	f.write ('''; Nanako - RAM1 for the ZX Spectrum 128K (MODE_128K_DUAL)
; Generated by tools/nanako_music.py. Assemble: z80asm -b ram1.asm
;
; MK1 v3.2's WYZ player (dev/wyz/wyz_player.h) with the ZX's AY ports, the
; 128K note table, the original's instruments, drums and effects (scaled to
; the 128K's AY clock) and the songs, unpacked.
; The game pages RAM1 in at C000 and calls the jump table:

	org $C000

	jp  WYZ_PLAYER_ISR 			; C000 every frame (ISR)
	jp  WYZPLAYER_INIT 			; C003
	jp  CARGA_CANCION 			; C006 A = song
	jp  RAM1_FX 				; C009 A = effect
	jp  PLAYER_OFF 				; C00C stop the music

	defc WYZ_FX_CHANNEL = 1 	; Effects over channel B (the original's C, see tools/nanako_music.py)

.RAM1_FX
	ld  b, WYZ_FX_CHANNEL
	jp  INICIA_EFECTO

.WYZ_BEAT_CT
	defb 0
.WYZ_SIGNAL
	defb 0
''')
	f.write (player.replace ('\r', ''))
	f.write ('\n\t.TABLA_PAUTAS\n\t\tdefw ' + ', '.join ('PAUTA_%d' % i for i in range (len (pautas))) + '\n')
	f.write ('\t.TABLA_SONIDOS\n\t\tdefw SONIDO0, ' + ', '.join ('SONIDO%d' % (i + 1) for i in range (len (sonidos))) + '\n')
	for i, (name, new) in enumerate (pautas):
		f.write ('\t.PAUTA_%d\n' % i + defbs (new))
	f.write ('\t.SONIDO0\n\t\tdefb 0, 0, 0, 255\n')
	for i, (name, old) in enumerate (sonidos):
		f.write ('\t.SONIDO%d\n' % (i + 1) + defbs (sound_bytes (old, CLOCK_128K)))
	f.write ('\n\t.TABLA_EFECTOS\n')
	for i, n in enumerate (sfx_names):
		f.write ('\t\tdefw EFECTO_%s\n' % ('MUTE' if ORIG_FX.get (n) is None else str (ORIG_FX [n])))
	f.write ('\t.EFECTO_MUTE\n\t\tdefb 255\n')
	for o in range (len (effects)):
		f.write ('\t.EFECTO_%d\n' % o + defbs (effect_bytes (effects [o], CLOCK_128K)))
	f.write ('\n')
	for i, (name, orig, loop) in enumerate (SONGS):
		f.write ('\t.SONG_%d\n\t\tBINARY "../nanako_%s.mus"\n' % (i, name))
	f.write ('\n\t.BASE_WYZ\n\t\tdefs 128\n')

print ('%d instruments, %d drums, %d effects; ram1.asm' % (len (pautas), len (sonidos), len (effects)))
