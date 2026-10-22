// =========================================================================
//
//  cpcrslib-mt demo -- Amstrad CPC
//
//  Example standalone program that uses CPCRSLIB "Mojon Tilemap"
//  (cpcrslib-mt) the same way the MTE MK1 v3.2 engine does it:
//
//    - the library owns the screen: the background lives in the nametable
//      ($100) and the sprites are composed in the superbuffer ($9000); only
//      the "touched" cells are copied to the visible area ($C000).
//    - the cells that changed are marked in a bitfield (96 bytes) with
//      cpc_UpdTileTable / cpc_InvalidateRect.
//    - every sprite has a pair of routines: one that marks the cells it
//      covers (invfunc) and one that draws it (updfunc).
//    - the engine's own cpc_UpdateNow() is used verbatim to drive all of
//      that (see below).
//
//  What it does: fills the screen with random tiles and bounces two 16x16
//  sprites over them.
//
//  Build: see build.bat
//
// =========================================================================

#include <cpcrslib_mt.h>

// The game area, and the viewport the library clips the "touched tiles" to.
#define VIEWPORT_X		1
#define VIEWPORT_Y		2
#define VIEWPORT_W		30
#define VIEWPORT_H		20

#define SCR_ROWS		24
#define SCR_COLS		32

#define MIN_FAPS_PER_FRAME	2

// The tile graphics: 256 tiles of 16 bytes (8 rows of 2 bytes, mode 1).
extern unsigned char tiles [0];

// Assembly entry points and symbols the library needs from us.
#asm
	XREF _nametable

	LIB cpc_UpdTileTable
	LIB cpc_InvalidateRect

	XDEF viewport_x
	XDEF viewport_y
	defc viewport_x = VIEWPORT_X
	defc viewport_y = VIEWPORT_Y

	; The tile graphics: the library indexes them with the nametable bytes.
	XDEF tiles
	.tiles
	._tiles
	defs 256*16, 0
#endasm

// ---------------------------------------------------------------- memory

// The transparency LUT for the sprites lives at $FE00.
#define BASE_LUT	0xfe00

// The sprite structures (the engine's own layout).
#define BASE_SPRITES	0xe600
#define SW_SPRITES_ALL	2

typedef struct sprite {
	unsigned int sp0;			// 0  pointer to the graphic
	unsigned int sp1;			// 2  pointer to the mask
	unsigned int coord0;		// 4
	signed char cox, coy;		// 6 7 offset of the sprite face
	unsigned char cx, cy; 		// 8 9 position
	unsigned char ox, oy;		// 10 11 previous position
	void *invfunc;				// 12 mark the cells it covers
	void *updfunc;				// 14 draw it
} SPR;

SPR sp_sw [SW_SPRITES_ALL] @ BASE_SPRITES;

// Sprite graphics: 16x16 pixels, 16 rows of 4 bytes (mode 1).
unsigned char spr_data [2*16*4];

// The transparency LUT, built at startup and copied to $FE00.
unsigned char lut [256];

// The palette: colour number + hardware value, 16 colours.
unsigned char palette [16*2] = {
	0, 0x54,	// 0: black (also the "transparent" colour of the sprites)
	1, 0x4b,	// 1: white
	2, 0x43,	// 2: yellow
	3, 0x5b,	// 3: cyan
	4, 0x4b, 5, 0x4b, 6, 0x4b, 7, 0x4b,
	8, 0x4b, 9, 0x4b, 10, 0x4b, 11, 0x4b,
	12, 0x4b, 13, 0x4b, 14, 0x4b, 15, 0x4b
};

// A pointer to the nametable.
unsigned char *ntable;

// Globals used by the assembly helpers.
unsigned char bg_row, bg_col;

// Interrupt counters.
unsigned char isr_c1;
unsigned char isr_c2;

// Sprite state.
signed int spr_x [2], spr_y [2];
signed char spr_dx [2], spr_dy [2];

unsigned int seed = 0xc0de;

// ---------------------------------------------------------------- utils

unsigned char rnd (void) {
	seed ^= seed << 7;
	seed ^= seed >> 9;
	seed ^= seed << 8;
	return (unsigned char) (seed >> 8);
}

// Mode 1 packs 4 pixels of 2 bits in every byte, in the CPC's "garbled"
// order: the high bit of the leftmost pixel is in bit 3, its low bit in
// bit 7, and so on.
unsigned char encode_4 (unsigned char c0, unsigned char c1, unsigned char c2, unsigned char c3) {
	return ((c0 & 2) >> 1) << 2 | ((c1 & 2) >> 1) << 1 | (c2 & 2) >> 1 | (c3 & 2) >> 1 |
	       ((c0 & 1) << 7) | ((c1 & 1) << 6) | ((c2 & 1) << 5) | ((c3 & 1) << 4);
}

// The mask of background bits that a sprite byte leaves alone: the pixels
// whose colour is not 0 (transparent) overwrite the background.
unsigned char keep_mask (unsigned char v) {
	unsigned char m = 0xff, p, hi, lo;

	for (p = 0; p < 4; p ++) {
		hi = (v >> (3-p)) & 1;
		lo = (v >> (7-p)) & 1;
		if (hi | lo) {
			m &= ~(1 << (3-p));
			m &= ~(1 << (7-p));
		}
	}

	return m;
}

