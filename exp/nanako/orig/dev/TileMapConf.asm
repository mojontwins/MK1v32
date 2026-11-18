; TILEMAPCONF PARA NANAKO IN CLASSIC JAPANESE MONSTER CASTLE / CPC
; (C) 2009 THE MOJON TWINS

	XLIB TileMapConf
	
	XDEF tiles
	XDEF pantalla_juego
	XDEF tiles_tocados
	XDEF posiciones_pantalla
	XDEF posiciones_super_buffer
	
	XDEF posicion_inicial_area_visible
	XDEF posicion_inicial_superbuffer
	XDEF ancho_pantalla_bytes
	XDEF alto_pantalla_bytes
	XDEF ancho_pantalla_bytes_visible
	
	XDEF tile11

	XDEF mascara1
	XDEF mascara2
	
	; Voy a definir como máscara de los sprites el color 2
	
	; 0: $00, $00
	; 1: $80, $40
	; 2: $04, $08
	; 3: $44, $88
	; 4: $10, $20
	; 5: $50, $A0
	; 6: $14, $28
	; 7: $54, $A8
	; 8: $01, $02
	; 9: $41, $82
	; 10: $05, $0A
	; 11: $45, $8A
	; 12: $11, $22
	
	defc mascara1 = $41 ;
	defc mascara2 = $82 ;
	
; VALORES QUE DEFINEN EL BUFFER Y LA PANTALLA

	defc posicion_inicial_area_visible = $c150 ; del Excel de Arta: y = 32, x = 32
	defc posicion_inicial_superbuffer = $100
	
	defc T_WIDTH = 	24 ;32 ;28 ;32 ;28 ;16			;max=40		;dimensiones de la pantalla en tiles
	defc T_HEIGHT = 16 ;16							;max=20
		
	defc ancho_pantalla_bytes = 2*T_WIDTH ; 2*T_WIDTH;		; El ancho de pantalla influye determinantemente en 
															; numerosas rutinas que hay que actualizar si se cambia
															; OJO con el modo
	defc alto_pantalla_bytes = 8*T_HEIGHT ;
	
	defc ancho_pantalla_bytes_visible = 2*T_WIDTH ;32  ; 64;		;dentro del area definida, cuanto se debe mostrar. 2*T_WIDTH

;El tamaño del buffer es ancho_pantalla_bytes*alto_pantalla_bytes


.TileMapConf

.posiciones_pantalla		; Posiciones en las que se dibujan los tiles
	defw posicion_inicial_area_visible+$50*0
	defw posicion_inicial_area_visible+$50*1  
	defw posicion_inicial_area_visible+$50*2
	defw posicion_inicial_area_visible+$50*3
	defw posicion_inicial_area_visible+$50*4
	defw posicion_inicial_area_visible+$50*5 
	defw posicion_inicial_area_visible+$50*6 
	defw posicion_inicial_area_visible+$50*7 
	defw posicion_inicial_area_visible+$50*8 
	defw posicion_inicial_area_visible+$50*9 
	defw posicion_inicial_area_visible+$50*10 
	defw posicion_inicial_area_visible+$50*11 
	defw posicion_inicial_area_visible+$50*12 
	defw posicion_inicial_area_visible+$50*13 
	defw posicion_inicial_area_visible+$50*14 
	defw posicion_inicial_area_visible+$50*15
	defw posicion_inicial_area_visible+$50*16
	;defw posicion_inicial_area_visible+$50*17
	;defw posicion_inicial_area_visible+$50*18
	;defw posicion_inicial_area_visible+$50*19
	;defw posicion_inicial_area_visible+$50*20
	;defw posicion_inicial_area_visible+$50*21
	;defw posicion_inicial_area_visible+$50*22
	;defw posicion_inicial_area_visible+$50*23

