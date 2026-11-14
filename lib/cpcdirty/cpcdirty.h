/*
	cpcdirty.h

	cpcdirty: dirty cell renderer for the Amstrad CPC, without superbuffer.
	Copyleft 2026 The Mojon Twins. See README.md.

	Three modes, one library; the linker takes only the modules the game
	calls (cd_init_*, cd_sprites_*, cd_pal_*, cd_put_char_*):

	  CD_MODE_M1  mode 1: 4 pens shared by tiles and sprites, sprite pen 0
	              transparent (masked).
	  CD_MODE_P0  planar mode 0: split palette, pen = bg * 4 + spr; tiles in
	              pen bits 2-3, sprites in bits 0-1, ORed (no mask). Per slot
	              sprite geometry (8-32 lines, 8 or 16 pixels).
	  CD_MODE_N0  nibble mode 0: 16 pens shared by tiles and sprites, sprite
	              pen CD_TPEN transparent (masked).

	Include it ONCE (it emits the constants the library links against),
	after defining:

	  one of CD_MODE_M1, CD_MODE_P0, CD_MODE_N0
	  CD_NSPR       sprites in the list (1-8 in the masked modes, 1-16 in P0)
	  CD_SPR_LIST   address of the sprite list: CD_NSPR records of 16 bytes,
	                +0 frame (word), +8 x, +9 y. x in pixels of the mode
	                (mode 1: 0-319, mode 0: 0-159), y in lines, both screen
	                relative.
	  CD_VPX        viewport left, in chars of 2 bytes
	  CD_VPY        viewport top, in char rows
	optionally:
	  CD_BLANK_PEN  M1 (0-3), N0 (0-15): the pen pattern 0 cells are filled
	                with (pattern 0 must be all that pen). Default 0.
	  CD_TPEN       N0: the sprites' transparent pen. Default 0.
	  CD_CACHE_END  P0 with a geometry table: caches from B000 up to here
	                (page aligned), records and stamps after it.

	  CD_SCRW       bytes per screen line: 64 (default, MK1's CRTC R1 = 32)
	                or 80 (the standard screen)
	  CD_SCRH       screen lines, for cd_screen_copy: default 192 (200 if
	                CD_SCRW is 80)

	The screen is at C000, CD_SCRW bytes a line, 30 x 20 cells (2 bytes x 8
	lines) of viewport. The game's ISR must increment the byte
	cd_intc once per interrupt (300 Hz): LIB cd_intc / ld hl, cd_intc /
	inc (hl).
*/

#ifndef _CPCDIRTY_H
#define _CPCDIRTY_H

// Fixed layout (9000-AFFF)
#define CD_PAT 		0x9000
#define CD_ROWBUF 	0xA000
#define CD_BUCKET 	0xA400
#define CD_ROWD 	0xA700
#define CD_ROWINT 	0xA800
#define CD_ROWSN 	0xA900
#define CD_ROWSPR 	0xAA00
#define CD_ROWTAB 	0xAB00
#define CD_GEN 		0xAB8F
#define CD_TMAP 	0xAC00
#define CD_KEY_SP 	0xAE60
#define CD_KEY_PH 	0xAE80
#define CD_SPR_EMPTY 0xAE90
#define CD_RECSZ 	16

// Per mode
#define CD_CACHE 	0xB000
#if defined CD_MODE_M1 || defined CD_MODE_N0
	#define CD_CSIZE 	160
	#define CD_SPRT 	0xB500
	#define CD_MLUT 	0xB600
	#define CD_DIRTY 	0xB700
	#define CD_FREE 	0xB958
#elif defined CD_MODE_P0
	#define CD_CSIZE 	96
	#define CD_MLUT 	0
	#ifdef CD_CACHE_END
		#define CD_SPRT 	CD_CACHE_END
		#define CD_DIRTY 	CD_CACHE_END + 0x100 	// (no brackets: z80asm reads "ld hl, (x)" as a load)
		#define CD_FREE 	CD_CACHE_END + 0x358
	#else
		#define CD_SPRT 	0xB300
		#define CD_DIRTY 	0xB400
		#define CD_FREE 	0xB658
	#endif
#else
	#error "cpcdirty.h: define CD_MODE_M1, CD_MODE_P0 or CD_MODE_N0"
#endif

#ifndef CD_BLANK_PEN
	#define CD_BLANK_PEN 0