// ---------------------------------------------------------------- system

void set_palette (void) {
	#asm
		ld  c, 0x7f
		ld  b, 16
		ld  hl, _palette
	.pal_loop
		ld  a, (hl)
		out (c), a
		inc hl
		ld  a, (hl)
		out (c), a
		inc hl
		djnz pal_loop
	#endasm
}

// Build the transparency LUT at $FE00.
void init_lut (void) {
	unsigned char i;

	for (i = 0; i < 256; i ++) lut [i] = keep_mask (i);

	#asm
		ld  de, _lut
		ld  hl, BASE_LUT
		ld  bc, 256
		ldir
	#endasm
}

void system_init (void) {
	#asm
		di

		; clear the visible area
		ld  hl, 0xC000
		xor a
		ld  (hl), a
		ld  de, 0xC001
		ld  bc, 0x3DFF
		ldir

		; install the interrupt handler
		ld  a, 195
		ld  (0x38), a
		ld  hl, _isr
		ld  (0x39), hl

		; wait for the raster to sync
		ld  b, 0xf5
	.wait_vsync
		in a, (c)
		rra
		jp nc, wait_vsync

		xor a
		ld  (_isr_c1), a
		ld  (_isr_c2), a
		ei

		; the nametable
		ld  hl, _nametable
		ld  (_ntable), hl
	#endasm

	// Mode 1 (4 colours).
	cpc_SetMode (1);

	// The engine's tweaked CRTC: 32 x 24 characters.
	#asm
		; Horizontal chars (32), CRTC REG #1
		ld    b, 0xbc
		ld    c, 1
		out   (c), c
		inc   b
		ld    c, 32
		out   (c), c

		; Horizontal pos (42), CRTC REG #2
		ld    b, 0xbc
		ld    c, 2
		out   (c), c
		inc   b
		ld    c, 42
		out   (c), c

		; Vertical chars (24), CRTC REG #6
		ld    b, 0xbc
		ld    c, 6
		out   (c), c
		inc   b
		ld    c, 24
		out   (c), c
	#endasm

	set_palette ();
	init_lut ();

	// The rotation LUTs the pixel-perfect sprite routines need.
	cpc_MakeM1RotationLUTs ();
}

// The interrupt handler: counts frames.
//
// Like the engine's, the first six interrupts are just used to sync with
// the raster; from then on every interrupt counts as a frame.
void isr (void) {
	#asm
		push af
		push hl
		push de
		push bc

		ld  a, (_isr_c1)
		inc a
		ld  (_isr_c1), a
		cp  6
		jr  c, _isr_done

		ld  hl, _isr_c2
		inc (hl)

	._isr_done
		pop bc
		pop de
		pop hl
		pop af
		ei
		ret
	#endasm
}

// ---------------------------------------------------------------- tiles

void init_tiles (void) {
	unsigned char i, j, b, p;

	for (i = 0; i < 256; i ++) {
		for (j = 0; j < 8; j ++) {
			for (b = 0; b < 2; b ++) {
				// a frame, plus a checkerboard inside
				p = 0;
				if (j == 0 || j == 7) p = 1;
				else if ((i + j + b) & 1) p = 2;
				if (i & 0x20) p = 3 - p;

				tiles [i*16 + j*2 + b] = encode_4 (p, p, p, p);
			}
		}
	}
}

// Fill the nametable with random tiles.
void random_background (void) {
	unsigned char row, col;

	for (row = 0; row < SCR_ROWS; row ++) {
		for (col = 0; col < SCR_COLS; col ++) {
			ntable [row*SCR_COLS + col] = rnd ();

			// mark the cell as dirty
			bg_row = row;
			bg_col = col;
			#asm
				ld  a, (_bg_col)
				ld  e, a
				ld  a, (_bg_row)
				ld  d, a
				call cpc_UpdTileTable
			#endasm
		}
	}
}

// ---------------------------------------------------------------- sprites

// Build a 16x16 sprite: 16 rows of 4 bytes.
void build_sprite (unsigned char *data, unsigned char style) {
	unsigned char row, b, px, col [4];

	for (row = 0; row < 16; row ++) {
		for (b = 0; b < 4; b ++) {
			for (px = 0; px < 4; px ++) {
				unsigned char x = b*4 + px;
				unsigned char y = row;
				unsigned char on = 0;

				if (style == 0) {
					// hollow square
					if (x == 0 || x == 15 || y == 0 || y == 15) on = 1;
				} else {
					// filled disc
					unsigned char dx = x > 7 ? x - 7 : 7 - x;
					unsigned char dy = y > 7 ? y - 7 : 7 - y;
					if (dx + dy <= 8) on = 1;
				}

				// colour 1 (white); 0 is transparent
				col [px] = on ? 1 : 0;
			}

			data [row*4 + b] = encode_4 (col [0], col [1], col [2], col [3]);
		}
	}
}

