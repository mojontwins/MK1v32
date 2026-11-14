// MTE MK1 v3.2
// Copyleft 2011, 2026 by The Mojon Twins

// cpc_dirty.h - CPC backend without superbuffer (CPC_DIRTY): the glue
// between the engine and the cpcdirty library (lib/cpcdirty, link with
// -lcpcdirty instead of -lcpcrslib_mt). The renderer itself, the memory map
// and the three modes are described in lib/cpcdirty/README.md.
//
//   MODE_1                mode 1, 4 pens, sprite pen 0 transparent
//   MODE_0                planar mode 0, split palette (pen = bg * 4 + spr),
//                         sprites ORed; CD_GEOMETRY: per slot sizes
//                         (cd_geom [], CD_CACHE_END)
//   MODE_0 + MODE_0_NIBBLE  nibble mode 0, 16 pens, sprite pen
//                         TRANSPARENT_PEN (config.h, default 0) transparent
//
// BLACK_PEN (MODE_1, MODE_0_NIBBLE): the pen pattern 0 is filled with.
// CPC_SCREEN_80: the standard 80 x 200 screen instead of MK1's 64 x 192.

#ifdef MODE_1
	#define CD_MODE_M1
#elif defined MODE_0_NIBBLE
	#define CD_MODE_N0
	#ifdef TRANSPARENT_PEN
		#define CD_TPEN 	TRANSPARENT_PEN
	#endif
#else
	#define CD_MODE_P0
#endif
#ifdef BLACK_PEN
	#define CD_BLANK_PEN 	BLACK_PEN
#endif
#define CD_NSPR 		SW_SPRITES_ALL
#define CD_SPR_LIST 	BASE_SPRITES
#define CD_VPX 			VIEWPORT_X
#define CD_VPY 			VIEWPORT_Y
#ifdef CPC_SCREEN_80
	#define CD_SCRW 		80
#endif

#include <cpcdirty.h>

#asm
	#ifdef CD_MODE_M1
		LIB cd_init_m1
		LIB cd_sprites_m1
		LIB cd_pal_m1
		LIB cd_put_char_raw
	#endif
	#ifdef CD_MODE_N0
		LIB cd_init_n0
		LIB cd_sprites_n0
		LIB cd_pal_n0
		LIB cd_put_char_raw
	#endif
	#ifdef CD_MODE_P0
		LIB cd_init_p0
		LIB cd_sprites_p0
		LIB cd_pal_p0
		LIB cd_put_char_p0
	#endif
		LIB cd_flush
		LIB cd_invalidate
		LIB cd_put_tile
		LIB cd_set_cell
		LIB cd_screen_copy
		LIB cd_intc 		// the beam clock, incremented by the ISR (cpc.h)
#endasm

#if defined CD_GEOMETRY && defined CD_MODE_P0
	extern unsigned char cd_geom [];	// the game's: h, w (bytes) per sprite slot
#endif

extern unsigned char cd_patterns_c [];
#asm
	._cd_patterns_c
		BINARY "cpc/patterns.zx0"
#endasm

unsigned char cd_i, cd_vis;
unsigned char *cd_pal_bg, *cd_pal_spr;

// MODE_1: cd_pal_bg is the 4 hardware inks of pens 0-3 (mkts_om mode=pals of
// a 4 colour png). MODE_0_NIBBLE: cd_pal_bg is the 16 inks. MODE_0:
// cd_pal_bg and cd_pal_spr are 16-entry ink tables, pen p = bg * 4 + s shows
// cd_pal_spr [s] if s, else cd_pal_bg [bg * 4]. The border gets cd_pal_bg [0].
void cd_set_palette (void) {
	#asm
			ld  hl, (_cd_pal_bg)
		#ifdef CD_MODE_M1
			call cd_pal_m1
		#endif
		#ifdef CD_MODE_N0
			call cd_pal_n0
		#endif
		#ifdef CD_MODE_P0
			ld  de, (_cd_pal_spr)
			call cd_pal_p0
		#endif
	#endasm
}

// Patterns, tables, empty map; the inks of pal.png (cpc/pal.h: my_inks)
void cd_init (void) {
	#asm
			ld  hl, _cd_patterns_c
			ld  de, CD_PAT
			call depack
			ld  hl, _spr_empty
		#ifdef CD_MODE_M1
			call cd_init_m1
		#endif
		#ifdef CD_MODE_N0
			call cd_init_n0
		#endif
		#ifdef CD_MODE_P0
			#ifdef CD_GEOMETRY
				ld  de, _cd_geom
			#else
				ld  de, 0
			#endif
			call cd_init_p0
		#endif
			call my_inks
	#endasm
}

// Tile __t at screen chars (__x, __y), inside the viewport: 4 patterns from tile_map
void cd_draw_tile (void) {
	#asm
			ld  a, (__y)
			sub VIEWPORT_Y
			ld  c, a
			ld  a, (__x)
			sub VIEWPORT_X
			ld  b, a
			ld  a, (__t)
			add a, a
			add a, a
			ld  e, a
			ld  d, 0
			ld  hl, _tile_map
			add hl, de
			ex  de, hl
			call cd_put_tile
	#endasm
}

// Pattern __t at screen chars (__x, __y), straight to the screen. MODE_0:
// its colour 3 pixels in sprite pen 3 (stable whatever the level palette),
// the rest black; the other modes: as it is. HUD, text outside the viewport.
void cd_put_char (void) {
	#asm
			ld  a, (__x)
			ld  b, a
			ld  a, (__y)
			ld  c, a
			ld  a, (__t)
		#ifdef CD_MODE_P0
			call cd_put_char_p0
		#else
			call cd_put_char_raw
		#endif
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
				ld  a, (_rda)
				ld  b, a
				ld  a, (_rdb)
				ld  c, a
				ld  a, (__t)
				call cd_set_cell
		#endasm
	} else cd_put_char ();
}

// Compressed screen: zx0 of a mkts_om superbuffer (64 bytes x 192 lines,
// linear). It goes through the 12K at 9000-BFFF, which cd_init rebuilds
// afterwards; the level's sprite cells there are gone too, so screens are
// unpacked before custom_level_setup. Palette: pal.png's (my_inks), by cd_init.
// CPC_SCREEN_80: zx0 of a standard screen (mkts_om mode=scr, 16384 bytes),
// unpacked straight to C000 (9000-BFFF is not touched, but cd_init still
// empties the map and sets the palette, as with the other screens).
void cd_unpack_screen (void) {
	#asm
			// HL = source (set by the caller)
		#ifdef CPC_SCREEN_80
			ld  de, 0xC000
			call depack
		#else
			ld  de, 0x9000
			call depack
			ld  hl, 0x9000
			call cd_screen_copy
		#endif
	#endasm
	cd_init ();
}

void cd_invalidate_viewport (void) {
	#asm
			call cd_invalidate
	#endasm
}

// Sprites from sp_sw, then the flush, racing the beam from viewport row 0
void cd_update (unsigned char sprites) {
	if (sprites) {
		#asm
			#ifdef CD_MODE_M1
				call cd_sprites_m1
			#endif
			#ifdef CD_MODE_N0
				call cd_sprites_n0
			#endif
			#ifdef CD_MODE_P0
				call cd_sprites_p0
			#endif
		#endasm
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
			call cd_flush
	#endasm
}
