// MTE MK1 (la Churrera) v5.0
// Copyleft 2010-2014, 2020 by the Mojon Twins

// Sprites.h
// No masks
 
extern unsigned char player_1_a []; 
extern unsigned char player_1_b []; 
extern unsigned char player_1_c []; 
extern unsigned char player_2_a []; 
extern unsigned char player_2_b []; 
extern unsigned char player_2_c []; 
extern unsigned char player_3_a []; 
extern unsigned char player_3_b []; 
extern unsigned char player_3_c []; 
extern unsigned char player_4_a []; 
extern unsigned char player_4_b []; 
extern unsigned char player_4_c []; 
extern unsigned char player_5_a []; 
extern unsigned char player_5_b []; 
extern unsigned char player_5_c []; 
extern unsigned char player_6_a []; 
extern unsigned char player_6_b []; 
extern unsigned char player_6_c []; 
extern unsigned char player_7_a []; 
extern unsigned char player_7_b []; 
extern unsigned char player_7_c []; 
extern unsigned char player_8_a []; 
extern unsigned char player_8_b []; 
extern unsigned char player_8_c []; 
 
#asm
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
 
; Sprites #0 y 1
; Primera columna
    ._player_1_a
        defb 0
    ._player_2_a
        defb 1
        defb 1, 3
        defb 3, 5
        defb 6, 2
        defb 5, 13
        defb 10, 10
        defb 21, 4
        defb 42, 11
        defb 21, 21
        defb 42, 26
        defb 24, 12
        defb 16, 4
        defb 48, 2
        defb 48, 0
        defb 6, 3
        defb 15, 3
        defb 15, 3
        defb 15, 1
        defb 7, 0
        defb 3, 0
        defb 7, 1
        defb 62, 1
        defb 56, 1
        defb 32, 1
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
 
; Segunda columna
    ._player_1_b
        defb 0
    ._player_2_b
        defb 0
        defb 0, 160
        defb 80, 80
        defb 168, 128
        defb 64, 120
        defb 184, 208
        defb 104, 208
        defb 104, 120
        defb 120, 32
        defb 32, 208
        defb 208, 200
        defb 200, 192
        defb 192, 248
        defb 124, 112
        defb 56, 128
        defb 0, 208
        defb 32, 208
        defb 112, 208
        defb 120, 208
        defb 24, 208
        defb 24, 160
        defb 24, 160
        defb 30, 160
        defb 0, 232
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
 
; tercera columna
    ._player_1_c
        defb 0
    ._player_2_c
        defb 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
 
; Sprites #2 y 3
; Primera columna
    ._player_3_a
        defb 0
    ._player_4_a
        defb 2
        defb 1, 3
        defb 3, 2
        defb 5, 5
        defb 10, 10
        defb 21, 21
        defb 43, 11
        defb 83, 21
        defb 41, 42
        defb 80, 17
        defb 95, 39
        defb 51, 14
        defb 100, 24
        defb 104, 55
        defb 16, 111
        defb 31, 111
        defb 27, 15
        defb 27, 7
        defb 12, 7
        defb 7, 7
        defb 7, 14
        defb 62, 28
        defb 60, 48
        defb 32, 48
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
 
; Segunda columna
    ._player_3_b
        defb 0
    ._player_4_b
        defb 0
        defb 0, 64
        defb 160, 160
        defb 80, 0
        defb 0, 240
        defb 240, 160
        defb 80, 160
        defb 80, 240
        defb 224, 96
        defb 192, 144
        defb 32, 136
        defb 16, 8
        defb 16, 16
        defb 28, 224
        defb 88, 48
        defb 192, 56
        defb 224, 124
        defb 240, 76
        defb 248, 12
        defb 24, 12
        defb 24, 6
        defb 24, 2
        defb 30, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
 
; tercera columna
    ._player_3_c
        defb 0
    ._player_4_c
        defb 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
 
; Sprites #4 y 5
; Primera columna
    ._player_5_a
        defb 0
    ._player_6_a
        defb 0
        defb 0, 5
        defb 5, 10
        defb 10, 1
        defb 0, 30
        defb 15, 11
        defb 10, 11
        defb 10, 30
        defb 7, 4
        defb 3, 11
        defb 4, 19
        defb 8, 3
        defb 8, 31
        defb 56, 14
        defb 26, 1
        defb 3, 11
        defb 7, 11
        defb 15, 11
        defb 31, 11
        defb 24, 11
        defb 24, 5
        defb 24, 5
        defb 120, 5
        defb 0, 23
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
 
; Segunda columna
    ._player_5_b
        defb 0
    ._player_6_b
        defb 128
        defb 128, 192
        defb 192, 160
        defb 160, 64
        defb 80, 176
        defb 168, 80
        defb 212, 32
        defb 202, 208
        defb 148, 168
        defb 10, 88
        defb 250, 48
        defb 204, 32
        defb 38, 64
        defb 22, 0
        defb 8, 192
        defb 248, 192
        defb 216, 192
        defb 216, 128
        defb 48, 0
        defb 224, 0
        defb 224, 128
        defb 124, 128
        defb 60, 128
        defb 4, 128
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
 
; tercera columna
    ._player_5_c
        defb 0
    ._player_6_c
        defb 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
 
; Sprites #6 y 7
; Primera columna
    ._player_7_a
        defb 0
    ._player_8_a
        defb 0
        defb 0, 2
        defb 10, 5
        defb 21, 0
        defb 2, 15
        defb 29, 5
        defb 22, 5
        defb 22, 15
        defb 30, 6
        defb 4, 9
        defb 11, 17
        defb 19, 16
        defb 3, 8
        defb 62, 7
        defb 28, 12
        defb 0, 28
        defb 4, 62
        defb 14, 50
        defb 30, 48
        defb 24, 48
        defb 24, 96
        defb 24, 64
        defb 120, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
 
; Segunda columna
    ._player_7_b
        defb 0
    ._player_8_b
        defb 64
        defb 128, 192
        defb 192, 64
        defb 96, 160
        defb 160, 80
        defb 80, 168
        defb 168, 208
        defb 84, 168
        defb 168, 84
        defb 84, 136
        defb 24, 228
        defb 8, 112
        defb 12, 24
        defb 12, 236
        defb 96, 246
        defb 240, 246
        defb 240, 240
        defb 240, 224
        defb 224, 224
        defb 192, 224
        defb 224, 112
        defb 124, 56
        defb 28, 12
        defb 4, 12
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
 
; tercera columna
    ._player_7_c
        defb 0
    ._player_8_c
        defb 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
 
#endasm
 
