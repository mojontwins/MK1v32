#!/usr/bin/env python3
# MSC4 - Mojon Script Compiler 4, Python port.
# Port of msc4/msc4.bas (msc v4.6.20260729) + mtparser.bas + mt5bitEsc.bas.
# Copyleft 2025 by The Mojon Twins (original), port 2026.
#
#   msc4.py in=script.spt v=3 target=zx|cpc [rooms=N] [text=text.bin]
#           [interpreter=msc4i.asm] [noindexed] [fastnpant] [jumptable]
#           [debug] [textdebug]
#
# The tokenizer, the bytecode and the generated interpreter are the same as
# the FreeBASIC original (quirks included: see the comments marked QUIRK).
# One addition: CALL to an undefined subroutine is reported as an error.

import sys

SPECCY, CPC = 0, 1
ENTERING_INDEX_OFFSET = 16
PRESS_FIRE_INDEX_OFFSET = 17
NTOK = 256

debug = False
textdebug = False
tokens = [''] * NTOK
curLineNo = 0
aliases = [''] * 129
AU = [False] * 256
CU = [False] * 256
LV = [False] * 256
RV = [False] * 256
sectOffs = [-1] * 256
sectBinary = bytearray()
mainBinary = bytearray()
outT = SPECCY
maxPants = 0
noIndexed = False
fastNPant = False
jumptable = False
itemSlot = 0
itemEmpty = 0
textWidth = 24
textPool = []
textOffsets = []
lastTextOffset = 0
lastCommandWasTerminator = False
errorCount = 0
subInfo = []


def dprint(*a):
	if debug:
		print(*a)


def reportError(msg):
	global errorCount
	errorCount += 1
	print('*** ERROR: ' + msg)


# ------------------------------------------------------------ mtparser ---

def parseTokenizeString(inString, ntok, ignore, brk):
	toks = [''] * ntok
	ignore = ignore + ' \t'
	inString = inString + ' '
	w = 0
	quotes = False
	for m in inString:
		if quotes:
			if m == '"':
				quotes = False
			else:
				toks[w] += m
		else:
			if brk and m in brk:
				break
			elif m == '"':
				quotes = True
			elif m in ignore:
				if toks[w] != '':
					if w < ntok - 1:
						w += 1
			else:
				toks[w] += m
	return toks


def parseCoordinatesString(s):
	out = []
	for p in (s + ',').split(',')[:-1]:
		out.append(fbval(p))
	return out + [-1] * 8


def fbval(s):
	# FreeBASIC Val (): leading number, 0 if none
	s = s.strip()
	n = ''
	for i, c in enumerate(s):
		if c.isdigit() or (i == 0 and c in '+-'):
			n += c
		else:
			break
	try:
		return int(n)
	except ValueError:
		return 0


def fb_replace(s, a, b):
	# QUIRK: the original skips the first character.
	if not s or not a or not b:
		return ''
	return s[0] + s[1:].replace(a, b)


# ---------------------------------------------------------- mt5bitEsc ---

def bin5(v):
	return format(v & 31, '05b')


def encodeWord(word):
	bs = ''
	for m in word:
		if m == ' ':
			bs += '11111'
		elif m == ',':
			bs += bin5(27)
		elif m == '.':
			bs += bin5(28)
		elif m == '?':
			bs += bin5(29)
		elif m == '!':
			bs += bin5(30)
		elif m < '?':
			bs += '00000' + bin5(ord(m) - 32)
		elif 'A' <= m <= 'Z':
			bs += bin5(ord(m) - 64)
		elif 'a' <= m <= 'z':
			bs += bin5(ord(m.upper()) - 64)
	return bs


def encodeBinaryString(bs):
	if len(bs) % 8:
		bs += '0' * (8 - len(bs) % 8)
	return bytes(int(bs[i:i + 8], 2) for i in range(0, len(bs), 8))


def encode5BitEsc(text, wrap):
	text = text + ' '
	cur = ''
	ll = 0
	bs = ''
	spaceAdded = False
	for i, m in enumerate(text, 1):
		if m == ' ':
			if ll + len(cur) > wrap:
				if spaceAdded:
					bs = bs[:-5]
				bs += '0000011111'
				ll = 0
			bs += encodeWord(cur)
			ll += len(cur)
			cur = ''
			spaceAdded = False
			if i == len(text):
				bs += '0000000000'
			elif ll < wrap:
				bs += '11111'
				ll += 1
				spaceAdded = True
		else:
			cur += m
	return encodeBinaryString(bs)


def decode5(enc):
	bs = ''.join(format(b, '08b') for b in enc)
	out = ''
	esc = False
	for i in range(0, len(bs) - 4, 5):
		d = int(bs[i:i + 5], 2)
		if esc:
			esc = False
			if d == 0:
				break
			out += chr(32 + d) if d < 31 else '|'
		else:
			if d == 0:
				esc = True
			else:
				out += chr(64 + d) if d <= 26 else ',.?! '[d - 27]
	return out


# -------------------------------------------------------------- compiler ---

def shiftTokens(frm):
	for i in range(frm, NTOK - 1):
		tokens[i] = tokens[i + 1]
	tokens[NTOK - 1] = ''


def debugPrintTokens():
	i = 0
	while i < 64 and tokens[i] != '':
		print('T%d=[%s]' % (i, tokens[i]))
		i += 1


def hexs(b):
	return ' '.join('%02X' % v for v in b)


def addOrResolveAlias(s):
	s = s.lower()
	for i, a in enumerate(aliases):
		if a == s:
			return i
	for i, a in enumerate(aliases):
		if a == '':
			aliases[i] = s
			return i
	print('Warning! Too many aliases, resolving to 0')
	return 0


SPECIALS = {
	'NPANT': 254, 'N_PANT': 254, 'PX': 253, 'PY': 252, 'KILLED': 251,
	'OBJS': 250, 'LIFE': 249, 'TX': 248, 'TY': 247, 'TN': 246,
	'HOTSPOT': 245, 'HIDE_HOTSPOTS': 240, 'DONT_MAKE_FANTIES': 239,
	'EN_T': 238, 'EN_N': 237, 'EN_X': 236, 'EN_Y': 235, 'OPANT': 234,
	'O_PANT': 234, 'PARAM': 233,
}


