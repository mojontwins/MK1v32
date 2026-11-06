// MTE MK1 v3.2
// Copyleft 2011, 2026 by The Mojon Twins

// cpc_dirty.h - CPC backend without superbuffer (CPC_DIRTY)
//
// Mode 0, split palette: pen = bg * 4 + spr. Background patterns use pen bits
// 2-3 (byte mask 0x33), sprites pen bits 0-1 (0xCC) and are ORed, unmasked.
// The viewport (30 x 20 char cells of 2 bytes x 8 lines) lives in CD_TMAP as
// pattern numbers. Tiles and sprites mark the cells they touch; cd_update ()
// rebuilds every dirty cell from its pattern plus the sprites over it, row by
// row from the top, racing behind the beam so a row changes between two
// frames, and writes it to the screen in one go: no superbuffer, no flicker.
//
// Sprites: sp_sw [] as in cpc.h (cx in mode 0 pixels, cy in lines, sp0 the
// frame: 4 bytes x 16 lines, sprite pens only; spr_empty = nothing). Each one
// has a 96 byte cache in cell-column order (3 columns x 16 lines x 2 bytes)
// that is rebuilt when its frame, byte parity or half-byte shift changes.
//
// Memory (the 12K the superbuffer used):
//   9000 CD_PAT      256 patterns x 16 bytes (2 bytes x 8 lines)
//   A000 CD_ROWBUF   30 cells, one every 32 bytes (the row being composed)
//   A400 CD_BUCKET   20 x 32: dirty columns of each row, $FF sentinel
//   A700 CD_ROWD     20 per-row dirty counts
//   A800 CD_ROWINT   20: interrupts after the 3rd one of a frame by which the
//                    beam has drawn row r
//   A900 CD_ROWSN    20: sprites crossing row r
//   AA00 CD_ROWSPR   20 x 8: low bytes of their CD_SPRT records
//   AB00 CD_ROWTAB   20 x 4: TMAP + r*30 (word), screen address of row r (word)
//   AB80 ...         variables
//   AC00 CD_TMAP     30 x 20 pattern numbers
//   B000 CD_CACHE    8 x 96: sprite caches (columns 32-aligned)
//   B300 CD_SPRT     8 x 10: x(byte) y cache(2) c0 c1 r0 r1 shift flags
//   B400 CD_DIRTY    30 x 20 generation stamps (== CD_GEN: dirty)
//   B658-BFFF        free for the game

#define CD_PAT 		0x9000
#define CD_ROWBUF 	0xA000
#define CD_BUCKET 	0xA400
#define CD_ROWD 	0xA700
#define CD_ROWINT 	0xA800
#define CD_ROWSN 	0xA900
#define CD_ROWSPR 	0xAA00
#define CD_ROWTAB 	0xAB00
#define CD_CUR_R 	0xAB80
#define CD_FL_BK 	0xAB82
#define CD_ROW_D 	0xAB84
#define CD_ROW_SCR 	0xAB86
#define CD_SAVESP 	0xAB88
#define CD_OR_K 	0xAB8A
#define CD_OR_3COL 	0xAB8B
#define CD_OR_ENTRY 0xAB8C
#define CD_MR_R 	0xAB8E
#define CD_GEN 		0xAB8F
#define CD_INTC0 	0xAB90
#define CD_FULLREC 	0xAB98
#define CD_CELLREC 	0xABA4
#define CD_TMAP 	0xAC00
#define CD_CACHE 	0xB000
#define CD_SPRT 	0xB300
#define CD_DIRTY 	0xB400
#define CD_TMP 		0xABB0

#define CD_NSPR 	SW_SPRITES_ALL

extern unsigned char cd_patterns_c [];
#asm
	._cd_patterns_c
		BINARY "cpc/patterns.zx0"
#endasm

unsigned char cd_intc;
unsigned char *cd_pal_bg, *cd_pal_spr;

