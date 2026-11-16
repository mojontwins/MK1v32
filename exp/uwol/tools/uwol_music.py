# uwol_music.py - one-shot conversion of the original Uwol songs (2009, the old
# WYZ player of the 128K version) to MK1 v3.2's CPC WYZ player.
#
# usage: python3 uwol_music.py          (paths are relative to this script's folder)
#
# Reads ../orig/mus and writes:
#   ../ogt/uwol_*.mus            the songs in MK1's format (comp_cpc.sh packs them)
#   ../dev/wyz/instrumentos.h    the original's instruments (PAUTAs) and drums (SONIDOs)
#   ../dev/wyz/songs.h           the song table
#
# The old format vs MK1's (WYZ 0.47c):
#  * header: old = tempo + the instruments of channels A, B, C. MK1 = tempo,
#    flags (bit 0: loop), 2 reserved bytes, 4 loop offsets (words). Each channel
#    here starts with its instrument command, and loops right after it (the old
#    player didn't set the instrument again when a channel looped).
#  * the instrument command has one more byte in MK1 (relative volume, 0).
#  * the drum command (7F nn): MK1 finds where channels start looking for 0
#    bytes, so drum 0 can't be 0: the drums move up one (SONIDO0 is a dummy).
#  * instruments: old = 1 byte per frame [loop|oct-1|oct+1|-|vol], MK1 = 2
#    [loop|oct-1|oct+1|ornament|vol] + pitch (0 here). An old loop byte with
#    offset 0 (it stays in silence) becomes a silent frame looping on itself.
#  * notes: the same numbers. The old table is for the 128K's AY clock
#    (1.7734 MHz) and MK1's for the CPC's (1 MHz): the same notes.
#  * drum periods are scaled from the 128K's clock to the CPC's.
#  * the old player played the drums over channel A; MK1's over channel B:
#    channels A and B are swapped, so the drums mask the same line. The sound
#    effects go over channel A (the old B), as in the original.
#
# The original's sound effects (EFECTO0-8, 2 bytes a step; 2, 4 and 6 have 3)
# become MK1's (3 bytes a step: period, period high / volume, noise).
#
# CPC (1 MHz AY): MK1's note table; drums and effects scaled from 1.7734 MHz.
#   ../dev/wyz/efectos.h: EFECTO0-9 (MK1's numbers, see SLOTS_CPC), SFX_*.
# ZX 128K (1.7734 MHz, as the original): the original's notes, drums and effects
# untouched, all in RAM1 (MODE_128K_DUAL):
#   ../ogt/ram1/ram1.asm: MK1's WYZ player (from ../dev/wyz/wyz_player.h, with
#   the ZX's AY ports) + jump table at C000, instruments, effects (the same
#   numbers as the beeper ones in sfx/sfx.txt), and the songs, unpacked.

import os, re, sys

H = os.path.dirname (os.path.abspath (__file__))
O = os.path.join (H, '..', 'orig', 'mus')
D = os.path.join (H, '..')

CLOCK = 1000000 / 1773400

# Songs: (MK1 number order) name, original file, loops?
# 0, 1, 2 are the numbers vanilla MK1 plays: title, in game, game over.
SONGS = [
	('title', 		'UWOLMenu', 		1),
	('zone1', 		'UWOLZona1', 		1),
	('gover', 		'UWOLGameOver', 	0),
	('zone2', 		'UWOLZona2', 		1),
	('zone3', 		'UWOLZona3', 		1),
	('zone4', 		'UWOLZona4', 		1),
	('fanty', 		'UWOLFantasma', 	1),
	('pyramid', 	'UWOLPiramide', 	1),
	('ending_ko', 	'UWOLEndingKO', 	0),
	('ending_ok', 	'UWOLEndingOK', 	0),
]

def parse_old (d):
	# -> tempo, [ins A, B, C], 4 channels as lists of raw byte groups
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
	# MK1's player plays the drums over channel B, the old one over A: swap
	# A and B so the drums mask the same line.
	chans [0], chans [1] = chans [1], chans [0]
	ins [0], ins [1] = ins [1], ins [0]
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
# Instruments and drums from the player's source

