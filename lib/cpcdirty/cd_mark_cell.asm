; cpcdirty - Copyleft 2026 The Mojon Twins
;
; cd_mark_cell: mark one cell dirty. A = column, C = row (viewport cells).
; Keeps BC.

	XLIB cd_mark_cell

	LIB cd_mark_rect

	INCLUDE "cd_layout.inc"

.cd_mark_cell
	push bc
	ld  (CD_CELLREC + 4), a
	ld  (CD_CELLREC + 5), a
	ld  a, c
	ld  (CD_CELLREC + 6), a
	ld  (CD_CELLREC + 7), a
	push ix
	ld  ix, CD_CELLREC
	call cd_mark_rect
	pop ix
	pop bc
	ret