.posiciones_super_buffer	; muestra el inicio de cada línea

	defw posicion_inicial_superbuffer+8*ancho_pantalla_bytes*0
	defw posicion_inicial_superbuffer+8*ancho_pantalla_bytes*1
	defw posicion_inicial_superbuffer+8*ancho_pantalla_bytes*2
	defw posicion_inicial_superbuffer+8*ancho_pantalla_bytes*3
	defw posicion_inicial_superbuffer+8*ancho_pantalla_bytes*4
	defw posicion_inicial_superbuffer+8*ancho_pantalla_bytes*5
	defw posicion_inicial_superbuffer+8*ancho_pantalla_bytes*6
	defw posicion_inicial_superbuffer+8*ancho_pantalla_bytes*7
	defw posicion_inicial_superbuffer+8*ancho_pantalla_bytes*8
	defw posicion_inicial_superbuffer+8*ancho_pantalla_bytes*9
	defw posicion_inicial_superbuffer+8*ancho_pantalla_bytes*10
	defw posicion_inicial_superbuffer+8*ancho_pantalla_bytes*11
	defw posicion_inicial_superbuffer+8*ancho_pantalla_bytes*12
	defw posicion_inicial_superbuffer+8*ancho_pantalla_bytes*13
	defw posicion_inicial_superbuffer+8*ancho_pantalla_bytes*14
	defw posicion_inicial_superbuffer+8*ancho_pantalla_bytes*15
	defw posicion_inicial_superbuffer+8*ancho_pantalla_bytes*16
	;defw posicion_inicial_superbuffer+8*ancho_pantalla_bytes*17
	;defw posicion_inicial_superbuffer+8*ancho_pantalla_bytes*18
	;defw posicion_inicial_superbuffer+8*ancho_pantalla_bytes*19
	;defw posicion_inicial_superbuffer+8*ancho_pantalla_bytes*20
	;defw posicion_inicial_superbuffer+8*ancho_pantalla_bytes*21
	;defw posicion_inicial_superbuffer+8*ancho_pantalla_bytes*22
	;defw posicion_inicial_superbuffer+8*ancho_pantalla_bytes*23

;Rutinas para transferir bloques independientes a la pantalla

;Se busca el subbloque de 8x8 correspondiente a las coordenadas x,y
;el bloque se calcula dividiendo la coordenada x /8

.pantalla_actual 
	defw 0

.pantalla_juego  ;en tiles
	defs T_WIDTH*T_HEIGHT
	defb $FF	;Este byte es importante, marca el fin de la pantalla.

.tiles_tocados defs 150	;cuando un tile es tocado, se marca en esta tabla para luego restaurarlo. Es una tabla sin duplicados.

.tiles ;Son de 2x8 bytes

; Extraidos con Img2CPC de Augusto Ruiz
; Y modificado a manubrio para quitar los datos 
; que no se necesitan (dimensiones del tile).

; Data from file nanako--set-import-8x8.png (256x64)... Mode 1.
._ncjmc_0
._ncjmc_0_DATA
	defb $5, $7
	defb $81, $B
	defb $5F, $7
	defb $AB, $AB
	defb $7, $85
	defb $EA, $1
	defb $AA, $A7
	defb $AA, $81
._ncjmc_1
._ncjmc_1_DATA
	defb $4B, $0
	defb $F, $80
	defb $7, $82
	defb $F, $A
	defb $C1, $A
	defb $40, $A
	defb $15, $A
	defb $80, $A
._ncjmc_2
._ncjmc_2_DATA
	defb $3, $7
	defb $B, $B
	defb $85, $AF
	defb $1, $EA
	defb $5, $0
	defb $5, $EA
	defb $40, $F
	defb $0, $C0
._ncjmc_3
._ncjmc_3_DATA
	defb $87, $A
	defb $F, $82
	defb $7, $80
	defb $7, $0
	defb $5, $0
	defb $43, $0
	defb $C2, $0
	defb $80, $0
._ncjmc_4
._ncjmc_4_DATA
	defb $81, $A9
	defb $40, $56
	defb $0, $81
	defb $0, $40
	defb $40, $7C
	defb $94, $A9
	defb $D4, $56
	defb $29, $3
