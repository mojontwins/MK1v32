DIM d AS STRING * 1

PRINT "Esto es tela de cutre, para quitar la cabecera de las imagenes convertidas"
SHELL "dir /w"
PRINT "Mete el nombre de tu SCR pero no le pongas scr:"
PRINT "Qu‚ cutre es esto, co¤o..."
INPUT nombre$

PRINT "Leyendo de " + nombre$ + ".scr"
PRINT "Escribiendo en " + nombre$ + ".bin"

OPEN nombre$ + ".scr" FOR BINARY AS #1
OPEN nombre$ + ".bin" FOR BINARY AS #2

FOR i% = 1 TO 128
    GET #1, , d
NEXT i%

FOR i% = 1 TO 16336
    GET #1, , d
    PUT #2, , d
NEXT i%

CLOSE

PRINT "Ea."
SYSTEM