asm = open (os.path.join (O, 'wyzplayer.asm'), 'rb').read ().decode ('latin-1').replace ('\r', '')
def table (label):
	m = re.search (label + r':\s*DW\s+([^\n;]+)', asm)
	return [x.strip () for x in m.group (1).split (',') if x.strip ()]
def data (label):
	m = re.search (r'^' + label + r':\s*DB\s+([^\n;]+)', asm, re.M)
	return [int (x) for x in m.group (1).split (',')]

pautas = []
for name in table ('TABLA_PAUTAS'):
	old, new = data (name), []
	for b in old:
		if b & 0x80:
			n = b & 0x0f
			if n == 0: new += [0, 0, 0x81, 0] 			# stays silent
			else: new += [0x80 | n, 0]
		else: new += [b, 0]
	pautas.append ((name, old, new))

def scale (v):
	return int (round (v * CLOCK))

sonidos = []
for name in table ('TABLA_SONIDOS'):
	old, new = data (name), []
	assert old [-1] == 255 and (len (old) - 1) % 3 == 0
	for i in range (0, len (old) - 1, 3):
		per = ((old [i + 1] >> 4) << 8) | old [i]
		per = min (scale (per), 0xfff)
		noise = old [i + 2]
		noise = max (1, scale (noise)) if noise else 0
		lo = per & 255
		if lo == 255: lo = 254 						# 255 ends the drum
		new += [lo, ((per >> 8) << 4) | (old [i + 1] & 15), noise]
	sonidos.append ((name, new + [255]))

def defbs (vals):
	return ''.join ('\t\tdefb %s\n' % ', '.join (str (v) for v in vals [i:i + 16]) for i in range (0, len (vals), 16))

with open (os.path.join (D, 'dev', 'wyz', 'instrumentos.h'), 'w') as f:
	f.write ('// Uwol - instruments and drums of the original (orig/mus/wyzplayer.asm)\n')
	f.write ('// Generated by tools/uwol_music.py\n\n#asm\n')
	f.write ('\t.TABLA_PAUTAS\n\t\tdefw ' + ', '.join ('PAUTA_%d' % i for i in range (len (pautas))) + '\n')
	f.write ('\t.TABLA_SONIDOS\n\t\tdefw SONIDO0, ' + ', '.join ('SONIDO%d' % (i + 1) for i in range (len (sonidos))) + '\n')
	for i, (name, old, new) in enumerate (pautas):
		f.write ('\t// %s\n\t.PAUTA_%d\n' % (name, i) + defbs (new))
	f.write ('\t// Dummy: drum 0 can\'t be used (see tools/uwol_music.py)\n\t.SONIDO0\n\t\tdefb 0, 0, 0, 255\n')
	for i, (name, new) in enumerate (sonidos):
		f.write ('\t// %s\n\t.SONIDO%d\n' % (name, i + 1) + defbs (new))
	f.write ('#endasm\n')

# ----------------------------------------------------------------------------
# Songs

os.makedirs (os.path.join (D, 'ogt'), exist_ok = True)
print ('%-10s %-14s %6s %6s' % ('song', 'original', 'old', 'mk1'))
for name, orig, loop in SONGS:
	d = open (os.path.join (O, orig + '.mus'), 'rb').read ()
	new = convert (d, loop)
	open (os.path.join (D, 'ogt', 'uwol_%s.mus' % name), 'wb').write (new)
	print ('%-10s %-14s %6d %6d' % (name, orig, len (d), len (new)))

with open (os.path.join (D, 'dev', 'wyz', 'songs.h'), 'w') as f:
	f.write ('// Uwol - songs (ogt/uwol_*.mus, packed by comp_cpc)\n')
	f.write ('// Generated by tools/uwol_music.py\n\n')
	f.write ('#include "wyz/songs_ids.h"\n')
	f.write ('\nextern unsigned char *wyz_songs [0];\n\n#asm\n')
	for name, orig, loop in SONGS:
		f.write ('\t._uwol_%s_mus_bin\n\t\tBINARY "../ogt/uwol_%s.mus.bin"\n' % (name, name))
	f.write ('\n\t._wyz_songs\n')
	for i in range (0, len (SONGS), 4):
		f.write ('\t\tdefw %s\n' % ', '.join ('_uwol_%s_mus_bin' % s [0] for s in SONGS [i:i + 4]))
	f.write ('#endasm\n')