._ncjmc_5
._ncjmc_5_DATA
	defb $BC, $80
	defb $68, $0
	defb $80, $0
	defb $0, $0
	defb $68, $0
	defb $BC, $0
	defb $56, $80
	defb $FC, $28
._ncjmc_6
._ncjmc_6_DATA
	defb $3, $3
	defb $A9, $AB
	defb $3, $3
	defb $A9, $AB
	defb $FC, $56
	defb $81, $A9
	defb $40, $FC
	defb $0, $C0
._ncjmc_7
._ncjmc_7_DATA
	defb $56, $28
	defb $FC, $A8
	defb $56, $28
	defb $FC, $A8
	defb $56, $28
	defb $BC, $80
	defb $68, $0
	defb $80, $0
._ncjmc_8
._ncjmc_8_DATA
	defb $FF, $FD
	defb $FE, $FE
	defb $FF, $BC
	defb $BC, $2C
	defb $0, $0
	defb $84, $C0
	defb $C, $48
	defb $2C, $2C
._ncjmc_9
._ncjmc_9_DATA
	defb $BC, $A8
	defb $7C, $28
	defb $3C, $28
	defb $C, $8
	defb $0, $0
	defb $C0, $80
	defb $48, $80
	defb $84, $80
._ncjmc_10
._ncjmc_10_DATA
	defb $C, $48
	defb $2C, $2C
	defb $C, $C
	defb $FF, $FD
	defb $FE, $FE
	defb $FF, $BC
	defb $FC, $3C
	defb $0, $0
._ncjmc_11
._ncjmc_11_DATA
	defb $48, $80
	defb $84, $80
	defb $48, $80
	defb $BC, $A8
	defb $7C, $28
	defb $3C, $28
	defb $C, $8
	defb $0, $0
._ncjmc_12
._ncjmc_12_DATA
	defb $FA, $F0
	defb $F0, $B0
	defb $70, $30
	defb $30, $CC
	defb $0, $0
	defb $C4, $C0
	defb $CC, $C8
	defb $64, $64
._ncjmc_13
._ncjmc_13_DATA
	defb $70, $20
	defb $B0, $20
	defb $30, $20
	defb $C8, $80
	defb $0, $0
	defb $C0, $80
	defb $C8, $80
	defb $C4, $80
._ncjmc_14
._ncjmc_14_DATA
	defb $CC, $C8
	defb $64, $64
	defb $CC, $CC
	defb $FA, $F0
	defb $F0, $B0
	defb $F0, $70
	defb $30, $CC
	defb $0, $0
._ncjmc_15
._ncjmc_15_DATA
	defb $C8, $80
	defb $C4, $80
	defb $C8, $80
	defb $F0, $20
	defb $B0, $A0
	defb $30, $20
	defb $C8, $80
	defb $0, $0
._ncjmc_16
._ncjmc_16_DATA
	defb $DC, $30
	defb $F8, $B0
	defb $F8, $70
	defb $F8, $B0
	defb $F8, $70
	defb $F8, $B0
	defb $D4, $64
	defb $0, $0
._ncjmc_17
._ncjmc_17_DATA
	defb $64, $80
	defb $98, $C8
	defb $64, $C0
	defb $30, $C8
	defb $64, $C0
	defb $C8, $C8
	defb $C0, $80
	defb $0, $0
._ncjmc_18
._ncjmc_18_DATA
	defb $64, $80
	defb $98, $C8
	defb $64, $C0
	defb $30, $C8
	defb $64, $C0
	defb $C8, $C8
	defb $C0, $80
	defb $0, $0
._ncjmc_19
._ncjmc_19_DATA
	defb $DC, $30
	defb $F8, $B0
	defb $F8, $70
	defb $F8, $B0
	defb $F8, $70
	defb $F8, $B0
	defb $D4, $64
	defb $0, $0
._ncjmc_20
._ncjmc_20_DATA
	defb $C, $C
	defb $C, $C
	defb $C, $C
	defb $C0, $C0
	defb $C, $C
	defb $C, $C
	defb $C, $C
	defb $C0, $C0
