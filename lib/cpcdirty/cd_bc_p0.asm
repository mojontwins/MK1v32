; cpcdirty - Copyleft 2026 The Mojon Twins
;
; cd_bc_p0: planar mode 0 sprite cache (sprite pens only, no mask).

	XLIB cd_bc_p0

	XDEF cd_bc_h
	XDEF cd_bc_w
	XDEF cd_bc_st

	INCLUDE "cd_layout.inc"

; HL = frame, DE = cache, B = b0, C = s; cd_bc_h lines, cd_bc_w bytes
; a line, cd_bc_st = 2h - 1 (CD_GEOMETRY; else 16 lines, 4 bytes)
.cd_bc_p0
	ld  (cd_bc_dst), de
	ld  a, b
	ld  (cd_bc_b0), a
	ld  a, c
	ld  (cd_bc_s), a
	ld  a, (cd_bc_h)
	ld  b, a
.cd_bc_line
	push bc
	; TMP = 7 zero bytes
	xor a
	ld  (CD_TMP), a
	ld  (CD_TMP + 1), a
	ld  (CD_TMP + 2), a
	ld  (CD_TMP + 3), a
	ld  (CD_TMP + 4), a
	ld  (CD_TMP + 5), a
	ld  (CD_TMP + 6), a
	ld  a, (cd_bc_b0)
	ld  e, a
	ld  d, 0
	push hl
	ld  hl, CD_TMP
	add hl, de
	ex  de, hl
	pop hl
	ld  a, (cd_bc_w)
	ld  b, a
	ld  a, (cd_bc_s)
	or  a
	jr  nz, cd_bc_shift
.cd_bc_cp
	ld  a, (hl)
	ld  (de), a
	inc hl
	inc de
	djnz cd_bc_cp
	jr  cd_bc_store
.cd_bc_shift
	; out = ((prev & 0x55) << 1) | ((cur & 0xAA) >> 1)
	ld  c, 0
.cd_bc_sh
	ld  a, c
	and $55
	add a, a
	ld  c, a
	ld  a, (hl)
	inc hl
	push af
	and $AA
	rrca
	or  c
	ld  (de), a
	inc de
	pop af
	ld  c, a
	djnz cd_bc_sh
	ld  a, c
	and $55
	add a, a
	ld  (de), a
.cd_bc_store
	; TMP [0..] -> cache + 0, 1, 2h, 2h + 1, 4h... (+ line * 2), w / 2 + 1 columns
	push hl
	ld  hl, (cd_bc_dst)
	ld  de, CD_TMP
	ld  a, (cd_bc_w)
	srl a
	inc a
	ld  b, a
.cd_bc_st_col
	ld  a, (de)
	ld  (hl), a
	inc de
	inc hl
	ld  a, (de)
	ld  (hl), a
	inc de
	push de
	ld  de, (cd_bc_st)
	add hl, de
	pop de
	djnz cd_bc_st_col
	ld  hl, (cd_bc_dst)
	inc hl
	inc hl
	ld  (cd_bc_dst), hl
	pop hl
	pop bc
	djnz cd_bc_line
	ret
.cd_bc_h
	defb 16
.cd_bc_w
	defb 4
.cd_bc_st
	defw 31
.cd_bc_dst
	defw 0
.cd_bc_b0
	defb 0
.cd_bc_s
	defb 0
