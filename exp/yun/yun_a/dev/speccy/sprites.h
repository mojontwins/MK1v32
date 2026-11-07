// MTE MK1 (la Churrera) v5.0
// Copyleft 2010-2014, 2020 by the Mojon Twins

// Sprites.h
// No masks
 
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
    ._sprite_1_a
        defb 0
    ._sprite_2_a
        defb 3
        defb 1, 12
        defb 6, 16
        defb 8, 32
        defb 16, 33
        defb 18, 64
        defb 39, 67
        defb 43, 75
        defb 41, 37
        defb 16, 13
        defb 62, 13
        defb 63, 6
        defb 31, 3
        defb 79, 1
        defb 126, 3
        defb 88, 3
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
 
; Segunda columna
    ._sprite_1_b
        defb 0
    ._sprite_2_b
        defb 240
        defb 248, 8
        defb 4, 0
        defb 0, 216
        defb 52, 216
        defb 116, 240
        defb 120, 0
        defb 0, 32
        defb 208, 32
        defb 32, 64
        defb 120, 64
        defb 124, 192
        defb 124, 128
        defb 4, 128
        defb 7, 0
        defb 0, 192
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
 
; tercera columna
    ._sprite_1_c
        defb 0
    ._sprite_2_c
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
 
; Sprites #2 y 3
; Primera columna
    ._sprite_3_a
        defb 0
    ._sprite_4_a
        defb 3
        defb 15, 12
        defb 16, 16
        defb 32, 32
        defb 67, 64
        defb 67, 79
        defb 89, 27
        defb 120, 0
        defb 200, 31
        defb 159, 215
        defb 59, 87
        defb 59, 123
        defb 27, 60
        defb 76, 0
        defb 126, 0
        defb 88, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
 
; Segunda columna
    ._sprite_3_b
        defb 0
    ._sprite_4_b
        defb 248
        defb 224, 4
        defb 16, 0
        defb 0, 108
        defb 96, 236
        defb 96, 120
        defb 240, 128
        defb 0, 0
        defb 128, 176
        defb 224, 248
        defb 248, 232
        defb 252, 232
        defb 220, 216
        defb 4, 48
        defb 7, 56
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
    ._sprite_3_c
        defb 0
    ._sprite_4_c
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
 
; Sprites #4 y 5
; Primera columna
    ._sprite_5_a
        defb 0
    ._sprite_6_a
        defb 15
        defb 7, 16
        defb 8, 0
        defb 0, 27
        defb 6, 27
        defb 6, 15
        defb 15, 0
        defb 0, 4
        defb 1, 4
        defb 7, 2
        defb 31, 2
        defb 63, 3
        defb 59, 1
        defb 32, 1
        defb 224, 0
        defb 0, 3
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
 
; Segunda columna
    ._sprite_5_b
        defb 0
    ._sprite_6_b
        defb 192
        defb 240, 48
        defb 8, 8
        defb 4, 4
        defb 194, 132
        defb 194, 2
        defb 154, 194
        defb 30, 210
        defb 19, 164
        defb 249, 176
        defb 220, 176
        defb 220, 96
        defb 216, 192
        defb 50, 128
        defb 126, 192
        defb 26, 192
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
 
; tercera columna
    ._sprite_5_c
        defb 0
    ._sprite_6_c
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
 
; Sprites #6 y 7
; Primera columna
    ._sprite_7_a
        defb 0
    ._sprite_8_a
        defb 31
        defb 31, 32
        defb 32, 0
        defb 0, 54
        defb 44, 55
        defb 46, 30
        defb 30, 1
        defb 0, 0
        defb 11, 13
        defb 4, 31
        defb 30, 23
        defb 62, 23
        defb 62, 27
        defb 32, 12
        defb 224, 28
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
    ._sprite_7_b
        defb 0
    ._sprite_8_b
        defb 192
        defb 128, 48
        defb 96, 8
        defb 16, 4
        defb 8, 2
        defb 72, 242
        defb 228, 216
        defb 212, 0
        defb 148, 248
        defb 8, 235
        defb 124, 234
        defb 252, 222
        defb 248, 60
        defb 242, 0
        defb 126, 0
        defb 26, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
 
; tercera columna
    ._sprite_7_c
        defb 0
    ._sprite_8_c
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
 
; Sprites #8 y 9
; Primera columna
    ._sprite_9_a
        defb 0
    ._sprite_10_a
        defb 0
        defb 0, 3
        defb 7, 12
        defb 24, 16
        defb 32, 32
        defb 64, 32
        defb 64, 64
        defb 128, 64
        defb 128, 64
        defb 64, 64
        defb 64, 32
        defb 32, 32
        defb 24, 16
        defb 7, 12
        defb 0, 3
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
    ._sprite_9_b
        defb 0
    ._sprite_10_b
        defb 0
        defb 0, 192
        defb 224, 48
        defb 24, 8
        defb 4, 36
        defb 50, 20
        defb 2, 2
        defb 1, 2
        defb 1, 2
        defb 2, 2
        defb 2, 4
        defb 4, 4
        defb 24, 8
        defb 224, 48
        defb 0, 192
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
    ._sprite_9_c
        defb 0
    ._sprite_10_c
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
 