._ncjmc_21
._ncjmc_21_DATA
	defb $C, $C
	defb $C, $C
	defb $C, $C
	defb $C0, $C0
	defb $C, $C
	defb $C, $C
	defb $C, $C
	defb $C0, $C0
._ncjmc_22
._ncjmc_22_DATA
	defb $C, $C
	defb $C0, $C0
	defb $C0, $C0
	defb $C0, $C0
	defb $C, $C
	defb $C0, $C0
	defb $C0, $C0
	defb $C0, $C0
._ncjmc_23
._ncjmc_23_DATA
	defb $C, $C
	defb $C0, $C0
	defb $C0, $C0
	defb $C0, $C0
	defb $C, $C
	defb $C0, $C0
	defb $C0, $C0
	defb $C0, $C0
._ncjmc_24
._ncjmc_24_DATA
	defb $C0, $C0
	defb $0, $0
	defb $C0, $C0
	defb $C0, $C0
	defb $C0, $C0
	defb $0, $0
	defb $C0, $C0
	defb $C0, $C0
._ncjmc_25
._ncjmc_25_DATA
	defb $C0, $C0
	defb $0, $0
	defb $C0, $C0
	defb $C0, $C0
	defb $C0, $C0
	defb $0, $0
	defb $C0, $C0
	defb $C0, $C0
._ncjmc_26
._ncjmc_26_DATA
	defb $C0, $C0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $C0, $C0
	defb $0, $0
	defb $0, $0
	defb $0, $0
._ncjmc_27
._ncjmc_27_DATA
	defb $C0, $C0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $C0, $C0
	defb $0, $0
	defb $0, $0
	defb $0, $0
._ncjmc_28
._ncjmc_28_DATA
	defb $40, $0
	defb $0, $80
	defb $0, $40
	defb $0, $0
	defb $0, $40
	defb $0, $0
	defb $0, $40
	defb $80, $80
._ncjmc_29
._ncjmc_29_DATA
	defb $40, $40
	defb $80, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $80, $80
._ncjmc_30
._ncjmc_30_DATA
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $40, $0
	defb $80, $80
._ncjmc_31
._ncjmc_31_DATA
	defb $40, $0
	defb $0, $80
	defb $0, $0
	defb $0, $80
	defb $0, $0
	defb $0, $80
	defb $40, $40
	defb $80, $80
._ncjmc_32
._ncjmc_32_DATA
	defb $40, $0
	defb $0, $80
	defb $0, $40
	defb $0, $0
	defb $0, $40
	defb $0, $0
	defb $0, $40
	defb $80, $80
._ncjmc_33
._ncjmc_33_DATA
	defb $40, $40
	defb $80, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $80, $80
._ncjmc_34
._ncjmc_34_DATA
	defb $40, $C0
	defb $C0, $F3
	defb $80, $0
	defb $0, $D1
	defb $0, $0
	defb $C0, $D1
	defb $0, $0
	defb $C0, $F3
._ncjmc_35
._ncjmc_35_DATA
	defb $84, $C0
	defb $59, $48
	defb $0, $40
	defb $59, $48
	defb $0, $40
	defb $48, $E2
	defb $0, $40
	defb $59, $80
._ncjmc_36
._ncjmc_36_DATA
	defb $C7, $CF
	defb $CF, $47
	defb $AB, $CF
	defb $FF, $47
	defb $AB, $CF
	defb $8B, $47
	defb $C7, $CF
	defb $0, $0
._ncjmc_37
._ncjmc_37_DATA
	defb $8F, $C2
	defb $F, $4B
	defb $8F, $C3
	defb $F, $4B
	defb $8F, $C3
	defb $F, $4B
	defb $8F, $C2
	defb $0, $0
._ncjmc_38
._ncjmc_38_DATA
	defb $C2, $80
	defb $C3, $C0
	defb $4B, $C0
	defb $C3, $C2
	defb $4B, $C0
	defb $C3, $C2
	defb $4B, $80
	defb $0, $0
