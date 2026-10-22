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
//  Note: this program has *no local variables*. z88dk keeps the locals of
//  a function in the stack, and the code it generates to reach them is
//  slow, so the engine works with globals only. See the "work" section
//  below.
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

// ---------------------------------------------------------------- work
//
// z88dk allocates the locals of a function in the stack, and the code it
// generates to reach them is slow: every access has to load the stack
// pointer and offset it. The engine has no locals at all for that reason,
// it uses globals, which live in the data segment and are reached
// directly.
//
// This example follows the same rule: every local became a global. Each
// function owns its own set of variables, so a function can call another
// one without the two stepping on each other. The parameters of our own
// functions are globals too, for the same reason.

unsigned char enc_px [4];                    // encode_4: the four pixels
unsigned char it_i, it_j, it_b, it_p;        // init_tiles
unsigned char km_v, km_m, km_p, km_hi, km_lo;// keep_mask
unsigned char il_i;                          // init_lut
unsigned char bs_row, bs_b, bs_px;           // build_sprite
unsigned char bs_x, bs_y, bs_on, bs_dx, bs_dy;
unsigned char *bs_data;                      // build_sprite: what to build
unsigned char bs_style;                      // build_sprite: what to build
unsigned char is_i;                          // init_sprites
unsigned char ms_i;                          // move_sprites
unsigned char upd_sprites;                   // cpc_UpdateNow

// ---------------------------------------------------------------- utils

unsigned char rnd (void) {
	seed ^= seed << 7;
	seed ^= seed >> 9;
	seed ^= seed << 8;
	return (unsigned char) (seed >> 8);
}

// Mode 1 packs 4 pixels of 2 bits in every byte, in the CPC's "garbled"
// order: the high bit of the leftmost pixel is in bit 3, its low bit in
// bit 7, and so on. The four pixels are in enc_px.
unsigned char encode_4 (void) {
	return ((enc_px [0] & 2) >> 1) << 2 | ((enc_px [1] & 2) >> 1) << 1 | (enc_px [2] & 2) >> 1 | (enc_px [3] & 2) >> 1 |
	       ((enc_px [0] & 1) << 7) | ((enc_px [1] & 1) << 6) | ((enc_px [2] & 1) << 5) | ((enc_px [3] & 1) << 4);
}

// The mask of background bits that a sprite byte leaves alone: the pixels
// whose colour is not 0 (transparent) overwrite the background. The byte
// to work on is in km_v.
unsigned char keep_mask (void) {
	km_m = 0xff;

	for (km_p = 0; km_p < 4; km_p ++) {
		km_hi = (km_v >> (3-km_p)) & 1;
		km_lo = (km_v >> (7-km_p)) & 1;
		if (km_hi | km_lo) {
			km_m &= ~(1 << (3-km_p));
			km_m &= ~(1 << (7-km_p));
		}
	}

	return km_m;
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
	for (il_i = 0; il_i < 256; il_i ++) {
		km_v = il_i;
		lut [il_i] = keep_mask ();
	}
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
	for (it_i = 0; it_i < 256; it_i ++) {
		for (it_j = 0; it_j < 8; it_j ++) {
			for (it_b = 0; it_b < 2; it_b ++) {
				// a frame, plus a checkerboard inside
				it_p = 0;
				if (it_j == 0 || it_j == 7) it_p = 1;
				else if ((it_i + it_j + it_b) & 1) it_p = 2;
				if (it_i & 0x20) it_p = 3 - it_p;

				enc_px [0] = it_p; enc_px [1] = it_p;
				enc_px [2] = it_p; enc_px [3] = it_p;

				tiles [it_i*16 + it_j*2 + it_b] = encode_4 ();
			}
		}
	}
}

