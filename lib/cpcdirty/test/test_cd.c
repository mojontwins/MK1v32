// cpcdirty - test bench. Copyleft 2026 The Mojon Twins
//
// Fills the patterns and the viewport map with junk, makes 4 sprite frames,
// then every 50 frames (from frame 100) moves 8 sprites to new pseudo-random
// places (some hidden, some at the edges, odd and even x), changes some
// cells and a tile, and runs the sprite pass + the flush. check.py compares
// the screen with a model built from a memory dump taken 40 frames after
// each step. Build with -DCD_MODE_xx (test.sh: N0, M1, P0), optionally
// -DCD_SCRW=80 -DCD_VPX=n -DCD_VPY=n.

#define CD_NSPR 		8
#define CD_SPR_LIST 	0x8000
#ifndef CD_VPX
	#define CD_VPX 		1
#endif
#ifndef CD_VPY
	#define CD_VPY 		2
#endif
#ifdef CD_MODE_N0
	#define CD_TPEN 		5
	#define CD_BLANK_PEN 	9
#endif
#ifdef CD_MODE_M1
	#define CD_BLANK_PEN 	2
#endif

#include <cpcdirty.h>

#asm
	#ifdef CD_MODE_N0
		LIB cd_init_n0
		LIB cd_sprites_n0
	#endif
	#ifdef CD_MODE_M1
		LIB cd_init_m1
		LIB cd_sprites_m1
	#endif
	#ifdef CD_MODE_P0
		LIB cd_init_p0
		LIB cd_sprites_p0
	#endif
		LIB cd_flush
		LIB cd_invalidate
		LIB cd_set_cell
		LIB cd_put_tile
		LIB cd_intc
#endasm

#define FRAMES 		0x8400		// 4 frames x 64 bytes
#define EMPTY 		0x8500		// the "hidden" frame (never drawn)
#define STEP 		0x8600		// steps done (for check.py)

unsigned int seed;
unsigned char rnd (void) {
	seed = seed * 25173 + 13849;
	return seed >> 8;
}

unsigned char i, j, k, step, x, y;
unsigned int n, ticks, until;
unsigned char *p;
unsigned char tl [4];

void main (void) {
	#asm
			di
			ld  sp, 0x8f00
			ld  a, 195
			ld  (0x38), a
			ld  hl, isr
			ld  (0x39), hl
			im  1
			jp  isr_done
		.isr
			push af
			push hl
			ld  hl, cd_intc
			inc (hl)
			ld  hl, (_ticks)
			inc hl
			ld  (_ticks), hl
			pop hl
			pop af
			ei
			ret
		.isr_done
			ld  bc, 0x7F8C 			// mode 0, ROMs off
		#ifdef CD_MODE_M1
			inc c
		#endif
			out (c), c
			ei
	#endasm

	seed = 1234;
	// patterns: junk, pattern 0 blank
	p = (unsigned char *) CD_PAT;
	for (n = 0; n < 4096; n ++) { *p = n < 16 ? CD_PEN_BYTE (CD_BLANK_PEN) : rnd (); p ++; }
	// frames
	p = (unsigned char *) FRAMES;
	for (n = 0; n < 256; n ++) { *p = rnd (); p ++; }
	// sprite list: all hidden
	p = (unsigned char *) CD_SPR_LIST;
	for (i = 0; i < 128; i ++) { p [i] = 0; }
	for (i = 0; i < 8; i ++) { p [i << 4] = EMPTY & 0xff; p [(i << 4) + 1] = EMPTY >> 8; }
	*((unsigned char *) STEP) = 0;

	#asm
			ld  hl, EMPTY
		#ifdef CD_MODE_N0
			call cd_init_n0
		#endif
		#ifdef CD_MODE_M1
			call cd_init_m1
		#endif
		#ifdef CD_MODE_P0
			ld  de, 0
			call cd_init_p0
		#endif
	#endasm

	// map: junk patterns (some 0)
	for (y = 0; y < 20; y ++) for (x = 0; x < 30; x ++) {
		k = rnd (); if (k < 40) k = 0;
		#asm
				ld  a, (_x)
				ld  b, a
				ld  a, (_y)
				ld  c, a
				ld  a, (_k)
				call cd_set_cell
		#endasm
	}
	#asm
			call cd_invalidate
	#endasm

	ticks = 0;
	until = 600;
	for (step = 0; step < 16; step ++) {
		// wait for frame 100 + 50 * step (6 interrupts a frame)
		while (ticks < until) {
			#asm
					halt
			#endasm
		}
		until += 300;

		// sprites
		p = (unsigned char *) CD_SPR_LIST;
		for (i = 0; i < 8; i ++) {
			j = rnd ();
			if ((j & 7) == 0) {
				p [0] = EMPTY & 0xff; p [1] = EMPTY >> 8;
			} else {
				p [0] = (FRAMES + ((j >> 4) & 3) * 64) & 0xff;
				p [1] = (FRAMES + ((j >> 4) & 3) * 64) >> 8;
			}
			#ifdef CD_MODE_M1
				x = rnd () % 225; y = rnd () % 145;
				if ((j & 0x0C) == 0) x = (j & 1) ? 0 : 224;
				p [8] = x + CD_VPX * 8;
			#else
				x = rnd () % 113; y = rnd () % 145;
				if ((j & 0x0C) == 0) x = (j & 1) ? 0 : 112;
				p [8] = x + CD_VPX * 4;
			#endif
			if ((j & 0x0C) == 4) y = (j & 1) ? 0 : 144;
			p [9] = y + CD_VPY * 8;
			p += 16;
		}
		// some cells and a tile
		for (i = 0; i < 6; i ++) {
			x = rnd () % 30; y = rnd () % 20; k = rnd ();
			#asm
					ld  a, (_x)
					ld  b, a
					ld  a, (_y)
					ld  c, a
					ld  a, (_k)
					call cd_set_cell
			#endasm
		}
		x = (rnd () % 15) << 1; y = (rnd () % 10) << 1;
		tl [0] = rnd (); tl [1] = rnd (); tl [2] = rnd (); tl [3] = 0;
		#asm
				ld  a, (_x)
				ld  b, a
				ld  a, (_y)
				ld  c, a
				ld  de, _tl
				call cd_put_tile
		#endasm

		#asm
			#ifdef CD_MODE_N0
				call cd_sprites_n0
			#endif
			#ifdef CD_MODE_M1
				call cd_sprites_m1
			#endif
			#ifdef CD_MODE_P0
				call cd_sprites_p0
			#endif
				call cd_flush
		#endasm
		*((unsigned char *) STEP) = step + 1;
	}
	while (1) {
		#asm
				halt
		#endasm
	}
}
