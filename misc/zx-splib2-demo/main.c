// =========================================================================
//
//  splib2 demo -- ZX Spectrum
//
//  Example standalone program that uses Sprite Pack 2.2 (splib2) the same
//  way the MTE MK1 v3.2 engine does it:
//
//    - splib2 owns the screen: the background lives in its display list and
//      is painted by SPUpdateNow, which only repaints the "dirty" cells.
//    - the tile graphics are registered in splib2's tile array (SPTileArray),
//      so the display list only stores tile numbers.
//    - sprites are splib2 sprites: sp_CreateSpr + sp_AddColSpr, moved with
//      sp_MoveSprAbs and clipped to the visible area.
//    - the memory for the sprite structures comes from splib2's block
//      allocator (sp_AddMemory / sp_BlockAlloc), hooked up through the
//      u_malloc / u_free pointers the library expects.
//    - animation is done with the engine's "bitdef displacement" trick: the
//      new frame is at a constant distance from the old one, and that
//      distance is handed over to sp_MoveSprAbs as the "animate" parameter.
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

#include <spritepack.h>

// The assembly-only entry points of splib2 that we need.
#asm
	LIB SPInitIM2
	LIB SPCreateGenericISR
	LIB SPRegisterHook
	LIB SPInitialize
	LIB SPUpdateNow
#endasm

// ---------------------------------------------------------------- memory
//
// The engine's memory map for the ZX target (see src/dev/speccy.h):
//
//   23296        isrc        frame counter, incremented by the ISR
//   24200-...    main binary
//   60840-61439  AD_FREE     block allocator (splib2)
//   61440-61696  IM2 vector table (splib2)
//   61697-61936  free
//   61936        stack
//   61952-65535  horizontal rotation tables (splib2)

#define STACK_ADDR      61936
#define NUMBLOCKS       16
#define AD_FREE         61440-(NUMBLOCKS*15)

// Screen size, in characters.
#define SCR_ROWS        24
#define SCR_COLS        32

// How many frames to wait for each iteration of the main loop.
#define MIN_FAPS        2

unsigned char isrc @ 23296;

// Tile graphics: 256 tiles of 8 bytes (one byte per scanline).
unsigned char tile_graphics [256*8];

// Sprite bitdefs.
//
// A masked sprite is made of columns of 8 pixels. Each line of a column is
// two bytes: the graphic and the mask. The result on screen is
// (background & mask) | graphic, so a mask of 255 leaves the background
// alone wherever the sprite has no pixel set.
//
// A column needs 8 extra lines of padding at the end: splib2 moves the
// graphic pointer up to 7 lines up or down (the vertical sub-pixel
// position), and that must not read past the data.
#define SPR_COL_BYTES   48                              // 24 lines x 2 bytes
#define SPR_FRAME_BYTES (2*SPR_COL_BYTES)              // 2 columns
unsigned char sprite_data [4*SPR_FRAME_BYTES];         // 2 sprites x 2 frames

// ---------------------------------------------------------------- state

struct sp_SS *spr [2];
struct sp_Rect screen_clip = { 0, 0, SCR_ROWS, SCR_COLS };

unsigned char *cur_frame [2];
unsigned char *next_frame [2];

signed int spr_x [2], spr_y [2];
signed char spr_dx [2], spr_dy [2];

unsigned int seed = 0xace1;

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

unsigned char ti, tj, tp;                    // init_tiles
unsigned char bf_col, bf_line, bf_bit;       // build_frame
unsigned char bf_px, bf_py, bf_on, bf_g, bf_m;
unsigned char bf_dx, bf_dy;
unsigned char *bf_data;                      // build_frame: what to build
unsigned char bf_style, bf_frame;            // build_frame: what to build
unsigned char si;                            // init_sprites
unsigned char *sd;                           // init_sprites
unsigned char rb_row, rb_col, rb_attr, rb_r; // random_background
unsigned char mi;                            // move_sprites
unsigned char *mb;                           // move_sprites

// ---------------------------------------------------------------- utils

unsigned char rnd (void) {
	// xorshift
	seed ^= seed << 7;
	seed ^= seed >> 9;
	seed ^= seed << 8;
	return (unsigned char) (seed >> 8);
}

// ---------------------------------------------------------------- splib2

// splib2 asks for memory through these two pointers.
void *my_malloc (uint bytes) { return sp_BlockAlloc (0); }
void *u_malloc = my_malloc;
void *u_free = NULL;

// The interrupt handler: counts frames.
void ISR (void) {
	#asm
		ld  hl, _isrc
		inc (hl)
	#endasm
}

void system_init (void) {
	#asm
		di

		; IM2 interrupt mode, with the generic ISR copied to $F1F1
		ld  bc, 0xf1f1
		call SPInitIM2

		ld  de, 0xf1f1
		call SPCreateGenericISR

		; hook our frame counter into the generic ISR
		ld  l, 255
		ld  bc, _ISR
		call SPRegisterHook

		; empty display list, everything dirty, build rotation tables
		ld  de, 0
		call SPInitialize
		ei
	#endasm

	// Memory for the sprite structures (14 bytes each: one per sprite plus
	// one per column and row of every sprite).
	sp_AddMemory (0, NUMBLOCKS, 14, AD_FREE);
}

// Register the tile graphics in splib2's tile array, so the display list
// can just store tile numbers.
void init_tiles (void) {
	for (ti = 0; ti < 256; ti ++) {
		for (tj = 0; tj < 8; tj ++) {
			tp = 0;

			// a frame, plus a bit of noise so tiles are told apart
			if (tj == 0 || tj == 7) tp = 0xff;
			else if ((ti + tj) & 1) tp = 0xaa;
			if (ti & 0x20) tp = ~tp;

			tile_graphics [ti*8 + tj] = tp;
		}

		// splib2's tile array: character code -> address of the graphic
		sp_TileArray (ti, tile_graphics + ti*8);
	}
}

