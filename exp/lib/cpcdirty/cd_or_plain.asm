; cpcdirty - Copyleft 2026 The Mojon Twins
;
; cd_or_plain: the row composer of planar mode 0 (sprites ORed, unmasked),
; called by cd_flush through CD_OR_JP with IX = sprite record.

	XLIB cd_or_plain

	INCLUDE "cd_layout.inc"

; OR the part of sprite IX inside char row CD_CUR_R into CD_ROWBUF.
; Sprite caches are stored in cell-column order (h lines x 2 bytes per
; column, columns 2h-aligned, 2 or 3 of them, the cache pointer in ix+2/3
; past the lines cut at the top). CD_ROWBUF cells are 32 bytes apart:
; after a column the destination moves on 32 - 2*lines, the source
; 2h - 2*lines.
.cd_or_plain
	ld  a, (CD_CUR_R)
	add a, a
	add a, a
	add a, a
	ld  c, a
	ld  a, (ix+1)
	sub c
	jr  c, cd_or_above
	; sprite starts inside this row: cell lines l0..7, sprite lines 0..
	ld  e, a
	neg
	add a, 8
	ld  b, a
	ld  a, (ix+10)
	cp  b
	jr  nc, cd_or_in_cnt
	ld  b, a 			; a short sprite may end inside the row
.cd_or_in_cnt
	ld  d, 0
	jr  cd_or_go
.cd_or_above
	; sprite started above: cell lines 0.., sprite lines sy0..
	neg
	ld  d, a
	ld  c, a
	ld  a, (ix+10)
	sub c
	cp  8
	jr  c, cd_or_cnt
	ld  a, 8
.cd_or_cnt
	ld  b, a
	ld  e, 0
.cd_or_go
	; b = lines, d = sy0, e = l0
	ld  a, b
	add a, a
	ld  c, a
	neg
	add a, 32
	ld  (CD_OR_K), a
	ld  a, (ix+11)
	add a, a
	sub c
	ld  (CD_OR_K2), a
	; entry into the unrolled block: or_block + (8 - lines) * 10
	ld  a, 8
	sub b
	ld  c, a
	add a, a
	add a, a
	add a, c
	add a, a
	ld  hl, cd_or_block
	add a, l
	ld  l, a
	adc a, h
	sub l
	ld  h, a
	push hl
	ld  a, (ix+5)
	sub (ix+4)
	inc a
	ld  (CD_OR_3COL), a 	; columns
	; src = cache + sy0*2
	ld  a, d
	add a, a
	add a, (ix+2)
	ld  l, a
	ld  a, (ix+3)
	adc a, 0
	ld  h, a
	; dst = CD_ROWBUF + c0*32 + l0*2
	sla e
	ld  a, (ix+4)
	rrca
	rrca
	rrca
	ld  c, a
	and $E0
	or  e
	ld  e, a
	ld  a, c
	and $1F
	or  CD_ROWBUF / 256
	ld  d, a
	pop ix
.cd_or_cols
	call cd_orp_jump
	ld  a, (CD_OR_3COL)
	dec a
	ret z
	ld  (CD_OR_3COL), a
	ld  a, (CD_OR_K)
	add a, e
	ld  e, a
	adc a, d
	sub e
	ld  d, a
	ld  a, (CD_OR_K2)
	add a, l
	ld  l, a
	adc a, h
	sub l
	ld  h, a
	jr  cd_or_cols
.cd_orp_jump
	jp  (ix)
.cd_or_block
	ld  a, (de)
	or  (hl)
	ld  (de), a
	inc l
	inc e
	ld  a, (de)
	or  (hl)
	ld  (de), a
	inc l
	inc e
	ld  a, (de)
	or  (hl)
	ld  (de), a
	inc l
	inc e
	ld  a, (de)
	or  (hl)
	ld  (de), a
	inc l
	inc e
	ld  a, (de)
	or  (hl)
	ld  (de), a
	inc l
	inc e
	ld  a, (de)
	or  (hl)
	ld  (de), a
	inc l
	inc e
	ld  a, (de)
	or  (hl)
	ld  (de), a
	inc l
	inc e
	ld  a, (de)
	or  (hl)
	ld  (de), a
	inc l
	inc e
	ld  a, (de)
	or  (hl)
	ld  (de), a
	inc l
	inc e
	ld  a, (de)
	or  (hl)
	ld  (de), a
	inc l
	inc e
	ld  a, (de)
	or  (hl)
	ld  (de), a
	inc l
	inc e
	ld  a, (de)
	or  (hl)
	ld  (de), a
	inc l
	inc e
	ld  a, (de)
	or  (hl)
	ld  (de), a
	inc l
	inc e
	ld  a, (de)
	or  (hl)
	ld  (de), a
	inc l
	inc e
	ld  a, (de)
	or  (hl)
	ld  (de), a
	inc l
	inc e
	ld  a, (de)
	or  (hl)
	ld  (de), a
	inc hl		; the last byte of a column may end a page
	inc e
	ret
