; cpcdirty - Copyleft 2026 The Mojon Twins
;
; cd_mark_rect: mark cells c0..c1 x r0..r1 of the rect at IX+4..7 (viewport
; cells) dirty. A cell that wasn't is stamped with the generation and its
; column goes into its row's bucket.

	XLIB cd_mark_rect

	INCLUDE "cd_layout.inc"

.cd_mark_rect
	ld  a, (ix+6)
	ld  (CD_MR_R), a
.cd_mr_row
	ld  a, (CD_MR_R)
	ld  c, a
	add a, a
	add a, a
	ld  l, a
	ld  h, CD_ROWTAB / 256
	ld  e, (hl)
	inc l
	ld  d, (hl)
	ld  hl, CDK_DIRTY - CD_TMAP
	add hl, de
	ld  e, (ix+4)
	ld  d, 0
	add hl, de
	; DE = BUCKET + r*32 + ROWD [r]
	ld  e, c
	ld  d, CD_ROWD / 256
	ld  a, (de)
	ld  b, a
	ld  a, c
	and 7
	rrca
	rrca
	rrca
	or  b
	ld  e, a
	ld  a, c
	srl a
	srl a
	srl a
	add a, CD_BUCKET / 256
	ld  d, a
	ld  a, (ix+5)
	sub (ix+4)
	inc a
	ld  b, a
	ld  c, (ix+4)
.cd_mr_col
	ld  a, (CD_GEN)
	cp  (hl)
	jr  z, cd_mr_skip
	ld  (hl), a
	ld  a, c
	ld  (de), a
	inc e
.cd_mr_skip
	inc hl
	inc c
	djnz cd_mr_col
	ld  a, (CD_MR_R)
	ld  l, a
	ld  h, CD_ROWD / 256
	ld  a, e
	and 31
	ld  (hl), a
	ld  a, (CD_MR_R)
	cp  (ix+7)
	ret nc
	inc a
	ld  (CD_MR_R), a
	jr  cd_mr_row
