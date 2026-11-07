// MTE MK1 v3.2
// Copyleft 2011, 2026 by The Mojon Twins

// cpc_dirty_flush.h - the flush of cpc_dirty.h (adapted from the cpcdirty bench)

void cd_flush_dummy (void) {
	#asm
	; ------------------------------------------------------------ flush
	; Rows top-down (the beam also goes top-down, so it is raced once per
	; row). Per dirty row: (a) patterns of its dirty cells -> CD_ROWBUF + c*16,
	; (b) OR every sprite crossing the row, (c) dirty cells -> screen.
	; The bucket of the row gets a $FF sentinel so loops need no counter.
	.cd_flush
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
	#ifdef MODE_1
		ld  hl, CD_BLACK_WORD 	// pattern 0 is all BLACK_PEN
	#else
		ld  hl, 0
	#endif
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
		ld  d, CD_SPRT / 256
		push hl
		push bc
		push de
		pop ix
		call cd_or_row
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
		ld  a, (_cd_intc)
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
		// new dirty generation (every 255: wipe the stamps, start again at 1)
		ld  a, (CD_GEN)
		inc a
		jr  nz, cd_fl_gen
		ld  hl, CD_DIRTY
		ld  de, CD_DIRTY + 1
		ld  bc, 599
		ld  (hl), a
		ldir
		inc a
	.cd_fl_gen
		ld  (CD_GEN), a
		ret

	#ifdef MODE_1
	; MODE_1: OR the part of sprite IX inside char row CD_CUR_R into
	; CD_ROWBUF. The cache (cd_build_cache) is in byte-column order: 4 or 5
	; columns of 16 lines x (AND mask, data), 32-aligned, one per screen byte
	; xb + k; each goes to its cell (CD_ROWBUF + (bx >> 1) * 32) and side
	; (bx & 1), 2 bytes per line.
	.cd_or_row
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
	#else
	; OR the part of sprite IX inside char row CD_CUR_R into CD_ROWBUF.
	; Sprite graphics are stored in cell-column order (16 lines x 2 bytes
	; per column, columns 32-aligned): 2 columns for even x at gfx,
	; 3 columns (shifted one byte) for odd x at gfx+64; mark_all leaves the
	; right one in ix+2/3. CD_ROWBUF cells are 32 bytes apart too, so after a
	; column both pointers move on by the same 32 - 2*lines.
	.cd_or_row
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
		ld  a, b
		add a, a
		neg
		add a, 32
		ld  (CD_OR_K), a
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
		ld  (CD_OR_3COL), a
		; src = gfx (parity) + sy0*2
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
		ld  a, (CD_OR_K)
		ld  c, a
		ld  b, 0
		pop ix
		call cd_or_jp
		ex  de, hl
		add hl, bc
		ex  de, hl
		add hl, bc
		call cd_or_jp
		ld  a, (CD_OR_3COL)
		dec a
		ret z
		ex  de, hl
		add hl, bc
		ex  de, hl
		add hl, bc
	.cd_or_jp
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
	#endif
	#endasm
}
