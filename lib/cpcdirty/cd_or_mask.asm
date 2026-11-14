; cpcdirty - Copyleft 2026 The Mojon Twins
;
; cd_or_mask: the row composer of the masked modes (mode 1, nibble mode 0),
; called by cd_flush through CD_OR_JP with IX = sprite record.

	XLIB cd_or_mask

	INCLUDE "cd_layout.inc"

; AND / OR the part of sprite IX inside char row CD_CUR_R into
; CD_ROWBUF. The cache (cd_build_cache) is in byte-column order: 4 or 5
; columns of 16 lines x (AND mask, data), 32-aligned, one per screen byte
; xb + k; each goes to its cell (CD_ROWBUF + (bx >> 1) * 32) and side
; (bx & 1), 2 bytes per line.
.cd_or_mask
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
	ld  d, 0
	jr  cd_or_go
.cd_or_above
	; sprite started above: cell lines 0.., sprite lines sy0..
	neg
	ld  d, a
	ld  c, a
	ld  a, 16
	sub c
	cp  8
	jr  c, cd_or_cnt
	ld  a, 8
.cd_or_cnt
	ld  b, a
	ld  e, 0
.cd_or_go
	; b = lines, d = sy0, e = l0
	; entry into the unrolled block: or_block + (8 - lines) * 8
	ld  a, 8
	sub b
	add a, a
	add a, a
	add a, a
	ld  hl, cd_or_block
	add a, l
	ld  l, a
	adc a, h
	sub l
	ld  h, a
	ld  (cd_or_call + 1), hl
	; l0 * 2: the line offset in a cell
	sla e
	ld  a, e
	ld  (cd_or_l0), a
	; src = cache + sy0*2
	ld  a, d
	add a, a
	add a, (ix+2)
	ld  l, a
	ld  a, (ix+3)
	adc a, 0
	ld  h, a
	; columns: 4, 5 if the sprite is shifted
	ld  a, (ix+8)
	add a, 4
	ld  b, a
	ld  c, (ix+0) 			; bx
.cd_or_col
	push bc
	push hl
	; de = CD_ROWBUF + (bx >> 1) * 32 + (bx & 1) + l0 * 2
	ld  a, c
	srl a
	rrca
	rrca
	rrca
	ld  d, a
	and $E0
	ld  e, a
	xor d
	or  CD_ROWBUF / 256
	ld  d, a
	ld  a, c
	and 1
	ld  c, a
	ld  a, (cd_or_l0)
	or  c
	or  e
	ld  e, a
.cd_or_call
	call 0 				; patched: the entry into cd_or_block
	pop hl
	ld  bc, 32
	add hl, bc
	pop bc
	inc c
	djnz cd_or_col
	ret
.cd_or_l0
	defb 0

	; (de) = (de) AND mask OR data, 8 lines of one byte column
.cd_or_block
	ld  a, (de)
	and (hl)
	inc l
	or  (hl)
	inc l
	ld  (de), a
	inc e
	inc e
	ld  a, (de)
	and (hl)
	inc l
	or  (hl)
	inc l
	ld  (de), a
	inc e
	inc e
	ld  a, (de)
	and (hl)
	inc l
	or  (hl)
	inc l
	ld  (de), a
	inc e
	inc e
	ld  a, (de)
	and (hl)
	inc l
	or  (hl)
	inc l
	ld  (de), a
	inc e
	inc e
	ld  a, (de)
	and (hl)
	inc l
	or  (hl)
	inc l
	ld  (de), a
	inc e
	inc e
	ld  a, (de)
	and (hl)
	inc l
	or  (hl)
	inc l
	ld  (de), a
	inc e
	inc e
	ld  a, (de)
	and (hl)
	inc l
	or  (hl)
	inc l
	ld  (de), a
	inc e
	inc e
	ld  a, (de)
	and (hl)
	inc l
	or  (hl)
	inc l
	ld  (de), a
	inc e
	inc e
	ret