// cd_pal_bg and cd_pal_spr are 16-entry hardware ink tables (mkts_om mode=pals):
// pen p = bg * 4 + s shows cd_pal_spr [s] if s, else cd_pal_bg [bg * 4] (so a
// pal_cnvtilesN.png gives the background inks and pal_cnvspr.png the sprite
// ones). The border gets cd_pal_bg [0].
void cd_set_palette (void) {
	#asm
			ld  e, 0
		.cd_sp_loop
			ld  a, e
			and 3
			jr  z, cd_sp_bg
			ld  hl, (_cd_pal_spr)
			jr  cd_sp_get
		.cd_sp_bg
			ld  a, e
			ld  hl, (_cd_pal_bg)
		.cd_sp_get
			add a, l
			ld  l, a
			jr  nc, cd_sp_nc
			inc h
		.cd_sp_nc
			ld  a, (hl)
			or  0x40
			ld  bc, 0x7F00
			out (c), e
			out (c), a
			inc e
			ld  a, e
			cp  16
			jr  nz, cd_sp_loop
			ld  hl, (_cd_pal_bg)
			ld  a, (hl)
			or  0x40
			ld  c, 16
			out (c), c
			out (c), a
	#endasm
}
unsigned char cd_i, cd_vis, cd_xb, cd_s, cd_ph;
unsigned char *cd_sp0, *cd_cache_dst;
unsigned char *cd_key_sp [CD_NSPR];
unsigned char cd_key_ph [CD_NSPR];
signed int cd_px;
unsigned char cd_py;
SPR *cd_spp;
unsigned char *cd_rec;

// ----------------------------------------------------------------------------
// Init: tables, patterns, empty state

void cd_init (void) {
	#asm
			// patterns
			ld  hl, _cd_patterns_c
			ld  de, CD_PAT
			call depack

			// clear A000-BFFF but the patterns: buffers, counts, map, stamps
			ld  hl, 0xA000
			ld  de, 0xA001
			ld  bc, 0x1FFF
			xor a
			ld  (hl), a
			ldir

			// ROWTAB: TMAP + r*30, 0xC000 + (VIEWPORT_Y + r) * 64 + VIEWPORT_X * 2
			ld  ix, CD_ROWTAB
			ld  hl, CD_TMAP
			ld  de, 0xC000 + VIEWPORT_Y * 64 + VIEWPORT_X * 2
			ld  b, 20
		.cd_init_rowtab
			ld  (ix+0), l
			ld  (ix+1), h
			ld  (ix+2), e
			ld  (ix+3), d
			push bc
			ld  bc, 30
			add hl, bc
			ex  de, hl
			ld  bc, 64
			add hl, bc
			ex  de, hl
			pop bc
			inc ix
			inc ix
			inc ix
			inc ix
			djnz cd_init_rowtab

			// sprite records: caches
			ld  ix, CD_SPRT
			ld  hl, CD_CACHE
			ld  de, 96
			ld  b, CD_NSPR
		.cd_init_sprt
			ld  (ix+2), l
			ld  (ix+3), h
			add hl, de
			push de
			ld  de, 10
			add ix, de
			pop de
			djnz cd_init_sprt

			// full viewport rect: c0 c1 r0 r1 at +4
			ld  hl, CD_FULLREC + 4
			ld  (hl), 0
			inc hl
			ld  (hl), 29
			inc hl
			ld  (hl), 0
			inc hl
			ld  (hl), 19

			ld  a, 1
			ld  (CD_GEN), a
	#endasm
	// the 16 pens of pal.png (cpc/pal.h): titles, frame, ending
	#asm
			call my_inks
	#endasm
	for (cd_i = 0; cd_i < CD_NSPR; cd_i ++) cd_key_sp [cd_i] = 0;

	// ROWINT: screen row VIEWPORT_Y + r has been drawn at VSYNC + 72 + 8 (row + 1);
	// interrupt n comes at VSYNC + 2 + 52 (n - 1); count from the 3rd one.
	for (cd_i = 0; cd_i < 20; cd_i ++) {
		cd_px = 72 + 8 * (VIEWPORT_Y + cd_i + 1);
		cd_s = 1;
		while (2 + 52 * (cd_s - 1) < cd_px) cd_s ++;
		((unsigned char *) CD_ROWINT) [cd_i] = cd_s > 3 ? cd_s - 3 : 0;
	}
}

// ----------------------------------------------------------------------------
// Marking

