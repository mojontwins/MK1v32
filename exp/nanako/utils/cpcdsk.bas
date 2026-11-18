' cpcdsk v0.1 [MK1 v3.2]
' Copyleft 2026 by The Mojon Twins
'
' Makes a .dsk image for the Amstrad CPC: DATA format (40 tracks, 1 side,
' 9 x 512 byte sectors C1-C9, 1K blocks, 64 directory entries) with binary
' files with their AMSDOS header (RUN"NAME runs them).
'
' Compile with fbc cpcdsk.bas cmdlineparser.bas

#include "file.bi"
#include "cmdlineparser.bi"

Sub usage
	Print "usage:"
	Print
	Print "$ cpcdsk out=game.dsk file1=game.bin,GAME.BIN,1024,1024 [file2=...] ..."
	Print
	Print "fileN   lname file, name on the disk (8.3), load address, entry address"
	Print "Numbers in decimal, 0x... or &h..."
End Sub

Function num (s As String) As Integer
	s = Trim (s)
	If Lcase (Left (s, 2)) = "0x" Then Return Val ("&h" & Mid (s, 3))
	Return Val (s)
End Function

Const TRACKS = 40
Const SECS = 9
Dim Shared As uByte img (TRACKS * SECS * 512 - 1)
Dim Shared As uByte dirb (2047)
Dim Shared As Integer nextBlock, nextEntry

' The data of block b (1K) is logical sectors 2b, 2b+1
Function blockOfs (b As Integer) As Integer
	Return b * 1024
End Function

Sub addFile (lname As String, dname As String, loadA As Integer, execA As Integer)
	Dim As Integer f, l, i, n, nb, b, e, ex, rc, ck, p
	Dim As String bn, ext
	f = FreeFile
	If Open (lname For Binary Access Read As f) Then Print "Can't open " & lname: End 1
	l = Lof (f)
	Dim As uByte d (l + 127)
	For i = 0 To l - 1: Get #f, , d (128 + i): Next i
	Close f

	dname = Ucase (dname)
	p = Instr (dname, ".")
	If p Then bn = Left (dname, p - 1): ext = Mid (dname, p + 1) Else bn = dname: ext = ""
	bn = Left (bn & "        ", 8): ext = Left (ext & "   ", 3)

	' AMSDOS header
	For i = 0 To 127: d (i) = 0: Next i
	For i = 0 To 7: d (1 + i) = Asc (Mid (bn, i + 1, 1)): Next i
	For i = 0 To 2: d (9 + i) = Asc (Mid (ext, i + 1, 1)): Next i
	d (18) = 2
	d (21) = loadA And 255: d (22) = (loadA Shr 8) And 255
	d (23) = 255
	d (24) = l And 255: d (25) = (l Shr 8) And 255
	d (26) = execA And 255: d (27) = (execA Shr 8) And 255
	d (64) = l And 255: d (65) = (l Shr 8) And 255: d (66) = (l Shr 16) And 255
	ck = 0: For i = 0 To 66: ck += d (i): Next i
	d (67) = ck And 255: d (68) = (ck Shr 8) And 255

	n = l + 128
	nb = (n + 1023) \ 1024
	' data
	For b = 0 To nb - 1
		If nextBlock >= TRACKS * SECS \ 2 Then Print "Disk full": End 1
		For i = 0 To 1023
			If b * 1024 + i < n Then img (blockOfs (nextBlock + b) + i) = d (b * 1024 + i)
		Next i
	Next b
	' directory: an extent per 16 blocks
	For ex = 0 To (nb - 1) \ 16
		If nextEntry >= 64 Then Print "Directory full": End 1
		e = nextEntry * 32: nextEntry += 1
		dirb (e) = 0
		For i = 0 To 7: dirb (e + 1 + i) = Asc (Mid (bn, i + 1, 1)): Next i
		For i = 0 To 2: dirb (e + 9 + i) = Asc (Mid (ext, i + 1, 1)): Next i
		dirb (e + 12) = ex: dirb (e + 13) = 0: dirb (e + 14) = 0
		rc = (n - ex * 16384 + 127) \ 128: If rc > 128 Then rc = 128
		dirb (e + 15) = rc
		For i = 0 To 15
			b = ex * 16 + i
			If b < nb Then dirb (e + 16 + i) = nextBlock + b Else dirb (e + 16 + i) = 0
		Next i
	Next ex
	nextBlock += nb
	Print "cpcdsk: " & lname & " (" & l & " bytes) as " & Trim (bn) & "." & Trim (ext) & ", " & nb & "K"
End Sub

' GO!

Dim As Integer i, t, s, fo, k
Dim As String v, a (3)

sclpParseAttrs
If sclpGetValue ("out") = "" Or sclpGetValue ("file1") = "" Then usage: End

For i = 0 To Ubound (img): img (i) = &hE5: Next i
For i = 0 To 2047: dirb (i) = &hE5: Next i
nextBlock = 2 			' blocks 0-1: the directory
nextEntry = 0

k = 1
Do
	v = sclpGetValue ("file" & k)
	If v = "" Then Exit Do
	Dim As Integer c1 = Instr (v, ","), c2, c3
	c2 = Instr (c1 + 1, v, ","): c3 = Instr (c2 + 1, v, ",")
	If c1 = 0 Or c2 = 0 Or c3 = 0 Then usage: End 1
	addFile Left (v, c1 - 1), Mid (v, c1 + 1, c2 - c1 - 1), num (Mid (v, c2 + 1, c3 - c2 - 1)), num (Mid (v, c3 + 1))
	k += 1
Loop
For i = 0 To 2047: img (i) = dirb (i): Next i

If FileExists (sclpGetValue ("out")) Then Kill sclpGetValue ("out")
fo = FreeFile
Open sclpGetValue ("out") For Binary As fo
Dim As uByte hdr (255)
Dim As String sig = "MV - CPCEMU Disk-File" & Chr (13, 10) & "Disk-Info" & Chr (13, 10) & "cpcdsk MK1   "
For i = 1 To Len (sig): hdr (i - 1) = Asc (Mid (sig, i, 1)): Next i
hdr (&h30) = TRACKS: hdr (&h31) = 1: hdr (&h32) = &h00: hdr (&h33) = &h13
Put #fo, , hdr ()
For t = 0 To TRACKS - 1
	Dim As uByte th (255)
	Dim As String ti = "Track-Info" & Chr (13, 10)
	For i = 0 To 255: th (i) = 0: Next i
	For i = 1 To Len (ti): th (i - 1) = Asc (Mid (ti, i, 1)): Next i
	th (&h10) = t: th (&h11) = 0: th (&h14) = 2: th (&h15) = SECS: th (&h16) = &h4E: th (&h17) = &hE5
	For s = 0 To SECS - 1
		th (&h18 + s * 8) = t: th (&h19 + s * 8) = 0: th (&h1A + s * 8) = &hC1 + s: th (&h1B + s * 8) = 2
	Next s
	Put #fo, , th ()
	For i = 0 To SECS * 512 - 1: Put #fo, , img (t * SECS * 512 + i): Next i
Next t
Close fo
Print "cpcdsk: " & sclpGetValue ("out") & ", " & nextBlock & "K used"