._ncjmc_39
._ncjmc_39_DATA
	defb $87, $F
	defb $CF, $8F
	defb $47, $4F
	defb $8B, $8F
	defb $47, $F
	defb $8F, $8F
	defb $C7, $F
	defb $0, $0
._ncjmc_40
._ncjmc_40_DATA
	defb $81, $AB
	defb $57, $FF
	defb $AB, $AB
	defb $57, $57
	defb $3, $3
	defb $3, $52
	defb $A1, $F0
	defb $3, $52
._ncjmc_41
._ncjmc_41_DATA
	defb $3, $A1
	defb $3, $52
	defb $3, $F0
	defb $3, $52
	defb $3, $A1
	defb $3, $56
	defb $A1, $3
	defb $3, $56
._ncjmc_42
._ncjmc_42_DATA
	defb $3, $56
	defb $3, $E8
	defb $56, $A8
	defb $3, $A8
	defb $56, $A8
	defb $A9, $80
	defb $81, $0
	defb $54, $0
._ncjmc_43
._ncjmc_43_DATA
	defb $56, $42
	defb $3, $A8
	defb $56, $A8
	defb $52, $80
	defb $56, $0
	defb $42, $0
	defb $A0, $0
	defb $80, $0
._ncjmc_44
._ncjmc_44_DATA
	defb $40, $64
	defb $90, $8C
	defb $24, $C8
	defb $C, $4C
	defb $C0, $C
	defb $64, $84
	defb $8C, $60
	defb $4C, $4C
._ncjmc_45
._ncjmc_45_DATA
	defb $CC, $8C
	defb $C0, $4C
	defb $98, $84
	defb $4C, $C4
	defb $8C, $84
	defb $48, $64
	defb $90, $8C
	defb $64, $4C
._ncjmc_46
._ncjmc_46_DATA
	defb $C, $8C
	defb $C0, $C
	defb $64, $84
	defb $C, $C8
	defb $C, $C
	defb $84, $C
	defb $40, $C
	defb $0, $C0
._ncjmc_47
._ncjmc_47_DATA
	defb $8C, $8C
	defb $4C, $C
	defb $C, $C
	defb $C, $48
	defb $48, $64
	defb $90, $CC
	defb $C4, $8C
	defb $40, $C0
._ncjmc_48
._ncjmc_48_DATA
	defb $98, $8C
	defb $48, $48
	defb $8, $84
	defb $4C, $40
	defb $C, $88
	defb $C, $4C
	defb $C, $8
	defb $C, $4
._ncjmc_49
._ncjmc_49_DATA
	defb $C8, $0
	defb $48, $80
	defb $84, $80
	defb $48, $80
	defb $C0, $0
	defb $0, $0
	defb $CC, $88
	defb $48, $8
._ncjmc_50
._ncjmc_50_DATA
	defb $48, $40
	defb $84, $88
	defb $48, $C4
	defb $0, $40
	defb $CC, $88
	defb $48, $C0
	defb $84, $C0
	defb $C0, $C0
._ncjmc_51
._ncjmc_51_DATA
	defb $80, $8
	defb $44, $80
	defb $C8, $80
	defb $C0, $80
	defb $40, $80
	defb $80, $80
	defb $C0, $0
	defb $80, $0
._ncjmc_52
._ncjmc_52_DATA
	defb $30, $99
	defb $CC, $CC
	defb $33, $32
	defb $33, $30
	defb $32, $32
	defb $31, $30
	defb $30, $98
	defb $C0, $80
._ncjmc_53
._ncjmc_53_DATA
	defb $64, $32
	defb $CC, $CC
	defb $33, $30
	defb $32, $32
	defb $33, $30
	defb $32, $98
	defb $64, $98
	defb $C0, $C0
._ncjmc_54
._ncjmc_54_DATA
	defb $30, $99
	defb $CC, $CC
	defb $31, $33
	defb $31, $31
	defb $31, $32
	defb $31, $64
	defb $30, $CC
	defb $C0, $C0
