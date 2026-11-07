// Punchy - big sprites, the software flip, pieces and cocos
//
// The player and the 3 enemies are 16x32 sprites (BIG_SPRITES in the engine:
// 5 rows on ZX, 32-line slots in CPC_DIRTY), drawn 16 px above their 16x16
// box. Frames face right; facing left, the frame is flipped once into a
// buffer of that actor when its frame or its facing changes, and the
// buffer is drawn instead.
//
// The extra 8 px of a 24 px wide NES frame (punches, kicks) is a "piece":
// an 8x16 sprite of its own, next to the body. There are 3 piece sprites:
// the player's and two for punchies (two punch at a time at most), and 4
// 8x8 cocos.
//
// Actors (flip buffers): 0 player, 1-3 enemies, 4 player piece, 5-6 punchy
// pieces. On CPC they are also the sprite slots (sp_sw [], cd_geom []):
// 0 player, 1-3 enemies (engine), 4-6 pieces, 7-10 cocos (custom).

#define PN_ACTORS 		7
#define PN_PIECES 		3
#define PN_COCOS 		4
#define PN_NOFRAME 		0xff

#ifdef CPC
	// 256 byte flip table and flip buffers, in the free memory over CD_FREE
	#define PN_LUT 		0xB900
	#define PN_TBUF 	0xBA00		// 4 x 128
	#define PN_PBUF 	0xBC00		// 3 x 32

	// CPC_DIRTY slot sizes: lines, bytes (cd_init, CD_GEOMETRY)
	unsigned char cd_geom [] = {
		32, 4,  32, 4,  32, 4,  32, 4,		// player, enemies
		16, 2,  16, 2,  16, 2,				// pieces
		8, 2,  8, 2,  8, 2,  8, 2 			// cocos
	};

	// sp_sw [] records of the pieces and the cocos
	#define PN_SW_PIECE 	BASE_SPRITES + 16 * SP_CUSTOM_BASE
	#define PN_SW_COCO 		BASE_SPRITES + 16 * SP_CUSTOM_BASE + 16 * PN_PIECES
#else
	// ZX: the table over the binary, under the sprite blocks (AD_FREE; not
	// under 24200: the stack goes there), buffers in the binary: 16 empty
	// bytes (8 lines before column a) + a frame each, every 256 / 128 bytes
	#define PN_LUT 		0xE800
	unsigned char pn_tbuf [4 * 256];
	unsigned char pn_pbuf [PN_PIECES * 128];

	struct sp_SS *sp_piece [PN_PIECES];
	struct sp_SS *sp_coco [PN_COCOS];
	unsigned char *pn_spr_cur [PN_PIECES + PN_COCOS];
	unsigned char pn_spr_shown [PN_PIECES + PN_COCOS];
	unsigned char *pn_sp;
	int pn_off;
#endif

unsigned char pn_bf [PN_ACTORS];		// frame flipped in each buffer
unsigned char *pn_fp, *pn_src, *pn_dst;
unsigned char pn_i, pn_j, pn_a, pn_f, pn_x, pn_y;

// x of the player's body sprite relative to gpx (punch, kick frames)
signed char pn_pdx;

// The pieces and cocos to draw this frame (x, y in game pixels; y can be
// above the viewport)
unsigned char pc_on [PN_PIECES], pc_f [PN_PIECES], pc_left [PN_PIECES];
signed int pc_x [PN_PIECES], pc_y [PN_PIECES];

unsigned char co_on [PN_COCOS], co_x [PN_COCOS], co_y [PN_COCOS];
signed char co_mx [PN_COCOS];

// ----------------------------------------------------------------------------
// Flip

