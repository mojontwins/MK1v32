; cpcdirty - Copyleft 2026 The Mojon Twins
;
; cd_put_char_raw: a pattern straight to the screen, as it is (mode 1,
; nibble mode 0). B = x (chars of 2 bytes), C = y (char rows), A = pattern.
; The screen is CD_SCRW bytes wide, at $C000.

	XLIB cd_put_char_raw

	INCLUDE "cd_layout.inc"

.cd_put_char_raw
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
	jr  cd_pcr_my
.cd_pcr_ml
	add hl, de 					; y * CD_SCRW
.cd_pcr_my
	dec c
	jr  nz, cd_pcr_ml
	ld  a, b
	add a, a
	ld  c, a
	ld  b, $C0
	add hl, bc 					; HL = screen (x even: inc l never carries)
	pop de 						; DE = pattern
	ld  b, 8
.cd_pcr_l
	ld  a, (de)
	inc de
	ld  (hl), a
	inc l
	ld  a, (de)
	inc de
	ld  (hl), a
	dec l
	ld  a, h
	add a, 8
	ld  h, a
	djnz cd_pcr_l
	ret
