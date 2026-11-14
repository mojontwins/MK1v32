; cpcdirty - Copyleft 2026 The Mojon Twins
;
; cd_pal_n0: nibble mode 0 palette. HL = the 16 hardware inks of pens 0-15
; (mkts_om mode=pals); the border gets pen 0's.

	XLIB cd_pal_n0

.cd_pal_n0
	ld  bc, $7F00
	ld  e, 0
	push hl
.cd_pn0_loop
	ld  a, (hl)
	inc hl
	or  $40
	out (c), e
	out (c), a
	inc e
	ld  a, e
	cp  16
	jr  nz, cd_pn0_loop
	pop hl
	ld  a, (hl)
	or  $40
	out (c), e
	out (c), a
	ret
