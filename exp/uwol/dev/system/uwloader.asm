; Uwol (CPC) - tape: the loader, at BA00: black screen, the game (zx0, raw)
; below BA00, unpacked to 0400, run. sizes.inc: MAIN_SIZE (the zx0's).

	org $BA00

	defc COLORES_CARGA = $564E

	INCLUDE "sizes.inc"

	ld   sp, $BFFF
	; mode 0, ROMs off; all pens and the border black (hardware 20)
	ld   bc, $7F8C
	out  (c), c
	ld   e, 16
.ld_pal
	out  (c), e
	ld   a, $54
	out  (c), a
	dec  e
	jp   p, ld_pal
	; a black screen
	ld   hl, $C000
	ld   de, $C001
	ld   bc, $3FFF
	ld   (hl), 0
	ldir

	ld   ix, $BA00 - MAIN_SIZE
	ld   de, MAIN_SIZE
	call cpct_miniload_asm
	jr   nc, ld_fail
	ld   hl, $BA00 - MAIN_SIZE
	ld   de, $0400
	call dzx0_standard
	jp   $0400
.ld_fail
	rst  0 					; reset: try again

	INCLUDE "miniload.inc"
	INCLUDE "dzx0.inc"