void pn_flip_init (void) {
	// The table: ZX reverses the 8 bits, CPC mode 0 swaps the 2 pixels.
	// pn_bf [] = PN_NOFRAME.
	// ZX: the 4 tall buffers = spr_empty with its 8 empty lines before
	// (16 + 240 bytes), the 3 piece ones = spr_piece_empty (16 + 96).
	#asm
			ld  hl, PN_LUT
		.pn_fi_loop
			ld  a, l
		#ifdef CPC
			ld  c, a
			and $AA
			rrca
			ld  b, a
			ld  a, c
			and $55
			rlca
			or  b
		#else
			ld  b, 8
			ld  c, a
			xor a
		.pn_fi_bits
			rr  c
			rla
			djnz pn_fi_bits
		#endif
			ld  (hl), a
			inc l
			jr  nz, pn_fi_loop

			ld  hl, _pn_bf
			ld  b, PN_ACTORS
			ld  a, PN_NOFRAME
		.pn_fi_bf
			ld  (hl), a
			inc hl
			djnz pn_fi_bf

		#ifndef CPC
			ld  de, _pn_tbuf
			ld  b, 4
		.pn_fi_tb
			push bc
			ld  hl, _spr_empty - 16
			ld  bc, 16 + PN_TALL_SZ
			ldir 						// DE ends 256 bytes on: the next one
			pop bc
			djnz pn_fi_tb

			ld  de, _pn_pbuf
			ld  b, PN_PIECES
		.pn_fi_pb
			push bc
			push de
			ld  hl, _spr_piece_empty - 16
			ld  bc, 16 + PN_PIECE_SZ
			ldir
			pop hl
			ld  de, 128
			add hl, de
			ex  de, hl
			pop bc
			djnz pn_fi_pb
		#endif
	#endasm
}

#ifdef CPC
	// pn_src -> pn_dst: 32 lines of 4 bytes, mirrored
	void pn_flip_tall (void) {
		#asm
				ld  hl, (_pn_src)
				ld  de, (_pn_dst)
				ld  b, 32
			.pn_ft_line
				push bc
				inc hl
				inc hl
				inc hl 					// last byte of the line
				ld  b, 4
			.pn_ft_byte
				ld  c, (hl)
				dec hl
				push hl
				ld  h, PN_LUT / 256
				ld  l, c
				ld  a, (hl)
				ld  (de), a
				inc de
				pop hl
				djnz pn_ft_byte
				ld  bc, 5
				add hl, bc 				// next line
				pop bc
				djnz pn_ft_line
		#endasm
	}

	// pn_src -> pn_dst: 16 lines of 2 bytes, mirrored
	void pn_flip_piece (void) {
		#asm
				ld  hl, (_pn_src)
				ld  de, (_pn_dst)
				ld  b, 16
			.pn_fp_line
				push bc
				ld  c, (hl)
				inc hl
				ld  b, (hl)
				inc hl
				push hl
				ld  h, PN_LUT / 256
				ld  l, b
				ld  a, (hl)
				ld  (de), a
				inc de
				ld  l, c
				ld  a, (hl)
				ld  (de), a
				inc de
				pop hl
				pop bc
				djnz pn_fp_line
		#endasm
	}
#else
	// pn_src -> pn_dst: column a = column b mirrored and the other way
	// round, 40 lines of (graphic, mask) each; column c stays empty
	void pn_flip_tall (void) {
		#asm
				ld  hl, (_pn_src)
				ld  bc, 80
				add hl, bc 				// column b
				ld  de, (_pn_dst) 		// to column a
				call pn_ft_col
				ld  hl, (_pn_src) 		// column a
				ld  de, (_pn_dst)
				ld  a, e
				add a, 80
				ld  e, a
				adc a, d
				sub e
				ld  d, a 				// to column b
			.pn_ft_col
				ld  b, 80
			.pn_ft_byte
				ld  a, (hl)
				inc hl
				push hl
				ld  h, PN_LUT / 256
				ld  l, a
				ld  a, (hl)
				ld  (de), a
				inc de
				pop hl
				djnz pn_ft_byte
				ret
		#endasm
	}

	// pn_src -> pn_dst: the data column, 24 lines of (graphic, mask)
	void pn_flip_piece (void) {
		#asm
				ld  hl, (_pn_src)
				ld  de, (_pn_dst)
				ld  b, 48
			.pn_fp_byte
				ld  a, (hl)
				inc hl
				push hl
				ld  h, PN_LUT / 256
				ld  l, a
				ld  a, (hl)
				ld  (de), a
				inc de
				pop hl
				djnz pn_fp_byte
		#endasm
	}
#endif