with open (os.path.join (D, 'dev', 'wyz', 'songs_ids.h'), 'w') as f:
	f.write ('// Uwol - song numbers (CPC: wyz/songs.h, ZX 128K: ogt/ram1/ram1.asm)\n')
	f.write ('// Generated by tools/uwol_music.py\n\n')
	for i, (name, orig, loop) in enumerate (SONGS):
		f.write ('#define MUSIC_%s %d\n' % (name.upper (), i))

print ('%d instruments, %d drums' % (len (pautas), len (sonidos)))

# ----------------------------------------------------------------------------
# Sound effects

def old_effects ():
	m = re.search (r'TABLA_EFECTOS:\s*DW\s+([^\n;]+)', asm)
	names = [x.strip () for x in m.group (1).split (',')]
	effs = []
	for i, name in enumerate (names):
		i0 = asm.index ('\n' + name + ':')
		block = asm [i0:]
		vals = []
		for line in block.split ('\n') [1:] if False else block.split ('\n'):
			mm = re.search (r'\bDB\s+([^;\n]+)', line)
			if not mm: 
				if vals: break
				continue
			vals += [int (x.strip ().replace ('$', '0x'), 16) for x in mm.group (1).split (',')]
			if vals [-1] == 0xff: break
		vals = vals [:-1]
		w = 3 if i in (2, 4, 6) else 2
		steps = [vals [j:j + w] + [0] * (3 - w) for j in range (0, len (vals), w)]
		effs.append (steps)
	return effs

effects = old_effects ()

def effect_bytes (steps, scaled):
	out = []
	for lo, hv, noise in steps:
		per = ((hv >> 4) << 8) | lo
		if scaled:
			per = min (scale (per), 0xfff)
			noise = max (1, scale (noise)) if noise else 0
		lo = per & 255
		if lo == 255: lo = 254 					# 255 ends the effect
		out += [lo, ((per >> 8) << 4) | (hv & 15), noise]
	return out + [255]

# CPC: MK1's 10 numbers (the engine plays them), as the closest original effect.
# None = silent. Then Uwol's names for custom.h.
SLOTS_CPC = [
	('enemy destroyed', None), ('enemy hit', 1), ('player hit', 1), ('jump', 7),
	('player hit 2', 1), ('enemy destroyed 2', None), ('shot', None),
	('item (t-shirt)', 3), ('key (coin)', 5), ('life', 3)
]
SFX_CPC = [('COIN', 8), ('SHIRT', 7), ('DEATH', 2), ('FANTY', 0), ('EXITS', 0), ('EXTRA_LIFE', 9), ('ENDING', 0)]

with open (os.path.join (D, 'dev', 'wyz', 'efectos.h'), 'w') as f:
	f.write ('// Uwol - the original\'s sound effects (orig/mus/wyzplayer.asm) in MK1\'s numbers\n')
	f.write ('// Generated by tools/uwol_music.py\n\n')
	for name, n in SFX_CPC:
		f.write ('#define SFX_%s %d\n' % (name, n))
	f.write ('\n#asm\n')
	for i, (name, o) in enumerate (SLOTS_CPC):
		f.write ('\t// %d: %s -> %s\n\t.EFECTO%d\n' % (i, name, 'original %d' % o if o is not None else 'silent', i))
		f.write (defbs (effect_bytes (effects [o], True) if o is not None else [255]))
	f.write ('#endasm\n')

