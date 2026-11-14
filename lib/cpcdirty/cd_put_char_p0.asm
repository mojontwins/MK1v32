; cpcdirty - Copyleft 2026 The Mojon Twins
;
; cd_put_char_p0: a pattern straight to the screen, planar mode 0: its
; colour 3 pixels in sprite pen 3 (stable whatever the level palette), the
; rest black. B = x (chars of 2 bytes), C = y (char rows), A = pattern.
; The screen is CD_SCRW bytes wide, at $C000.

	XLIB cd_put_char_p0

	INCLUDE "cd_layout.inc"

.cd_put_char_p0
	ld  l, a
	ld  h, 0
	add hl, hl
	add hl, hl
	add hl, hl
	add hl, hl
	ld  de, CD_PAT
	add hl, de
	push hl 					; pattern
	ld  hl, 0
	inc c
	ld  de, CDK_SCRW
	jr  cd_pcp_my
.cd_pcp_ml
	add hl, de 					; y * CD_SCRW
.cd_pcp_my
	dec c
	jr  nz, cd_pcp_ml
	ld  a, b
	add a, a
	ld  c, a
	ld  b, $C0
	add hl, bc 					; HL = screen (x even: inc l never carries)
	pop de 						; DE = pattern
	ld  b, 8
.cd_pcp_l
	call cd_pcp_byte
	inc l
	call cd_pcp_byte
	dec l
	ld  a, h
	add a, 8
	ld  h, a
	djnz cd_pcp_l
	ret

	; (DE) -> (HL): pixels of colour 3 (bits b2 and b3 set) in sprite
	; pen 3, the rest 0. Colour 3 in pixel 0: bits 5 and 1, pixel 1: 4 and 0
.cd_pcp_byte
	ld  a, (de)
	inc de
	ld  c, a
	rrca
	rrca
	rrca
	rrca
	and c
	and 3 						; bit 1: pixel 0, bit 0: pixel 1
	push hl
	ld  hl, cd_pcp_lut
	add a, l
	ld  l, a
	jr  nc, cd_pcp_nc
	inc h
.cd_pcp_nc
	ld  a, (hl)
	pop hl
	ld  (hl), a
	ret
.cd_pcp_lut
	defb $00, $44, $88, $CC
