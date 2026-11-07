// MTE MK1 v3.2
// Copyleft 2011, 2026 by The Mojon Twins

// Punchy: the MK1_NES tester 02_tester_punchy on MK1 v3.2 (ZX and CPC mode 0).
// pn_gfx.h big sprites, flip, pieces, cocos; pn_enems.h enemies; pn_player.h
// the player.
//
// Every function is written in assembly (sccz80 only makes the C shells):
// the C original of each one is kept in its comment. Calling conventions:
// no parameters (globals), results in L (HL); the engine's peta_el_beeper
// takes L, draw_coloured_tile is entered at _draw_coloured_tile_do with
// __x, __y, __t set.

#include "pn_frames.h"

// The tile map (mkts_om tmaps): 8 bytes per tile on ZX, 4 on CPC
extern unsigned char tile_map [];
#asm
	._tile_map
	#ifdef CPC
		BINARY "cpc/ts_tmaps.bin"
	#else
		BINARY "ts_tmaps.bin"
	#endif
#endasm

#ifdef CPC
	#include "cpc/bg_pal.h"
	#include "cpc/spr_pal.h"
#endif

// Engine functions used before engine.h
unsigned char collide_enem (void);
unsigned char rand (void);
void enems_kill (unsigned char damage);

// NES frames per MK1 frame = 1.2 * MIN_FAPS_PER_FRAME = PN_NUM / 5;
// PN_Tn = n NES frames in MK1 frames. Plain numbers: they go into the
// assembly too (z80asm reads "ld a, (x)" as a load from memory).
#if MIN_FAPS_PER_FRAME == 3
	#define PN_NUM 			18
	#define PN_T16 			4
	#define PN_T8 			2
	#define PN_SHOOT_MASK 	7		// NES: 1 in 32 frames
	#define PN_SPD2 		7		// NES 2 px / frame
	#define PN_VX_MIN 		230		// NES PLAYER_VX_MIN (64)
	#define PN_VY_LADDER 	345		// NES 96
#else
	#define PN_NUM 			12
	#define PN_T16 			7
	#define PN_T8 			3
	#define PN_SHOOT_MASK 	15
	#define PN_SPD2 		4
	#define PN_VX_MIN 		153
	#define PN_VY_LADDER 	230
#endif
#define PN_FROZEN_FRAMES 	PN_T16	// NES 16: nothing moves after a hit lands

// Shared by the modules
unsigned char pn_t1, pn_t2, pn_cx, pn_cy, pn_k, pn_l;
unsigned char pn_hitact, pn_frozen;
signed int pn_hx, pn_hy;
unsigned char pn_warp, pn_warp_t, pn_warp_x, pn_warp_y;

// Helpers
#asm
	// HL = HL / 5 (unsigned), A = the remainder. Uses B.
	.pn_div5
		xor a
		ld  b, 16
	.pn_d5_loop
		add hl, hl
		rla
		cp  5
		jr  c, pn_d5_next
		sub 5
		inc l
	.pn_d5_next
		djnz pn_d5_loop
		ret

	// HL = HL + A (A unsigned)
	.pn_add_a
		add a, l
		ld  l, a
		adc a, h
		sub l
		ld  h, a
		ret
#endasm

unsigned char pn_attr (void) {
	// behaviour of tile (pn_cx, pn_cy), 0 out of the screen; in A and HL
	// if (pn_cx > 14 || pn_cy > 9) return 0;
	// return map_attr [pn_cx + (pn_cy << 4) - pn_cy];
	#asm
			ld  a, (_pn_cx)
			cp  15
			jr  nc, pn_attr_out
			ld  c, a
			ld  a, (_pn_cy)
			cp  10
			jr  nc, pn_attr_out
			ld  b, a
			add a, a
			add a, a
			add a, a
			add a, a
			sub b
			add a, c
			ld  hl, _map_attr
			call pn_add_a
			ld  a, (hl)
			ld  l, a
			ld  h, 0
			ret
		.pn_attr_out
			xor a
			ld  l, a
			ld  h, a
	#endasm
}

void pn_player_hurt (void) {
	// if (player.estado == EST_NORMAL) {
	//	player.is_dead = PLAYER_KILLED_BY_ENEM; player.drain_amount = 1;
	//	peta_el_beeper (2);
	// }
	#asm
			ld  a, (_player + 23) 		// estado
			or  a
			ret nz
			ld  a, PLAYER_KILLED_BY_ENEM
			ld  (_player + 36), a 		// is_dead
			ld  a, 1
			ld  (_player + 46), a 		// drain_amount
			ld  l, 2
			call _peta_el_beeper
	#endasm
}

#include "pn_gfx.h"
#include "pn_enems.h"
#include "pn_player.h"

void custom_game_start (void) {
	// CPC: split palette, background inks (pens 0, 4, 8, 12), sprite inks
	// (1-3): cd_pal_bg = bg_pal; cd_pal_spr = spr_pal; cd_set_palette ();
	// pn_ladder = pn_jump = pn_punch = pn_kick = pn_hitact = pn_frozen = 0;
	// pn_warp = pn_warp_t = 0; pn_nvy = 0;
	#asm
		#ifdef CPC
			ld  hl, _bg_pal
			ld  (_cd_pal_bg), hl
			ld  hl, _spr_pal
			ld  (_cd_pal_spr), hl
			call _cd_set_palette
		#endif
			xor a
			ld  (_pn_ladder), a
			ld  (_pn_jump), a
			ld  (_pn_punch), a
			ld  (_pn_kick), a
			ld  (_pn_hitact), a
			ld  (_pn_frozen), a
			ld  (_pn_warp), a
			ld  (_pn_warp_t), a
			ld  h, a
			ld  l, a
			ld  (_pn_nvy), hl
	#endasm
}

void custom_screen_enter (void) {
	// CPC: the title / frame screens are unpacked over 9000-BFFF, the flip
	// table and buffers (B900-BC5F) have to be made again: pn_flip_init ();
	// pc_on [], co_on [], br_ct [] = 0; pn_propellers_setup ();
	// pn_safe_x = player.x; pn_safe_y = player.y;
	#asm
		#ifdef CPC
			call _pn_flip_init
		#endif
			xor a
			ld  hl, _pc_on
			ld  b, PN_PIECES
		.pn_se_pc
			ld  (hl), a
			inc hl
			djnz pn_se_pc
			ld  hl, _co_on
			ld  b, PN_COCOS
		.pn_se_co
			ld  (hl), a
			inc hl
			djnz pn_se_co
			ld  hl, _br_ct
			ld  b, PN_BRK_MAX
		.pn_se_br
			ld  (hl), a
			inc hl
			djnz pn_se_br
			call _pn_propellers_setup
			ld  hl, (_player)
			ld  (_pn_safe_x), hl
			ld  hl, (_player + 2)
			ld  (_pn_safe_y), hl
	#endasm
}
