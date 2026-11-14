; cpcdirty - Copyleft 2026 The Mojon Twins
;
; cd_set_cell: one cell of the viewport map, marked dirty.
; B = column, C = row (viewport cells), A = pattern.

	XLIB cd_set_cell

	LIB cd_mark_cell

	INCLUDE "cd_layout.inc"

.cd_set_cell
	ld  e, a
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
	jr  nc, cd_sc_nc
	inc h
.cd_sc_nc
	ld  (hl), e
	ld  a, b
	jp  cd_mark_cell
