; cpcdirty - Copyleft 2026 The Mojon Twins
;
; cd_mark_sprites: for every record, mark where the sprite was drawn last
; time and, if it is visible, where it is now (that becomes "drawn"), and
; list it in the rows it crosses (8 at most per row).
; Record: +0 x (byte), +1 y (line), +2/3 cache (past lines cut at the top),
; +4..7 c0 c1 r0 r1, +8 one more byte (shifted), +9 flags (bit 0 visible,
; bit 1 drawn), +10 lines drawn, +11 h, +12 w (bytes), +13/14 cache base.

	XLIB cd_mark_sprites

	LIB cd_mark_rect

	INCLUDE "cd_layout.inc"

.cd_mark_sprites
	ld  hl, CD_ROWSN
	ld  de, CD_ROWSN + 1
	ld  bc, 19
	ld  (hl), 0
	ldir
	ld  ix, CDK_SPRT
	ld  b, CDK_NSPR
.cd_ms_loop
	push bc
	bit 1, (ix+9)
	call nz, cd_mark_rect
	res 1, (ix+9)
	bit 0, (ix+9)
	jp  z, cd_ms_next
	set 1, (ix+9)
	; c0 = x >> 1, c1 = (x + w - 1 + s) >> 1, r0 = y >> 3,
	; r1 = (y + lines - 1) >> 3
	ld  a, (ix+0)
	ld  c, a
	srl a
	ld  (ix+4), a
	ld  a, c
	add a, (ix+12)
	dec a
	add a, (ix+8)
	srl a
	ld  (ix+5), a
	ld  a, (ix+1)
	ld  c, a
	srl a
	srl a
	srl a
	ld  (ix+6), a
	ld  a, c
	add a, (ix+10)
	dec a
	srl a
	srl a
	srl a
	ld  (ix+7), a
	call cd_mark_rect
	; list it in the rows it crosses (8 at most per row)
	ld  c, (ix+6)
.cd_ms_rows
	ld  l, c
	ld  h, CD_ROWSN / 256
	ld  a, (hl)
	cp  8
	jr  nc, cd_ms_full
	inc (hl)
	ld  e, a
	ld  a, c
	add a, a
	add a, a
	add a, a
	add a, e
	ld  l, a
	ld  h, CD_ROWSPR / 256
	ld  a, ixl
	ld  (hl), a
.cd_ms_full
	ld  a, c
	inc c
	cp  (ix+7)
	jr  c, cd_ms_rows
.cd_ms_next
	ld  de, CD_RECSZ
	add ix, de
	pop bc
	dec b
	jp  nz, cd_ms_loop
	ret