def parseScriptLine(linea):
	global tokens
	dprint('Parsing: ' + linea)
	quotes = False
	cur = ''
	index = 0
	i = 1
	while i <= len(linea):
		m = linea[i - 1]
		if quotes:
			if m == '"':
				quotes = False
		else:
			if m == '"':
				quotes = True
			elif m in '<>=!':
				if cur == '':
					index = i
				cur += m
			else:
				if cur != '':
					prefix = linea[:index - 1] if index > 1 else ''
					sufix = linea[i - 1:]
					linea = prefix + ' ' + cur + ' ' + sufix
					i += 2
					cur = ''
		i += 1

	if len(linea) > 3 and linea[:3].upper() == 'ON ':
		linea = linea[3:]

	tokens = parseTokenizeString(linea, NTOK, ',()[]', '#')

	i = 0
	while i < NTOK - 1 and tokens[i] != '':
		t = tokens[i]
		ap = t.find('%') + 1
		if len(t) > 2 and t[:2] == '&%':
			tokens[i] = str(addOrResolveAlias(t[2:]))
		elif ap > 0:
			tokens[i] = t[:ap - 1] + '$' + str(addOrResolveAlias(t[ap:]))
		elif t.upper() in SPECIALS:
			tokens[i] = '$%d' % SPECIALS[t.upper()]
		if tokens[i].upper() == 'EMPTY':
			tokens[i] = str(itemEmpty)
		i += 1

	i = 0
	while i < NTOK - 1 and tokens[i] != '':
		if tokens[i].upper() in ('TILE', 'BEH') and tokens[i + 1].upper() == 'AT':
			ind = '=' + tokens[i] + '=' + tokens[i + 2] + '=' + tokens[i + 3]
			shiftTokens(i); shiftTokens(i); shiftTokens(i)
			tokens[i] = ind
		elif tokens[i].upper() == 'DIALOG':
			ind = ('=' + tokens[i] + '=' + fb_replace(tokens[i + 1], ' ', '_') + '=' +
				fb_replace(tokens[i + 2], ' ', '_') + '=' + fb_replace(tokens[i + 3], ' ', '_'))
			shiftTokens(i); shiftTokens(i); shiftTokens(i)
			tokens[i] = ind
		i += 1

	i = 0
	while i < NTOK - 1 and tokens[i] != '':
		ind = ''
		while tokens[i].upper() == 'FLAG' and i < NTOK - 1:
			ind += '$'
			shiftTokens(i)
		if ind:
			tokens[i] = ind + tokens[i]
		i += 1

	if debug:
		debugPrintTokens()


def correctLvalue(s):
	return s != '' and s[0] == '$'


def isNumber(s):
	return all('0' <= c <= '9' for c in s)


def isFlagIndex(s):
	if not s:
		return False
	if s[0] == '$':
		return True
	if not isNumber(s):
		return False
	return fbval(s) <= 254


def correctSubExpresion(s):
	return len(s) >= 2 and s[0] == '='


def isNumberOrVar(s):
	return correctSubExpresion(s) or correctLvalue(s) or isNumber(s)


def isComp(s):
	return s in ('=', '!=', '<>', '<', '>', '<=', '>=')


def getTextOffset(text):
	global lastTextOffset
	if text == '':
		return b'\xff\xff'
	enc = encode5BitEsc(text, textWidth)
	offset = -1
	for i, t in enumerate(textPool):
		if t == enc:
			offset = textOffsets[i]
			break
	if offset == -1:
		offset = lastTextOffset
		textOffsets.append(offset)
		textPool.append(enc)
		if textdebug:
			print('Encoded text: ' + hexs(enc))
			print('Pretty Print: ' + decode5(enc))
		lastTextOffset += len(enc)
	return bytes([offset & 255, offset >> 8])


def pVal(e):
	if len(e) > 1 and e[0] == '=':
		dprint('Parsing expresion ' + e)
		st = parseTokenizeString(e + '=', 17, '=', '#')
		if st[0] == 'DIALOG':
			r = (b'\xff\xe6' + getTextOffset(fb_replace(st[1], '_', ' ')) +
				getTextOffset(fb_replace(st[2], '_', ' ')) + getTextOffset(fb_replace(st[3], '_', ' ')))
			RV[0xE6] = True
		elif st[0] == 'TILE':
			r = b'\xff\xe8' + pVal(st[1]) + pVal(st[2])
			RV[0xE8] = True
		elif st[0] == 'BEH':
			r = b'\xff\xe7' + pVal(st[1]) + pVal(st[2])
			RV[0xE7] = True
		else:
			reportError('Wrong expresion @ %d' % curLineNo)
			return b'\x00'
		dprint('Result bytecode = ' + hexs(r))
		return r
	elif len(e) > 1 and e[0] == '$':
		if not isFlagIndex(e[1:]):
			reportError("Bad flag reference '%s' @ %d" % (e, curLineNo))
			return b'\x00'
		return b'\xff' + pVal(e[1:])
	else:
		if len(e) == 0:
			return b'\x00'
		if not isNumber(e):
			reportError("Wrong value '%s' @ %d" % (e, curLineNo))
			return b'\x00'
		v = fbval(e)
		if v <= 254:
			RV[v] = True
			return bytes([v])
		reportError('Wrong value @ %d' % curLineNo)
		return b'\x00'


def lValR(e):
	if len(e) > 1 and e[0] == '$':
		return b'\xff' + pVal(e[1:])
	v = fbval(e)
	if v <= 254:
		return bytes([v])
	print('Wrong value @ %d' % curLineNo)
	return b'\x00'


def lVal(e):
	if correctLvalue(e):
		if not isFlagIndex(e[1:]):
			reportError("Bad flag reference '%s' @ %d" % (e, curLineNo))
			return b'\x00'
		v = lValR(e[1:])
		LV[v[-1]] = True
		return v
	reportError('Expecting lValue @ %d' % curLineNo)
	return b'\x00'


def buildCond(b):
	if b:
		CU[b[0]] = True
	return b


def buildAction(b):
	if b:
		AU[b[0]] = True
	return b


def syntaxError():
	reportError('Syntax error at line %d' % curLineNo)


def processIf(linea):
	parseScriptLine(linea)
	t = tokens
	code = b''
	if fastNPant and t[1].lower() == '$254' and t[2] == '=':
		code = buildCond(b'\x05' + pVal(t[3]))
	elif isNumberOrVar(t[1]) and isComp(t[2]) and isNumberOrVar(t[3]):
		op = t[2]
		if op == '=':
			code = buildCond(b'\x01' + pVal(t[1]) + pVal(t[3]))
		elif op == '<':
			code = buildCond(b'\x02' + pVal(t[1]) + pVal(t[3]))
		elif op == '>':
			code = buildCond(b'\x02' + pVal(t[3]) + pVal(t[1]))
		elif op == '>=':
			code = buildCond(b'\x03' + pVal(t[1]) + pVal(t[3]))
		elif op == '<=':
			code = buildCond(b'\x03' + pVal(t[3]) + pVal(t[1]))
		elif op in ('<>', '!='):
			code = buildCond(b'\x04' + pVal(t[1]) + pVal(t[3]))
	else:
		cmd = t[1].lower()
		if cmd == 'true':
			code = buildCond(b'\xf0')
		elif cmd == 'player':
			s = t[2].lower()
			if s == 'in_x':
				code = buildCond(b'\x21' + pVal(t[3]) + pVal(t[4]))
			elif s == 'in_y':
				code = buildCond(b'\x22' + pVal(t[3]) + pVal(t[4]))
			elif s in ('touches', 'at'):
				code = buildCond(b'\x23' + pVal(t[3]) + pVal(t[4]))
			elif s == 'falling':
				code = buildCond(b'\x24')
			elif s == 'not_falling':
				code = buildCond(b'\x25')
			elif s == 'not':
				if t[3].lower() == 'falling':
					code = buildCond(b'\x25')
			elif s == 'still':
				code = buildCond(b'\x26')
	if not code:
		reportError('Unknown condition @ line %d: %s' % (curLineNo, linea))
	return code