void pn_frame (void) {
	// Frame pn_f of actor pn_a (0-3: tall, 4-6: piece), facing left if pn_i:
	// the address to draw in pn_fp. Facing right, the frame itself; facing
	// left, the actor's buffer, flipped again if it held another frame
	// (CPC: and its cache key reset, cd_key_sp [pn_a] = 0, to rebuild it).
	#asm
			ld  a, (_pn_f)
			ld  l, a
			ld  h, 0
			ld  a, (_pn_a)
			cp  4
			jr  nc, pn_fr_piece

			// tall: pn_src = spr_tall + pn_f * PN_TALL_SZ
		#ifdef CPC
			add hl, hl
			add hl, hl
			add hl, hl
			add hl, hl
			add hl, hl
			add hl, hl
			add hl, hl 					// * 128
		#else
			ld  d, l
			ld  e, 0 					// * 256
			add hl, hl
			add hl, hl
			add hl, hl
			add hl, hl 					// * 16
			ex  de, hl
			or  a
			sbc hl, de 					// * 240
		#endif
			ld  de, _spr_tall
			add hl, de
			ld  (_pn_src), hl
			ld  a, (_pn_i)
			or  a
			jr  nz, pn_fr_tleft
			ld  (_pn_fp), hl
			ret
		.pn_fr_tleft
			// pn_dst = the buffer of actor pn_a
			ld  a, (_pn_a)
		#ifdef CPC
			ld  l, 0
			srl a
			rr  l
			ld  h, a 					// * 128
			ld  de, PN_TBUF
		#else
			ld  h, a
			ld  l, 0 					// * 256
			ld  de, _pn_tbuf + 16
		#endif
			add hl, de
			ld  (_pn_dst), hl
			call pn_fr_new
			jr  nz, pn_fr_done
			call _pn_flip_tall
			jr  pn_fr_flipped

		.pn_fr_piece
			// piece: pn_src = spr_piece + pn_f * PN_PIECE_SZ
			add hl, hl
			add hl, hl
			add hl, hl
			add hl, hl
			add hl, hl 					// * 32
		#ifndef CPC
			ld  d, h
			ld  e, l
			add hl, hl
			add hl, de 					// * 96
		#endif
			ld  de, _spr_piece
			add hl, de
			ld  (_pn_src), hl
			ld  a, (_pn_i)
			or  a
			jr  nz, pn_fr_pleft
			ld  (_pn_fp), hl
			ret
		.pn_fr_pleft
			ld  a, (_pn_a)
			sub 4
			ld  l, a
			ld  h, 0
			add hl, hl
			add hl, hl
			add hl, hl
			add hl, hl
			add hl, hl 					// * 32
		#ifdef CPC
			ld  de, PN_PBUF
		#else
			add hl, hl
			add hl, hl 					// * 128
			ld  de, _pn_pbuf + 16
		#endif
			add hl, de
			ld  (_pn_dst), hl
			call pn_fr_new
			jr  nz, pn_fr_done
			call _pn_flip_piece

		.pn_fr_flipped
		#ifdef CPC
			ld  hl, _cd_key_sp
			ld  a, (_pn_a)
			add a, a
			call pn_add_a
			xor a
			ld  (hl), a
			inc hl
			ld  (hl), a
		#endif
		.pn_fr_done
			ld  hl, (_pn_dst)
			ld  (_pn_fp), hl
			ret

			// NZ if pn_bf [pn_a] == pn_f (already flipped), else Z and
			// pn_bf [pn_a] = pn_f
		.pn_fr_new
			ld  hl, _pn_bf
			ld  a, (_pn_a)
			call pn_add_a
			ld  a, (_pn_f)
			cp  (hl)
			jr  z, pn_fr_new_same
			ld  (hl), a
			xor a 						// Z
			ret
		.pn_fr_new_same
			or  1 						// NZ
	#endasm
}

// ----------------------------------------------------------------------------
// Pieces and cocos: custom sprites

