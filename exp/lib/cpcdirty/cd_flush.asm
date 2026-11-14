; cpcdirty - Copyleft 2026 The Mojon Twins
;
; cd_flush: rows top-down (the beam also goes top-down, so it is raced once
; per row). Per dirty row: (a) patterns of its dirty cells -> CD_ROWBUF +
; c*32, (b) the mode's composer (CD_OR_JP) for every sprite crossing the
; row, (c) dirty cells -> screen, once the beam has drawn the row in this
; frame. The bucket of the row gets a $FF sentinel so loops need no counter.
; Call it right after the 3rd interrupt of a frame (CD_ROWINT counts from
; there); cd_intc is latched here.

	XLIB cd_flush

	LIB cd_intc

	INCLUDE "cd_layout.inc"

.cd_flush
	ld  a, (cd_intc)
	ld  (CD_INTC0), a
	xor a
	ld  (CD_CUR_R), a
.cd_fl_row
	ld  a, (CD_CUR_R)
	ld  l, a
	ld  h, CD_ROWD / 256
	ld  a, (hl)
	or  a
	jp  z, cd_fl_row_next
	ld  (hl), 0
	ld  b, a

	ld  a, (CD_CUR_R)
	ld  c, a
	and 7
	rrca
	rrca
	rrca
	ld  l, a
	or  b
	ld  e, a
	ld  a, c
	srl a
	srl a
	srl a
	add a, CD_BUCKET / 256
	ld  h, a
	ld  d, a
	ld  (CD_FL_BK), hl
	ld  a, $FF
	ld  (de), a
	ld  a, c
	add a, a
	add a, a
	ld  l, a
	ld  h, CD_ROWTAB / 256
	ld  e, (hl)
	inc l
	ld  d, (hl)
	inc l
	ld  (CD_ROW_D), de
	ld  e, (hl)
	inc l
	ld  d, (hl)
	ld  (CD_ROW_SCR), de

	; (a) patterns -> CD_ROWBUF + c*32 (pattern 0 is blank: those cells
	; are just cleared, with interrupts off while SP points there)
	ld  (CD_SAVESP), sp
	ld  iy, (CD_FL_BK)
.cd_fa_loop
	ld  a, (iy+0)
	inc iy
	cp  $FF
	jp  z, cd_fa_done
		ld  c, a
	ld  b, 0
	ld  hl, (CD_ROW_D)
	add hl, bc
	ld  a, (hl)
	or  a
	jp  z, cd_fa_blank
	; hl = CD_PAT + tile*16 (CD_PAT is 4K aligned)
	rlca
	rlca
	rlca
	rlca
	ld  b, a
	and $F0
	ld  l, a
	xor b
	or  CD_PAT / 256
	ld  h, a
	; de = CD_ROWBUF + c*32 (CD_ROWBUF is 1K aligned)
	ld  a, c
	rrca
	rrca
	rrca
	ld  b, a
	and $E0
	ld  e, a
	xor b
	or  CD_ROWBUF / 256
	ld  d, a
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	ldi
	jp  cd_fa_loop
.cd_fa_blank
	ld  a, c
	rrca
	rrca
	rrca
	ld  b, a
	and $E0
	or  $10
	ld  l, a
	ld  a, b
	and $1F
	or  CD_ROWBUF / 256
	ld  h, a
	di
	ld  sp, hl
	ld  hl, (CD_BLANK)
	push hl
	push hl
	push hl
	push hl
	push hl
	push hl
	push hl
	push hl
	ld  sp, (CD_SAVESP)
	ei
	jp  cd_fa_loop
.cd_fa_done

.cd_fb_start
	; (b) sprites crossing the row (listed by mark_all)
	ld  a, (CD_CUR_R)
	ld  l, a
	ld  h, CD_ROWSN / 256
	ld  b, (hl)
	inc b
	dec b
	jr  z, cd_fb_none
	add a, a
	add a, a
	add a, a
	ld  l, a
	ld  h, CD_ROWSPR / 256
.cd_fb_loop
	ld  e, (hl)
	ld  d, CDK_SPRT / 256
	push hl
	push bc
	push de
	pop ix
	call CD_OR_JP
	pop bc
	pop hl
	inc l
	djnz cd_fb_loop
.cd_fb_none

.cd_fc_start
	; (c) CD_ROWBUF -> screen. First make sure the beam has already drawn
	; this row in the current frame (CD_ROWINT [r] = interrupts needed),
	; so the row changes between two frames and never under the beam.
	; Cells are popped from CD_ROWBUF with interrupts off, one at a time.
	ld  a, (CD_CUR_R)
	ld  l, a
	ld  h, CD_ROWINT / 256
	ld  b, (hl)
.cd_fc_guard
	ld  a, (cd_intc)
	ld  hl, CD_INTC0
	sub (hl)
	cp  b
	jr  c, cd_fc_guard
	ld  iy, (CD_FL_BK)
.cd_fc_loop
	ld  a, (iy+0)
	inc iy
	cp  $FF
	jp  z, cd_fc_done
	di
	ld  c, a
	; sp = CD_ROWBUF + c*32
	rrca
	rrca
	rrca
	ld  b, a
	and $E0
	ld  l, a
	xor b
	or  CD_ROWBUF / 256
	ld  h, a
	ld  sp, hl
	; hl = CD_ROW_SCR + c*2 (even, so inc l never carries)
	ld  hl, (CD_ROW_SCR)
	ld  a, c
	add a, a
	add a, l
	ld  l, a
	adc a, h
	sub l
	ld  h, a
	pop bc
	ld  (hl), c
	inc l
	ld  (hl), b
	dec l
	ld  a, h
	add a, 8
	ld  h, a
	pop bc
	ld  (hl), c
	inc l
	ld  (hl), b
	dec l
	ld  a, h
	add a, 8
	ld  h, a
	pop bc
	ld  (hl), c
	inc l
	ld  (hl), b
	dec l
	ld  a, h
	add a, 8
	ld  h, a
	pop bc
	ld  (hl), c
	inc l
	ld  (hl), b
	dec l
	ld  a, h
	add a, 8
	ld  h, a
	pop bc
	ld  (hl), c
	inc l
	ld  (hl), b
	dec l
	ld  a, h
	add a, 8
	ld  h, a
	pop bc
	ld  (hl), c
	inc l
	ld  (hl), b
	dec l
	ld  a, h
	add a, 8
	ld  h, a
	pop bc
	ld  (hl), c
	inc l
	ld  (hl), b
	dec l
	ld  a, h
	add a, 8
	ld  h, a
	pop bc
	ld  (hl), c
	inc l
	ld  (hl), b
	ld  sp, (CD_SAVESP)
	ei
	jp  cd_fc_loop
.cd_fc_done

.cd_fl_row_next
	ld  a, (CD_CUR_R)
	inc a
	ld  (CD_CUR_R), a
	cp  20
	jp  nz, cd_fl_row
	; new dirty generation (every 255: wipe the stamps, start again at 1)
	ld  a, (CD_GEN)
	inc a
	jr  nz, cd_fl_gen
	ld  hl, CDK_DIRTY
	ld  de, CDK_DIRTY + 1
	ld  bc, 599
	ld  (hl), a
	ldir
	inc a
.cd_fl_gen
	ld  (CD_GEN), a
	ret