// Fill the nametable with random tiles.
void random_background (void) {
	// bg_row and bg_col are the globals the assembly helper needs, so
	// they double as the loop counters.
	for (bg_row = 0; bg_row < SCR_ROWS; bg_row ++) {
		for (bg_col = 0; bg_col < SCR_COLS; bg_col ++) {
			ntable [bg_row*SCR_COLS + bg_col] = rnd ();

			// mark the cell as dirty
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

// Build a 16x16 sprite: 16 rows of 4 bytes. The sprite to build is in
// bs_data (where) and bs_style (what shape).
void build_sprite (void) {
	for (bs_row = 0; bs_row < 16; bs_row ++) {
		for (bs_b = 0; bs_b < 4; bs_b ++) {
			for (bs_px = 0; bs_px < 4; bs_px ++) {
				bs_x = bs_b*4 + bs_px;
				bs_y = bs_row;
				bs_on = 0;

				if (bs_style == 0) {
					// hollow square
					if (bs_x == 0 || bs_x == 15 || bs_y == 0 || bs_y == 15) bs_on = 1;
				} else {
					// filled disc
					bs_dx = bs_x > 7 ? bs_x - 7 : 7 - bs_x;
					bs_dy = bs_y > 7 ? bs_y - 7 : 7 - bs_y;
					if (bs_dx + bs_dy <= 8) bs_on = 1;
				}

				// colour 1 (white); 0 is transparent
				enc_px [bs_px] = bs_on ? 1 : 0;
			}

			bs_data [bs_row*4 + bs_b] = encode_4 ();
		}
	}
}

void init_sprites (void) {
	for (is_i = 0; is_i < 2; is_i ++) {
		bs_data = spr_data + is_i*16*4;
		bs_style = is_i;
		build_sprite ();

		sp_sw [is_i].sp0 = (unsigned int) (spr_data + is_i*16*4);
		sp_sw [is_i].sp1 = sp_sw [is_i].sp0;

		// the routines of the library: mark the cells it covers, draw it
		sp_sw [is_i].invfunc = (void *) cpc_PutSpTileMap16x16PxM1;
		sp_sw [is_i].updfunc = (void *) cpc_PutTrSp16x16TileMapPxM1LUT;

		spr_x [is_i] = VIEWPORT_X*8 + 20 + is_i*60;
		spr_y [is_i] = VIEWPORT_Y*8 + 30 + is_i*40;
		spr_dx [is_i] = 1 + is_i;
		spr_dy [is_i] = 1;
	}
}

void move_sprites (void) {
	for (ms_i = 0; ms_i < 2; ms_i ++) {
		spr_x [ms_i] += spr_dx [ms_i];
		spr_y [ms_i] += spr_dy [ms_i];

		// bounce inside the game area
		if (spr_x [ms_i] < VIEWPORT_X*8) { spr_x [ms_i] = VIEWPORT_X*8; spr_dx [ms_i] = -spr_dx [ms_i]; }
		if (spr_x [ms_i] > (VIEWPORT_X+VIEWPORT_W)*8 - 17) { spr_x [ms_i] = (VIEWPORT_X+VIEWPORT_W)*8 - 17; spr_dx [ms_i] = -spr_dx [ms_i]; }
		if (spr_y [ms_i] < VIEWPORT_Y*8) { spr_y [ms_i] = VIEWPORT_Y*8; spr_dy [ms_i] = -spr_dy [ms_i]; }
		if (spr_y [ms_i] > (VIEWPORT_Y+VIEWPORT_H)*8 - 17) { spr_y [ms_i] = (VIEWPORT_Y+VIEWPORT_H)*8 - 17; spr_dy [ms_i] = -spr_dy [ms_i]; }

		// X in pixels (0..255), Y in scanlines
		sp_sw [ms_i].cx = (unsigned char) spr_x [ms_i];
		sp_sw [ms_i].cy = (unsigned char) spr_y [ms_i];
	}
}

// ---------------------------------------------------------------- update
//
// This is the engine's cpc_UpdateNow() (src/dev/cpc.h), used verbatim but
// with the "sprites" flag turned into a global, so that there are no
// locals anywhere.

void cpc_UpdateNow (void) {
	if (upd_sprites) {
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

	if (upd_sprites) {
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

	// draw the sprites too
	upd_sprites = 1;

	while (1) {
		move_sprites ();
		cpc_UpdateNow ();
	}
}
