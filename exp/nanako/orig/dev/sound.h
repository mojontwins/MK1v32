// Música y sonidos para Phantomas Tales #1: Marsport.
// ===================================================

extern unsigned char wyz_sound_table [];
extern unsigned char wyz_pattern_table [];
extern unsigned char wyz_effects_table [];
extern unsigned char wyz_song_table [];
extern unsigned char song_0[];
extern unsigned char song_1[];
extern unsigned char song_2[];
extern unsigned char song_3[];
extern unsigned char song_4[];

// REMEMBER: Las canciones más largas deben descomprimirse previamente
// sobre el buffer de tiles (en $100) - (ver wyz_song_table).

#asm
	._wyz_sound_table
				defw 	_sound1, _sound2, _sound3, _sound4
		
	._wyz_pattern_table
				defw	_pattern1, _pattern2, _pattern3, _pattern4, _pattern5, _pattern6
	
	._wyz_effects_table
				defw	_effect0, _effect1, _effect2, _effect3
	
	._wyz_song_table
				defw 	$100, $100, $100

; ***********************************************
;	Datos (sonidos, patrones, efectos y músicas)
; ***********************************************

; Sonidos:
	._sound1	defb	209,62,0,186,92,0,255
	._sound2	defb	139,46,0,232,43,8,255
	._sound3	defb	0,11,1,0,6,1,255
	._sound4	defb	186,58,0,0,102,0,162,131,0,255
	
; Patrones (o pautas):
	._pattern1	defb	47,15,13,11,4,129
	._pattern2	defb	77,6,8,13,8,6,12,7,3,129
	._pattern3	defb	12,9,7,9,11,9,10,10,8,10,129
	._pattern4	defb	10,12,13,13,13,12,11,11,11,10,10,10,10,10,9,9,9,9,9,138
	._pattern5	defb	71,6,7,6,5,129
	._pattern6	defb	6,8,9,8,7,6,129
	
; Efectos de sonido:

	; [0] Salto
	._effect0	defb	$C3,$0E
				defb	$CC,$0D
				defb	$D5,$0A
				defb	$DE,$06
				defb	$35,$03
				defb	$50,$0B
				defb	$47,$0C
				defb	$3E,$08
				defb	$FF
				
	; [1] Coger objeto
	._effect1	defb	$60,$0f
				defb	$40,$0c
				defb	$20,$08
				defb	$10,$0f
				defb	$60,$0f
				defb	$40,$0c
				defb	$20,$08
				defb	$10,$0f
				defb	$60,$0f
				defb	$50,$0c
				defb	$10,$0a
				defb	$10,$08
				defb	$10,$06
				defb	$10,$04
				defb	$10,$02
				defb	$60,$00
				defb	$ff
				
	; [2] Dejar Objeto
	._effect2	defb	$20,$0f
				defb	$20,$0f
				defb	$20,$0a
				defb	$20,$08
				defb	$10,$04
				defb	$20,$00
				defb	$20,$0d
				defb	$20,$0d
				defb	$10,$09
				defb	$20,$07
				defb	$10,$03
				defb	$20,$00
				defb	$20,$0a
				defb	$20,$0a
				defb	$20,$07
				defb	$20,$06
				defb	$10,$02
				defb	$20,$00
				defb	$ff

	; [3] Colisión con enemigo (muerte)
	._effect3	defb	$60,$1f
				defb	$60,$1f
				defb	$60,$1f
				defb	$60,$1f
				defb	$60,$1f
				defb	$60,$1f
				defb	$60,$1f
				defb	$60,$4f
				defb	$60,$3f
				defb	$60,$3f
				defb	$60,$2f
				defb	$60,$af
				defb	$10,$3f
				defb	$20,$0f
				defb	$20,$0f
				defb	$30,$0c
				defb	$30,$0c
				defb	$40,$09
				defb	$40,$09
				defb	$50,$06
				defb	$50,$06
				defb	$60,$03
				defb	$60,$03
				defb	$10,$3b
				defb	$20,$0b
				defb	$30,$08
				defb	$40,$05
				defb	$50,$03
				defb	$60,$02
				defb	$10,$37
				defb	$20,$07
				defb	$30,$05
				defb	$40,$04
				defb	$50,$02
				defb	$60,$01
				defb	$60,$00
				defb	$ff
	
; Músicas:

	; [0] Title
	._song_0
				BINARY "ncjmc-title.exo"
	
	; [1] Ingame
	._song_1		
				BINARY "ncjmc-ingame.exo"
	
	; [2] Jingle
	._song_2
				BINARY "ncjmc-jingle.exo"

		
#endasm

/* 
"I fought the law"

Breakin rocks in the hot sun
I fought the law and the law won (twice)
I needed money cause I had none
I fought the law and the law won (twice)

I left my baby and it feels so bad
Guess my race is run
Shes the best girl that I ever had
I fought the law and the law won
I fought the law and the

Robbin people with a six-gun
I fought the law and the law won (twice)
I lost my girl and I lost my fun
I fought the law and the law won (twice)

I left my baby and it feels so bad
Guess my race is run
Shes the best girl that I ever had
I fought the law and the law won
I fought the law and the

I fought the law and the law won (7 times)
I fought the law and the ...
*/
