; cpcdirty - Copyleft 2026 The Mojon Twins
;
; cd_invalidate: mark the whole viewport dirty.

	XLIB cd_invalidate

	LIB cd_mark_rect

	INCLUDE "cd_layout.inc"

.cd_invalidate
	push ix
	ld  ix, CD_FULLREC
	call cd_mark_rect
	pop ix
	ret