def subAddress(name):
	name = name.lower()
	for n, a in subInfo:
		if n.lower() == name:
			return a
	return -1


def processCommand(linea):
	global lastCommandWasTerminator
	parseScriptLine(linea)
	t = tokens
	code = b''
	errBefore = errorCount
	if t[1] == '=':
		if correctLvalue(t[0]) and isNumberOrVar(t[2]):
			code = buildAction(b'\x00' + lVal(t[0]) + pVal(t[2]))
		else:
			syntaxError()
	cmd = t[0].lower()
	scmd = t[1].lower()
	lastCommandWasTerminator = False
	if cmd == 'inc' or (cmd == 'add' and scmd != 'flags'):
		if correctLvalue(t[1]) and isNumberOrVar(t[2]):
			code = buildAction(b'\x01' + lVal(t[1]) + pVal(t[2]))
		else:
			syntaxError()
	elif cmd == 'dec' or (cmd == 'sub' and scmd != 'flags'):
		if correctLvalue(t[1]) and isNumberOrVar(t[2]):
			code = buildAction(b'\x02' + lVal(t[1]) + pVal(t[2]))
		else:
			syntaxError()
	elif cmd == 'add':
		code = buildAction(b'\x01' + pVal(t[2]) + b'\xff' + pVal(t[3]))
	elif cmd == 'sub':
		code = buildAction(b'\x02' + pVal(t[2]) + b'\xff' + pVal(t[3]))
	elif cmd == 'set':
		if scmd in ('tile', 'beh'):
			if t[4] == '=' and isNumberOrVar(t[2]) and isNumberOrVar(t[3]) and isNumberOrVar(t[5]):
				op = b'\x20' if scmd == 'tile' else b'\x21'
				code = buildAction(op + pVal(t[2]) + pVal(t[3]) + pVal(t[5]))
			else:
				syntaxError()
		elif correctLvalue(t[1]) and t[2] == '=' and isNumberOrVar(t[3]):
			code = buildAction(b'\x00' + lVal(t[1]) + pVal(t[3]))
		else:
			syntaxError()
	elif cmd == 'get':
		if scmd == 'item' and correctLvalue(t[3]):
			code = buildAction(b'\x30' + lVal(t[3]))
		else:
			syntaxError()
	elif cmd == 'print':
		if scmd == 'tile' and t[4] == '=' and isNumberOrVar(t[2]) and isNumberOrVar(t[3]) and isNumberOrVar(t[5]):
			code = buildAction(b'\x50' + pVal(t[2]) + pVal(t[3]) + pVal(t[5]))
		else:
			syntaxError()
	elif cmd == 'warp':
		if scmd == 'to' and isNumberOrVar(t[2]) and isNumberOrVar(t[3]) and isNumberOrVar(t[4]):
			code = buildAction(b'\x6d' + pVal(t[2]) + pVal(t[3]) + pVal(t[4]))
		else:
			syntaxError()
	elif cmd == 'sound':
		code = buildAction(b'\xe0' + pVal(t[1]))
	elif cmd == 'show':
		code = buildAction(b'\xe1')
	elif cmd == 'recharge':
		code = buildAction(b'\xe2')
	elif cmd == 'text':
		if scmd == 'box':
			code = buildAction(b'\xe6') + getTextOffset(t[2])
		else:
			s = t[1].encode('ascii', 'replace')
			code = buildAction(b'\xe3') + bytes([1 + len(s)]) + s + b'\x00'
	elif cmd == 'extern':
		code = buildAction(b'\xe4' + pVal(t[1]) + pVal(t[2]))
	elif cmd == 'pause':
		code = buildAction(b'\xe5' + pVal(t[1]))
	elif cmd == 'game':
		if scmd == 'over':
			code = buildAction(b'\xf1')
			lastCommandWasTerminator = True
		else:
			syntaxError()
	elif cmd == 'win':
		if scmd == 'game':
			code = buildAction(b'\xf0')
			lastCommandWasTerminator = True
		else:
			syntaxError()
	elif cmd == 'break':
		code = buildAction(b'\xf2')
		lastCommandWasTerminator = True
	elif cmd == 'rerun':
		if isNumberOrVar(t[1]):
			code = buildAction(b'\xf3' + pVal(t[1]))
			lastCommandWasTerminator = True
		else:
			syntaxError()
	elif cmd == 'call':
		indexSize = ENTERING_INDEX_OFFSET if jumptable else ENTERING_INDEX_OFFSET + maxPants * 2
		a = subAddress(t[1])
		if a < 0:
			reportError('Undefined subroutine %s @ %d (subs must be defined before use)' % (t[1], curLineNo))
		else:
			a += indexSize * 2
			code = buildAction(bytes([0xf4, a & 255, a >> 8]))
	if not code and errorCount == errBefore and len(t[0]) > 0:
		reportError('Unknown command @ line %d: %s' % (curLineNo, linea))
	return code


class LineReader:
	def __init__(self, lines):
		self.lines = lines
		self.i = 0

	def eof(self):
		return self.i >= len(self.lines)

	def read(self):
		global curLineNo
		l = self.lines[self.i]
		self.i += 1
		curLineNo += 1
		return l.strip(' \t\r\n')


def processCurrentSection(f):
	global lastCommandWasTerminator
	section = b''
	clausule = b''
	state = 0
	done = False
	while not done and not f.eof():
		linea = f.read()
		lc = linea.lower()
		if linea == '':
			pass
		elif lc in ('end', 'end if'):
			if state == 0:
				done = True
			elif state == 1:
				done = True
				reportError('Unexpected END @ line %d' % curLineNo)
			elif state == 3:
				clausule += b'\xff'
				state = 2
				dprint('Stopped parsing decos')
			else:
				if not lastCommandWasTerminator:
					clausule += b'\xff'
				if len(clausule) + 1 > 255:
					reportError('Clause too long (%d bytes) ending @ line %d' % (len(clausule) + 1, curLineNo))
				section += bytes([(len(clausule) + 1) & 255]) + clausule
				clausule = b''
				state = 0
		elif lc == 'then':
			if state == 1:
				clausule += b'\xff'
				state = 2
				lastCommandWasTerminator = False
			else:
				done = True
				reportError('Unexpected THEN @ line %d' % curLineNo)
		elif len(lc) > 3 and lc[:3] == 'if ':
			if state == 2:
				done = True
				reportError('Unexpected IF @ line %d' % curLineNo)
			else:
				state = 1
				clausule += processIf(linea)
		elif lc == 'decos':
			if state == 2:
				state = 3
				clausule += b'\x22'
				AU[0x22] = True
				dprint('Began parsing decos')
			else:
				done = True
				reportError('Unexpected DECOS @ line %d' % curLineNo)
		else:
			if state == 3:
				d = parseCoordinatesString(linea)
				clausule += bytes([d[2] & 255, ((d[1] & 15) * 16 + (d[0] & 15)) & 255])
				dprint('Parsed deco %d %d %d' % (d[0], d[1], d[2]))
			elif state == 2:
				if lastCommandWasTerminator:
					print('Warning! unreachable code @ %d' % curLineNo)
				clausule += processCommand(linea)
			elif linea[:1] != '#':
				done = True
				reportError('Unexpected command @ line %d' % curLineNo)
	section += b'\xff'
	return section


