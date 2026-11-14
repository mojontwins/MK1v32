; cpcdirty - Copyleft 2026 The Mojon Twins
;
; cd_pal_m1: mode 1 palette. HL = the 4 hardware inks of pens 0-3 (mkts_om
; mode=pals of a 4 colour png); the border gets pen 0's.

	XLIB cd_pal_m1

.cd_pal_m1
	ld  bc, $7F00
	ld  e, 0
	push hl
.cd_pm1_loop
	ld  a, (hl)
	inc hl
	or  $40
	out (c), e
	out (c), a
	inc e
	ld  a, e
	cp  4
	jr  nz, cd_pm1_loop
	pop hl
	ld  a, (hl)
	or  $40
	ld  e, 16
	out (c), e
	out (c), a
	ret