// Build one frame of one sprite: 2 columns of 24 lines of (graphic, mask).
// The frame to build is in bf_data (where), bf_style (what shape) and
// bf_frame (which of the two frames).
void build_frame (void) {
	for (bf_col = 0; bf_col < 2; bf_col ++) {
		for (bf_line = 0; bf_line < 24; bf_line ++) {
			bf_g = 0;
			bf_m = 0xff;    // transparent

			if (bf_line < 16) {
				for (bf_bit = 0; bf_bit < 8; bf_bit ++) {
					bf_px = bf_col*8 + bf_bit;   // 0..15
					bf_py = bf_line;              // 0..15
					bf_on = 0;

					if (bf_style == 0) {
						// hollow square
						if (bf_px == 0 || bf_px == 15 || bf_py == 0 || bf_py == 15) bf_on = 1;
						// second frame adds a diagonal
						if (bf_frame && bf_px == bf_py) bf_on = 1;
					} else {
						// filled disc
						bf_dx = bf_px > 7 ? bf_px - 7 : 7 - bf_px;
						bf_dy = bf_py > 7 ? bf_py - 7 : 7 - bf_py;
						if (bf_dx + bf_dy <= 8) bf_on = 1;
						// second frame makes it a ring
						if (bf_frame && bf_dx + bf_dy > 5) bf_on = 0;
					}

					if (bf_on) bf_g |= (0x80 >> bf_bit);
				}
			}

			bf_data [bf_col*SPR_COL_BYTES + bf_line*2] = bf_g;
			bf_data [bf_col*SPR_COL_BYTES + bf_line*2 + 1] = bf_m;
		}
	}
}

void init_sprites (void) {
	for (si = 0; si < 2; si ++) {
		sd = sprite_data + si*2*SPR_FRAME_BYTES;

		// frame 0 and frame 1 of this sprite, laid out so that the
		// distance between them is always SPR_FRAME_BYTES
		bf_data = sd; bf_style = si; bf_frame = 0;
		build_frame ();
		bf_data = sd + SPR_FRAME_BYTES; bf_frame = 1;
		build_frame ();

		// sprite: 16x16 pixels = 2 columns of 8 pixels, 2 rows of characters
		spr [si] = sp_CreateSpr (sp_MASK_SPRITE, 2, sd, 1);
		sp_AddColSpr (spr [si], sd + SPR_COL_BYTES);

		cur_frame [si] = sd;
		next_frame [si] = sd;

		spr_x [si] = 20 + si*60;
		spr_y [si] = 30 + si*40;
		spr_dx [si] = 1 + si;
		spr_dy [si] = 1;
	}
}

// Fill the display list with random tiles.
void random_background (void) {
	for (rb_row = 0; rb_row < SCR_ROWS; rb_row ++) {
		for (rb_col = 0; rb_col < SCR_COLS; rb_col ++) {
			// attribute: bright white on black, or one of its cousins
			rb_attr = 0x07;
			rb_r = rnd ();
			if (rb_r == 0) rb_attr = 0x0e;
			else if (rb_r == 1) rb_attr = 0x1a;

			sp_PrintAtInv (rb_row, rb_col, rb_attr, rnd ());
		}
	}
}

// ---------------------------------------------------------------- game

void move_sprites (void) {
	for (mi = 0; mi < 2; mi ++) {
		mb = sprite_data + mi*2*SPR_FRAME_BYTES;

		spr_x [mi] += spr_dx [mi];
		spr_y [mi] += spr_dy [mi];

		// bounce
		if (spr_x [mi] < 0) { spr_x [mi] = 0; spr_dx [mi] = -spr_dx [mi]; }
		if (spr_x [mi] > SCR_COLS*8 - 17) { spr_x [mi] = SCR_COLS*8 - 17; spr_dx [mi] = -spr_dx [mi]; }
		if (spr_y [mi] < 0) { spr_y [mi] = 0; spr_dy [mi] = -spr_dy [mi]; }
		if (spr_y [mi] > SCR_ROWS*8 - 17) { spr_y [mi] = SCR_ROWS*8 - 17; spr_dy [mi] = -spr_dy [mi]; }

		// animate: toggle between the two frames
		if (cur_frame [mi] == mb) next_frame [mi] = mb + SPR_FRAME_BYTES;
		else next_frame [mi] = mb;

		// the displacement between the current and the new frame: splib2
		// adds it to the graphic pointers of the sprite
		sp_MoveSprAbs (
			spr [mi],
			&screen_clip,
			next_frame [mi] - cur_frame [mi],
			(unsigned char) (spr_y [mi] >> 3),   // row
			(unsigned char) (spr_x [mi] >> 3),   // column
			(unsigned char) (spr_x [mi] & 7),    // horizontal pixel
			(unsigned char) (spr_y [mi] & 7)     // vertical pixel
		);

		cur_frame [mi] = next_frame [mi];
	}
}

// Wait for the number of frames the engine waits for.
void wait_frame (void) {
	#asm
		.ml_min_faps_loop
			ld  a, (_isrc)
			cp  MIN_FAPS
			jr  nc, ml_min_faps_loop_end
			halt
			jr  ml_min_faps_loop
		.ml_min_faps_loop_end
			xor a
			ld  (_isrc), a
	#endasm
}

void main (void) {
	#asm
		ld  sp, STACK_ADDR
	#endasm

	system_init ();
	init_tiles ();
	init_sprites ();
	random_background ();

	while (1) {
		move_sprites ();

		// paint the dirty cells: background tiles + sprites
		#asm
			call SPUpdateNow
		#endasm

		wait_frame ();
	}
}
