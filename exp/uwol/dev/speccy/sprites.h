// MTE MK1 (la Churrera) v5.0
// Copyleft 2010-2014, 2020 by the Mojon Twins

// Sprites.h
 
extern unsigned char sprite_1_a []; 
extern unsigned char sprite_1_b []; 
extern unsigned char sprite_1_c []; 
extern unsigned char sprite_2_a []; 
extern unsigned char sprite_2_b []; 
extern unsigned char sprite_2_c []; 
extern unsigned char sprite_3_a []; 
extern unsigned char sprite_3_b []; 
extern unsigned char sprite_3_c []; 
extern unsigned char sprite_4_a []; 
extern unsigned char sprite_4_b []; 
extern unsigned char sprite_4_c []; 
extern unsigned char sprite_5_a []; 
extern unsigned char sprite_5_b []; 
extern unsigned char sprite_5_c []; 
extern unsigned char sprite_6_a []; 
extern unsigned char sprite_6_b []; 
extern unsigned char sprite_6_c []; 
extern unsigned char sprite_7_a []; 
extern unsigned char sprite_7_b []; 
extern unsigned char sprite_7_c []; 
extern unsigned char sprite_8_a []; 
extern unsigned char sprite_8_b []; 
extern unsigned char sprite_8_c []; 
extern unsigned char sprite_9_a []; 
extern unsigned char sprite_9_b []; 
extern unsigned char sprite_9_c []; 
extern unsigned char sprite_10_a []; 
extern unsigned char sprite_10_b []; 
extern unsigned char sprite_10_c []; 
extern unsigned char sprite_11_a []; 
extern unsigned char sprite_11_b []; 
extern unsigned char sprite_11_c []; 
extern unsigned char sprite_12_a []; 
extern unsigned char sprite_12_b []; 
extern unsigned char sprite_12_c []; 
extern unsigned char sprite_13_a []; 
extern unsigned char sprite_13_b []; 
extern unsigned char sprite_13_c []; 
extern unsigned char sprite_14_a []; 
extern unsigned char sprite_14_b []; 
extern unsigned char sprite_14_c []; 
extern unsigned char sprite_15_a []; 
extern unsigned char sprite_15_b []; 
extern unsigned char sprite_15_c []; 
extern unsigned char sprite_16_a []; 
extern unsigned char sprite_16_b []; 
extern unsigned char sprite_16_c []; 
extern unsigned char sprite_17_a []; 
extern unsigned char sprite_17_b []; 
extern unsigned char sprite_17_c []; 
extern unsigned char sprite_18_a []; 
extern unsigned char sprite_18_b []; 
extern unsigned char sprite_18_c []; 
extern unsigned char sprite_19_a []; 
extern unsigned char sprite_19_b []; 
extern unsigned char sprite_19_c []; 
extern unsigned char sprite_20_a []; 
extern unsigned char sprite_20_b []; 
extern unsigned char sprite_20_c []; 
extern unsigned char sprite_21_a []; 
extern unsigned char sprite_21_b []; 
extern unsigned char sprite_21_c []; 
extern unsigned char sprite_22_a []; 
extern unsigned char sprite_22_b []; 
extern unsigned char sprite_22_c []; 
extern unsigned char sprite_23_a []; 
extern unsigned char sprite_23_b []; 
extern unsigned char sprite_23_c []; 
extern unsigned char sprite_24_a []; 
extern unsigned char sprite_24_b []; 
extern unsigned char sprite_24_c []; 
extern unsigned char sprite_25_a []; 
extern unsigned char sprite_25_b []; 
extern unsigned char sprite_25_c []; 
 
