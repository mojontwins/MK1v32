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
	unsigned char i, j;

	for (i = 0; i < 256; i ++) {
		for (j = 0; j < 8; j ++) {
			unsigned char p = 0;

			// a frame, plus a bit of noise so tiles are told apart
			if (j == 0 || j == 7) p = 0xff;
			else if ((i + j) & 1) p = 0xaa;
			if (i & 0x20) p = ~p;

			tile_graphics [i*8 + j] = p;
		}

		// splib2's tile array: character code -> address of the graphic
		sp_TileArray (i, tile_graphics + i*8);
	}
}

// Build one frame of one sprite: 2 columns of 24 lines of (graphic, mask).
void build_frame (unsigned char *data, unsigned char style, unsigned char frame) {
	unsigned char col, line, bit;

	for (col = 0; col < 2; col ++) {
		for (line = 0; line < 24; line ++) {
			unsigned char g = 0;
			unsigned char m = 0xff;    // transparent

			if (line < 16) {
				for (bit = 0; bit < 8; bit ++) {
					unsigned char px = col*8 + bit;   // 0..15
					unsigned char py = line;           // 0..15
					unsigned char on = 0;

					if (style == 0) {
						// hollow square
						if (px == 0 || px == 15 || py == 0 || py == 15) on = 1;
						// second frame adds a diagonal
						if (frame && px == py) on = 1;
					} else {
						// filled disc
						unsigned char dx = px > 7 ? px - 7 : 7 - px;
						unsigned char dy = py > 7 ? py - 7 : 7 - py;
						if (dx + dy <= 8) on = 1;
						// second frame makes it a ring
						if (frame && dx + dy > 5) on = 0;
					}

					if (on) g |= (0x80 >> bit);
				}
			}

			data [col*SPR_COL_BYTES + line*2] = g;
			data [col*SPR_COL_BYTES + line*2 + 1] = m;
		}
	}
}

void init_sprites (void) {
	unsigned char i;

	for (i = 0; i < 2; i ++) {
		unsigned char *data = sprite_data + i*2*SPR_FRAME_BYTES;

		// frame 0 and frame 1 of this sprite, laid out so that the
		// distance between them is always SPR_FRAME_BYTES
		build_frame (data, i, 0);
		build_frame (data + SPR_FRAME_BYTES, i, 1);

		// sprite: 16x16 pixels = 2 columns of 8 pixels, 2 rows of characters
		spr [i] = sp_CreateSpr (sp_MASK_SPRITE, 2, data, 1);
		sp_AddColSpr (spr [i], data + SPR_COL_BYTES);

		cur_frame [i] = data;
		next_frame [i] = data;

		spr_x [i] = 20 + i*60;
		spr_y [i] = 30 + i*40;
		spr_dx [i] = 1 + i;
		spr_dy [i] = 1;
	}
}

// Fill the display list with random tiles.
void random_background (void) {
	unsigned char row, col;

	for (row = 0; row < SCR_ROWS; row ++) {
		for (col = 0; col < SCR_COLS; col ++) {
			// attribute: bright white on black, or one of its cousins
			unsigned char attr = 0x07;
			unsigned char r = rnd ();
			if (r == 0) attr = 0x0e;
			else if (r == 1) attr = 0x1a;

			sp_PrintAtInv (row, col, attr, rnd ());
		}
	}
}

// ---------------------------------------------------------------- game

void move_sprites (void) {
	unsigned char i;

	for (i = 0; i < 2; i ++) {
		unsigned char *base = sprite_data + i*2*SPR_FRAME_BYTES;

		spr_x [i] += spr_dx [i];
		spr_y [i] += spr_dy [i];

		// bounce
		if (spr_x [i] < 0) { spr_x [i] = 0; spr_dx [i] = -spr_dx [i]; }
		if (spr_x [i] > SCR_COLS*8 - 17) { spr_x [i] = SCR_COLS*8 - 17; spr_dx [i] = -spr_dx [i]; }
		if (spr_y [i] < 0) { spr_y [i] = 0; spr_dy [i] = -spr_dy [i]; }
		if (spr_y [i] > SCR_ROWS*8 - 17) { spr_y [i] = SCR_ROWS*8 - 17; spr_dy [i] = -spr_dy [i]; }

		// animate: toggle between the two frames
		if (cur_frame [i] == base) next_frame [i] = base + SPR_FRAME_BYTES;
		else next_frame [i] = base;

		// the displacement between the current and the new frame: splib2
		// adds it to the graphic pointers of the sprite
		sp_MoveSprAbs (
			spr [i],
			&screen_clip,
			next_frame [i] - cur_frame [i],
			(unsigned char) (spr_y [i] >> 3),   // row
			(unsigned char) (spr_x [i] >> 3),   // column
			(unsigned char) (spr_x [i] & 7),    // horizontal pixel
			(unsigned char) (spr_y [i] & 7)     // vertical pixel
		);

		cur_frame [i] = next_frame [i];
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