#asm
	// Mark one cell: A = column, C = row (viewport cells). Keeps BC.
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

	// Mark cells c0..c1 x r0..r1 of the rect at IX+4..7
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
		ld  hl, CD_DIRTY - CD_TMAP
		add hl, de
		ld  e, (ix+4)
		ld  d, 0
		add hl, de
		// DE = BUCKET + r*32 + ROWD [r]
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

	// Sprites: erase where they were, list and mark where they are
	.cd_mark_sprites
		ld  hl, CD_ROWSN
		ld  de, CD_ROWSN + 1
		ld  bc, 19
		ld  (hl), 0
		ldir
		ld  ix, CD_SPRT
		ld  b, CD_NSPR
	.cd_ms_loop
		push bc
		bit 1, (ix+9)
		call nz, cd_mark_rect
		res 1, (ix+9)
		bit 0, (ix+9)
		jp  z, cd_ms_next
		set 1, (ix+9)
		// c0 = x >> 1, c1 = (x + 3 + s) >> 1, r0 = y >> 3, r1 = (y + 15) >> 3
		ld  a, (ix+0)
		ld  c, a
		srl a
		ld  (ix+4), a
		ld  a, c
		add a, 3
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
		add a, 15
		srl a
		srl a
		srl a
		ld  (ix+7), a
		call cd_mark_rect
		// list it in the rows it crosses
		ld  c, (ix+6)
	.cd_ms_rows
		ld  l, c
		ld  h, CD_ROWSN / 256
		ld  a, (hl)
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
		ld  a, c
		inc c
		cp  (ix+7)
		jr  c, cd_ms_rows
	.cd_ms_next
		ld  de, 10
		add ix, de
		pop bc
		dec b
		jp  nz, cd_ms_loop
		ret
#endasm

// ----------------------------------------------------------------------------
// Sprite caches: frame (4 x 16) -> 3 columns x 16 lines x 2 bytes, starting
// at byte b0 (0/1) of the first column, shifted s (0/1) mode 0 pixels right.

#asm
	// HL = frame, DE = cache, B = b0, C = s
	.cd_build_cache
		ld  (cd_bc_dst), de
		ld  a, b
		ld  (cd_bc_b0), a
		ld  a, c
		ld  (cd_bc_s), a
		ld  b, 16
	.cd_bc_line
		push bc
		// TMP = 7 zero bytes
		xor a
		ld  (CD_TMP), a
		ld  (CD_TMP + 1), a
		ld  (CD_TMP + 2), a
		ld  (CD_TMP + 3), a
		ld  (CD_TMP + 4), a
		ld  (CD_TMP + 5), a
		ld  (CD_TMP + 6), a
		ld  a, (cd_bc_b0)
		ld  e, a
		ld  d, 0
		push hl
		ld  hl, CD_TMP
		add hl, de
		ex  de, hl
		pop hl
		ld  a, (cd_bc_s)
		or  a
		jr  nz, cd_bc_shift
		ldi
		ldi
		ldi
		ldi
		jr  cd_bc_store
	.cd_bc_shift
		// out = ((prev & 0x55) << 1) | ((cur & 0xAA) >> 1)
		ld  c, 0
		ld  b, 4
	.cd_bc_sh
		ld  a, c
		and $55
		add a, a
		ld  c, a
		ld  a, (hl)
		inc hl
		push af
		and $AA
		rrca
		or  c
		ld  (de), a
		inc de
		pop af
		ld  c, a
		djnz cd_bc_sh
		ld  a, c
		and $55
		add a, a
		ld  (de), a
	.cd_bc_store
		// TMP [0..5] -> cache + 0, 1, 32, 33, 64, 65 (+ line * 2)
		push hl
		ld  hl, (cd_bc_dst)
		ld  de, 31
		ld  a, (CD_TMP)
		ld  (hl), a
		inc hl
		ld  a, (CD_TMP + 1)
		ld  (hl), a
		add hl, de
		ld  a, (CD_TMP + 2)
		ld  (hl), a
		inc hl
		ld  a, (CD_TMP + 3)
		ld  (hl), a
		add hl, de
		ld  a, (CD_TMP + 4)
		ld  (hl), a
		inc hl
		ld  a, (CD_TMP + 5)
		ld  (hl), a
		ld  hl, (cd_bc_dst)
		inc hl
		inc hl
		ld  (cd_bc_dst), hl
		pop hl
		pop bc
		djnz cd_bc_line
		ret
	.cd_bc_dst
		defw 0
	.cd_bc_b0
		defb 0
	.cd_bc_s
		defb 0