void init_sprites (void) {
	unsigned char i;

	for (i = 0; i < 2; i ++) {
		build_sprite (spr_data + i*16*4, i);

		sp_sw [i].sp0 = (unsigned int) (spr_data + i*16*4);
		sp_sw [i].sp1 = sp_sw [i].sp0;

		// the routines of the library: mark the cells it covers, draw it
		sp_sw [i].invfunc = (void *) cpc_PutSpTileMap16x16PxM1;
		sp_sw [i].updfunc = (void *) cpc_PutTrSp16x16TileMapPxM1LUT;

		spr_x [i] = VIEWPORT_X*8 + 20 + i*60;
		spr_y [i] = VIEWPORT_Y*8 + 30 + i*40;
		spr_dx [i] = 1 + i;
		spr_dy [i] = 1;
	}
}

void move_sprites (void) {
	unsigned char i;

	for (i = 0; i < 2; i ++) {
		spr_x [i] += spr_dx [i];
		spr_y [i] += spr_dy [i];

		// bounce inside the game area
		if (spr_x [i] < VIEWPORT_X*8) { spr_x [i] = VIEWPORT_X*8; spr_dx [i] = -spr_dx [i]; }
		if (spr_x [i] > (VIEWPORT_X+VIEWPORT_W)*8 - 17) { spr_x [i] = (VIEWPORT_X+VIEWPORT_W)*8 - 17; spr_dx [i] = -spr_dx [i]; }
		if (spr_y [i] < VIEWPORT_Y*8) { spr_y [i] = VIEWPORT_Y*8; spr_dy [i] = -spr_dy [i]; }
		if (spr_y [i] > (VIEWPORT_Y+VIEWPORT_H)*8 - 17) { spr_y [i] = (VIEWPORT_Y+VIEWPORT_H)*8 - 17; spr_dy [i] = -spr_dy [i]; }

		// X in pixels (0..255), Y in scanlines
		sp_sw [i].cx = (unsigned char) spr_x [i];
		sp_sw [i].cy = (unsigned char) spr_y [i];
	}
}

// ---------------------------------------------------------------- update
//
// This is the engine's cpc_UpdateNow() (src/dev/cpc.h), used verbatim.

void cpc_UpdateNow (unsigned char sprites) {
	if (sprites) {
		#asm
			// Call the invalidate function for all sprites

			ld  a, #((SW_SPRITES_ALL)*16)
			._cpc_screen_update_inv_loop
				sub 16
				push af

				ld d, 0
				ld e, a
				ld hl, _sp_sw
				add hl, de

				// Save paremeter
				ld  b, h
				ld  c, l

				// Push return address into stack
				ld  de, _cpc_screen_update_inv_ret
				push de

				// Push function pointer into stack
				// Offset 12 into the structure: invfunc
				ld  de, 12
				add hl, de
				ld  e, (hl)
				inc hl
				ld  d, (hl)
				push de

				// __fastcall__ expects parameter in HL
				ld  h, b
				ld  l, c

				// ret will pop the function pointer from the
				// stack and jp to it. Next ret will get to
				// _cpc_screen_update_inv_ret
				ret

			._cpc_screen_update_inv_ret
				pop af
				or  a
				jr  nz, _cpc_screen_update_inv_loop
		#endasm
	}

	#asm
		._cpc_screen_update_upd_buffer
			call cpc_UpdScr
	#endasm

	if (sprites) {
		// Call the drawing function for all sprites

		#asm
			ld  a, #((SW_SPRITES_ALL)*16)
			._cpc_screen_update_upd_loop
				sub 16
				push af

				ld d, 0
				ld e, a
				ld hl, _sp_sw
				add hl, de

				// Save paremeter
				ld  b, h
				ld  c, l

				// Push return address into stack
				ld  de, _cpc_screen_update_upd_ret
				push de

				// Push function pointer into stack
				// Offset 14 into the structure: updfunc
				ld  de, 14
				add hl, de
				ld  e, (hl)
				inc hl
				ld  d, (hl)
				push de

				// __fastcall__ expects parameter in HL
				ld  h, b
				ld  l, c

				// ret will pop the function pointer from the
				// stack and jp to it. Next ret will get to
				// _cpc_screen_update_upd_ret
				ret

			._cpc_screen_update_upd_ret
				pop af
				or  a
				jr  nz, _cpc_screen_update_upd_loop

			._cpc_screen_update_done
		#endasm
	}

	#ifdef MIN_FAPS_PER_FRAME
		#asm
			.ml_min_faps_loop
				ld  a, (_isr_c2)
				cp  MIN_FAPS_PER_FRAME
				jr  nc, ml_min_faps_loop_end
				halt
				jr  ml_min_faps_loop

			.ml_min_faps_loop_end
			xor a
			ld  (_isr_c2), a
		#endasm
	#endif

	#asm
		call cpc_ShowTouchedTiles
		call cpc_ResetTouchedTiles
	#endasm
}

// ---------------------------------------------------------------- main

void main (void) {
	system_init ();
	init_tiles ();
	init_sprites ();
	random_background ();

	while (1) {
		move_sprites ();
		cpc_UpdateNow (1);
	}
}