def startsWith(spec):
	st = parseTokenizeString(spec, NTOK, '', '')
	i = 0
	while i < NTOK - 1 and st[i] != '':
		if tokens[i] != st[i]:
			return False
		i += 1
	return True


def processScript(lines):
	global tokens, maxPants, noIndexed, fastNPant, itemSlot, itemEmpty, textWidth, sectBinary, mainBinary
	f = LineReader(lines)
	sectBinary = bytearray()
	for i in range(256):
		sectOffs[i] = -1
	while not f.eof():
		linea = f.read()
		tokens = parseTokenizeString(linea.lower(), NTOK, ',_()[]', '#')
		if linea == '' or tokens[0] == '':
			continue
		if debug:
			print('==========================================')
			print('LINEA ' + linea)
			debugPrintTokens()
		listRooms = []
		cError = False
		if tokens[0] == 'rooms':
			maxPants = fbval(tokens[2])
		elif tokens[0] == 'alias':
			tokens = parseTokenizeString(linea.lower(), NTOK, ',()[]', '#')
			if tokens[1][:1] == '%':
				tokens[1] = tokens[1][1:]
			if tokens[1] and tokens[2] == '=' and isNumber(tokens[3]) and tokens[3]:
				aliases[fbval(tokens[3])] = tokens[1].lower()
				dprint('Set alias %' + tokens[1] + ' for flag $' + tokens[3])
			else:
				print('Wrong alias definition at %d' % curLineNo)
		elif tokens[0] == 'noindexed':
			noIndexed = True
		elif tokens[0] == 'fastnpant':
			fastNPant = True
		elif startsWith('item slot') and tokens[2] == '=' and isNumber(tokens[3]):
			itemSlot = fbval(tokens[3])
		elif startsWith('item empty') and tokens[2] == '=' and isNumber(tokens[3]):
			itemEmpty = fbval(tokens[3])
		elif startsWith('text width') and tokens[2] == '=' and isNumber(tokens[3]):
			textWidth = fbval(tokens[3])
		elif tokens[0] == 'sub':
			if tokens[1]:
				print('Adding Sub %s @ %d' % (tokens[1], len(sectBinary)))
				subInfo.append((tokens[1], len(sectBinary)))
				bc = processCurrentSection(f)
				dprint('Sub Bytecode: ' + hexs(bc))
				sectBinary += bc
			else:
				print('Nameless sub at %d' % curLineNo)
		else:
			if startsWith('entering screen') or startsWith('press fire at screen'):
				if tokens[0] == 'entering':
					lt, so = 2, ENTERING_INDEX_OFFSET
				else:
					lt, so = 4, PRESS_FIRE_INDEX_OFFSET
				i = 0
				while i + lt < NTOK - 1 and tokens[i + lt] != '':
					r = tokens[i + lt]
					if isNumber(r):
						r = str(fbval(r) * 2 + so)
					listRooms.append(r)
					i += 1
			else:
				section = -1
				for spec, n in (('entering game', 0), ('entering any', 1), ('press fire at any', 2),
						('player got something', 3), ('player kills enemy', 4),
						('special tile touched', 5), ('enemy touched', 8)):
					if startsWith(spec):
						section = n
						break
				listRooms = [str(section)]
			for r in listRooms:
				if isNumber(r):
					dprint('Adding %d @ sect %s' % (len(sectBinary), r))
					sectOffs[fbval(r)] = len(sectBinary)
				else:
					print('Wrong section at %d' % curLineNo)
					reportError('Wrong section header @ line %d: %s' % (curLineNo, linea))
					cError = True
			if not cError:
				bc = processCurrentSection(f)
				dprint('Bytecode: ' + hexs(bc))
				sectBinary += bc

	mainBinary = bytearray()
	if maxPants == 0 and not noIndexed:
		print('WARNING! undefined # of rooms! The script won\'t work!')
	indexSize = ENTERING_INDEX_OFFSET if jumptable else ENTERING_INDEX_OFFSET + maxPants * 2
	for i in range(indexSize):
		if sectOffs[i] < 0:
			v = 0
		else:
			v = sectOffs[i] + indexSize * 2
		mainBinary += bytes([v & 255, (v >> 8) & 255])
	print('Index section: %d bytes' % len(mainBinary))
	mainBinary += sectBinary
	print('Bytecode section: %d bytes' % len(sectBinary))


# ------------------------------------------------------------ interpreter ---

class Asm:
	def __init__(self):
		self.lines = []

	def w(self, s):
		for t in s.split('|'):
			if t == '':
				self.lines.append('')
			elif t[0] in '.;':
				self.lines.append(t)
			else:
				self.lines.append('\t' + t)