#endasm

// ----------------------------------------------------------------------------
// Tiles and text

// Tile t at screen chars (x, y), inside the viewport: 4 patterns from tile_map
void cd_draw_tile (void) {
	#asm
			ld  a, (__y)
			sub VIEWPORT_Y
			ld  c, a 					// row
			add a, a
			add a, a
			ld  l, a
			ld  h, CD_ROWTAB / 256
			ld  e, (hl)
			inc l
			ld  d, (hl) 				// DE = TMAP row
			ld  a, (__x)
			sub VIEWPORT_X
			ld  b, a 					// column
			ld  l, a
			ld  h, 0
			add hl, de
			push hl
			ld  a, (__t)
			add a, a
			add a, a
			ld  e, a
			ld  d, 0
			ld  hl, _tile_map
			add hl, de
			ex  de, hl 					// DE = 4 patterns
			pop hl 						// HL = TMAP cell
			ld  a, (de)
			ld  (hl), a
			inc de
			inc hl
			ld  a, (de)
			ld  (hl), a
			inc de
			push de
			ld  de, 29
			add hl, de
			pop de
			ld  a, (de)
			ld  (hl), a
			inc de
			inc hl
			ld  a, (de)
			ld  (hl), a
			// mark the 4 cells
			ld  a, b
			call cd_mark_cell
			ld  a, b
			inc a
			call cd_mark_cell
			inc c
			ld  a, b
			call cd_mark_cell
			ld  a, b
			inc a
			call cd_mark_cell
	#endasm
}

// Pattern __t at screen chars (__x, __y), straight to the screen: its colour
// 3 pixels in sprite pen 3 (stable whatever the level palette), the rest
// black. HUD, text outside the viewport.
void cd_put_char (void) {
	#asm
			ld  a, (__t)
			ld  l, a
			ld  h, 0
			add hl, hl
			add hl, hl
			add hl, hl
			add hl, hl
			ld  de, CD_PAT
			add hl, de
			ex  de, hl 					// DE = pattern
			ld  a, (__y)
			ld  l, a
			ld  h, 0
			add hl, hl
			add hl, hl
			add hl, hl
			add hl, hl
			add hl, hl
			add hl, hl 					// y * 64
			ld  a, (__x)
			add a, a
			ld  c, a
			ld  b, 0xC0
			add hl, bc 					// HL = screen (x even: no carry into H)
			ld  b, 8
		.cd_pc_l
			call cd_pc_byte
			inc l
			call cd_pc_byte
			dec l
			ld  a, h
			add a, 8
			ld  h, a
			djnz cd_pc_l
			ret

		// (DE) -> (HL): pixels of colour 3 (bits b2 and b3 set) in sprite
		// pen 3, the rest 0. Colour 3 in pixel 0: bits 5 and 1, pixel 1: 4 and 0
		.cd_pc_byte
			ld  a, (de)
			inc de
			ld  c, a
			rrca
			rrca
			rrca
			rrca
			and c
			and 3 					// bit 1: pixel 0, bit 0: pixel 1
			push hl
			ld  hl, cd_pc_lut
			add a, l
			ld  l, a
			jr  nc, cd_pc_nc
			inc h
		.cd_pc_nc
			ld  a, (hl)
			pop hl
			ld  (hl), a
			ret
		.cd_pc_lut
			defb 0x00, 0x44, 0x88, 0xCC
	#endasm
}

// Pattern __t at screen chars (__x, __y): into the map if inside the viewport,
// straight to the screen otherwise.
void cd_char (void) {
	// (one comparison per if: sccz80 1.10 duplicates labels in chains of >=)
	cd_vis = 0;
	rda = _x - VIEWPORT_X;
	rdb = _y - VIEWPORT_Y;
	if (rda < 30) { if (rdb < 20) cd_vis = 1; }
	if (cd_vis) {
		#asm
				ld  a, (__y)
				sub VIEWPORT_Y
				ld  c, a
				add a, a
				add a, a
				ld  l, a
				ld  h, CD_ROWTAB / 256
				ld  e, (hl)
				inc l
				ld  d, (hl)
				ld  a, (__x)
				sub VIEWPORT_X
				ld  l, a
				ld  h, 0
				add hl, de
				ld  b, a
				ld  a, (__t)
				ld  (hl), a
				ld  a, b
				call cd_mark_cell
		#endasm
	} else cd_put_char ();
}