#ifdef CPC
	void custom_sprites_create (void) {
		#asm
				call _pn_flip_init
		#endasm
	}

	void custom_sprites_render (void) {
		// sp_sw [SP_PLAYER].cox = pn_pdx;
		// for each piece j: if pc_on [j] && 0 <= pc_x [j] < 248: pn_frame of
		//   actor 4 + j (pc_f [j], pc_left [j]); cx = (pc_x [j] + VIEWPORT_X * 8)
		//   >> 1, cy = pc_y [j] + VIEWPORT_Y * 8, sp0 = pn_fp; else spr_empty.
		// for each coco: cx = (co_x + VIEWPORT_X * 8) >> 1, cy = co_y +
		//   VIEWPORT_Y * 8, sp0 = spr_coco, or spr_empty.
		#asm
				ld  a, (_pn_pdx)
				ld  (BASE_SPRITES + 16 * SP_PLAYER + 6), a

				xor a
				ld  (_pn_j), a
			.pn_csr_pc
				ld  hl, _pc_on
				ld  a, (_pn_j)
				call pn_add_a
				ld  a, (hl)
				or  a
				jr  z, pn_csr_pc_off
				ld  hl, _pc_x
				ld  a, (_pn_j)
				add a, a
				call pn_add_a
				ld  a, (hl)
				inc hl
				ld  b, (hl)
				inc b
				dec b
				jr  nz, pn_csr_pc_off 	// < 0 or > 255
				cp  248
				jr  nc, pn_csr_pc_off
				ld  (_pn_x), a
				ld  hl, _pc_f
				ld  a, (_pn_j)
				call pn_add_a
				ld  a, (hl)
				ld  (_pn_f), a
				ld  hl, _pc_left
				ld  a, (_pn_j)
				call pn_add_a
				ld  a, (hl)
				ld  (_pn_i), a
				ld  a, (_pn_j)
				add a, 4
				ld  (_pn_a), a
				call _pn_frame
				call pn_csr_rec 		// IX = the record
				ld  a, (_pn_x)
				add a, VIEWPORT_X * 8
				srl a
				ld  (ix + 8), a
				ld  hl, _pc_y
				ld  a, (_pn_j)
				add a, a
				call pn_add_a
				ld  a, (hl)
				add a, VIEWPORT_Y * 8
				ld  (ix + 9), a
				ld  hl, (_pn_fp)
				jr  pn_csr_pc_set
			.pn_csr_pc_off
				call pn_csr_rec
				ld  hl, _spr_empty
			.pn_csr_pc_set
				ld  (ix + 0), l
				ld  (ix + 1), h
				ld  a, (_pn_j)
				inc a
				ld  (_pn_j), a
				cp  PN_PIECES
				jp  c, pn_csr_pc

				ld  ix, PN_SW_COCO
				ld  hl, _co_on
				ld  de, _co_x
				ld  bc, _co_y
				ld  a, PN_COCOS
			.pn_csr_co
				push af
				ld  a, (hl)
				or  a
				jr  z, pn_csr_co_off
				ld  a, (de)
				add a, VIEWPORT_X * 8
				srl a
				ld  (ix + 8), a
				ld  a, (bc)
				add a, VIEWPORT_Y * 8
				ld  (ix + 9), a
				push hl
				ld  hl, _spr_coco
				jr  pn_csr_co_set
			.pn_csr_co_off
				push hl
				ld  hl, _spr_empty
			.pn_csr_co_set
				ld  (ix + 0), l
				ld  (ix + 1), h
				pop hl
			.pn_csr_co_next
				inc hl
				inc de
				inc bc
				push de
				ld  de, 16
				add ix, de
				pop de
				pop af
				dec a
				jr  nz, pn_csr_co
				ret

				// IX = sp_sw [SP_CUSTOM_BASE + pn_j]
			.pn_csr_rec
				ld  a, (_pn_j)
				add a, a
				add a, a
				add a, a
				add a, a
				ld  e, a
				ld  d, 0
				ld  ix, PN_SW_PIECE
				add ix, de
		#endasm
	}

	void custom_sprites_hide (void) {
		// sp_sw [SP_CUSTOM_BASE + n].sp0 = spr_empty, pieces and cocos
		#asm
				ld  hl, PN_SW_PIECE
				ld  de, _spr_empty
				ld  bc, 15
				ld  a, PN_PIECES + PN_COCOS
			.pn_csh_loop
				ld  (hl), e
				inc hl
				ld  (hl), d
				add hl, bc
				dec a
				jr  nz, pn_csh_loop
		#endasm
	}