def writeInterpreter(fn):
	a = Asm()
	w = a.w
	w('defc PLAYER_LIFE=99 ;; Find a way to solve this')
	w('; Imports|XREF _flags|XREF _n_pant|XREF _on_pant|XREF _gpx|XREF _gpy|XREF _tpx|XREF _tpy|XREF _tat|XREF _tqt|XREF _player|XREF _attr_2|XREF qtile_do|XREF set_map_tile_do|XREF _draw_coloured_tile|XREF __x|XREF __y|XREF __t|XREF __n|XREF _comportamiento_tiles|XREF _map_attr|XREF _peta_el_beeper|XREF _do_extern_action|XREF draw_line_of_text|XREF _hotspot_t|XREF _scenery_info|XREF __en_t|XREF _enit|XREF __en_x|XREF __en_y|XREF _script_param')
	if RV[0xE6] or AU[0xE6]:
		w('; 5bit text decoder|XREF _run_text_box')
	if RV[0xE6]:
		w('; Dialogs|XREF _addr1|XREF _addr2|XREF _addr3|XREF _run_dialog')
	w('XREF script_bytecode')
	if outT == SPECCY:
		w('; Target SPECCY|LIB SPUpdateNow')
	else:
		w('; Target CPC|XREF _cpc_UpdateNow')
	w('; Exports|XDEF _script_do|XDEF _script_n|XDEF _script_result')
	w('._script_n|defw 0|._script_result|defb 0')
	w('; Script pointer|.script|defw 0')
	w('; Skip to next clausule|.skip|defw 0')
	w('; Coordinate pair|.sc_x|defb 0|.sc_y|defb 0')
	w('; From the engine|._script_tx|defb 0|._script_ty|defb 0|._script_tn|defb 0')
	w('; Control|.sc_terminado|defb 0')

	if jumptable:
		w('.script_jump_table|ld  a, (_script_n)')
		isEntering = True
		room = 0
		for i in range(ENTERING_INDEX_OFFSET, ENTERING_INDEX_OFFSET + maxPants * 2):
			if sectOffs[i] >= 0:
				w(('; ENTERING %d' if isEntering else '; FIRE @   %d') % room)
				w('ld  hl, 0x%04X|cp  %d|ret z' % (sectOffs[i] + ENTERING_INDEX_OFFSET * 2, i))
			isEntering = not isEntering
			if isEntering:
				room += 1
		w('ld  a, 0xff|ret')
		w('._script_do')
		w('ld  a, (_script_n)|cp  %d|jr  nc, get_from_jump_table' % ENTERING_INDEX_OFFSET)
		w('.get_from_index|; Point to offset in script index|ld  hl, (_script_n)|add hl, hl|ld  bc, script_bytecode|add hl, bc')
		w('; Read offset|ld  a, (hl)|inc hl|ld  h, (hl)|ld  l, a')
		w(';  If zero do abort|or  h|ret z')
		w('jr make_pointer')
		w('.get_from_jump_table|call script_jump_table')
		w('; If no script, A = 0xff|cp 0xff|ret z')
		w('.make_pointer|; Make & store pointer')
		w('ld  bc, script_bytecode|add hl, bc|ld  (script), hl')
	else:
		w('._script_do')
		w('; Point to offset in script index|ld  hl, (_script_n)|add hl, hl|ld  bc, script_bytecode|add hl, bc')
		w('; Read offset|ld  a, (hl)|inc hl|ld  h, (hl)|ld  l, a')
		w(';  If zero do abort|or  h|ret z')
		w('; Make & store pointer|add hl, bc|ld  (script), hl')

	w('.script_loop|; Calculate address of next clausule')
	w('ld  hl, (script)|push hl')
	w('call read_byte \t\t; A = clausule size|ld  b, 0|ld  c, a')
	w('pop hl|add hl, bc|ld  (skip), hl')
	w('cp  0xFF \t\t\t; End of section?|ret z')
	w('; Process conditions|.script_clausule|call read_byte \t\t;A = opcode')
	w('; If we get to 0xFF (THEN), jump to actions|cp  0xFF|jp  z, script_actions')
	w(';;; Decode OPCODE & jump to interpreter')

	if CU[0x01]: w(';; OPCODE 0x01|;; IF A = B|cp  0x01|jr  nz, copcode_01_end|.copcode_01|call read_vbyte|ld  b, a|call read_vbyte|cp  b|jp  nz, skip_clausule|jp  script_clausule|.copcode_01_end')
	if CU[0x02]: w(';; OPCODE 0x02|;; IF A < B|cp  0x02|jr  nz, copcode_02_end|.copcode_02|call read_vbyte|ld  c, a|call read_vbyte|ld  b, a|ld  a, c|cp  b|jp  nc, skip_clausule|jp  script_clausule|.copcode_02_end')
	if CU[0x03]: w(';; OPCODE 0x03|;; IF A >= B|cp  0x03|jr  nz, copcode_03_end|.copcode_03|call read_vbyte|ld  c, a|call read_vbyte|ld  b, a|ld  a, c|cp  b|jp  c, skip_clausule|jp  script_clausule|.copcode_03_end')
	if CU[0x04]: w(';; OPCODE 0x04|;; IF A <> B|cp  0x04|jr  nz, copcode_04_end|.copcode_04|call read_vbyte|ld  b, a|call read_vbyte|cp  b|jp  z, skip_clausule|jp  script_clausule|.copcode_04_end')
	if CU[0x05]: w(';; OPCODE 0x05|;; IF NPANT = N|cp  0x05|jr  nz, copcode_05_end|.copcode_05|call read_vbyte|ld  b, a|ld  a, (_n_pant)|cp  b|jr  nz, skip_clausule|jp script_clausule|.copcode_05_end')
	if CU[0x21]: w(';; OPCODE 0x21|;; IF PLAYER IN_X (X1, X2)|cp  0x21|jr  nz, copcode_21_end|.copcode_21|;; gpx < X1 -> exit|call read_vbyte|ld  c, a|ld  a, (_gpx)|cp  c|jp  c, skip_clausule|;; X2 < gpx -> exit|ld  a, (_gpx)|ld  c, a|call read_vbyte|cp  c|jp  c, skip_clausule|jp  script_clausule|.copcode_21_end')
	if CU[0x22]: w(';; OPCODE 0x22|;; IF PLAYER IN_Y (Y1, Y2)|cp  0x22|jr  nz, copcode_22_end|.copcode_22|;; gpy < Y1 -> exit|call read_vbyte|ld  c, a|ld  a, (_gpy)|cp  c|jp  c, skip_clausule|;; Y2 < gpy -> exit|ld  a, (_gpy)|ld  c, a|call read_vbyte|cp  c|jp  c, skip_clausule|jp  script_clausule|.copcode_22_end')
	if CU[0x23]: w(';; OPCODE 0x23|;; IF PLAYER AT (X, Y)|cp  0x23|jr  nz, copcode_23_end|.opcode23|;; (gpx + 8) >> 4 != X -> exit|ld  a, (_tpx)|ld  c, a|call read_vbyte|cp  c|jp  nz, skip_clausule|;; (gpy + 8) >> 4 != Y -> exit|ld  a, (_tpy)|ld  c, a|call read_vbyte|cp  c|jp  nz, skip_clausule|jp  script_clausule|.copcode_23_end')
	if CU[0x24]: w(';; OPCODE 0x24|;; IF PLAYER FALLING (vy > 0 and not possee)|cp  0x24|jr  nz, copcode_24_end|.copcode_24|ld  hl, (_player + 8) \t; player.vy|ld  a, h|and 0x80|jp  nz, copcode_24_false\t; vy < 0|ld  a, l|or  a|jp  nz, copcode_24_moving|ld  a, h|or  a|jp  z, copcode_24_false\t; vy == 0|.copcode_24_moving|ld  a, (_player + 26) \t; player.possee|or  a|jp  nz, skip_clausule\t; standing on something|jp  script_clausule|.copcode_24_false|jp  skip_clausule|.copcode_24_end')
	if CU[0x25]: w(';; OPCODE 0x25|;; IF PLAYER NOT FALLING (vy <= 0 or possee)|cp  0x25|jr  nz, copcode_25_end|.copcode_25|ld  a, (_player + 26) \t; player.possee|or  a|jp  nz, script_clausule\t; standing on something|ld  hl, (_player + 8) \t; player.vy|ld  a, h|and 0x80|jp  nz, script_clausule\t; vy < 0|ld  a, l|or  a|jp  nz, skip_clausule\t; vy > 0|ld  a, h|or  a|jp  nz, skip_clausule\t; vy > 0|jp  script_clausule\t; vy == 0|.copcode_25_end')
	if CU[0x26]: w(';; OPCODE 0x26|;; PLAYER_STILL|cp  0x26|jr  nz, copcode_26_end|.copcode_26|ld  a, (_player + 6) \t; player.vx LSB|ld  hl, (_player + 7)\t; player.vx MSB|or  (hl)|ld  hl, (_player + 8) \t; player.vy LSB|or  (hl)|ld  hl, (_player + 9) \t; player.vy MSB|or  (hl)|jp  nz, skip_clausule|jp  script_clausule|.copcode_26_end')
	if CU[0xF0]: w(';; OPCODE 0xF0|;; TRUE|cp  0xf0|jr  nz, copcode_F0_end|.copcode_F0|jp  script_clausule|.copcode_F0_end')

	w(';; UNKNOWN|jp  script_clausule')
	w('.skip_clausule|ld  hl, (skip)|ld  (script), hl|jp  script_loop')
	w('; Process actions|.script_actions|call read_byte \t\t;A = opcode')
	w('; If we get to 0xFF (END), jump to next clausule|cp  0xFF|jp  z, script_loop')
	w(';;; Decode OPCODE & jump to interpreter')

	if AU[0x00]: w(';; OPCODE 0x00|;; FLAGS[N] = V|cp  0x00|jr  nz, aopcode_00_end|.aopcode_00|call read_i_v\t\t; HL -> FLAGS[N], A -> V|ld  (hl), a|jp  script_actions|.aopcode_00_end')
	if AU[0x01]: w(';; OPCODE 0x01|;; FLAGS[N] += V|cp  0x01|jr  nz, aopcode_01_end|.aopcode_01|call read_i_v\t\t; HL -> FLAGS[N], A -> V|ld  b, (hl)|add b|ld  (hl), a|jp  script_actions|.aopcode_01_end')
	if AU[0x02]: w(';; OPCODE 0x02|;; FLAGS[N] -= V|cp  0x02|jr  nz, aopcode_02_end|.aopcode_02|call read_i_v\t\t; HL -> FLAGS[N], A -> V|ld  b, a|ld  a, (hl)|sub b|ld  (hl), a|jp  script_actions|.aopcode_02_end')
	if AU[0x20]: w(';; OPCODE 0x20|;; SET TILE (X, Y) = T|cp  0x20|jr  nz, aopcode_20_end|.aopcode_20|call read_x_y|call read_vbyte|ld  (__t), a|ld  b, 0|ld  c, a|ld  hl, _comportamiento_tiles|add hl, bc|ld  a, (hl)|ld  (__n), a|ld  a, (sc_x)|ld  (__x), a|ld  c, a|ld  a, (sc_y)|ld  (__y), a|call set_map_tile_do|jp  script_actions|.aopcode_20_end')
	if AU[0x21]: w(';; OPCODE 0x21|;; SET BEH (X, Y) = B|cp  0x21|jr  nz, aopcode_21_end|.aopcode_21|call read_x_y|ld  a, (sc_x)|ld  c, a|ld  a, (sc_y)|ld  b, a|sla a|sla a|sla a|sla a|sub b|add c|ld  b, 0|ld  c, a|call read_vbyte|ld  hl, _map_attr|add hl, bc|ld  (hl), a|jp  script_actions|.aopcode_21_end')
	if AU[0x22]: w(';; OPCODE 0x22|;; DECOS XY T XY T ... 0xFF|cp  0x22|jr  nz, aopcode_22_end|.aopcode_22|call read_byte|cp  0xff|jr  z, aopcode_22_end|ld  (__t), a|ld  b, 0|ld  c, a|ld  hl, _comportamiento_tiles|add hl, bc|ld  a, (hl)|ld  (__n), a|call read_byte|ld  b, a|and 0xf|ld  c, a|ld  (__x), a|ld  a, b|srl a|srl a|srl a|srl a|ld  (__y), a|call set_map_tile_do|jr  aopcode_22|.aopcode_22_end')
	if AU[0x30]: w(';; OPCODE 0x30|;; GET ITEM SET $F <- [FILL I]|cp  0x30|jr  nz, aopcode_30_end|.aopcode_30|;; Get LValue: Flag to modify|call read_vbyte|ld  b, 0|ld  c, a|ld  hl, _flags|add hl, bc|; No item in slot?|ld  a, (_flags + %d)|cp  %d|jr  nz, aopcode_30_end|; Write 1 to LValue|inc a|ld  (hl), a|; Assign item|ld  a, (_tqt)|ld  (_flags + %d), a|; Clear from screen|xor a|ld  (__n), a|ld  (__t), a|ld  a, (_tpx)|ld  c, a|ld  (__x), a|ld  a, (_tpy)|ld  (__y), a|call set_map_tile_do|jp  script_actions|.aopcode_30_end' % (itemSlot, itemEmpty, itemSlot))
	if AU[0x50]: w(';; OPCODE 0x50|;; PRINT TILE (X, Y) = N|cp  0x50|jr  nz, aopcode_50_end|.aopcode_50|call read_vbyte|ld  h, 0|ld  l, a|push hl|call read_vbyte|ld  h, 0|ld  l, a|push hl|call read_vbyte|ld  h, 0|ld  l, a|push hl|call _draw_coloured_tile|pop bc|pop bc|pop bc|jp  script_actions|.aopcode_50_end')
	if AU[0x6D]: w(';; OPCODE 0x6D|;; WARP TO N, X, Y|cp  0x6D|jr  nz, aopcode_6D_end|.aopcode_6D|call read_vbyte|ld  (_n_pant), a|call read_vbyte|sla a|sla a|sla a|sla a|ld  (_gpx), a|call read_vbyte|sla a|sla a|sla a|sla a|ld  (_gpy), a|jp  script_actions|.aopcode_6D_end')
	if AU[0xE0]: w(';; OPCODE 0xE0|;; SOUND N|cp  0xE0|jr  nz, aopcode_E0_end|.aopcode_E0|call read_vbyte|ld  h, 0|ld  l, a|call _peta_el_beeper|jp  script_actions|.aopcode_E0_end')
	if AU[0xE1]:
		if outT == SPECCY:
			w(';; OPCODE 0xE1|;; SHOW|cp  0xE1|jr  nz, aopcode_E1_end|.aopcode_E1|ld  hl, 1|push hl|call SPUpdateNow|pop bc|jp  script_actions|.aopcode_E1_end')
		else:
			w(';; OPCODE 0xE1|;; SHOW|cp  0xE1|jr  nz, aopcode_E1_end|.aopcode_E1|ld  hl, 1|push hl|call _cpc_UpdateNow|pop bc|jp  script_actions|.aopcode_E1_end')
	if AU[0xE2]: w(';; OPCODE 0xE2|;; RECHARGE|cp  0xE2|jr  nz, aopcode_E2_end|.aopcode_E2|ld  a, PLAYER_LIFE|ld  (_player + 29), a \t; player.life LSB|xor a|ld  (_player + 30), a \t; player.life MSB|jp script_actions|.aopcode_E2_end')
	if AU[0xE3]: w(';; OPCODE 0xE3|;; TEXT L <CHARS> 0|cp  0xE3|jr  nz, aopcode_E3_end|.aopcode_E3|call read_byte \t\t\t; String length|ld  b, 0|ld  c, a|add hl, bc \t\t\t\t; Move after the string|push hl|ld  hl, (script)|call draw_line_of_text|pop hl|ld  (script), hl \t\t; Get past the string|jp script_actions|.aopcode_E3_end')
	if AU[0xE4]: w(';; OPCODE 0xE4|;; EXTERN N M|cp  0xE4|jr  nz, aopcode_E4_end|.aopcode_E4|call read_x_y|ld  a, (sc_x)|ld  h, 0|ld  l, a|push hl|ld  a, (sc_y)|ld  h, 0|ld  l, a|push hl|call _do_extern_action|pop bc|pop bc|jp script_actions|.aopcode_E4_end')
	if AU[0xE5]: w(';; OPCODE 0xE5|;; PAUSE N|cp  0xE5|jr  nz, aopcode_E5_end|.aopcode_E5|call read_vbyte|ld  b, a|.aopcode_E5_loop|halt|djnz aopcode_E5_loop|jp script_actions|.aopcode_E5_end')
	if AU[0xE6]: w(';; OPCODE 0xE6|;; TEXT BOX LSB MSB|cp  0xE6|jr  nz, aopcode_E6_end|.aopcode_E6|call read_addr|call _run_text_box|jp script_actions|.aopcode_E6_end')
	if AU[0xF0]: w(';; OPCODE 0xF0|;; WIN GAME|cp  0xf0|jr  nz, aopcode_F0_end|.aopcode_F0|ld  a, 1|ld  (_script_result), a|ret|.aopcode_F0_end')
	if AU[0xF1]: w(';; OPCODE 0xF1|;; GAME OVER|cp  0xf1|jr  nz, aopcode_F1_end|.aopcode_F1|ld  a, 2|ld  (_script_result), a|ret|.aopcode_F1_end')
	if AU[0xF2]: w(';; OPCODE 0xF2|;; BREAK|cp  0xf2|jr  nz, aopcode_F2_end|.aopcode_F2|ret|.aopcode_F2_end')
	if AU[0xF3]: w(';; OPCODE 0xF3|;; RERUN v|cp  0xf3|jr  nz, aopcode_F3_end|.aopcode_F3|call read_vbyte|ld  (_script_param), a|jp  _script_do|.aopcode_F3_end')
	if AU[0xF4]: w(';; OPCODE 0xF4|;; CALL lsb msb|cp  0xf4|jr  nz, aopcode_F4_end|.aopcode_F4|call read_addr \t; Subroutine offset in HL|ex  de, hl \t\t; Subroutine offset in DE|; Save script pointer|ld  hl, (script)|push hl|; Calculate new script ponter|ld  hl, script_bytecode|add hl, de|ld  (script), hl|; Call this interpreter recursively!|call script_loop|; Restore script pointer|pop hl|ld  (script), hl|jp script_actions|.aopcode_F4_end')

	w(';; UNKNOWN|jp script_actions')
	w(';; Reads a byte from pointer, inc pointer, return value in A|.read_byte|ld  hl, (script)|ld  a, (hl)|inc hl|ld  (script), hl|ret')
	w(';; Reads a value (may be recursive flag), inc pointer, return value in A|;; New flags encoding is $FF means next value is flag (can be $FF, etc)|.read_vbyte|call read_byte|cp  0xff|jr  z, read_vbyte_rec|ret')
	w('.read_vbyte_rec|call read_vbyte')
	if RV[0xFE]: w('; NPANT RVALUE|cp  0xFE|jr  nz, rvb_set_n_pant_done|ld  a, (_n_pant)|ret|.rvb_set_n_pant_done')
	if RV[0xFD]: w('; PX RVALUE|cp  0xFD|jr  nz, rvb_set_gpx_done|ld  a, (_gpx)|ret|.rvb_set_gpx_done')
	if RV[0xFC]: w('; PY RVALUE|cp  0xFC|jr  nz, rvb_set_gpy_done|ld  a, (_gpy)|ret|.rvb_set_gpy_done')
	if RV[0xFB]: w('; KILLED RVALUE|cp  0xFB|jr  nz, rvb_set_player_killed_done|ld  a, (_player + 32) \t; player.killed|ret|.rvb_set_player_killed_done')
	if RV[0xFA]: w('; OBJS RVALUE|cp  0xFA|jr  nz, rvb_set_player_objs_done|ld  a, (_player + 27) \t; player.objs|ret|.rvb_set_player_objs_done')
	if RV[0xF9]: w('; LIFE RVALUE|cp  0xF9|jr  nz, rvb_set_player_life_done|ld  a, (_player + 29) \t; player.life MSB|ret|.rvb_set_player_life_done')
	if RV[0xF8]: w('; TX RVALUE|cp  0xF8|jr  nz, rvb_set_tx_done|ld  a, (_tpx)|ret|.rvb_set_tx_done')
	if RV[0xF7]: w('; TY RVALUE|cp  0xF7|jr  nz, rvb_set_ty_done|ld  a, (_tpy)|ret|.rvb_set_ty_done')
	if RV[0xF6]: w('; TN RVALUE|cp  0xF6|jr  nz, rvb_set_tile_done|ld  a, (_tqt)|ret|.rvb_set_tile_done')
	if RV[0xF5]: w('; HOTSPOT RVALUE|cp  0xF5|jr  nz, rvb_set_hotspot_done|ld  a, (_hotspot_t)|ret|.rvb_set_hotspot_done')
	# QUIRK: the original's label for HIDE_HOTSPOTS does not match its jump
	# (it would not assemble). Fixed here.
	if RV[0xF0]: w('; HIDE_HOTSPOTS RVALUE|cp  0xF0|jr  nz, rvb_set_hide_hotspots_done|ld  a, (_scenery_info + 0)|ret|.rvb_set_hide_hotspots_done')
	if RV[0xEF]: w('; DONT_MAKE_FANTIES RVALUE|cp  0xEF|jr  nz, rvb_set_dont_make_fanties_done|ld  a, (_scenery_info + 1)|ret|.rvb_set_dont_make_fanties_done')
	if RV[0xEE]: w('; EN_T RVALUE|cp  0xEE|jr  nz, rvb_set_en_t_done|ld  a, (__en_t)|ret|.rvb_set_en_t_done')
	if RV[0xED]: w('; EN_N RVALUE|cp  0xED|jr  nz, rvb_set_en_n_done|ld  a, (_enit)|ret|.rvb_set_en_n_done')
	if RV[0xEC]: w('; EN_X RVALUE|cp  0xEC|jr  nz, rvb_set_en_x_done|ld  a, (__en_x)|ret|.rvb_set_en_x_done')
	if RV[0xEB]: w('; EN_Y RVALUE|cp  0xEB|jr  nz, rvb_set_en_y_done|ld  a, (__en_y)|ret|.rvb_set_en_y_done')
	if RV[0xEA]: w('; OPANT RVALUE|cp  0xEA|jr  nz, rvb_set_on_pant_done|ld  a, (_on_pant)|ret|.rvb_set_on_pant_done')
	if RV[0xE9]: w('; PARAM RVALUE|cp  0xE9|jr  nz, rvb_set_script_param_done|ld  a, (_script_param)|ret|.rvb_set_script_param_done')
	if RV[0xE8]: w('; TILE AT EXPRESION RVALUE|cp 0xE8|jr  nz, rvb_set_script_tile_at_done|push bc|; Read two rvalues|call read_x_y|ld  a, (sc_x)|ld  c, a|ld  a, (sc_y)|call qtile_do|ld  a, l|pop bc|ret|.rvb_set_script_tile_at_done')
	if RV[0xE7]: w('; BEH AT EXPRESION RVALUE|cp 0xE7|jr  nz, rvb_set_script_beh_at_done|push bc|; Read two rvalues|call read_x_y|ld  a, (sc_x)|ld  c, a|ld  a, (sc_y)|call _attr_2|ld  a, l|pop bc|ret|.rvb_set_script_beh_at_done')
	if RV[0xE6]: w('; DIALOG EXPRESION RVALUE|cp 0xE6|jr  nz, rvb_set_script_dialog_done|; Read three addresses|call read_addr|ld  (_addr1), hl|call read_addr|ld  (_addr2), hl|call read_addr|ld  (_addr3), hl|; this is a C function in the engine:|call _run_dialog|; run dialog returns result in L|ld  a, l|ret|.rvb_set_script_dialog_done')

	w('ld  d, 0|ld  e, a|ld  hl, _flags|add hl, de|ld  a, (hl)|ret')
	w('.read_x_y|call read_vbyte|ld  (sc_x), a|call read_vbyte|ld  (sc_y), a|ret')
	if AU[0xE6] or AU[0x70] or RV[0xE6] or RV[0x70] or AU[0xF4]:
		w('.read_addr|call read_byte|ld  c, a|call read_byte|ld  h, a|ld  l, c|ret')
	w(';; Read flag index and value, returns pointer in HL and value in A.|.read_i_v|call read_vbyte  \t\t; Read flag index|ld  c, a|call read_vbyte \t\t; Read value|ld  (sc_y), a')
	w('ld  a, c  \t\t\t\t; C = flag index')
	if LV[0xFE]: w('; NPANT LVALUE|cp  0xFE|jr  nz, riv_set_n_pant_done|ld  hl, _n_pant|jr  read_i_v_cont|.riv_set_n_pant_done')
	if LV[0xFD]: w('; PX LVALUE|cp  0xFD|jr  nz, riv_set_gpx_done|ld  hl, _gpx|jr  read_i_v_cont|.riv_set_gpx_done')
	if LV[0xFC]: w('; PY LVALUE|cp  0xFC|jr  nz, riv_set_gpy_done|ld  hl, _gpy|jr  read_i_v_cont|.riv_set_gpy_done')
	if LV[0xFB]: w('; KILLED LVALUE|cp  0xFB|jr  nz, riv_set_player_killed_done|ld  hl, _player + 32\t; player.killed|jr  read_i_v_cont|.riv_set_player_killed_done')
	if LV[0xFA]: w('; OBJS LVALUE|cp  0xFA|jr  nz, riv_set_player_objs_done|ld  hl, _player + 27\t; player.objs|jr  read_i_v_cont|.riv_set_player_objs_done')
	if LV[0xF9]: w('; LIFE LVALUE|cp  0xF9|jr  nz, riv_set_player_life_done|ld  hl, _player + 29\t; player.life LSB|jr  read_i_v_cont|.riv_set_player_life_done')
	if LV[0xF0]: w('; HIDE_HOTSPOTS LVALUE|cp  0xF0|jr  nz, riv_set_hide_hotspots_done|ld  hl, _scenery_info + 0\t; scenery_info.hide_hotspots|jr  read_i_v_cont|.riv_set_hide_hotspots_done')
	if LV[0xEF]: w('; DONT_MAKE_FANTIES LVALUE|cp  0xEF|jr  nz, riv_set_dont_make_fanties_done|ld  hl, _scenery_info + 1\t; scenery_info.dont_make_rr|jr  read_i_v_cont|.riv_set_dont_make_fanties_done')
	if LV[0xEE]: w('; EN_T LVALUE|cp  0xEE|jr  nz, riv_set_en_t_done|ld  hl, __en_t|jr  read_i_v_cont|.riv_set_en_t_done')
	if LV[0xEC]: w('; EN_X LVALUE|cp  0xEC|jr  nz, riv_set_en_x_done|ld  hl, __en_x|jr  read_i_v_cont|.riv_set_en_x_done')
	if LV[0xEB]: w('; EN_Y LVALUE|cp  0xEB|jr  nz, riv_set_en_y_done|ld  hl, __en_y|jr  read_i_v_cont|.riv_set_en_y_done')
	if LV[0xEA]: w('; OPANT LVALUE|cp  0xEA|jr  nz, riv_set_on_pant_done|ld  hl, _on_pant|jr  read_i_v_cont|.riv_set_on_pant_done')
	if LV[0xE9]: w('; PARAM LVALUE|cp  0xE9|jr  nz, riv_set_cript_param_done|ld  hl, _script_param|jr  read_i_v_cont|.riv_set_cript_param_done')
	w('ld  b, 0 \t\t\t\t; BC = flag index|ld  hl, _flags|add hl, bc \t\t\t\t; HL -> FLAGS [X]')
	w('.read_i_v_cont|ld  a, (sc_y) \t\t\t; A = value|ret')
	open(fn, 'w').write('\n'.join(a.lines) + '\n')


