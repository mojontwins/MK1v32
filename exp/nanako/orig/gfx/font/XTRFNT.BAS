DEFINT A-Z

DIM d AS STRING * 1

SCREEN 13
OPEN "Kun-i.raw" FOR BINARY AS #1
FOR y = 0 TO 7
    FOR x = 0 TO 319
        GET #1, , d
        PSET (x, y), ASC(d)
NEXT x, y
CLOSE #1

     
OPEN "kun.asm" FOR OUTPUT AS #1
PRINT #1, "; Set de caracteres para CPCRSLIB"
PRINT #1, "; generado automaticamente por na_th_an"
PRINT #1, "; Para CJMC"

PRINT #1, " "
print #1, "XLIB cpc_Chars"
print #1, " "
print #1, "XDEF first_char"
print #1, " "
print #1, ".first_char defb 48	;first defined char number"
print #1, " "
print #1, ".cpc_Chars   ;cpc_Chars codificadas... cada pixel se define con 2 bits que definen el color."
print #1, " "

xx = 0
FOR ch = 0 TO 42
    PRINT #1, "; " + CHR$(48 + ch)
    FOR y = 0 TO 7
        LET o$ = "    defb @"
        FOR x = 0 TO 3
            SELECT CASE POINT(xx + x, y)
                CASE 3
                    o$ = o$ + "00"
                CASE 0
                    o$ = o$ + "01"
                CASE 1
                    o$ = o$ + "10"
                CASE 2
                    o$ = o$ + "11"
                CASE ELSE
                    o$ = o$ + "00"
            END SELECT
        NEXT x
        PRINT #1, o$
    NEXT y
    PRINT #1, " "
    xx = xx + 4
NEXT ch

CLOSE #1
SYSTEM