#endif
#ifndef CD_TPEN
	#define CD_TPEN 0
#endif
#ifndef CD_SCRW
	#define CD_SCRW 64
#endif
#ifndef CD_SCRH
	#if CD_SCRW == 80
		#define CD_SCRH 200
	#else
		#define CD_SCRH 192
	#endif
#endif

// A byte with both pixels in pen p. Mode 1: pen bit 0 in the high nibble,
// bit 1 in the low one. Mode 0: pixel 0 bits 7 5 3 1 = pen bits 0 2 1 3,
// pixel 1 the next ones down.
#ifdef CD_MODE_M1
	#define CD_PEN_BYTE(p) 	(((p) % 2) * 0xF0 + (((p) / 2) % 2) * 0x0F)
#else
	#define CD_PEN_BYTE(p) 	(((p) % 2) * 0xC0 + (((p) / 2) % 2) * 0x0C + (((p) / 4) % 2) * 0x30 + (((p) / 8) % 2) * 0x03)
#endif

#asm
	XDEF CDK_NSPR
	XDEF CDK_SPR_LIST
	XDEF CDK_VPX
	XDEF CDK_VPY
	XDEF CDK_SCRW
	XDEF CDK_SCRH
	XDEF CDK_CACHE
	XDEF CDK_SPRT
	XDEF CDK_DIRTY
	XDEF CDK_FREE
	XDEF CDK_MLUT
	XDEF CDK_BLANK_W
	XDEF CDK_TPEN_B
	defc CDK_NSPR = CD_NSPR
	defc CDK_SPR_LIST = CD_SPR_LIST
	defc CDK_VPX = CD_VPX
	defc CDK_VPY = CD_VPY
	defc CDK_SCRW = CD_SCRW
	defc CDK_SCRH = CD_SCRH
	defc CDK_CACHE = CD_CACHE
	defc CDK_SPRT = CD_SPRT
	defc CDK_DIRTY = CD_DIRTY
	defc CDK_FREE = CD_FREE
	defc CDK_MLUT = CD_MLUT
	defc CDK_BLANK_W = CD_PEN_BYTE (CD_BLANK_PEN) * 257
	defc CDK_TPEN_B = CD_PEN_BYTE (CD_TPEN)
#endasm

// Init (the patterns must be at CD_PAT; unpack them first). Builds every
// table and empties the map; call again after anything is unpacked into
// 9000-BFFF. Asm: HL = the frame that means "hidden" (spr_empty); P0 also
// DE = geometry table (h, w per slot) or 0 for 16 x 16 everywhere.
extern void __FASTCALL__ __LIB__ cd_init_m1 (unsigned char *empty);
extern void __FASTCALL__ __LIB__ cd_init_n0 (unsigned char *empty);
// cd_init_p0: asm only (HL, DE)

// Per frame: the sprite pass (read the list, rebuild caches, mark), then,
// right after the 3rd interrupt of a frame, the flush.
extern void __LIB__ cd_sprites_m1 (void);
extern void __LIB__ cd_sprites_p0 (void);
extern void __LIB__ cd_sprites_n0 (void);
extern void __LIB__ cd_flush (void);

// The whole viewport dirty
extern void __LIB__ cd_invalidate (void);

// Palettes: HL = hardware inks (P0: HL background, DE sprites: asm only)
extern void __FASTCALL__ __LIB__ cd_pal_m1 (unsigned char *inks);
extern void __FASTCALL__ __LIB__ cd_pal_n0 (unsigned char *inks);

// A CD_SCRW x CD_SCRH linear screen image -> the screen
extern void __FASTCALL__ __LIB__ cd_screen_copy (unsigned char *src);

// Asm only:
//   cd_put_tile      B column, C row (viewport cells), DE 4 patterns
//   cd_set_cell      B column, C row, A pattern
//   cd_mark_cell     A column, C row
//   cd_mark_rect     IX + 4..7 = c0 c1 r0 r1
//   cd_put_char_raw  B x, C y (screen chars), A pattern (M1, N0)
//   cd_put_char_p0   B x, C y, A pattern (its colour 3 in sprite pen 3)
//   cd_pal_p0        HL background inks, DE sprite inks

// Keyboard and mode, from cpcrslib (same names and tables)
extern void  __FASTCALL__ __LIB__ cpc_SetMode (unsigned char mode);
extern char  __FASTCALL__ __LIB__ cpc_TestKey (unsigned char tecla);

#endif