def main():
	global debug, textdebug, outT, noIndexed, fastNPant, jumptable, maxPants, curLineNo
	opts = {}
	for a in sys.argv[1:]:
		if '=' in a:
			k, v = a.split('=', 1)
			opts[k] = v
		else:
			opts[a] = '1'
	print('msc v4.6.20260729 (Python port) ~ ', end='')
	if not all(k in opts for k in ('in', 'v', 'target')):
		print('usage: msc4.py in=script.spt v=3 target=zx|cpc [rooms=N] [text=text.bin] [interpreter=msc4i.asm] [noindexed] [fastnpant] [jumptable] [debug] [textdebug]')
		sys.exit(1)
	debug = 'debug' in opts
	textdebug = 'textdebug' in opts
	interp = opts.get('interpreter', 'msci.asm')
	outT = CPC if opts['target'] == 'cpc' else SPECCY
	noIndexed = 'noindexed' in opts
	fastNPant = 'fastnpant' in opts
	jumptable = 'jumptable' in opts
	textfn = opts.get('text', 'text.bin')
	for fn in opts['in'].split(','):
		curLineNo = 0
		for i in range(len(aliases)):
			aliases[i] = ''
		maxPants = fbval(opts.get('rooms', '0'))
		print('Processing ' + fn)
		processScript(open(fn).read().splitlines())
		if errorCount == 0:
			open(fn + '.bin', 'wb').write(mainBinary)
			print('Wrote %d bytes to %s.bin' % (len(mainBinary), fn))
		else:
			print('NOT writing %s.bin because of errors' % fn)
		if debug:
			for i, a in enumerate(aliases):
				if a:
					print('%%%s -> $%d' % (a, i))
	if textPool:
		print('Exporting encoded text')
		data = b''.join(textPool)
		open(textfn, 'wb').write(data)
		print('Wrote %d bytes to %s' % (len(data), textfn))
	if errorCount > 0:
		print('*** %d ERROR(S) FOUND! Not writing the interpreter. ***' % errorCount)
		sys.exit(1)
	print('Writing interpreter')
	writeInterpreter(interp)
	print('DONE!')


if __name__ == '__main__':
	main()