# ZX 128K: the same numbers as sfx/sfx.txt (peta_el_beeper (n) is the same n)
SLOTS_ZX = {
	'enemy_destroyed': None, 'enemy_hit': 1, 'player_hit': 1, 'jump': 7, 'player_hit_2': 1,
	'game_over': None, 'shot': None, 'item': 3, 'key': 3, 'life': 5,
	'coin': 5, 'shirt': 3, 'death': 1, 'fanty': None, 'exits': None, 'extra_life': 3, 'ending': None
}
sfx_names = re.findall (r'^sfx\s+(\w+)', open (os.path.join (D, 'sfx', 'sfx.txt')).read (), re.M)

# ----------------------------------------------------------------------------
# ZX 128K: RAM1

src = open (os.path.join (D, 'dev', 'wyz', 'wyz_player.h'), 'rb').read ().decode ('latin-1')
player = src [src.index ('\n#asm', src.index ('void wyz_stop_sound')) + 5:src.rindex ('#endasm')]

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
# The original's notes (the 128K's AY clock)
i0 = player.index ('\t.DATOS_NOTAS')
i1 = player.index ('\t.TABLA_SONG')
import struct
notes = struct.unpack ('<62H', open (os.path.join (O, 'NOTAS.DAT'), 'rb').read () [:124])
player = player [:i0] + '\t.DATOS_NOTAS\n' + ''.join ('\t\tdefw %s\n' % ', '.join (str (v) for v in notes [i:i + 10]) for i in range (0, 62, 10)) + '\n' + player [i1:]
sub ('\t\tdefw \tWYZ_SONG_BUFFER', '\t\tdefw ' + ', '.join ('SONG_%d' % i for i in range (len (SONGS))))
i0 = player.index ('\t.TABLA_EFECTOS')
i1 = player.index ('EFECTO9', i0) + 7
player = player [:i0] + player [i1:]

os.makedirs (os.path.join (D, 'ogt', 'ram1'), exist_ok = True)
with open (os.path.join (D, 'ogt', 'ram1', 'ram1.asm'), 'w') as f:
	f.write ('''; Uwol - RAM1 for the ZX Spectrum 128K (MODE_128K_DUAL)
; Generated by tools/uwol_music.py. Assemble: z80asm -b ram1.asm
;
; MK1 v3.2's WYZ player (dev/wyz/wyz_player.h) with the ZX's AY ports, the
; original's instruments, notes and effects, and all the songs, unpacked.
; The game pages RAM1 in at C000 and calls the jump table:

	org $C000

	jp  WYZ_PLAYER_ISR 			; C000 every frame (ISR)
	jp  WYZPLAYER_INIT 			; C003
	jp  CARGA_CANCION 			; C006 A = song
	jp  RAM1_FX 				; C009 A = effect
	jp  PLAYER_OFF 				; C00C stop the music

	defc WYZ_FX_CHANNEL = 0 	; Effects over channel A (the original's B)

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
	for i, (name, old, new) in enumerate (pautas):
		f.write ('\t.PAUTA_%d\n' % i + defbs (new))
	f.write ('\t.SONIDO0\n\t\tdefb 0, 0, 0, 255\n')
	for i, name in enumerate (table ('TABLA_SONIDOS')):
		d = data (name)
		assert 255 not in d [:-1] [::3]
		f.write ('\t.SONIDO%d\n' % (i + 1) + defbs (d))
	f.write ('\n\t.TABLA_EFECTOS\n')
	for i, name in enumerate (sfx_names):
		f.write ('\t\tdefw EFECTO_%s\n' % ('MUTE' if SLOTS_ZX [name] is None else str (SLOTS_ZX [name])))
	f.write ('\t.EFECTO_MUTE\n\t\tdefb 255\n')
	for o in sorted (set (v for v in SLOTS_ZX.values () if v is not None)):
		f.write ('\t.EFECTO_%d\n' % o + defbs (effect_bytes (effects [o], False)))
	f.write ('\n')
	for i, (name, orig, loop) in enumerate (SONGS):
		f.write ('\t.SONG_%d\n\t\tBINARY "../uwol_%s.mus"\n' % (i, name))
	f.write ('\n\t.BASE_WYZ\n\t\tdefs 128\n')

print ('%d effects; ram1.asm' % len (effects))