#else
	void custom_sprites_create (void) {
		// pn_flip_init (); then for each piece: sp_CreateSpr (sp_MASK_SPRITE,
		// 3, spr_piece_empty, 1) + a dummy column, nulled by fix_sprites (3
		// members); each coco: 2 rows, spr_coco (fix_sprites 2).
		// pn_spr_cur [] = their frame, pn_spr_shown [] = 1.
		#asm
				call _pn_flip_init
				ld  hl, _sp_piece
				ld  de, _spr_piece_empty
				ld  bc, 0x0303 			// B = 3 sprites, C = 3 rows
				ld  ix, _pn_spr_cur
				call pn_csc_make
				ld  hl, _sp_coco
				ld  de, _spr_coco
				ld  bc, 0x0402 			// 4 sprites, 2 rows
				ld  ix, _pn_spr_cur + 2 * PN_PIECES
				call pn_csc_make
				ld  hl, _pn_spr_shown
				ld  b, PN_PIECES + PN_COCOS
			.pn_csc_shown
				ld  (hl), 1
				inc hl
				djnz pn_csc_shown
				ret

				// B sprites of C rows with frame DE, stored at HL; their
				// current frame at IX
			.pn_csc_make
				ld  (pn_csc_dst), hl
				ld  (pn_csc_fr), de
				ld  a, c
				ld  (pn_csc_rows), a
				ld  (pn_csc_cur), ix
			.pn_csc_loop
				push bc
				ld  hl, sp_MASK_SPRITE
				push hl
				ld  a, (pn_csc_rows)
				ld  l, a
				ld  h, 0
				push hl
				ld  hl, (pn_csc_fr)
				push hl
				ld  hl, 1
				push hl
				call sp_CreateSpr
				pop bc
				pop bc
				pop bc
				pop bc
				ld  (_pn_sp), hl
				ex  de, hl
				ld  hl, (pn_csc_dst)
				ld  (hl), e
				inc hl
				ld  (hl), d
				inc hl
				ld  (pn_csc_dst), hl
				// a dummy column (the empty one after the data), nulled by
				// fix_sprites, which skips the rows of the data column
				ld  hl, (_pn_sp)
				push hl
				ld  a, (pn_csc_rows)
				add a, a
				add a, a
				add a, a
				add a, a 				// rows * 8 lines * 2 bytes
				ld  hl, (pn_csc_fr)
				call pn_add_a
				push hl
				call sp_AddColSpr
				pop bc
				pop bc
				ld  a, (pn_csc_rows)
				ld  b, a
				ld  hl, (_pn_sp)
				call _fix_sprites
				ld  hl, (pn_csc_cur)
				ld  de, (pn_csc_fr)
				ld  (hl), e
				inc hl
				ld  (hl), d
				inc hl
				ld  (pn_csc_cur), hl
				pop bc
				djnz pn_csc_loop
				ret
			.pn_csc_dst
				defw 0
			.pn_csc_fr
				defw 0
			.pn_csc_cur
				defw 0
			.pn_csc_rows
				defb 0
		#endasm
	}

	void pn_move (void) {
		// sprite pn_sp to (pn_x, pn_y - 16) with frame pn_fp, pn_i = its
		// index (pn_y is y + 16, so it is never negative: rows from
		// VIEWPORT_Y - 2). pn_off = pn_fp - pn_spr_cur [pn_i];
		// pn_spr_cur [pn_i] = pn_fp; ...; pn_spr_shown [pn_i] = 1;
		#asm
				ld  hl, _pn_spr_cur
				ld  a, (_pn_i)
				add a, a
				call pn_add_a
				ld  e, (hl)
				inc hl
				ld  d, (hl)
				ld  bc, (_pn_fp)
				ld  (hl), b
				dec hl
				ld  (hl), c
				ld  h, b
				ld  l, c
				or  a
				sbc hl, de
				ld  b, h
				ld  c, l 				// BC = the frame offset
				ld  ix, (_pn_sp)
				ld  iy, vpClipStruct
				ld  a, (_pn_y)
				srl a
				srl a
				srl a
				add VIEWPORT_Y - 2
				ld  h, a
				ld  a, (_pn_x)
				srl a
				srl a
				srl a
				add VIEWPORT_X
				ld  l, a
				ld  a, (_pn_x)
				and 7
				ld  d, a
				ld  a, (_pn_y)
				and 7
				ld  e, a
				call SPMoveSprAbs
				ld  hl, _pn_spr_shown
				ld  a, (_pn_i)
				call pn_add_a
				ld  (hl), 1
		#endasm
	}

	void pn_hide (void) {
		// if pn_spr_shown [pn_i]: sprite pn_sp out of the screen, shown = 0
		#asm
				ld  hl, _pn_spr_shown
				ld  a, (_pn_i)
				call pn_add_a
				ld  a, (hl)
				or  a
				ret z
				ld  (hl), 0
				ld  ix, (_pn_sp)
				ld  iy, vpClipStruct
				ld  bc, 0
				ld  hl, 0xfefe
				ld  de, 0
				call SPMoveSprAbs
		#endasm
	}

	void custom_sprites_render (void) {
		// pieces: if pc_on [j] && 0 <= pc_x [j] < 248 && pc_y [j] >= -16:
		//   pn_frame of actor 4 + j, pn_move to (pc_x, pc_y + 16); else pn_hide.
		// cocos: pn_move spr_coco to (co_x, co_y + 16), or pn_hide.
		#asm
				xor a
				ld  (_pn_j), a
			.pn_zsr_pc
				ld  hl, _sp_piece
				ld  a, (_pn_j)
				ld  (_pn_i), a
				add a, a
				call pn_add_a
				ld  e, (hl)
				inc hl
				ld  d, (hl)
				ld  (_pn_sp), de
				ld  hl, _pc_on
				ld  a, (_pn_j)
				call pn_add_a
				ld  a, (hl)
				or  a
				jr  z, pn_zsr_pc_off
				ld  hl, _pc_x
				ld  a, (_pn_j)
				add a, a
				call pn_add_a
				ld  a, (hl)
				inc hl
				ld  b, (hl)
				inc b
				dec b
				jr  nz, pn_zsr_pc_off
				cp  248
				jr  nc, pn_zsr_pc_off
				ld  (_pn_x), a
				ld  hl, _pc_y
				ld  a, (_pn_j)
				add a, a
				call pn_add_a
				ld  e, (hl)
				inc hl
				ld  d, (hl)
				ld  hl, 16
				add hl, de 				// pc_y + 16 >= 0
				bit 7, h
				jr  nz, pn_zsr_pc_off
				ld  a, l
				ld  (_pn_y), a
				ld  hl, _pc_f
				ld  a, (_pn_j)
				call pn_add_a
				ld  a, (hl)
				ld  (_pn_f), a
				ld  hl, _pc_left
				ld  a, (_pn_j)
				call pn_add_a
				ld  a, (hl)
				ld  (_pn_i), a
				ld  a, (_pn_j)
				add a, 4
				ld  (_pn_a), a
				call _pn_frame
				ld  a, (_pn_j)
				ld  (_pn_i), a
				call _pn_move
				jr  pn_zsr_pc_next
			.pn_zsr_pc_off
				call _pn_hide
			.pn_zsr_pc_next
				ld  a, (_pn_j)
				inc a
				ld  (_pn_j), a
				cp  PN_PIECES
				jp  c, pn_zsr_pc

				xor a
				ld  (_pn_j), a
			.pn_zsr_co
				ld  hl, _sp_coco
				ld  a, (_pn_j)
				add a, a
				call pn_add_a
				ld  e, (hl)
				inc hl
				ld  d, (hl)
				ld  (_pn_sp), de
				ld  a, (_pn_j)
				add a, PN_PIECES
				ld  (_pn_i), a
				ld  hl, _co_on
				ld  a, (_pn_j)
				call pn_add_a
				ld  a, (hl)
				or  a
				jr  z, pn_zsr_co_off
				ld  hl, _spr_coco
				ld  (_pn_fp), hl
				ld  hl, _co_x
				ld  a, (_pn_j)
				call pn_add_a
				ld  a, (hl)
				ld  (_pn_x), a
				ld  hl, _co_y
				ld  a, (_pn_j)
				call pn_add_a
				ld  a, (hl)
				add a, 16
				ld  (_pn_y), a
				call _pn_move
				jr  pn_zsr_co_next
			.pn_zsr_co_off
				call _pn_hide
			.pn_zsr_co_next
				ld  a, (_pn_j)
				inc a
				ld  (_pn_j), a
				cp  PN_COCOS
				jr  c, pn_zsr_co
		#endasm
	}

	void custom_sprites_hide (void) {
		// every piece and coco out of the screen (pn_spr_shown = 1, pn_hide)
		#asm
				ld  hl, _pn_spr_shown
				ld  b, PN_PIECES + PN_COCOS
			.pn_zsh_set
				ld  (hl), 1
				inc hl
				djnz pn_zsh_set
				xor a
			.pn_zsh_loop
				ld  (_pn_i), a
				cp  PN_PIECES
				jr  nc, pn_zsh_coco
				ld  hl, _sp_piece
				jr  pn_zsh_sp
			.pn_zsh_coco
				sub PN_PIECES
				ld  hl, _sp_coco
			.pn_zsh_sp
				add a, a
				call pn_add_a
				ld  e, (hl)
				inc hl
				ld  d, (hl)
				ld  (_pn_sp), de
				call _pn_hide
				ld  a, (_pn_i)
				inc a
				cp  PN_PIECES + PN_COCOS
				jr  c, pn_zsh_loop
		#endasm
	}
#endif
