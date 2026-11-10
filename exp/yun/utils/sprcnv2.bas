#include "fbpng.bi"
#include "fbgfx.bi"
#include once "crt.bi"

#define RGBA_R( c ) ( CUInt( c ) Shr 16 And 255 )
#define RGBA_G( c ) ( CUInt( c ) Shr  8 And 255 )
#define RGBA_B( c ) ( CUInt( c )        And 255 )
#define RGBA_A( c ) ( CUInt( c ) Shr 24         )

Dim Shared As Any Ptr img 			' will contain the image loaded from the PNG

Function inCommand (spec As String) As Integer
	Dim As Integer res, i

	i = 0: res = 0

	Do
		If Command (i) = "" Then Exit Do
		If Command (i) = spec Then res = -1: Exit Do
		i = i + 1
	Loop

	Return res
End Function

Sub Usage () 
	Print "** USO **"
	Print "   sprcnv2 archivo.png archivo.h n [nomask]"
	Print "   sprcnv2 archivo.png archivo.bin n nomask bin"
	Print "   ... tall: celdas de 16x24 en vez de 16x16 (filas de 24 pixels en el png)"
	Print "   ... from=k: empieza en la celda k (par), las etiquetas siguen numeradas desde 1"
	Print "   ... label=nombre: etiquetas nombre_N_a/b/c en vez de sprite_N_a/b/c"
	Print
	Print "Convierte un Spriteset de n sprites"
	Print "Con `bin`, archivo.bin recibe los bloques tal cual (sin las 8 lineas iniciales),"
	Print "para sets que se descomprimen en tiempo de ejecucion. Cada pareja de sprites ocupa"
	Print "144 bytes: el sprite 2k empieza en 144k, el 2k+1 en 144k+1; columnas a +48, +96."
End Sub

Function ReadBinValue (x As Integer, y As Integer) As Integer
	Dim As Integer ac, xx, p1
	ac = 0
	for xx = 0 to 7
		p1 = point (x + xx, y, img)
		if RGBA_R (p1) <> 0 Or RGBA_G (p1) <> 0 Or RGBA_B (p1) <> 0 Then
			ac = ac + 2 ^ (7 - xx)
		end if
	next xx
	Return ac
End Function

Dim Shared As String labelBase

' Value of a key=value option ("" if not there)
Function optVal (key As String) As String
	Dim As Integer i
	i = 1
	Do
		If Command (i) = "" Then Exit Do
		If Left (Command (i), Len (key) + 1) = key & "=" Then Return Mid (Command (i), Len (key) + 2)
		i = i + 1
	Loop
	Return ""
End Function

Function labelName (extra As Integer, i As Integer) As String
	If labelBase <> "" Then Return labelBase
	If i <= 16 Or extra = 0 Then Return "sprite"
	Return "extra_sprite"
End Function

'
'
'

Dim as String filename, o
Dim as integer i, j, f, sprite, x, y, xx, yy, ac, nomask, extra, n, s, h
Dim as unsigned long p1, p2

If Len (Command (1)) = 0 Or Len (Command (2)) = 0 Or len (Command (3)) = 0 Then 
	Usage
	End
End If

If inCommand ("nomask") Then nomask = -1 else nomask = 0
If inCommand ("extra") Then extra = -1 else extra = 0
' tall: 16x24 cells (columns of 24 + 8 lines)
If inCommand ("tall") Then h = 24 else h = 16
labelBase = optVal ("label")
Dim As Integer firstCell
firstCell = Val (optVal ("from"))
If firstCell And 1 Then Print "from must be even": End

If inCommand ("bin") Then
	If Not nomask Then Print "bin needs nomask": End
	screenres 640, 480, 32, , -1
	img = png_load (Command (1))
	If img = 0 Then Print "Failed to load": End
	Dim As uByte ub
	Kill Command (2)
	f = FreeFile
	Open Command (2) For Binary As #f
	n = Val (Command (3))
	x = 0: y = 0
	For sprite = 1 To n Step 2
		' 3 columns: pixels x..x+7, x+8..x+15, empty; sprite and sprite+1 interleaved
		For j = 0 To 2
			For yy = 0 To h - 1
				If j < 2 Then ub = ReadBinValue (x + 8 * j, y + yy) Else ub = 0
				Put #f, , ub
				If j < 2 Then ub = ReadBinValue (x + 16 + 8 * j, y + yy) Else ub = 0
				Put #f, , ub
			Next yy
			ub = 0
			For yy = 0 To 15: Put #f, , ub: Next yy
		Next j
		x = x + 32: If x >= 256 Then x = 0: y = y + h
	Next sprite
	Close f
	Print "sprcnv2: " & n & " sprites (" & (n \ 2 * 6 * (h + 8)) & " bytes) to " & Command (2)
	End
End If