// Compressed screen: offsets of 8 zx0 streams (words, from the start), then
// the streams: pixel line k of the 24 char rows each (64 x 24 bytes)
// A screen: zx0 of a mkts_om superbuffer (64 bytes x 192 lines, linear).
// It goes through the 12K at 9000-BFFF, which cd_init rebuilds afterwards;
// the level's sprite cells there are gone too, so screens are unpacked
// before custom_level_setup. Palette: pal.png's (my_inks), by cd_init.
void cd_unpack_screen (void) {
	#asm
			// HL = source (set by the caller)
			ld  de, 0x9000
			call depack
			ld  hl, 0x9000
			ld  de, 0xC000
			ld  a, 192
		.cd_us_loop
			ld  bc, 64
			push de
			ldir
			pop de
			ex  de, hl
			ld  bc, 0x0800 		// next line
			add hl, bc
			jr  nc, cd_us_nc
			ld  bc, 0xC040 		// past the 8th line: next row (wraps)
			add hl, bc
		.cd_us_nc
			ex  de, hl
			dec a
			jr  nz, cd_us_loop
	#endasm
	cd_init ();
}

// ----------------------------------------------------------------------------
// Update: sprites from sp_sw, then the flush

void cd_sprites (void) {
	for (cd_i = 0; cd_i < CD_NSPR; cd_i ++) {
		cd_spp = &sp_sw [cd_i];
		cd_rec = (unsigned char *) (CD_SPRT + cd_i * 10);
		cd_sp0 = (unsigned char *) cd_spp->sp0;
		cd_vis = 0;
		if (cd_sp0 != spr_empty) {
			cd_px = cd_spp->cx - VIEWPORT_X * 4;
			cd_py = cd_spp->cy - VIEWPORT_Y * 8;
			if (cd_px >= 0) { if (cd_px <= 112) { if (cd_py <= 144) cd_vis = 1; } }
		}
		if (cd_vis) {
			cd_xb = cd_px >> 1; cd_s = cd_px & 1;
			cd_ph = ((cd_xb & 1) << 1) | cd_s;
			if (cd_key_sp [cd_i] != cd_sp0 || cd_key_ph [cd_i] != cd_ph) {
				cd_key_sp [cd_i] = cd_sp0; cd_key_ph [cd_i] = cd_ph;
				cd_cache_dst = (unsigned char *) (CD_CACHE + cd_i * 96);
				#asm
						ld  hl, (_cd_sp0)
						ld  de, (_cd_cache_dst)
						ld  a, (_cd_xb)
						and 1
						ld  b, a
						ld  a, (_cd_s)
						ld  c, a
						call cd_build_cache
				#endasm
			}
			cd_rec [0] = cd_xb; cd_rec [1] = cd_py; cd_rec [8] = cd_s;
			cd_rec [9] |= 1;
		} else cd_rec [9] &= 0xfe;
	}
	#asm
			call cd_mark_sprites
	#endasm
}

void cd_invalidate_viewport (void) {
	#asm
			ld  ix, CD_FULLREC
			call cd_mark_rect
	#endasm
}

#include "cpc_dirty_flush.h"

void cd_update (unsigned char sprites) {
	if (sprites) {
		cd_sprites ();
		#ifdef MIN_FAPS_PER_FRAME
			#asm
				.cd_min_faps_loop
					ld  a, (isr_c2)
					cp  MIN_FAPS_PER_FRAME
					jr  nc, cd_min_faps_done
					halt
					jr  cd_min_faps_loop
				.cd_min_faps_done
					xor a
					ld  (isr_c2), a
			#endasm
		#endif
		// start when the beam has just left viewport row 0: 3rd interrupt
		#asm
			.cd_phase_wait
				ld  a, (isr_c1)
				cp  3
				jr  nz, cd_phase_wait
		#endasm
	}
	#asm
			ld  a, (_cd_intc)
			ld  (CD_INTC0), a
			call cd_flush
	#endasm
}
