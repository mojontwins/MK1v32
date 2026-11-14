; cpcdirty - Copyleft 2026 The Mojon Twins
;
; cd_put_tile: 2x2 cells into the viewport map, marked dirty.
; B = column, C = row (viewport cells, top-left), DE = 4 patterns (top-left,
; top-right, bottom-left, bottom-right).

	XLIB cd_put_tile

	LIB cd_mark_cell

	INCLUDE "cd_layout.inc"

.cd_put_tile
	ld  a, c
	add a, a
	add a, a
	ld  l, a
	ld  h, CD_ROWTAB / 256
	ld  a, (hl)
	inc l
	ld  h, (hl)
	add a, b
	ld  l, a
	jr  nc, cd_pt_nc
	inc h
.cd_pt_nc 						; HL = TMAP cell
	ld  a, (de)
	ld  (hl), a
	inc de
	inc hl
	ld  a, (de)
	ld  (hl), a
	inc de
	push de
	ld  de, 29
	add hl, de
	pop de
	ld  a, (de)
	ld  (hl), a
	inc de
	inc hl
	ld  a, (de)
	ld  (hl), a
	; mark the 4 cells
	ld  a, b
	call cd_mark_cell
	ld  a, b
	inc a
	call cd_mark_cell
	inc c
	ld  a, b
	call cd_mark_cell
	ld  a, b
	inc a
	jp  cd_mark_cell
