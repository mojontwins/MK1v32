' cpctape v0.1 [MK1 v3.2]
' Copyleft 2026 by The Mojon Twins
'
' Adds a file to a .cdt (TZX) tape image for the Amstrad CPC:
'
'   mode=fw    a firmware file (RUN"" / LOAD"" from BASIC): 2K blocks, each a
'              header record and a data record, 256 byte segments with their
'              CRC (CCITT), 1000 baud.
'   mode=raw   a raw block for cpct_miniload (raw1full): pilot, 2 short sync
'              pulses, then 2 pulses per bit (a 0: 2 x rl, a 1: 2 x 2 rl,
'              rl in T-states at 3.5 MHz, default 740), MSB first, and a last
'              byte making the XOR of everything 255. Load it with the exact
'              size of the file.
'
' Compile with fbc cpctape.bas cmdlineparser.bas

#include "file.bi"
#include "cmdlineparser.bi"

Sub usage
	Print "usage:"
	Print
	Print "$ cpctape in=file.bin out=game.cdt mode=fw|raw [new] [name=NAME]"
	Print "          [load=1024] [exec=1024] [rl=740] [pause=2000]"
	Print
	Print "in      the file to add"
	Print "out     the .cdt: appended to, created if it does not exist (or with 'new')"
	Print "mode    fw: a firmware binary file (name, load, exec)"
	Print "        raw: a cpct_miniload block (rl: pulse length, T-states at 3.5 MHz)"
	Print "pause   silence after the block, ms"
	Print "Numbers in decimal, 0x... or &h..."
End Sub

Function num (s As String, def As Integer) As Integer
	If s = "" Then Return def
	If Lcase (Left (s, 2)) = "0x" Then Return Val ("&h" & Mid (s, 3))
	Return Val (s)
End Function

Dim Shared As Integer fOut

Sub putB (v As Integer)
	Dim As uByte b = v And 255
	Put #fOut, , b
End Sub

Sub putW (v As Integer)
	putB v: putB v Shr 8
End Sub

' TZX block 0x11 (turbo speed data)
Sub turbo (pilot As Integer, sync1 As Integer, sync2 As Integer, zero As Integer, one As Integer, pilotN As Integer, pauseMs As Integer, d() As uByte, n As Integer)
	Dim As Integer i
	putB &h11
	putW pilot: putW sync1: putW sync2: putW zero: putW one: putW pilotN
	putB 8: putW pauseMs
	putB n: putB n Shr 8: putB n Shr 16
	For i = 0 To n - 1: Put #fOut, , d (i): Next i
End Sub

Function crc16 (d() As uByte, ofs As Integer) As Integer
	Dim As Integer c = &hFFFF, i, j
	For i = 0 To 255
		c = c Xor (d (ofs + i) Shl 8)
		For j = 0 To 7
			If c And &h8000 Then c = ((c Shl 1) Xor &h1021) And &hFFFF Else c = (c Shl 1) And &hFFFF
		Next j
	Next i
	Return c Xor &hFFFF
End Function

' A firmware record: sync byte, segments of 256 + CRC, 4 bytes trailer
Sub fwRecord (syncB As Integer, src() As uByte, ofs As Integer, n As Integer, pauseMs As Integer)
	Dim As Integer segs = (n + 255) \ 256, i, j, c, p
	Dim As uByte seg (255)
	Dim As uByte d (segs * 258 + 5)
	p = 0: d (p) = syncB: p += 1
	For i = 0 To segs - 1
		For j = 0 To 255
			If i * 256 + j < n Then seg (j) = src (ofs + i * 256 + j) Else seg (j) = 0
			d (p) = seg (j): p += 1
		Next j
		c = crc16 (seg (), 0)
		d (p) = c Shr 8: d (p + 1) = c And 255: p += 2
	Next i
	For i = 0 To 3: d (p) = 255: p += 1: Next i
	' 1000 baud: a 0 is 2 x 333 us, a 1 2 x 667 us (T-states at 3.5 MHz)
	turbo 2333, 1167, 1167, 1167, 2333, 4096, pauseMs, d (), p
End Sub

' GO!

Dim As Integer fIn, l, i, nBlocks, blk, bl, rl, pauseMs, loadA, execA, x
Dim As String mode, nm

sclpParseAttrs
If sclpGetValue ("in") = "" Or sclpGetValue ("out") = "" Then usage: End

mode = Lcase (sclpGetValue ("mode"))
If mode <> "fw" And mode <> "raw" Then usage: End

fIn = FreeFile
If Open (sclpGetValue ("in") For Binary Access Read As fIn) Then Print "Can't open " & sclpGetValue ("in"): End 1
l = Lof (fIn)
Dim As uByte dat (l + 1)
For i = 0 To l - 1: Get #fIn, , dat (i): Next i
Close fIn

pauseMs = num (sclpGetValue ("pause"), 2000)

If sclpGetValue ("new") <> "" Or Not FileExists (sclpGetValue ("out")) Then
	If FileExists (sclpGetValue ("out")) Then Kill sclpGetValue ("out")
	fOut = FreeFile
	Open sclpGetValue ("out") For Binary As fOut
	Dim As String sig = "ZXTape!"
	Put #fOut, , sig
	putB &h1A: putB 1: putB 20
Else
	fOut = FreeFile
	Open sclpGetValue ("out") For Binary As fOut
	Seek #fOut, Lof (fOut) + 1
End If

If mode = "raw" Then
	rl = num (sclpGetValue ("rl"), 740)
	x = 0
	For i = 0 To l - 1: x = x Xor dat (i): Next i
	dat (l) = 255 Xor x
	' pilot: 2 rl pulses (miniload's tone), 2 short sync pulses
	turbo 2 * rl, rl, rl, rl, 2 * rl, 2048, pauseMs, dat (), l + 1
	Print "cpctape: " & sclpGetValue ("in") & " (" & l & " bytes) raw, " & rl & " T"
Else
	nm = Ucase (sclpGetValue ("name"))
	If nm = "" Then nm = "MK1"
	loadA = num (sclpGetValue ("load"), 1024)
	execA = num (sclpGetValue ("exec"), loadA)
	nBlocks = (l + 2047) \ 2048
	For blk = 0 To nBlocks - 1
		bl = l - blk * 2048: If bl > 2048 Then bl = 2048
		Dim As uByte h (63)
		For i = 0 To 63: h (i) = 0: Next i
		For i = 0 To 15
			If i < Len (nm) Then h (i) = Asc (Mid (nm, i + 1, 1))
		Next i
		h (16) = blk + 1
		If blk = nBlocks - 1 Then h (17) = 255
		h (18) = 2 						' binary
		h (19) = bl And 255: h (20) = bl Shr 8
		h (21) = (loadA + blk * 2048) And 255: h (22) = ((loadA + blk * 2048) Shr 8) And 255
		If blk = 0 Then h (23) = 255
		h (24) = l And 255: h (25) = (l Shr 8) And 255
		h (26) = execA And 255: h (27) = (execA Shr 8) And 255
		fwRecord &h2C, h (), 0, 64, 15
		fwRecord &h16, dat (), blk * 2048, bl, IIf (blk = nBlocks - 1, pauseMs, 15)
	Next blk
	Print "cpctape: " & sclpGetValue ("in") & " (" & l & " bytes) as " & nm & ", " & nBlocks & " blocks"
End If

Close fOut