._ncjmc_55
._ncjmc_55_DATA
	defb $30, $99
	defb $CC, $CC
	defb $31, $33
	defb $31, $32
	defb $31, $31
	defb $31, $32
	defb $31, $64
	defb $C0, $80
._ncjmc_56
._ncjmc_56_DATA
	defb $C0, $C0
	defb $CC, $CC
	defb $98, $98
	defb $30, $30
	defb $B0, $B0
	defb $78, $78
	defb $1C, $1C
	defb $0, $0
._ncjmc_57
	
._ncjmc_57_DATA
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
._ncjmc_58
	
._ncjmc_58_DATA
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
._ncjmc_59
	
._ncjmc_59_DATA
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
._ncjmc_60
	
._ncjmc_60_DATA
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
._ncjmc_61
	
._ncjmc_61_DATA
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
._ncjmc_62
	
._ncjmc_62_DATA
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
._ncjmc_63
	
._ncjmc_63_DATA
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
._ncjmc_64
	
._ncjmc_64_DATA
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
._ncjmc_65
	
._ncjmc_65_DATA
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
._ncjmc_66
	
._ncjmc_66_DATA
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
._ncjmc_67
	
._ncjmc_67_DATA
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
._ncjmc_68
	
._ncjmc_68_DATA
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
._ncjmc_69
	
._ncjmc_69_DATA
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
._ncjmc_70
	
._ncjmc_70_DATA
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
._ncjmc_71
	
._ncjmc_71_DATA
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
._ncjmc_72
	
._ncjmc_72_DATA
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
._ncjmc_73
	
._ncjmc_73_DATA
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
._ncjmc_74
	
._ncjmc_74_DATA
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
._ncjmc_75
	
._ncjmc_75_DATA
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
._ncjmc_76
	
._ncjmc_76_DATA
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
._ncjmc_77
	
._ncjmc_77_DATA
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
._ncjmc_78
	
._ncjmc_78_DATA
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
._ncjmc_79
	
._ncjmc_79_DATA
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
._ncjmc_80
	
._ncjmc_80_DATA
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $54, $B0
	defb $50, $C0
	defb $50, $0
	defb $10, $10
	defb $10, $10
._ncjmc_81
	
._ncjmc_81_DATA
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $64, $64
	defb $C4, $C0
	defb $4, $0
	defb $64, $CC
	defb $84, $C0
._ncjmc_82
	
._ncjmc_82_DATA
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $8C, $8C
	defb $C0, $C0
	defb $0, $0
	defb $4C, $C
	defb $C0, $C0
._ncjmc_83
	
._ncjmc_83_DATA
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $8C, $C
	defb $C0, $C0
	defb $0, $0
	defb $C, $C
	defb $C0, $C0
._ncjmc_84
	
._ncjmc_84_DATA
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $C, $C
	defb $C0, $C0
	defb $0, $0
	defb $C, $C
	defb $C0, $C0
._ncjmc_85
	
._ncjmc_85_DATA
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $C, $4C
	defb $C0, $C0
	defb $0, $0
	defb $C, $C
	defb $C0, $C0
._ncjmc_86
	
._ncjmc_86_DATA
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $4C, $4C
	defb $C0, $C0
	defb $0, $0
	defb $C, $8C
	defb $C0, $C0
._ncjmc_87
	
._ncjmc_87_DATA
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $98, $98
	defb $C0, $C8
	defb $0, $8
	defb $CC, $98
	defb $C0, $48
._ncjmc_88
	
._ncjmc_88_DATA
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $70, $A8
	defb $C0, $A0
	defb $0, $A0
	defb $20, $20
	defb $20, $20
._ncjmc_89
	
._ncjmc_89_DATA
	defb $44, $44
	defb $10, $4C
	defb $44, $C4
	defb $44, $4
	defb $4, $44
	defb $44, $4
	defb $4, $4
	defb $44, $4