#asm
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Sprite #0 y máscara
; Primera columna
    ._sprite_1_a
        defb 15, 240
        defb 8, 240
        defb 23, 224
        defb 46, 192
        defb 47, 192
        defb 47, 192
        defb 78, 128
        defb 182, 0
        defb 177, 0
        defb 174, 0
        defb 77, 128
        defb 48, 192
        defb 11, 240
        defb 20, 224
        defb 18, 224
        defb 9, 240
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Segunda columna
    ._sprite_1_b
        defb 192, 63
        defb 32, 31
        defb 208, 15
        defb 8, 7
        defb 84, 3
        defb 250, 1
        defb 250, 1
        defb 244, 3
        defb 202, 1
        defb 42, 1
        defb 228, 3
        defb 220, 3
        defb 34, 1
        defb 194, 1
        defb 4, 3
        defb 24, 7
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; tercera columna
    ._sprite_1_c
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Sprite #1 y máscara
; Primera columna
    ._sprite_2_a
        defb 0, 255
        defb 7, 248
        defb 8, 240
        defb 23, 224
        defb 46, 192
        defb 47, 192
        defb 79, 128
        defb 174, 0
        defb 176, 0
        defb 182, 0
        defb 78, 128
        defb 33, 192
        defb 20, 224
        defb 17, 224
        defb 32, 192
        defb 32, 192
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Segunda columna
    ._sprite_2_b
        defb 0, 255
        defb 192, 63
        defb 32, 31
        defb 208, 15
        defb 8, 7
        defb 84, 3
        defb 250, 1
        defb 250, 1
        defb 242, 1
        defb 202, 1
        defb 36, 3
        defb 232, 7
        defb 208, 15
        defb 28, 3
        defb 34, 1
        defb 18, 1
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; tercera columna
    ._sprite_2_c
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Sprite #2 y máscara
; Primera columna
    ._sprite_3_a
        defb 7, 248
        defb 8, 240
        defb 23, 224
        defb 46, 192
        defb 47, 192
        defb 47, 192
        defb 46, 192
        defb 86, 128
        defb 88, 128
        defb 91, 128
        defb 39, 192
        defb 16, 224
        defb 39, 192
        defb 64, 128
        defb 32, 192
        defb 24, 224
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Segunda columna
    ._sprite_3_b
        defb 192, 63
        defb 32, 31
        defb 208, 15
        defb 8, 7
        defb 84, 3
        defb 250, 1
        defb 250, 1
        defb 244, 3
        defb 72, 7
        defb 40, 7
        defb 80, 15
        defb 214, 9
        defb 9, 0
        defb 193, 0
        defb 33, 0
        defb 38, 1
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; tercera columna
    ._sprite_3_c
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Sprite #3 y máscara
; Primera columna
    ._sprite_4_a
        defb 7, 248
        defb 8, 240
        defb 23, 224
        defb 46, 192
        defb 47, 192
        defb 47, 192
        defb 46, 192
        defb 86, 128
        defb 88, 128
        defb 91, 128
        defb 39, 192
        defb 16, 224
        defb 39, 192
        defb 64, 128
        defb 32, 192
        defb 24, 224
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Segunda columna
    ._sprite_4_b
        defb 192, 63
        defb 32, 31
        defb 208, 15
        defb 8, 7
        defb 84, 3
        defb 250, 1
        defb 250, 1
        defb 244, 3
        defb 72, 7
        defb 40, 7
        defb 80, 15
        defb 214, 9
        defb 9, 0
        defb 193, 0
        defb 33, 0
        defb 38, 1
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; tercera columna
    ._sprite_4_c
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Sprite #4 y máscara
; Primera columna
    ._sprite_5_a
        defb 3, 252
        defb 4, 248
        defb 11, 240
        defb 16, 224
        defb 42, 192
        defb 95, 128
        defb 95, 128
        defb 47, 192
        defb 83, 128
        defb 84, 128
        defb 39, 192
        defb 59, 192
        defb 68, 128
        defb 67, 128
        defb 32, 192
        defb 24, 224
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Segunda columna
    ._sprite_5_b
        defb 240, 15
        defb 16, 15
        defb 232, 7
        defb 116, 3
        defb 244, 3
        defb 244, 3
        defb 114, 1
        defb 109, 0
        defb 141, 0
        defb 117, 0
        defb 178, 1
        defb 12, 3
        defb 208, 15
        defb 40, 7
        defb 72, 7
        defb 144, 15
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; tercera columna
    ._sprite_5_c
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Sprite #5 y máscara
; Primera columna
    ._sprite_6_a
        defb 0, 255
        defb 3, 252
        defb 4, 248
        defb 11, 240
        defb 16, 224
        defb 42, 192
        defb 95, 128
        defb 95, 128
        defb 79, 128
        defb 83, 128
        defb 36, 192
        defb 23, 224
        defb 11, 240
        defb 56, 192
        defb 68, 128
        defb 72, 128
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Segunda columna
    ._sprite_6_b
        defb 0, 255
        defb 224, 31
        defb 16, 15
        defb 232, 7
        defb 116, 3
        defb 244, 3
        defb 242, 1
        defb 117, 0
        defb 13, 0
        defb 109, 0
        defb 114, 1
        defb 132, 3
        defb 40, 7
        defb 136, 7
        defb 4, 3
        defb 4, 3
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; tercera columna
    ._sprite_6_c
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Sprite #6 y máscara
; Primera columna
    ._sprite_7_a
        defb 3, 252
        defb 4, 248
        defb 11, 240
        defb 16, 224
        defb 42, 192
        defb 95, 128
        defb 95, 128
        defb 47, 192
        defb 18, 224
        defb 20, 224
        defb 10, 240
        defb 107, 144
        defb 144, 0
        defb 131, 0
        defb 132, 0
        defb 100, 128
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Segunda columna
    ._sprite_7_b
        defb 224, 31
        defb 16, 15
        defb 232, 7
        defb 116, 3
        defb 244, 3
        defb 244, 3
        defb 116, 3
        defb 106, 1
        defb 26, 1
        defb 218, 1
        defb 228, 3
        defb 8, 7
        defb 228, 3
        defb 2, 1
        defb 4, 3
        defb 24, 7
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; tercera columna
    ._sprite_7_c
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Sprite #7 y máscara
; Primera columna
    ._sprite_8_a
        defb 3, 252
        defb 4, 248
        defb 11, 240
        defb 16, 224
        defb 42, 192
        defb 95, 128
        defb 95, 128
        defb 47, 192
        defb 18, 224
        defb 20, 224
        defb 10, 240
        defb 107, 144
        defb 144, 0
        defb 131, 0
        defb 132, 0
        defb 100, 128
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Segunda columna
    ._sprite_8_b
        defb 224, 31
        defb 16, 15
        defb 232, 7
        defb 116, 3
        defb 244, 3
        defb 244, 3
        defb 116, 3
        defb 106, 1
        defb 26, 1
        defb 218, 1
        defb 228, 3
        defb 8, 7
        defb 228, 3
        defb 2, 1
        defb 4, 3
        defb 24, 7
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; tercera columna
    ._sprite_8_c
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Sprite #8 y máscara
; Primera columna
    ._sprite_9_a
        defb 7, 248
        defb 8, 240
        defb 23, 224
        defb 46, 192
        defb 45, 192
        defb 79, 128
        defb 172, 0
        defb 183, 0
        defb 180, 0
        defb 75, 128
        defb 35, 192
        defb 20, 224
        defb 11, 240
        defb 20, 224
        defb 18, 224
        defb 9, 224
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Segunda columna
    ._sprite_9_b
        defb 192, 63
        defb 32, 31
        defb 16, 15
        defb 232, 7
        defb 84, 3
        defb 90, 1
        defb 250, 1
        defb 242, 1
        defb 138, 1
        defb 100, 3
        defb 104, 7
        defb 156, 3
        defb 34, 1
        defb 194, 1
        defb 4, 3
        defb 24, 7
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; tercera columna
    ._sprite_9_c
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Sprite #9 y máscara
; Primera columna
    ._sprite_10_a
        defb 0, 255
        defb 7, 248
        defb 8, 240
        defb 23, 224
        defb 46, 192
        defb 45, 192
        defb 79, 128
        defb 172, 0
        defb 183, 0
        defb 180, 0
        defb 75, 128
        defb 35, 192
        defb 20, 224
        defb 17, 224
        defb 32, 192
        defb 32, 192
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Segunda columna
    ._sprite_10_b
        defb 0, 255
        defb 192, 63
        defb 32, 31
        defb 16, 15
        defb 232, 7
        defb 84, 3
        defb 90, 1
        defb 250, 1
        defb 242, 1
        defb 138, 1
        defb 100, 3
        defb 104, 7
        defb 144, 15
        defb 28, 3
        defb 34, 1
        defb 18, 1
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; tercera columna
    ._sprite_10_c
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Sprite #10 y máscara
; Primera columna
    ._sprite_11_a
        defb 7, 248
        defb 8, 240
        defb 23, 224
        defb 46, 192
        defb 45, 192
        defb 79, 128
        defb 172, 0
        defb 183, 0
        defb 180, 0
        defb 67, 128
        defb 35, 192
        defb 20, 224
        defb 35, 192
        defb 64, 128
        defb 32, 192
        defb 24, 224
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Segunda columna
    ._sprite_11_b
        defb 192, 63
        defb 32, 31
        defb 16, 15
        defb 232, 7
        defb 84, 3
        defb 90, 1
        defb 250, 1
        defb 242, 1
        defb 138, 1
        defb 100, 3
        defb 104, 7
        defb 150, 1
        defb 105, 0
        defb 193, 0
        defb 33, 0
        defb 38, 1
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; tercera columna
    ._sprite_11_c
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Sprite #11 y máscara
; Primera columna
    ._sprite_12_a
        defb 7, 248
        defb 8, 240
        defb 23, 224
        defb 46, 192
        defb 45, 192
        defb 79, 128
        defb 172, 0
        defb 183, 0
        defb 180, 0
        defb 67, 128
        defb 35, 192
        defb 20, 224
        defb 35, 192
        defb 64, 128
        defb 32, 192
        defb 24, 224
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Segunda columna
    ._sprite_12_b
        defb 192, 63
        defb 32, 31
        defb 16, 15
        defb 232, 7
        defb 84, 3
        defb 90, 1
        defb 250, 1
        defb 242, 1
        defb 138, 1
        defb 100, 3
        defb 104, 7
        defb 150, 1
        defb 105, 0
        defb 193, 0
        defb 33, 0
        defb 38, 1
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; tercera columna
    ._sprite_12_c
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Sprite #12 y máscara
; Primera columna
    ._sprite_13_a
        defb 3, 252
        defb 4, 248
        defb 8, 240
        defb 23, 224
        defb 42, 192
        defb 90, 128
        defb 95, 128
        defb 79, 128
        defb 81, 128
        defb 38, 192
        defb 22, 224
        defb 57, 192
        defb 68, 128
        defb 67, 128
        defb 32, 192
        defb 24, 224
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Segunda columna
    ._sprite_13_b
        defb 224, 31
        defb 16, 15
        defb 232, 7
        defb 116, 3
        defb 180, 3
        defb 242, 1
        defb 53, 0
        defb 237, 0
        defb 45, 0
        defb 210, 1
        defb 196, 3
        defb 40, 7
        defb 208, 15
        defb 40, 7
        defb 72, 7
        defb 144, 7
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; tercera columna
    ._sprite_13_c
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Sprite #13 y máscara
; Primera columna
    ._sprite_14_a
        defb 0, 255
        defb 3, 252
        defb 4, 248
        defb 8, 240
        defb 23, 224
        defb 42, 192
        defb 90, 128
        defb 95, 128
        defb 79, 128
        defb 81, 128
        defb 38, 192
        defb 22, 224
        defb 9, 240
        defb 56, 192
        defb 68, 128
        defb 72, 128
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Segunda columna
    ._sprite_14_b
        defb 0, 255
        defb 224, 31
        defb 16, 15
        defb 232, 7
        defb 116, 3
        defb 180, 3
        defb 242, 1
        defb 53, 0
        defb 237, 0
        defb 45, 0
        defb 210, 1
        defb 196, 3
        defb 40, 7
        defb 136, 7
        defb 4, 3
        defb 4, 3
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; tercera columna
    ._sprite_14_c
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Sprite #14 y máscara
; Primera columna
    ._sprite_15_a
        defb 3, 252
        defb 4, 248
        defb 8, 240
        defb 23, 224
        defb 42, 192
        defb 90, 128
        defb 95, 128
        defb 79, 128
        defb 81, 128
        defb 38, 192
        defb 22, 224
        defb 105, 128
        defb 150, 0
        defb 131, 0
        defb 132, 0
        defb 100, 128
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Segunda columna
    ._sprite_15_b
        defb 224, 31
        defb 16, 15
        defb 232, 7
        defb 116, 3
        defb 180, 3
        defb 242, 1
        defb 53, 0
        defb 237, 0
        defb 45, 0
        defb 194, 1
        defb 196, 3
        defb 40, 7
        defb 196, 3
        defb 2, 1
        defb 4, 3
        defb 24, 7
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; tercera columna
    ._sprite_15_c
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Sprite #15 y máscara
; Primera columna
    ._sprite_16_a
        defb 3, 252
        defb 4, 248
        defb 8, 240
        defb 23, 224
        defb 42, 192
        defb 90, 128
        defb 95, 128
        defb 79, 128
        defb 81, 128
        defb 38, 192
        defb 22, 224
        defb 105, 128
        defb 150, 0
        defb 131, 0
        defb 132, 0
        defb 100, 128
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Segunda columna
    ._sprite_16_b
        defb 224, 31
        defb 16, 15
        defb 232, 7
        defb 116, 3
        defb 180, 3
        defb 242, 1
        defb 53, 0
        defb 237, 0
        defb 45, 0
        defb 194, 1
        defb 196, 3
        defb 40, 7
        defb 196, 3
        defb 2, 1
        defb 4, 3
        defb 24, 7
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; tercera columna
    ._sprite_16_c
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Sprite #16 y máscara
; Primera columna
    ._sprite_17_a
        defb 0, 255
        defb 0, 248
        defb 0, 224
        defb 4, 192
        defb 6, 128
        defb 21, 192
        defb 21, 128
        defb 5, 128
        defb 15, 192
        defb 8, 224
        defb 11, 192
        defb 7, 128
        defb 16, 192
        defb 0, 252
        defb 0, 253
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Segunda columna
    ._sprite_17_b
        defb 0, 255
        defb 0, 31
        defb 0, 7
        defb 32, 3
        defb 96, 1
        defb 168, 3
        defb 168, 1
        defb 160, 1
        defb 240, 3
        defb 16, 7
        defb 208, 3
        defb 224, 1
        defb 8, 3
        defb 0, 63
        defb 0, 191
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; tercera columna
    ._sprite_17_c
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Sprite #17 y máscara
; Primera columna
    ._sprite_18_a
        defb 0, 255
        defb 0, 248
        defb 0, 224
        defb 4, 192
        defb 6, 128
        defb 21, 192
        defb 21, 128
        defb 5, 128
        defb 15, 192
        defb 8, 224
        defb 11, 192
        defb 7, 128
        defb 16, 192
        defb 0, 252
        defb 0, 253
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Segunda columna
    ._sprite_18_b
        defb 0, 255
        defb 0, 31
        defb 0, 7
        defb 32, 3
        defb 96, 1
        defb 168, 3
        defb 168, 1
        defb 160, 1
        defb 240, 3
        defb 16, 7
        defb 208, 3
        defb 224, 1
        defb 8, 3
        defb 0, 63
        defb 0, 191
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; tercera columna
    ._sprite_18_c
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Sprite #18 y máscara
; Primera columna
    ._sprite_19_a
        defb 0, 255
        defb 0, 192
        defb 31, 192
        defb 26, 192
        defb 16, 0
        defb 90, 0
        defb 31, 0
        defb 16, 192
        defb 29, 192
        defb 29, 192
        defb 15, 224
        defb 12, 224
        defb 11, 224
        defb 7, 240
        defb 0, 240
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Segunda columna
    ._sprite_19_b
        defb 0, 255
        defb 0, 3
        defb 248, 3
        defb 184, 3
        defb 8, 0
        defb 186, 0
        defb 248, 0
        defb 8, 3
        defb 184, 3
        defb 184, 3
        defb 240, 7
        defb 48, 7
        defb 208, 7
        defb 224, 15
        defb 0, 15
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; tercera columna
    ._sprite_19_c
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Sprite #19 y máscara
; Primera columna
    ._sprite_20_a
        defb 0, 255
        defb 0, 192
        defb 31, 192
        defb 26, 192
        defb 16, 0
        defb 90, 0
        defb 31, 0
        defb 16, 192
        defb 29, 192
        defb 29, 192
        defb 15, 224
        defb 12, 224
        defb 11, 224
        defb 7, 240
        defb 0, 240
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Segunda columna
    ._sprite_20_b
        defb 0, 255
        defb 0, 3
        defb 248, 3
        defb 184, 3
        defb 8, 0
        defb 186, 0
        defb 248, 0
        defb 8, 3
        defb 184, 3
        defb 184, 3
        defb 240, 7
        defb 48, 7
        defb 208, 7
        defb 224, 15
        defb 0, 15
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; tercera columna
    ._sprite_20_c
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Sprite #20 y máscara
; Primera columna
    ._sprite_21_a
        defb 0, 255
        defb 0, 240
        defb 12, 224
        defb 30, 192
        defb 23, 128
        defb 88, 0
        defb 93, 0
        defb 45, 128
        defb 63, 128
        defb 48, 128
        defb 45, 128
        defb 36, 128
        defb 16, 192
        defb 15, 224
        defb 0, 240
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Segunda columna
    ._sprite_21_b
        defb 0, 255
        defb 0, 15
        defb 48, 7
        defb 120, 3
        defb 232, 1
        defb 26, 0
        defb 186, 0
        defb 180, 1
        defb 252, 1
        defb 12, 1
        defb 180, 1
        defb 36, 1
        defb 8, 3
        defb 240, 7
        defb 0, 15
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; tercera columna
    ._sprite_21_c
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Sprite #21 y máscara
; Primera columna
    ._sprite_22_a
        defb 0, 255
        defb 0, 240
        defb 12, 224
        defb 30, 192
        defb 23, 128
        defb 88, 0
        defb 93, 0
        defb 45, 128
        defb 63, 128
        defb 48, 128
        defb 45, 128
        defb 36, 128
        defb 16, 192
        defb 15, 224
        defb 0, 240
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Segunda columna
    ._sprite_22_b
        defb 0, 255
        defb 0, 15
        defb 48, 7
        defb 120, 3
        defb 232, 1
        defb 26, 0
        defb 186, 0
        defb 180, 1
        defb 252, 1
        defb 12, 1
        defb 180, 1
        defb 36, 1
        defb 8, 3
        defb 240, 7
        defb 0, 15
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; tercera columna
    ._sprite_22_c
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Sprite #22 y máscara
; Primera columna
    ._sprite_23_a
        defb 0, 254
        defb 1, 252
        defb 3, 248
        defb 7, 240
        defb 7, 240
        defb 7, 240
        defb 7, 240
        defb 7, 240
        defb 6, 240
        defb 13, 224
        defb 13, 224
        defb 30, 192
        defb 63, 128
        defb 25, 192
        defb 0, 230
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Segunda columna
    ._sprite_23_b
        defb 0, 31
        defb 224, 15
        defb 240, 7
        defb 240, 7
        defb 88, 3
        defb 88, 3
        defb 88, 3
        defb 248, 3
        defb 120, 3
        defb 184, 3
        defb 88, 3
        defb 248, 3
        defb 248, 3
        defb 152, 3
        defb 0, 103
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; tercera columna
    ._sprite_23_c
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Sprite #23 y máscara
; Primera columna
    ._sprite_24_a
        defb 0, 248
        defb 7, 240
        defb 15, 224
        defb 15, 224
        defb 26, 192
        defb 26, 192
        defb 26, 192
        defb 31, 192
        defb 30, 192
        defb 29, 192
        defb 26, 192
        defb 31, 192
        defb 31, 192
        defb 25, 192
        defb 0, 230
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Segunda columna
    ._sprite_24_b
        defb 0, 127
        defb 128, 63
        defb 192, 31
        defb 224, 15
        defb 224, 15
        defb 224, 15
        defb 224, 15
        defb 224, 15
        defb 96, 15
        defb 176, 7
        defb 176, 7
        defb 120, 3
        defb 252, 1
        defb 152, 3
        defb 0, 103
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; tercera columna
    ._sprite_24_c
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Sprite #24 y máscara
; Primera columna
    ._sprite_25_a
        defb 8, 240
        defb 23, 224
        defb 46, 192
        defb 47, 192
        defb 74, 128
        defb 173, 0
        defb 183, 0
        defb 180, 0
        defb 75, 128
        defb 35, 192
        defb 20, 224
        defb 22, 224
        defb 33, 192
        defb 67, 128
        defb 64, 128
        defb 32, 192
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; Segunda columna
    ._sprite_25_b
        defb 16, 239
        defb 232, 7
        defb 4, 3
        defb 170, 1
        defb 253, 0
        defb 125, 0
        defb 242, 1
        defb 138, 0
        defb 100, 3
        defb 104, 7
        defb 144, 15
        defb 136, 7
        defb 68, 3
        defb 194, 1
        defb 132, 3
        defb 136, 7
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
; tercera columna
    ._sprite_25_c
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
        defb 0, 255
 
#endasm
 