' Primero cargo el archivo de imagen
screenres 640, 480, 32, , -1

filename = Command (1)
img = png_load ( filename )

if img then
'	put( 0, 0 ), img
'	deallocate( img )
else
	print "Failed to load"
end if

' Ahora lo recorro y voy generando el código con sus etiquetas y
' sus mascaritas y toda la pesca

f = FreeFile

Open Command (2) for Output as f

Print #f, "// MTE MK1 (la Churrera) v5.0"
Print #f, "// Copyleft 2010-2014, 2020 by the Mojon Twins"
Print #f, ""
print #f, "// Sprites.h"
if nomask then print #f, "// No masks"
print #f, " "

for sprite = firstCell + 1 to Val (Command (3))

	print #f, "extern unsigned char " & labelName (extra, sprite) & "_" + Trim(Str(sprite)) + "_a []; "
	print #f, "extern unsigned char " & labelName (extra, sprite) & "_" + Trim(Str(sprite)) + "_b []; "
	print #f, "extern unsigned char " & labelName (extra, sprite) & "_" + Trim(Str(sprite)) + "_c []; "
next sprite

print #f, " "

print #f, "#asm"

' from=k: the cells before k are skipped (16 cells per 256 px row)
x = (firstCell * 16) Mod 256
y = ((firstCell * 16) \ 256) * h

if nomask then
	for i = 0 to 7: print #f, "        defb 0, 0": next i: print #f, " "
else
	for i = 0 to 7: print #f, "        defb 0, 255":next i: print #f, " "
endif

n = Val (Command (3))
If nomask then
	s = 2
else
	s = 1
end if

For sprite = firstCell + 1 To n Step s

	' Primera columna 
	
	if nomask then
		print #f,"; Sprites #" & str (sprite-1) & " y " & str (sprite)
	else
		print #f,"; Sprite #" & str (sprite-1) &" y máscara"
	end if
	print #f,"; Primera columna"
	print #f, "    ._" & labelName (extra, sprite) & "_" + Trim (Str(Sprite)) + "_a"
	
	' Ahora tengo que calcular los dos valores de la primera columna
	for yy = 0 to h - 1
		o = "        defb "
		ac = ReadBinValue (x, y+yy)		
		If nomask And yy = 0 Then
			o = o + Trim (Str(ac))
			Print #f, o
			o = "        defb "
			print #f, "    ._" & labelName (extra, sprite+1) & "_" + Trim (Str(Sprite+1)) + "_a"
		Else
			o = o + Trim (Str(ac)) + ", "
		End If
		ac = ReadBinValue (x + 16, y+yy)
		o = o + Trim (Str(ac))
		print #f, o
	next yy
	for yy = 0 to 7
		if nomask then 
			print #f, "        defb 0, 0"
		else
			print #f, "        defb 0, 255"
		end if
	next yy
	print #f, " "
	
	' Segunda columna 
	
	print #f,"; Segunda columna"
	print #f, "    ._" & labelName (extra, sprite) & "_" + Trim (Str(Sprite)) + "_b"
	
	' Ahora tengo que calcular los dos valores de la primera columna
	for yy = 0 to h - 1
		o = "        defb "
		ac = ReadBinValue (x + 8, y+yy)		
		If nomask And yy = 0 Then
			o = o + Trim (Str(ac))
			Print #f, o
			o = "        defb "
			print #f, "    ._" & labelName (extra, sprite+1) & "_" + Trim (Str(Sprite+1)) + "_b"
		Else
			o = o + Trim (Str(ac)) + ", "
		End If
		ac = ReadBinValue (x + 24, y+yy)
		o = o + Trim (Str(ac))
		print #f, o
	next yy
	for yy = 0 to 7
		if nomask then 
			print #f, "        defb 0, 0"
		else
			print #f, "        defb 0, 255"
		end if
	next yy
	print #f, " "
	
	' Tercera columna
	
	print #f, "; tercera columna"
	print #f, "    ._" & labelName (extra, sprite) & "_" + Trim (Str(Sprite)) + "_c"
	for yy = 0 to h + 7
		o = "        defb "
		ac = 0
		If nomask And yy = 0 Then
			o = o + Trim (Str(ac))
			Print #f, o
			o = "        defb "
			print #f, "    ._" & labelName (extra, sprite+1) & "_" + Trim (Str(Sprite+1)) + "_c"
		Else
			o = o + Trim (Str(ac)) + ", "
		End If
		If nomask Then
			ac = 0
		Else
			ac = 255
		End If
		o = o + Trim (Str(ac))
		print #f, o
	next yy
	print #f, " "
	
	x = x + 32
	if x = 256 then
		y = y + h
		x = 0
	end if
	
next sprite

print #f, "#endasm"
print #f, " "

Close f

puts "¡Todo correcto!"