._ncjmc_90
	
._ncjmc_90_DATA
	defb $44, $0
	defb $C, $0
	defb $C0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
._ncjmc_91
	
._ncjmc_91_DATA
	defb $0, $88
	defb $0, $C
	defb $0, $C0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
._ncjmc_92
	
._ncjmc_92_DATA
	defb $88, $88
	defb $8C, $20
	defb $C8, $88
	defb $8, $88
	defb $88, $8
	defb $8, $88
	defb $8, $8
	defb $8, $88
._ncjmc_93
	
._ncjmc_93_DATA
	defb $4, $4
	defb $4, $4
	defb $4, $4
	defb $4, $4
	defb $4, $4
	defb $4, $4
	defb $4, $4
	defb $4, $4
._ncjmc_94
	
._ncjmc_94_DATA
	defb $8, $8
	defb $8, $8
	defb $8, $8
	defb $8, $8
	defb $8, $8
	defb $8, $8
	defb $8, $8
	defb $8, $8
._ncjmc_95
	
._ncjmc_95_DATA
	defb $4, $4
	defb $44, $4
	defb $4, $4
	defb $44, $4
	defb $4, $44
	defb $44, $4
	defb $44, $C4
	defb $10, $4C
._ncjmc_96
	
._ncjmc_96_DATA
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $C0, $0
	defb $C, $0
._ncjmc_97
	
._ncjmc_97_DATA
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $0
	defb $0, $C0
	defb $0, $C
._ncjmc_98
	
._ncjmc_98_DATA
	defb $8, $8
	defb $8, $88
	defb $8, $8
	defb $8, $88
	defb $88, $8
	defb $8, $88
	defb $C8, $88
	defb $8C, $20
._ncjmc_99
	
._ncjmc_99_DATA
	defb $44, $44
	defb $10, $10
	defb $10, $10
	defb $50, $0
	defb $50, $C0
	defb $54, $B0
	defb $0, $0
	defb $0, $0
._ncjmc_100
	
._ncjmc_100_DATA
	defb $44, $0
	defb $84, $C0
	defb $64, $CC
	defb $4, $0
	defb $C4, $C0
	defb $64, $64
	defb $0, $0
	defb $0, $0
._ncjmc_101
	
._ncjmc_101_DATA
	defb $0, $0
	defb $C0, $C0
	defb $4C, $C
	defb $0, $0
	defb $C0, $C0
	defb $8C, $8C
	defb $0, $0
	defb $0, $0
._ncjmc_102
	
._ncjmc_102_DATA
	defb $0, $0
	defb $C0, $C0
	defb $C, $C
	defb $0, $0
	defb $C0, $C0
	defb $8C, $C
	defb $0, $0
	defb $0, $0
._ncjmc_103
	
._ncjmc_103_DATA
	defb $0, $0
	defb $C0, $C0
	defb $C, $C
	defb $0, $0
	defb $C0, $C0
	defb $C, $C
	defb $0, $0
	defb $0, $0
._ncjmc_104
	
._ncjmc_104_DATA
	defb $0, $0
	defb $C0, $C0
	defb $C, $C
	defb $0, $0
	defb $C0, $C0
	defb $C, $4C
	defb $0, $0
	defb $0, $0
._ncjmc_105
	
._ncjmc_105_DATA
	defb $0, $0
	defb $C0, $C0
	defb $C, $8C
	defb $0, $0
	defb $C0, $C0
	defb $4C, $4C
	defb $0, $0
	defb $0, $0
._ncjmc_106
	
._ncjmc_106_DATA
	defb $0, $88
	defb $C0, $48
	defb $CC, $98
	defb $0, $8
	defb $C0, $C8
	defb $98, $98
	defb $0, $0
	defb $0, $0
._ncjmc_107
	
._ncjmc_107_DATA
	defb $88, $88
	defb $20, $20
	defb $20, $20
	defb $0, $A0
	defb $C0, $A0
	defb $70, $A8
	defb $0, $0
	defb $0, $0
	