; Sprites #10 y 11
; Primera columna
    ._sprite_11_a
        defb 255
    ._sprite_12_a
        defb 255
        defb 255, 255
        defb 255, 255
        defb 255, 255
        defb 255, 255
        defb 0, 0
        defb 15, 15
        defb 15, 15
        defb 0, 0
        defb 1, 0
        defb 2, 1
        defb 4, 1
        defb 4, 1
        defb 2, 1
        defb 1, 1
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
    ._sprite_11_b
        defb 255
    ._sprite_12_b
        defb 255
        defb 255, 255
        defb 255, 255
        defb 255, 255
        defb 255, 255
        defb 0, 0
        defb 240, 240
        defb 240, 240
        defb 0, 0
        defb 128, 0
        defb 64, 128
        defb 32, 128
        defb 32, 128
        defb 64, 128
        defb 128, 128
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
    ._sprite_11_c
        defb 0
    ._sprite_12_c
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
 
; Sprites #12 y 13
; Primera columna
    ._sprite_13_a
        defb 15
    ._sprite_14_a
        defb 15
        defb 16, 16
        defb 32, 32
        defb 32, 32
        defb 32, 32
        defb 32, 33
        defb 32, 33
        defb 64, 65
        defb 64, 64
        defb 64, 64
        defb 64, 64
        defb 128, 128
        defb 128, 128
        defb 176, 134
        defb 73, 73
        defb 6, 48
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
 
; Segunda columna
    ._sprite_13_b
        defb 240
    ._sprite_14_b
        defb 240
        defb 8, 8
        defb 4, 4
        defb 4, 4
        defb 2, 2
        defb 218, 178
        defb 218, 178
        defb 218, 178
        defb 2, 2
        defb 2, 2
        defb 98, 98
        defb 158, 158
        defb 4, 4
        defb 196, 24
        defb 36, 32
        defb 24, 192
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
 
; tercera columna
    ._sprite_13_c
        defb 0
    ._sprite_14_c
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
 
; Sprites #14 y 15
; Primera columna
    ._sprite_15_a
        defb 6
    ._sprite_16_a
        defb 48
        defb 73, 73
        defb 176, 134
        defb 128, 128
        defb 128, 128
        defb 64, 64
        defb 64, 64
        defb 64, 64
        defb 64, 65
        defb 32, 33
        defb 32, 33
        defb 32, 32
        defb 32, 32
        defb 32, 32
        defb 16, 16
        defb 15, 15
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
 
; Segunda columna
    ._sprite_15_b
        defb 24
    ._sprite_16_b
        defb 192
        defb 36, 32
        defb 196, 24
        defb 4, 4
        defb 158, 158
        defb 98, 98
        defb 2, 2
        defb 2, 2
        defb 218, 178
        defb 218, 178
        defb 218, 178
        defb 2, 2
        defb 4, 4
        defb 4, 4
        defb 8, 8
        defb 240, 240
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
 
; tercera columna
    ._sprite_15_c
        defb 0
    ._sprite_16_c
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
 
; Sprites #16 y 17
; Primera columna
    ._sprite_17_a
        defb 14
    ._sprite_18_a
        defb 14
        defb 95, 95
        defb 95, 95
        defb 15, 15
        defb 70, 70
        defb 240, 240
        defb 240, 240
        defb 108, 108
        defb 30, 3
        defb 158, 117
        defb 158, 116
        defb 12, 59
        defb 224, 14
        defb 241, 231
        defb 245, 215
        defb 224, 176
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
 
; Segunda columna
    ._sprite_17_b
        defb 14
    ._sprite_18_b
        defb 14
        defb 95, 95
        defb 95, 95
        defb 15, 15
        defb 70, 70
        defb 240, 240
        defb 240, 240
        defb 108, 108
        defb 30, 161
        defb 158, 87
        defb 158, 23
        defb 12, 246
        defb 224, 184
        defb 241, 245
        defb 245, 237
        defb 224, 56
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
 
; tercera columna
    ._sprite_17_c
        defb 0
    ._sprite_18_c
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
 
; Sprites #18 y 19
; Primera columna
    ._sprite_19_a
        defb 0
    ._sprite_20_a
        defb 224
        defb 51, 96
        defb 119, 112
        defb 102, 63
        defb 108, 31
        defb 107, 31
        defb 58, 28
        defb 27, 27
        defb 27, 27
        defb 28, 58
        defb 31, 107
        defb 31, 108
        defb 63, 102
        defb 112, 119
        defb 96, 51
        defb 224, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
 
; Segunda columna
    ._sprite_19_b
        defb 0
    ._sprite_20_b
        defb 7
        defb 230, 6
        defb 247, 14
        defb 179, 252
        defb 19, 120
        defb 235, 248
        defb 174, 24
        defb 236, 104
        defb 104, 236
        defb 24, 174
        defb 248, 235
        defb 120, 19
        defb 252, 179
        defb 14, 247
        defb 6, 230
        defb 7, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
        defb 0, 0
 
; tercera columna
    ._sprite_19_c
        defb 0
    ._sprite_20_c
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
 
; Sprites #20 y 21
; Primera columna
    ._sprite_21_a
        defb 0
    ._sprite_22_a
        defb 0
        defb 0, 24
        defb 0, 61
        defb 0, 124
        defb 1, 124
        defb 7, 120
        defb 7, 32
        defb 15, 0
        defb 15, 16
        defb 7, 61
        defb 7, 123
        defb 1, 123
        defb 0, 113
        defb 0, 32
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
 
; Segunda columna
    ._sprite_21_b
        defb 0
    ._sprite_22_b
        defb 0
        defb 0, 240
        defb 0, 248
        defb 0, 224
        defb 128, 220
        defb 224, 28
        defb 224, 24
        defb 240, 64
        defb 240, 96
        defb 224, 252
        defb 224, 252
        defb 128, 248
        defb 0, 224
        defb 0, 192
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
 
; tercera columna
    ._sprite_21_c
        defb 0
    ._sprite_22_c
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
 
#endasm
 
