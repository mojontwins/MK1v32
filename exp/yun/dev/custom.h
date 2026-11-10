// MTE MK1 v3.2
// Copyleft 2011, 2026 by The Mojon Twins

// Yun (ZX / CPC) - custom.h
//
// Everything Yun-specific lives here (engine hooks used: ENABLE_CUSTOM_SPRITES,
// ENABLE_CUSTOM_GAME_START, ENABLE_CUSTOM_SCREEN_ENTER, ENABLE_CUSTOM_PRE_MOVE,
// ENABLE_CUSTOM_ENEMS; plus JUMP_ON_UP_ONLY, ENEMIES_CAN_DIE, WIN_ON_KILLS):
//
//  * Bubbles ("fabolees"): up to BUBS_MAX OR sprites. They fly forward
//    decelerating, then rise and bob at the top of the screen, like Yun v5
//    (tables in yun_luts.h, resampled from the NES ones). FIRE blows one.
//  * Riding: Yun can stand on a free bubble and is carried. We hand the engine
//    a vy / ptgmx before it moves her, so gravity, jump and collisions are the
//    engine's. Reaching the top flicks screen; a new bubble is created under
//    her in the screen above, like the NES game.
//  * Capturing: a free bubble touching a linear enemy (types 1-3) traps it.
//    The enemy freezes, drops to the floor, shows its upside-down frame and
//    is harmless. Touching it kills it. If the bubble pops first, it's free.
//  * Stepping on an enemy bounces Yun, without killing it.
//  * Levels (ENABLE_CUSTOM_LEVELS): YUN_LEVELS per load. custom_level_setup
//    decompresses the level area (map + bolts, enemies + hotspots, tile map;
//    see yun_level.h) and the level's enemy sprite cells, and copies its tile
//    behaviours; custom_level_won goes on to the next one, keeping Yun's life.
//  * Fanty (type 5), monococo (6, 8 in a wall), pezon (7), gyrosaw (9),
//    cannon (10): the NES precalc enemies resampled to the ZX / CPC rate.
//
// Every function is written in assembly; its C is in the comments. No
// locals, no parameters: everything goes through globals and registers.
// Results in L (HL). Calling conventions used from here:
//   peta_el_beeper: fastcall, the argument in L
//   attr: C = x, A = y, call _attr_2, result in L
//   draw_coloured_tile: set __x, __y, __t, call _draw_coloured_tile_do
//   draw_2_digits: set __x, __y, the number in A, call draw_2_digits_shortcut
//   other C functions (sp_CreateSpr, sp_AddColSpr, enems_kill): arguments
//   pushed left to right, popped after the call
// Bubble arrays and per-enemy arrays are laid out back to back (asm defs) so
// IX / IY can point at one bubble or enemy and reach every field by offset.
// Constants used in the asm must not start with a bracket: z80asm reads
// "ld a, (x)" as a memory read.

#include "yun_luts.h"
#include "yun_load.h"

#define BUBS_MAX 			3
// (BUB_COOLDOWN, frames between bubbles, and BUB_FALL, px/frame a trapped enemy
// falls, are in yun_luts.h: they depend on the frame rate)

// Engine functions defined after this file is included
unsigned char attr (unsigned char x, unsigned char y);
unsigned char collide_enem (void);
void enems_kill (unsigned char damage);
unsigned char rand (void);

// Sprite cells for the asm (CPC: sprite_n_a is spr_fixed + 64 * (n - 1), a macro)
#ifdef CPC
	#define YS9  			_spr_fixed + 512
	#define YS10 			_spr_fixed + 576
	#define YS13 			_spr_fixed + 768
	#define YS14 			_spr_fixed + 832
	#define YS17 			_spr_fixed + 1024
	#define YS18 			_spr_fixed + 1088
	#define YS19 			_spr_fixed + 1152
	#define YS21 			_spr_fixed + 1280
	#define YS23 			_spr_fixed + 1408
	#define YS24 			_spr_fixed + 1472
	#define YS25 			_spr_fixed + 1536
	#define YS27 			_spr_fixed + 1664
	#define YS28 			_spr_fixed + 1728
	#define YS29 			_spr_fixed + 1792
	#define YS30 			_spr_fixed + 1856
	#define YS31 			_spr_fixed + 1920
	#define YS32 			_spr_fixed + 1984
#else
	#define YS9  			_sprite_9_a
	#define YS10 			_sprite_10_a
	#define YS13 			_sprite_13_a
	#define YS14 			_sprite_14_a
	#define YS17 			_sprite_17_a
	#define YS18 			_sprite_18_a
	#define YS19 			_sprite_19_a
	#define YS21 			_sprite_21_a
	#define YS23 			_sprite_23_a
	#define YS24 			_sprite_24_a
	#define YS25 			_sprite_25_a
	#define YS27 			_sprite_27_a
	#define YS28 			_sprite_28_a
	#define YS29 			_sprite_29_a
	#define YS30 			_sprite_30_a
	#define YS31 			_sprite_31_a
	#define YS32 			_sprite_32_a
#endif

// ----------------------------------------------------------------------------
// Bubbles: unsigned char bub_ct [BUBS_MAX] (life, 0 = free), bub_x, bub_y,
// bub_ix, bub_iy (table indexes), bub_state (0 rising, 1 bobbing at the top),
// bub_fc (0 right, 1 left), signed char bub_mx (last horizontal displacement),
// bub_lock (enemy index or 0xff), bub_shown. With IX -> bub_ct [bi]:

#define YB_CT				0
#define YB_X				3		// BUBS_MAX * 1
#define YB_Y				6
#define YB_IX				9
#define YB_IY				12
#define YB_ST				15
#define YB_FC				18
#define YB_MX				21
#define YB_LK				24
#define YB_SH				27

// Per enemy of the screen: en_cap (0 or bubble index + 1), cap_x, cap_y
// (trapped), en_ct, en_st (pezon, gyrosaw, cannon). With IX -> en_cap [enit]:

#define YE_CAP				0
#define YE_CX				3		// MAX_ENEMS * 1
#define YE_CY				6
#define YE_CT				9
#define YE_ST				12

#asm
	._bub_ct		defs 3
	._bub_x			defs 3
	._bub_y			defs 3
	._bub_ix		defs 3
	._bub_iy		defs 3
	._bub_state		defs 3
	._bub_fc		defs 3
	._bub_mx		defs 3
	._bub_lock		defs 3
	._bub_shown		defs 3

	._en_cap		defs 3
	._cap_x			defs 3
	._cap_y			defs 3
	._en_ct			defs 3
	._en_st			defs 3

	._bub_ride		defb 255
#endasm

#ifndef CPC
	struct sp_SS *sp_bub [BUBS_MAX];
	struct sp_SS *sp_coco;
	unsigned char *bub_cur [BUBS_MAX];
#endif

unsigned char bub_cool;
unsigned char bi, bj;

// Levels
unsigned char level, level_cont, saved_life;
unsigned char level_scr_ini, level_ini_x, level_ini_y, level_kills;

// Fanty / monococo / cannon
unsigned char mc_phase, cdir;

#ifdef YUN_DARK_LEVEL
	// Dark rooms (NES level 8): lit once their bulb is taken, flashes of lightning
	unsigned char lit [MAX_SCREENS];
	unsigned char scr_dark, lightning;
	#ifndef CPC
		unsigned char dk_ink, dk_tgt;
		unsigned int dk_old;
	#endif
#endif

// Coco (one at a time)
unsigned char coco_on, coco_shown, crx, cry;
signed int coco_x, coco_y, coco_vx, coco_vy;
unsigned int cdist;
unsigned char cda, cdb;

#asm
	// ------------------------------------------------------------------------
	// Helpers

	// for (bi = 0; bi < BUBS_MAX; bi ++) { bub_ct [bi] = 0; bub_lock [bi] = 0xff; }
	// coco_on = 0;
	.yb_reset
		ld  hl, _bub_ct
		ld  de, _bub_lock
		ld  b, BUBS_MAX
	.yb_reset_l
		ld  (hl), 0
		inc hl
		ld  a, 255
		ld  (de), a
		inc de
		djnz yb_reset_l
		xor a
		ld  (_coco_on), a
		ret

	// A = player.facing ? 1 : 0
	.yb_facing
		ld  a, (_player + 22)
		or  a
		ret z
		ld  a, 1
		ret

	// HL = -HL
	.yb_neg_hl
		xor a
		sub l
		ld  l, a
		sbc a, a
		sub h
		ld  h, a
		ret

	// HL = ((int *) HL) [A]
	.yb_tab
		add a, a
		add a, l
		ld  l, a
		jr  nc, yb_tab_1
		inc h
	.yb_tab_1
		ld  a, (hl)
		inc hl
		ld  h, (hl)
		ld  l, a
		ret

	// IX -> en_cap [enit]
	.ye_ix
		ld  a, (_enit)
		ld  e, a
		ld  d, 0
		ld  ix, _en_cap
		add ix, de
		ret

	// en_an_next_frame [enit] = HL
	.ye_frame
		ex  de, hl
		ld  a, (_enit)
		add a, a
		ld  l, a
		ld  h, 0
		ld  bc, _en_an_next_frame
		add hl, bc
		ld  (hl), e
		inc hl
		ld  (hl), d
		ret

	// ------------------------------------------------------------------------
	// Coco

	// coco_x = rdx << 4; coco_y = rdy << 4;
	.ycn_xy
		ld  a, (_rdx)
		ld  l, a
		ld  h, 0
		add hl, hl
		add hl, hl
		add hl, hl
		add hl, hl
		ld  (_coco_x), hl
		ld  a, (_rdy)
		ld  l, a
		ld  h, 0
		add hl, hl
		add hl, hl
		add hl, hl
		add hl, hl
		ld  (_coco_y), hl
		ret

	// HL = (COCO_V * A) / cdist (COCO_V = 86 = 2 * 43 on CPC, 43 on ZX; 43 = 101011b)
	.ycn_vel
		ld  l, a
		ld  h, 0
	#if COCO_V == 86
		add hl, hl
	#endif
		ld  b, h
		ld  c, l
		add hl, hl
		add hl, hl
		add hl, bc
		add hl, hl
		add hl, hl
		add hl, bc
		add hl, hl
		add hl, bc
		ex  de, hl
		ld  hl, (_cdist)
		jp  l_div_u 					// HL = DE / HL

	.ycn_do
		// from (rdx, rdy) towards Yun, NES simplecoco_aimed_new
		// if (coco_on) return;
		ld  a, (_coco_on)
		or  a
		ret nz

		// cda = rdx > gpx ? rdx - gpx : gpx - rdx;
		ld  a, (_gpx)
		ld  c, a
		ld  a, (_rdx)
		sub c
		jr  nc, ycn_1
		neg
	.ycn_1
		ld  (_cda), a

		// cdb = rdy > gpy ? rdy - gpy : gpy - rdy;
		ld  a, (_gpy)
		ld  c, a
		ld  a, (_rdy)
		sub c
		jr  nc, ycn_2
		neg
	.ycn_2
		ld  (_cdb), a

		// rda = cda < cdb ? cda : cdb;
		ld  c, a
		ld  a, (_cda)
		cp  c
		jr  c, ycn_3
		ld  a, c
	.ycn_3
		ld  b, a

		// cdist = cda + cdb - (rda >> 1) - (rda >> 2) + (rda >> 4);
		ld  a, (_cda)
		ld  l, a
		ld  h, 0
		ld  e, c
		ld  d, 0
		add hl, de
		ld  a, b
		srl a
		ld  e, a
		and a
		sbc hl, de
		ld  a, b
		srl a
		srl a
		ld  e, a
		and a
		sbc hl, de
		ld  a, b
		rrca
		rrca
		rrca
		rrca
		and 15
		ld  e, a
		add hl, de
		ld  (_cdist), hl

		// if (cdist == 0) return;
		ld  a, h
		or  l
		ret z

		call ycn_xy

		// coco_vx = (COCO_V * cda) / cdist; if (gpx < rdx) coco_vx = -coco_vx;
		ld  a, (_cda)
		call ycn_vel
		ld  a, (_rdx)
		ld  c, a
		ld  a, (_gpx)
		cp  c
		call c, yb_neg_hl
		ld  (_coco_vx), hl

		// coco_vy = (COCO_V * cdb) / cdist; if (gpy < rdy) coco_vy = -coco_vy;
		ld  a, (_cdb)
		call ycn_vel
		ld  a, (_rdy)
		ld  c, a
		ld  a, (_gpy)
		cp  c
		call c, yb_neg_hl
		ld  (_coco_vy), hl

		// coco_on = 1;
	.ycn_on
		ld  a, 1
		ld  (_coco_on), a
		ret

	.ycs_do
		// from (rdx, rdy), direction rda: 0 left, 1 up, 2 right, 3 down
		// if (coco_on) return;
		ld  a, (_coco_on)
		or  a
		ret nz

		// coco_x = rdx << 4; coco_y = rdy << 4; coco_vx = coco_vy = 0;
		call ycn_xy
		ld  hl, 0
		ld  (_coco_vx), hl
		ld  (_coco_vy), hl

		// switch (rda) { case 0: coco_vx = -COCO_V; break; case 1: coco_vy = -COCO_V; break;
		//                case 2: coco_vx = COCO_V; break; default: coco_vy = COCO_V; }
		ld  a, (_rda)
		ld  hl, 65536 - COCO_V
		ld  de, _coco_vx
		or  a
		jr  z, ycs_set
		ld  de, _coco_vy
		dec a
		jr  z, ycs_set
		ld  hl, COCO_V
		ld  de, _coco_vx
		dec a
		jr  z, ycs_set
		ld  de, _coco_vy
	.ycs_set
		ex  de, hl
		ld  (hl), e
		inc hl
		ld  (hl), d

		// coco_on = 1;
		jr  ycn_on

	// signed int HL in 0 .. DE - 1? Carry set if not (out of the screen)
	.ycm_range
		bit 7, h
		scf
		ret nz
		push hl
		and a
		sbc hl, de
		pop hl
		ccf
		ret

	// A = HL >> 4 (HL < 4096)
	.yb_shr4
		add hl, hl
		add hl, hl
		add hl, hl
		add hl, hl
		ld  a, h
		ret

	.ycm_do
		// if (coco_on == 0) return;
		ld  a, (_coco_on)
		or  a
		ret z

		// coco_x += coco_vx; coco_y += coco_vy;
		// if (coco_x < 0 || coco_x > (232 << 4) || coco_y < 0 || coco_y > (152 << 4)) { coco_on = 0; return; }
		// crx = coco_x >> 4; cry = coco_y >> 4;
		ld  hl, (_coco_x)
		ld  de, (_coco_vx)
		add hl, de
		ld  (_coco_x), hl
		ld  de, 232 * 16 + 1
		call ycm_range
		jr  c, ycm_off
		call yb_shr4
		ld  (_crx), a

		ld  hl, (_coco_y)
		ld  de, (_coco_vy)
		add hl, de
		ld  (_coco_y), hl
		ld  de, 152 * 16 + 1
		call ycm_range
		jr  c, ycm_off
		call yb_shr4
		ld  (_cry), a

		// the coco is 8x8 at (crx + 4, cry + 4) in its cell
		// if (crx + 12 >= gpx + 4 && crx + 4 <= gpx + 11 && cry + 12 >= gpy + 2 && cry + 4 <= gpy + 15)
		ld  a, (_gpx)
		ld  c, a
		ld  a, (_crx)
		add 8
		cp  c
		ret c

		ld  a, (_crx)
		ld  c, a
		ld  a, (_gpx)
		add 7
		cp  c
		ret c

		ld  a, (_gpy)
		ld  c, a
		ld  a, (_cry)
		add 10
		cp  c
		ret c

		ld  a, (_cry)
		ld  c, a
		ld  a, (_gpy)
		add 11
		cp  c
		ret c

		// coco_on = 0;
		xor a
		ld  (_coco_on), a

		// if (player.estado == EST_NORMAL) {
		//     player.drain_amount = 1; player.is_dead = PLAYER_KILLED_BY_ENEM; peta_el_beeper (2); }
		ld  a, (_player + 23)
		or  a
		ret nz
		ld  a, 1
		ld  (_player + 46), a
		ld  a, PLAYER_KILLED_BY_ENEM
		ld  (_player + 36), a
		ld  l, 2
		jp  _peta_el_beeper

	.ycm_off
		xor a
		ld  (_coco_on), a
		ret

	// ------------------------------------------------------------------------
	// Bubbles

	.ybn_do
		// for (bi = 0; bi < BUBS_MAX; bi ++) if (bub_ct [bi] == 0) break;
		// if (bi == BUBS_MAX) return;
		ld  ix, _bub_ct
		ld  b, BUBS_MAX
	.ybn_l
		ld  a, (ix + YB_CT)
		or  a
		jr  z, ybn_f
		inc ix
		djnz ybn_l
		ret

	.ybn_f
		// bub_ct [bi] = BUB_LIFE; bub_lock [bi] = 0xff; bub_fc [bi] = player.facing ? 1 : 0;
		// bub_x [bi] = gpx; bub_y [bi] = gpy;
		// bub_ix [bi] = bub_iy [bi] = bub_state [bi] = bub_mx [bi] = 0;
		ld  (ix + YB_CT), BUB_LIFE
		ld  (ix + YB_LK), 255
		call yb_facing
		ld  (ix + YB_FC), a
		ld  a, (_gpx)
		ld  (ix + YB_X), a
		ld  a, (_gpy)
		ld  (ix + YB_Y), a
		xor a
		ld  (ix + YB_IX), a
		ld  (ix + YB_IY), a
		ld  (ix + YB_ST), a
		ld  (ix + YB_MX), a

		// bub_cool = BUB_COOLDOWN; peta_el_beeper (5);
		ld  a, BUB_COOLDOWN
		ld  (_bub_cool), a
		ld  l, 5
		jp  _peta_el_beeper

	.ybm_do
		// for (bi = 0; bi < BUBS_MAX; bi ++) { ... }
		ld  ix, _bub_ct
		xor a
	.ybm_l
		ld  (_bi), a
		call ybm_one
		inc ix
		ld  a, (_bi)
		inc a
		cp  BUBS_MAX
		jr  c, ybm_l
		ret

	.ybm_one
		// if (bub_ct [bi] == 0) continue;
		ld  a, (ix + YB_CT)
		or  a
		ret z

		// bub_ct [bi] --;
		dec (ix + YB_CT)
		jr  nz, ybm_alive

		// Pop! Free whatever was inside.
		// if (bub_lock [bi] != 0xff) { en_cap [bub_lock [bi]] = 0; bub_lock [bi] = 0xff; }
		ld  a, (ix + YB_LK)
		cp  255
		jr  z, ybm_p1
		ld  e, a
		ld  d, 0
		ld  hl, _en_cap
		add hl, de
		ld  (hl), 0
		ld  (ix + YB_LK), 255
	.ybm_p1
		// if (bub_ride == bi) bub_ride = 0xff;
		ld  a, (_bub_ride)
		ld  c, a
		ld  a, (_bi)
		cp  c
		ret nz
		ld  a, 255
		ld  (_bub_ride), a
		ret

	.ybm_alive
		// Trapped enemies move their bubble (extra_enems_move)
		// if (bub_lock [bi] != 0xff) continue;
		ld  a, (ix + YB_LK)
		inc a
		ret nz

		// Horizontal: decelerating, bounce on screen edges
		// bt = bub_dx [bub_ix [bi]]; if (bub_ix [bi] < BUB_DX_MAX) bub_ix [bi] ++;
		ld  a, (ix + YB_IX)
		ld  e, a
		ld  d, 0
		ld  hl, _bub_dx
		add hl, de
		ld  c, (hl)
		cp  BUB_DX_MAX
		jr  nc, ybm_h1
		inc (ix + YB_IX)
	.ybm_h1
		// bsi = bub_x [bi]; if (bub_fc [bi]) bsi -= bt; else bsi += bt;
		// if (bsi < 0) { bsi = 0; bub_fc [bi] = 0; } else if (bsi > 224) { bsi = 224; bub_fc [bi] = 1; }
		ld  b, (ix + YB_X)
		ld  a, (ix + YB_FC)
		or  a
		ld  a, b
		jr  z, ybm_r
		sub c
		jr  nc, ybm_h2
		xor a
		ld  (ix + YB_FC), a
		jr  ybm_h2
	.ybm_r
		add c
		cp  225
		jr  c, ybm_h2
		ld  a, 224
		ld  (ix + YB_FC), 1
	.ybm_h2
		// bub_mx [bi] = bsi - bub_x [bi]; bub_x [bi] = bsi;
		ld  (ix + YB_X), a
		sub b
		ld  (ix + YB_MX), a

		// Vertical: rise faster and faster, then bob at the top
		ld  b, (ix + YB_Y)
		ld  a, (ix + YB_ST)
		or  a
		jr  nz, ybm_bob

		// bsi -= bub_dy [bub_iy [bi]]; if (bub_iy [bi] < BUB_DY_MAX) bub_iy [bi] ++;
		ld  a, (ix + YB_IY)
		ld  e, a
		ld  d, 0
		ld  hl, _bub_dy
		add hl, de
		ld  c, (hl)
		cp  BUB_DY_MAX
		jr  nc, ybm_v1
		inc (ix + YB_IY)
	.ybm_v1
		// if (bsi < 0) { bsi = 0; bub_state [bi] = 1; bub_iy [bi] = 0; }
		ld  a, b
		sub c
		jr  nc, ybm_vy
		ld  (ix + YB_ST), 1
		ld  (ix + YB_IY), 0
		xor a
		jr  ybm_vy

	.ybm_bob
		// bsi += bub_bob [bub_iy [bi]]; bub_iy [bi] ++;
		// if (bub_iy [bi] == BUB_BOB_MAX) { bub_state [bi] = 0; bub_iy [bi] = 0; }
		ld  a, (ix + YB_IY)
		ld  e, a
		ld  d, 0
		ld  hl, _bub_bob
		add hl, de
		ld  a, (hl)
		add b
		ld  c, a
		inc (ix + YB_IY)
		ld  a, (ix + YB_IY)
		cp  BUB_BOB_MAX
		jr  nz, ybm_b1
		ld  (ix + YB_ST), 0
		ld  (ix + YB_IY), 0
	.ybm_b1
		ld  a, c

	.ybm_vy
		// bub_y [bi] = bsi;
		ld  (ix + YB_Y), a
		ret

	.ybr_do
		// if (bub_ride != 0xff) {
		ld  a, (_bub_ride)
		cp  255
		jr  z, ybr_find

		// bi = bub_ride;
		ld  (_bi), a
		ld  e, a
		ld  d, 0
		ld  ix, _bub_ct
		add ix, de

		// Still on it? Jumping, being stopped by a wall or ceiling drops her.
		// if (bub_ct [bi] == 0 || bub_lock [bi] != 0xff || player.saltando
		//     || gpx + 14 < bub_x [bi] || gpx > bub_x [bi] + 14
		//     || gpy + 20 < bub_y [bi]) { bub_ride = 0xff; return; }
		ld  a, (ix + YB_CT)
		or  a
		jr  z, ybr_drop
		ld  a, (ix + YB_LK)
		inc a
		jr  nz, ybr_drop
		ld  a, (_player + 19)
		or  a
		jr  nz, ybr_drop
		ld  a, (_gpx)
		add 14
		cp  (ix + YB_X)
		jr  c, ybr_drop
		ld  a, (_gpx)
		ld  c, a
		ld  a, (ix + YB_X)
		add 14
		cp  c
		jr  c, ybr_drop
		ld  a, (_gpy)
		add 20
		cp  (ix + YB_Y)
		jr  nc, ybr_carry
	.ybr_drop
		ld  a, 255
		ld  (_bub_ride), a
		ret

	.ybr_find
		// Land on a free bubble (only while falling or standing)
		// if (player.vy < 0) return;
		ld  a, (_player + 9)
		bit 7, a
		ret nz

		// for (bi = 0; bi < BUBS_MAX; bi ++) {
		//     if (bub_ct [bi] && bub_lock [bi] == 0xff && bub_y [bi] > 16
		//         && gpx + 12 >= bub_x [bi] && gpx <= bub_x [bi] + 12
		//         && gpy + 16 >= bub_y [bi] && gpy + 6 <= bub_y [bi]) break; }
		// if (bi == BUBS_MAX) return;
		// (NES uses a 5 px window at 1-2 px/frame; here bubbles rise up to 7 px/frame)
		ld  ix, _bub_ct
		xor a
	.ybr_l
		ld  (_bi), a
		ld  a, (ix + YB_CT)
		or  a
		jr  z, ybr_n
		ld  a, (ix + YB_LK)
		inc a
		jr  nz, ybr_n
		ld  a, (ix + YB_Y)
		cp  17
		jr  c, ybr_n
		ld  a, (_gpx)
		add 12
		cp  (ix + YB_X)
		jr  c, ybr_n
		ld  a, (_gpx)
		ld  c, a
		ld  a, (ix + YB_X)
		add 12
		cp  c
		jr  c, ybr_n
		ld  a, (_gpy)
		add 16
		cp  (ix + YB_Y)
		jr  c, ybr_n
		ld  a, (_gpy)
		add 6
		ld  c, a
		ld  a, (ix + YB_Y)
		cp  c
		jr  nc, ybr_got
	.ybr_n
		inc ix
		ld  a, (_bi)
		inc a
		cp  BUBS_MAX
		jr  c, ybr_l
		ret

	.ybr_got
		// bub_ride = bi;
		ld  a, (_bi)
		ld  (_bub_ride), a

	.ybr_carry
		// Carry her. This runs at the start of move (): the engine adds gravity to vy, then
		// applies vy and ptgmx with collisions, and flicks the screen up when gpy
		// hits 0 with vy < 0. gotten lets her jump off.
		// bsi = bub_y [bi] - 16 - gpy; player.vy = bsi * 64 - PLAYER_G;
		ld  a, (_gpy)
		ld  c, a
		ld  b, 0
		ld  a, (ix + YB_Y)
		ld  l, a
		ld  h, 0
		and a
		sbc hl, bc
		ld  bc, 16
		and a
		sbc hl, bc
		add hl, hl
		add hl, hl
		add hl, hl
		add hl, hl
		add hl, hl
		add hl, hl
		ld  bc, PLAYER_G
		and a
		sbc hl, bc
		ld  (_player + 8), hl

		// ptgmx = bub_mx [bi] * 64;
		ld  a, (ix + YB_MX)
		ld  l, a
		add a, a
		sbc a, a
		ld  h, a
		add hl, hl
		add hl, hl
		add hl, hl
		add hl, hl
		add hl, hl
		add hl, hl
		ld  (_ptgmx), hl

		// player.gotten = 1;
		ld  a, 1
		ld  (_player + 25), a
		ret
#endasm

// ----------------------------------------------------------------------------
// Sprites

#ifdef CPC
// Bubbles and the coco are sp_sw [SP_CUSTOM_BASE + n]; bubbles byte-aligned
void custom_sprites_create (void) {
	// for (bi = 0; bi < BUBS_MAX; bi ++) { bub_ct [bi] = 0; bub_lock [bi] = 0xff; } coco_on = 0;
	#asm
			jp  yb_reset
	#endasm
}

void custom_sprites_render (void) {
	#asm
			// for (bi = 0; bi < BUBS_MAX; bi ++) {
			ld  ix, _bub_ct
			ld  iy, BASE_SPRITES + 16 * SP_CUSTOM_BASE
			ld  b, BUBS_MAX
		.ycr_l
			// bt = SP_CUSTOM_BASE + bi;
			// if (bub_ct [bi] && (bub_ct [bi] > BUB_BLINK || half_life)) {
			ld  hl, _spr_empty
			ld  a, (ix + YB_CT)
			or  a
			jr  z, ycr_set
			cp  BUB_BLINK + 1
			jr  nc, ycr_on
			ld  a, (_half_life)
			or  a
			jr  z, ycr_set
		.ycr_on
			// sp_sw [bt].cx = ((bub_x [bi] + VIEWPORT_X * 8) >> 1) & 0xfe;
			ld  a, (ix + YB_X)
			add VIEWPORT_X * 8
			srl a
			and 0xfe
			ld  (iy + 8), a

			// sp_sw [bt].cy = bub_y [bi] + VIEWPORT_Y * 8;
			ld  a, (ix + YB_Y)
			add VIEWPORT_Y * 8
			ld  (iy + 9), a

			// sp_sw [bt].sp0 = (int) ((maincounter & YA_BUB) ? sprite_10_a : sprite_9_a);
			ld  hl, YS9
			ld  a, (_maincounter)
			and YA_BUB
			jr  z, ycr_set
			ld  hl, YS10

			// } else sp_sw [bt].sp0 = (int) spr_empty;
		.ycr_set
			ld  (iy + 0), l
			ld  (iy + 1), h
			ld  de, 16
			add iy, de
			inc ix
			djnz ycr_l

			// bt = SP_CUSTOM_BASE + BUBS_MAX;
			// if (coco_on) {
			ld  hl, _spr_empty
			ld  a, (_coco_on)
			or  a
			jr  z, ycr_cs

			// sp_sw [bt].cx = (crx + VIEWPORT_X * 8) >> 1;
			ld  a, (_crx)
			add VIEWPORT_X * 8
			srl a
			ld  (iy + 8), a

			// sp_sw [bt].cy = cry + VIEWPORT_Y * 8;
			ld  a, (_cry)
			add VIEWPORT_Y * 8
			ld  (iy + 9), a

			// sp_sw [bt].sp0 = (int) sprite_21_a;
			ld  hl, YS21

			// } else sp_sw [bt].sp0 = (int) spr_empty;
		.ycr_cs
			ld  (iy + 0), l
			ld  (iy + 1), h
	#endasm
}

void custom_sprites_hide (void) {
	// for (bi = 0; bi <= BUBS_MAX; bi ++) sp_sw [SP_CUSTOM_BASE + bi].sp0 = (int) spr_empty;
	#asm
			ld  hl, BASE_SPRITES + 16 * SP_CUSTOM_BASE
			ld  de, _spr_empty
			ld  bc, 16
			ld  a, BUBS_MAX + 1
		.ych_l
			ld  (hl), e
			inc hl
			ld  (hl), d
			dec hl
			add hl, bc
			dec a
			jr  nz, ych_l
	#endasm
}
#else
#asm
	// HL = sp_CreateSpr (NO_MASKS, 3, HL, 2); sp_AddColSpr (HL, DE) twice
	// (the second one is a dummy, nulled by fix_sprites)
	.yz_mk
		push de
		ld  bc, NO_MASKS
		push bc
		ld  bc, 3
		push bc
		push hl
		ld  bc, 2
		push bc
		call sp_CreateSpr
		pop bc
		pop bc
		pop bc
		pop bc
		pop de

		push hl
		push de
		push hl
		push de
		call sp_AddColSpr
		pop bc
		pop bc
		pop de
		pop hl

		push hl
		push hl
		push de
		call sp_AddColSpr
		pop bc
		pop bc
		pop hl

		push hl
		ld  b, 6
		call _fix_sprites
		pop hl
		ret

	// Move sprite IX to (A, E) px in the viewport, graphic offset BC
	.yz_move
		ld  d, a
		ld  a, e
		rrca
		rrca
		rrca
		and 31
		add VIEWPORT_Y
		ld  h, a
		ld  a, d
		rrca
		rrca
		rrca
		and 31
		add VIEWPORT_X
		ld  l, a
		ld  a, d
		and 7
		ld  d, a
		ld  a, e
		and 7
		ld  e, a
		ld  iy, vpClipStruct
		jp  SPMoveSprAbs

	// Move sprite IX out of the way
	.yz_hide
		ld  iy, vpClipStruct
		ld  bc, 0
		ld  hl, 0xfefe
		ld  de, 0
		jp  SPMoveSprAbs

	// IX = sp_bub [bi]; DE = bi
	.yz_spr
		ld  a, (_bi)
		ld  e, a
		ld  d, 0
		ld  hl, _sp_bub
		add hl, de
		add hl, de
		ld  a, (hl)
		inc hl
		ld  h, (hl)
		ld  l, a
		push hl
		pop ix
		ret
#endasm

void custom_sprites_create (void) {
	#asm
			// for (bi = 0; bi < BUBS_MAX; bi ++) {
			xor a
		.yzc_l
			ld  (_bi), a

			// sp_bub [bi] = sp_CreateSpr (NO_MASKS, 3, sprite_9_a, 2);
			// sp_AddColSpr (sp_bub [bi], sprite_9_b);
			// sp_AddColSpr (sp_bub [bi], sprite_9_b);	// dummy, nulled by fix_sprites
			// fix_sprites (sp_bub [bi])
			ld  hl, YS9
			ld  de, _sprite_9_b
			call yz_mk
			ld  b, h
			ld  c, l
			ld  a, (_bi)
			add a, a
			ld  e, a
			ld  d, 0
			ld  hl, _sp_bub
			add hl, de
			ld  (hl), c
			inc hl
			ld  (hl), b

			// bub_cur [bi] = sprite_9_a;
			ld  hl, _bub_cur
			add hl, de
			ld  bc, YS9
			ld  (hl), c
			inc hl
			ld  (hl), b

			// bub_shown [bi] = 1;		// so the first render moves it out of the way
			ld  a, (_bi)
			ld  e, a
			ld  d, 0
			ld  hl, _bub_shown
			add hl, de
			ld  (hl), 1

			ld  a, (_bi)
			inc a
			cp  BUBS_MAX
			jr  c, yzc_l

			// sp_coco = sp_CreateSpr (NO_MASKS, 3, sprite_21_a, 2); (+ 2 x sp_AddColSpr, fix_sprites)
			ld  hl, YS21
			ld  de, _sprite_21_b
			call yz_mk
			ld  (_sp_coco), hl

			// coco_shown = 1;
			ld  a, 1
			ld  (_coco_shown), a

			// bub_ct [bi] = 0; bub_lock [bi] = 0xff; coco_on = 0;
			jp  yb_reset
	#endasm
}

void custom_sprites_render (void) {
	#asm
		#ifdef YUN_DARK_LEVEL
			// the dark room's attributes (fading, the spot around Yun)
			call ydk_frame
		#endif

			// for (bi = 0; bi < BUBS_MAX; bi ++) {
			xor a
		.yzr_l
			ld  (_bi), a
			ld  e, a
			ld  d, 0

			// if (bub_ct [bi] && (bub_ct [bi] > BUB_BLINK || half_life)) {
			ld  hl, _bub_ct
			add hl, de
			ld  a, (hl)
			or  a
			jr  z, yzr_off
			cp  BUB_BLINK + 1
			jr  nc, yzr_on
			ld  a, (_half_life)
			or  a
			jr  z, yzr_off
		.yzr_on
			// bub_next = (maincounter & YA_BUB) ? sprite_10_a : sprite_9_a;
			ld  bc, YS9
			ld  a, (_maincounter)
			and YA_BUB
			jr  z, yzr_1
			ld  bc, YS10
		.yzr_1
			// bub_off = (int) bub_next - (int) bub_cur [bi]; bub_cur [bi] = bub_next;
			ld  hl, _bub_cur
			add hl, de
			add hl, de
			ld  e, (hl)
			ld  (hl), c
			inc hl
			ld  d, (hl)
			ld  (hl), b
			ld  h, b
			ld  l, c
			and a
			sbc hl, de
			push hl

			// bub_shown [bi] = 1;
			call yz_spr
			ld  hl, _bub_shown
			add hl, de
			ld  (hl), 1

			// SPMoveSprAbs at (bub_x [bi], bub_y [bi])
			ld  hl, _bub_y
			add hl, de
			ld  c, (hl)
			ld  hl, _bub_x
			add hl, de
			ld  a, (hl)
			ld  e, c
			pop bc
			call yz_move
			jr  yzr_next

			// } else if (bub_shown [bi]) { (out of the way); bub_shown [bi] = 0; }
		.yzr_off
			ld  hl, _bub_shown
			add hl, de
			ld  a, (hl)
			or  a
			jr  z, yzr_next
			ld  (hl), 0
			call yz_spr
			call yz_hide

		.yzr_next
			ld  a, (_bi)
			inc a
			cp  BUBS_MAX
			jr  c, yzr_l

			// if (coco_on) { (move it to crx, cry); coco_shown = 1; }
			ld  ix, (_sp_coco)
			ld  a, (_coco_on)
			or  a
			jr  z, yzr_coff
			ld  a, 1
			ld  (_coco_shown), a
			ld  bc, 0
			ld  a, (_cry)
			ld  e, a
			ld  a, (_crx)
			jp  yz_move

			// else if (coco_shown) { (out of the way); coco_shown = 0; }
		.yzr_coff
			ld  a, (_coco_shown)
			or  a
			ret z
			xor a
			ld  (_coco_shown), a
			jp  yz_hide
	#endasm
}

void custom_sprites_hide (void) {
	#asm
			// for (bi = 0; bi < BUBS_MAX; bi ++) { (out of the way); bub_shown [bi] = 0; }
			xor a
		.yzh_l
			ld  (_bi), a
			call yz_spr
			ld  hl, _bub_shown
			add hl, de
			ld  (hl), 0
			call yz_hide
			ld  a, (_bi)
			inc a
			cp  BUBS_MAX
			jr  c, yzh_l

			// (the coco out of the way); coco_shown = 0;
			xor a
			ld  (_coco_shown), a
			ld  ix, (_sp_coco)
			jp  yz_hide
	#endasm
}
#endif

void before_vsync (void) {
}

// ----------------------------------------------------------------------------
// Hooks

void custom_game_start (void) {
	#asm
			// for (bi = 0; bi < BUBS_MAX; bi ++) { bub_ct [bi] = 0; bub_lock [bi] = 0xff; }
			// coco_on = 0;
			call yb_reset

			// bub_ride = 0xff; bub_cool = 0;
			xor a
			ld  (_bub_cool), a
			dec a
			ld  (_bub_ride), a

			// if (level_cont) { player.life = saved_life; level_cont = 0; }
			ld  a, (_level_cont)
			or  a
			ret z
			ld  a, (_saved_life)
			ld  l, a
			ld  h, 0
			ld  (_player + 29), hl
			xor a
			ld  (_level_cont), a
	#endasm
}

// ----------------------------------------------------------------------------
// Levels

void custom_level_setup (void) {
	#asm
			// if (level_cont == 0) level = 0; (or YUN_DEBUG_LEVEL)
			ld  a, (_level_cont)
			or  a
			jr  nz, yls_1
		#ifdef YUN_DEBUG_LEVEL
			ld  a, YUN_DEBUG_LEVEL 		// (YUNFLAGS=-DYUN_DEBUG_LEVEL=n: start at level n of the load)
		#endif
			ld  (_level), a
		.yls_1

		#ifndef CPC
			// DIRECT_TO_PLAY: the title is the frame, and cortina () wiped it at the
			// end of the last level: if (level_cont) unpack_screen (s_title);
			ld  a, (_level_cont)
			or  a
			jr  z, yls_2
			ld  hl, _s_title
			call _unpack_screen
		.yls_2
		#endif

			// the level area: map + locks, enemies + hotspots, tile map (yun_level.h)
			// dzx0_standard (lvl_area [level], level_area)
			ld  a, (_level)
			ld  hl, _lvl_area
			call yb_tab
			ld  de, _level_area
			call dzx0_standard

		#ifndef CPC
			// ZX (75% of NES time at 25 Hz: half the CPC per-frame speeds): linear
			// enemies and platforms (types 1-4) at half speed, 1 stays 1. Their paths
			// are whole tiles, so the speeds still divide them.
			// for each malote: if (t >= 1 && t <= 4) { mx = half (mx); my = half (my); }
			ld  hl, _malotes + 6 		// mx; my at + 1, t at + 2
			ld  b, 3 * MAX_SCREENS
		.yls_sp
			inc hl
			inc hl
			ld  a, (hl)
			dec hl
			dec hl
			dec a
			cp  4
			jr  nc, yls_sp_n
			ld  a, (hl)
			call yls_half
			ld  (hl), a
			inc hl
			ld  a, (hl)
			call yls_half
			ld  (hl), a
			dec hl
		.yls_sp_n
			ld  de, 9
			add hl, de
			djnz yls_sp
			jr  yls_sp_done

			// A = A / 2 (signed), but 1 stays 1
		.yls_half
			or  a
			ret z
			sra a
			ret nz
			inc a
			ret
		.yls_sp_done
		#endif

			// the level's enemy sprite cells
			// dzx0_standard (lvl_spr [level], SPR_VAR_BASE)
			ld  a, (_level)
			ld  hl, _lvl_spr
			call yb_tab
			ld  de, SPR_VAR_BASE
			call dzx0_standard

			// tile behaviours: memcpy (comportamiento_tiles, lvl_behs + (level << 4), 16)
			ld  a, (_level)
			add a, a
			add a, a
			add a, a
			add a, a
			ld  e, a
			ld  d, 0
			ld  hl, _lvl_behs
			add hl, de
			ld  de, _comportamiento_tiles
			ld  bc, 16
			ldir

			ld  a, (_level)
			ld  e, a
			ld  d, 0

		#ifdef VARIABLE_MAP_W
			// map_w = lvl_map_w [level];
			ld  hl, _lvl_map_w
			add hl, de
			ld  a, (hl)
			ld  (_map_w), a
		#endif

		#ifdef YUN_DARK_LEVEL
			// for (bi = 0; bi < MAX_SCREENS; bi ++) lit [bi] = 0;
			// lightning = 0; scr_dark = 0;
			ld  hl, _lit
			ld  b, 0 + MAX_SCREENS
			xor a
		.yls_lit
			ld  (hl), a
			inc hl
			djnz yls_lit
			ld  (_lightning), a
			ld  (_scr_dark), a
		#ifndef CPC
			// Sprite Pack paints attributes (SPAttrWrite = ld (de), a)
			ld  a, 0x12
			ld  (SPAttrWrite), a
		#endif
		#endif

			// level_scr_ini = lvl_scr_ini [level]; level_ini_x = lvl_ini_x [level];
			// level_ini_y = lvl_ini_y [level]; level_kills = lvl_kills [level];
			ld  hl, _lvl_scr_ini
			add hl, de
			ld  a, (hl)
			ld  (_level_scr_ini), a
			ld  hl, _lvl_ini_x
			add hl, de
			ld  a, (hl)
			ld  (_level_ini_x), a
			ld  hl, _lvl_ini_y
			add hl, de
			ld  a, (hl)
			ld  (_level_ini_y), a
			ld  hl, _lvl_kills
			add hl, de
			ld  a, (hl)
			ld  (_level_kills), a

		#ifdef CPC
			// split palette: the level's background inks, the sprite inks
			// cd_pal_bg = lvl_pal [level]; cd_pal_spr = spr_pal; cd_set_palette ();
			ld  a, (_level)
			ld  hl, _lvl_pal
			call yb_tab
			ld  (_cd_pal_bg), hl
			ld  hl, _spr_pal
			ld  (_cd_pal_spr), hl
			call _cd_set_palette
		#endif

			// kills to win, after the '/' of the frame
			// draw_2_digits (HUD_TOTAL_X, 0, level_kills);
			ld  a, HUD_TOTAL_X
			ld  (__x), a
			xor a
			ld  (__y), a
			ld  a, (_level_kills)
			jp  draw_2_digits_shortcut
	#endasm
}

unsigned char custom_level_won (void) {
	#asm
			// if (level + 1 < YUN_LEVELS) { level ++; level_cont = 1; saved_life = player.life; return 1; }
			ld  a, (_level)
			inc a
			cp  YUN_LEVELS
			jr  nc, ylw_end
			ld  (_level), a
			ld  a, 1
			ld  (_level_cont), a
			ld  a, (_player + 29)
			ld  (_saved_life), a
			ld  hl, 1
			ret

			// level_cont = 0; return 0;
		.ylw_end
			xor a
			ld  (_level_cont), a
			ld  hl, 0
	#endasm
}

#ifdef YUN_DARK_LEVEL
#asm
	#ifndef CPC
		// ZX dark rooms. The game is linked with lib/splib2a.lib, a Sprite Pack
		// whose updater can be told not to write attributes (SPAttrWrite: the
		// opcode of its only attribute write; $00 = nop, $12 = ld (de),a). In a
		// dark room the attributes are ours: the viewport in INK dk_ink on black
		// (0 = nothing shows), the 3x3 cells around Yun in DARK_SPOT_ATTR.
		// Lightning: dk_ink goes up to 7 one step per frame, stays there a
		// moment (lightning) and comes back down to 0. No tile is redrawn.
		LIB SPAttrWrite

		#ifdef PLAYER_TALL
			#define YDK_H 4 			// rows of the lit spot around Yun
		#else
			#define YDK_H 3
		#endif

		// Fill the viewport's attributes with A, forget the spot
		.ydk_fill
			ld  hl, 0x5800 + 32 * VIEWPORT_Y + VIEWPORT_X
			ld  de, 2
			ld  c, 20
		.ydk_fill_y
			ld  b, 30
		.ydk_fill_x
			ld  (hl), a
			inc hl
			djnz ydk_fill_x
			add hl, de
			dec c
			jr  nz, ydk_fill_y
			ld  hl, 0
			ld  (_dk_old), hl
			ret

		// 3 x YDK_H attributes at HL = A
		.ydk_block
			ld  de, 30
			ld  b, YDK_H
		.ydk_block_l
			ld  (hl), a
			inc hl
			ld  (hl), a
			inc hl
			ld  (hl), a
			add hl, de
			djnz ydk_block_l
			ret

		// Every frame, from custom_sprites_render (Yun already moved)
		.ydk_frame
			// if (scr_dark == 0) return;
			ld  a, (_scr_dark)
			or  a
			ret z

			// if (dk_ink != dk_tgt) { dk_ink += dk_ink < dk_tgt ? 1 : -1; (fill the viewport with dk_ink) }
			ld  a, (_dk_tgt)
			ld  c, a
			ld  a, (_dk_ink)
			cp  c
			jr  z, ydk_spot
			jr  c, ydk_up
			dec a
			jr  ydk_set
		.ydk_up
			inc a
		.ydk_set
			ld  (_dk_ink), a
			call ydk_fill

		.ydk_spot
			// the old spot back to dk_ink
			ld  hl, (_dk_old)
			ld  a, h
			or  l
			jr  z, ydk_new
			ld  a, (_dk_ink)
			call ydk_block

		.ydk_new
			// the cells Yun may be on: 3 x YDK_H from (gpx >> 3, (gpy >> 3) - YDK_H + 3), inside the viewport
			ld  a, (_gpx)
			rrca
			rrca
			rrca
			and 31
			cp  28
			jr  c, ydk_n1
			ld  a, 27
		.ydk_n1
			add VIEWPORT_X
			ld  e, a
			ld  a, (_gpy)
			rrca
			rrca
			rrca
			and 31
		#ifdef PLAYER_TALL
			// 16x24: one row above her box
			or  a
			jr  z, ydk_n0
			dec a
		.ydk_n0
		#endif
			cp  21 - YDK_H
			jr  c, ydk_n2
			ld  a, 20 - YDK_H
		.ydk_n2
			add VIEWPORT_Y
			ld  l, a
			ld  h, 0
			add hl, hl
			add hl, hl
			add hl, hl
			add hl, hl
			add hl, hl
			ld  d, 0x58
			add hl, de
			ld  (_dk_old), hl
			ld  a, DARK_SPOT_ATTR
			jp  ydk_block
	#endif

	.yds_do
		// dark_set (A): ZX: Sprite Pack stops / goes back to painting the attributes;
		// CPC: the background inks (A = 2: lightning)
		// scr_dark = d;
		ld  (_scr_dark), a
	#ifdef CPC
		// cd_pal_bg = d ? (d == 2 ? light_pal : dark_pal) : lvl_pal [level]; cd_set_palette ();
		or  a
		jr  nz, yds_1
		ld  a, (_level)
		ld  hl, _lvl_pal
		call yb_tab
		jr  yds_set
	.yds_1
		ld  hl, _dark_pal
		cp  2
		jr  nz, yds_set
		ld  hl, _light_pal
	.yds_set
		ld  (_cd_pal_bg), hl
		jp  _cd_set_palette
	#else
		or  a
		jr  z, yds_lit

		// Dark: SPAttrWrite = nop; dk_ink = dk_tgt = lightning = 0; the viewport black
		xor a
		ld  (SPAttrWrite), a
		ld  (_dk_ink), a
		ld  (_dk_tgt), a
		ld  (_lightning), a
		jp  ydk_fill

	.yds_lit
		// Lit: SPAttrWrite = ld (de), a; invalidate the viewport, so the next
		// update paints every attribute again
		ld  a, 0x12
		ld  (SPAttrWrite), a
		ld  b, VIEWPORT_Y
		ld  c, VIEWPORT_X
		ld  d, VIEWPORT_Y + 19
		ld  e, VIEWPORT_X + 29
		ld  iy, fsClipStruct
		jp  SPInvalidate
	#endif

	// A = (level == YUN_DARK_LEVEL && lit [n_pant] == 0)
	.ydk_now
		ld  a, (_level)
		cp  YUN_DARK_LEVEL
		jr  nz, ydk_no
		ld  a, (_n_pant)
		ld  e, a
		ld  d, 0
		ld  hl, _lit
		add hl, de
		ld  a, (hl)
		or  a
		jr  nz, ydk_no
		inc a
		ret
	.ydk_no
		xor a
		ret
#endasm
#endif

void custom_screen_enter (void) {
	#asm
		#ifdef YUN_DARK_LEVEL
			// lightning = 0;
			// rda = (level == YUN_DARK_LEVEL && lit [n_pant] == 0);
			// if (rda != scr_dark) dark_set (rda);
			xor a
			ld  (_lightning), a
			call ydk_now
		#ifndef CPC
			// (ZX: a dark room always starts black, without lightning)
			or  a
			jr  nz, yse_dark
		#endif
			ld  hl, _scr_dark
			cp  (hl)
			jr  z, yse_dark_done
		.yse_dark
			call yds_do
		.yse_dark_done
		#endif

			// Bubbles burst and trapped enemies are reset...
			// for (bi = 0; bi < BUBS_MAX; bi ++) { bub_ct [bi] = 0; bub_lock [bi] = 0xff; }
			// for (bi = 0; bi < MAX_ENEMS; bi ++) en_cap [bi] = 0;
			// coco_on = 0;
			call yb_reset
			ld  hl, _en_cap
			ld  b, MAX_ENEMS
		.yse_cap
			ld  (hl), 0
			inc hl
			djnz yse_cap

			// ...unless Yun rode a bubble here: give her a new one (NES does this).
			// if (bub_ride != 0xff) {
			ld  a, (_bub_ride)
			inc a
			ret z

			// if (gpy >= 16) { gpy -= 16; player.y = gpy * 64; }
			ld  a, (_gpy)
			sub 16
			jr  c, yse_1
			ld  (_gpy), a
			ld  l, a
			ld  h, 0
			add hl, hl
			add hl, hl
			add hl, hl
			add hl, hl
			add hl, hl
			add hl, hl
			ld  (_player + 2), hl
		.yse_1
			// bub_ct [0] = BUB_LIFE; bub_x [0] = gpx; bub_y [0] = gpy + 16;
			// bub_ix [0] = BUB_DX_MAX; bub_iy [0] = BUB_DY_MAX; bub_state [0] = 0;
			// bub_fc [0] = player.facing ? 1 : 0; bub_mx [0] = 0; bub_ride = 0;
			ld  ix, _bub_ct
			ld  (ix + YB_CT), BUB_LIFE
			ld  a, (_gpx)
			ld  (ix + YB_X), a
			ld  a, (_gpy)
			add 16
			ld  (ix + YB_Y), a
			ld  (ix + YB_IX), BUB_DX_MAX
			ld  (ix + YB_IY), BUB_DY_MAX
			call yb_facing
			ld  (ix + YB_FC), a
			xor a
			ld  (ix + YB_ST), a
			ld  (ix + YB_MX), a
			ld  (_bub_ride), a
	#endasm
}

void custom_pre_move (void) {
	#asm
			// coco_move (); bubbles_move ();
			call ycm_do
			call ybm_do

			// if (bub_cool) bub_cool --;
			ld  a, (_bub_cool)
			or  a
			jr  z, ypm_1
			dec a
			ld  (_bub_cool), a
		.ypm_1
			// if ((pad_this_frame & sp_FIRE) == 0 && bub_cool == 0) bubble_new ();
			jr  nz, ypm_2
			ld  a, (_pad_this_frame)
			and sp_FIRE
			call z, ybn_do
		.ypm_2

			// bubble_ride ();
			call ybr_do

			// Conveyors (NES CONVEYOR_BIT 32, the evil bit means rightwards)
			// if (player.possee) {
			ld  a, (_player + 26)
			or  a
			jp  z, ypm_conv_done

			// cda = attr ((gpx + 4) >> 4, (gpy + 16) >> 4);
			ld  a, (_gpx)
			add 4
			call ypm_x
			ld  a, (_gpy)
			add 16
			call ypm_attr
			ld  (_cda), a

			// cdb = attr ((gpx + 11) >> 4, (gpy + 16) >> 4);
			ld  a, (_gpx)
			add 11
			call ypm_x
			ld  a, (_gpy)
			add 16
			call ypm_attr
			ld  (_cdb), a

			// if ((cda | cdb) & 32) {
			ld  c, a
			ld  a, (_cda)
			or  c
			and 32
			jp  z, ypm_conv_done

			// if ((cda & 32) == 0) cda = cdb;
			ld  a, (_cda)
			bit 5, a
			jr  nz, ypm_c1
			ld  a, c
		.ypm_c1
			// if (cda & 1) {
			rrca
			jr  nc, ypm_left

			// if (gpx < 224 && (attr ((gpx + 16) >> 4, (gpy + 8) >> 4) & 8) == 0) player.x += CONVEYOR_V;
			ld  a, (_gpx)
			cp  224
			jp  nc, ypm_conv_done
			add 16
			call ypm_x
			ld  a, (_gpy)
			add 8
			call ypm_attr
			and 8
			jp  nz, ypm_conv_done
			ld  hl, (_player)
			ld  de, CONVEYOR_V
			add hl, de
			ld  (_player), hl
			jp  ypm_conv_done

		.ypm_left
			// } else { if (gpx > 0 && (attr ((gpx - 1) >> 4, (gpy + 8) >> 4) & 8) == 0) player.x -= CONVEYOR_V; }
			ld  a, (_gpx)
			or  a
			jp  z, ypm_conv_done
			dec a
			call ypm_x
			ld  a, (_gpy)
			add 8
			call ypm_attr
			and 8
			jp  nz, ypm_conv_done
			ld  hl, (_player)
			ld  de, CONVEYOR_V
			and a
			sbc hl, de
			ld  (_player), hl

		.ypm_conv_done

		#ifdef YUN_DARK_LEVEL
			// Lightning in the dark rooms
			// if (level == YUN_DARK_LEVEL && lit [n_pant] == 0) {
			call ydk_now
			or  a
			ret z

		#ifndef CPC
			// ZX (ydk_frame does the fading):
			// if (dk_tgt) { if (dk_ink == 7 && -- lightning == 0) dk_tgt = 0; }
			ld  a, (_dk_tgt)
			or  a
			jr  z, ypm_z1
			ld  a, (_dk_ink)
			cp  7
			ret nz
			ld  hl, _lightning
			dec (hl)
			ret nz
			xor a
			ld  (_dk_tgt), a
			ret

			// else if (dk_ink == 0 && (rand () & DARK_RAND) == 1) { dk_tgt = 7; lightning = DARK_FLASH; peta_el_beeper (5); }
		.ypm_z1
			ld  a, (_dk_ink)
			or  a
			ret nz
			call _rand
			ld  a, l
			and DARK_RAND
			cp  1
			ret nz
			ld  a, 7
			ld  (_dk_tgt), a
			ld  a, DARK_FLASH
			ld  (_lightning), a
			ld  l, 5
			jp  _peta_el_beeper
		#else
			// if (lightning) { lightning --; if (lightning == 0) dark_set (1); }
			ld  a, (_lightning)
			or  a
			jr  z, ypm_l1
			dec a
			ld  (_lightning), a
			ret nz
			inc a
			jp  yds_do

			// else if ((rand () & DARK_RAND) == 1) { lightning = 3; dark_set (2); peta_el_beeper (5); }
		.ypm_l1
			call _rand
			ld  a, l
			and DARK_RAND
			cp  1
			ret nz
			ld  a, 3
			ld  (_lightning), a
			dec a
			call yds_do
			ld  l, 5
			jp  _peta_el_beeper
		#endif
		#else
			ret
		#endif

			// C = A >> 4
		.ypm_x
			rrca
			rrca
			rrca
			rrca
			and 15
			ld  c, a
			ret

			// A = attr (C, A >> 4)
		.ypm_attr
			rrca
			rrca
			rrca
			rrca
			and 15
			call _attr_2
			ld  a, l
			ret
	#endasm
}

// ----------------------------------------------------------------------------
// Enemies

#ifdef ENABLE_CUSTOM_ENEMS

void extra_enems_init (void) {
	#asm
			// en_ct [enit] = en_st [enit] = 0;
			call ye_ix
			xor a
			ld  (ix + YE_CT), a
			ld  (ix + YE_ST), a

			// switch (_en_t) {
			ld  a, (__en_t)

			// case 7: (pezon: waits at (x1, y1)) _en_x = _en_x1; _en_y = _en_y1; en_ct [enit] = PZ_WAIT; break;
			cp  7
			jr  nz, yei_1
			ld  (ix + YE_CT), PZ_WAIT
			jr  yei_x1y1
		.yei_1
			// case 9: (gyrosaw: from (x1, y1)) _en_x = _en_x1; _en_y = _en_y1; break;
			cp  9
			jr  nz, yei_2
		.yei_x1y1
			ld  a, (__en_x1)
			ld  (__en_x), a
			ld  a, (__en_y1)
			ld  (__en_y), a
			ret
		.yei_2
			// case 10: (cannon: first shot after a full wait) en_ct [enit] = cannon_wait (); break;
			cp  10
			ret nz
			call ycw_do
			ld  (ix + YE_CT), a
			ret

		.ycw_do
			// cannon_wait: seconds in the path's speed (mx or my), direction in its sign
			// rda = _en_mx ? _en_mx : _en_my; if (rda & 0x80) rda = -rda; return rda * CANNON_SECOND;
			ld  a, (__en_mx)
			or  a
			jr  nz, ycw_1
			ld  a, (__en_my)
		.ycw_1
			bit 7, a
			jr  z, ycw_2
			neg
		.ycw_2
		#if CANNON_SECOND == 16
			add a, a
			add a, a
			add a, a
			add a, a
		#else
			// CANNON_SECOND = 33: A * 32 + A
			ld  c, a
			add a, a
			add a, a
			add a, a
			add a, a
			add a, a
			add c
		#endif
			ret

		.ypz_do
			// NES precalc pezon: wait at y1, jump (pz_y), back to y1
			// if (en_st [enit] == 0) { if (en_ct [enit]) en_ct [enit] --; else en_st [enit] = 1; }
			ld  a, (ix + YE_ST)
			or  a
			jr  nz, ypz_jump
			ld  a, (ix + YE_CT)
			or  a
			jr  z, ypz_go
			dec (ix + YE_CT)
			jr  ypz_fr
		.ypz_go
			ld  (ix + YE_ST), 1
			jr  ypz_fr

		.ypz_jump
			// rda = en_st [enit] - 1; rdb = -pz_y [rda];
			dec a
			ld  c, a
			ld  e, a
			ld  d, 0
			ld  hl, _pz_y
			add hl, de
			ld  a, (hl)
			neg
			ld  b, a

			// _en_y = _en_y1 > rdb ? _en_y1 - rdb : 0;
			ld  a, (__en_y1)
			sub b
			jr  nc, ypz_1
			xor a
		.ypz_1
			ld  (__en_y), a

			// if (rda == PZ_N - 1) { en_st [enit] = 0; en_ct [enit] = PZ_WAIT; _en_y = _en_y1; }
			ld  a, c
			cp  PZ_N - 1
			jr  nz, ypz_inc
			ld  (ix + YE_ST), 0
			ld  (ix + YE_CT), PZ_WAIT
			ld  a, (__en_y1)
			ld  (__en_y), a
			jr  ypz_fr

			// else en_st [enit] ++;
		.ypz_inc
			inc (ix + YE_ST)

		.ypz_fr
			// en_an_next_frame [enit] = sprite_25_a;
			ld  hl, YS25
			jp  ye_frame

		.ygy_do
			// NES gyrosaw: around a 32x32 square, clockwise if mx != 0 (state in en_st)
			// rda = en_st [enit];
			// gdelta = (_en_mx ? (rda >> 1) : (((rda + 1) & 3) >> 1)) ? GYRO_V : -GYRO_V;
			ld  a, (ix + YE_ST)
			ld  c, a
			ld  a, (__en_mx)
			or  a
			ld  a, c
			jr  nz, ygy_1
			inc a
			and 3
		.ygy_1
			srl a
			ld  b, GYRO_V
			jr  nz, ygy_2
			ld  b, 256 - GYRO_V
		.ygy_2
			// if (rda & 1) _en_y += gdelta; else _en_x += gdelta;
			ld  hl, __en_x
			bit 0, c
			jr  z, ygy_3
			ld  hl, __en_y
		.ygy_3
			ld  a, (hl)
			add b
			ld  (hl), a

			// en_ct [enit] = (en_ct [enit] + GYRO_V) & 31;
			// if (en_ct [enit] == 0) en_st [enit] = (rda + 1) & 3;
			ld  a, (ix + YE_CT)
			add GYRO_V
			and 31
			ld  (ix + YE_CT), a
			jr  nz, ygy_4
			ld  a, c
			inc a
			and 3
			ld  (ix + YE_ST), a
		.ygy_4
			// en_an_next_frame [enit] = (maincounter & YA_GYRO) ? sprite_28_a : sprite_27_a;
			ld  hl, YS27
			ld  a, (_maincounter)
			and YA_GYRO
			jp  z, ye_frame
			ld  hl, YS28
			jp  ye_frame

		.yca_do
			// NES steady shooter: shoots straight every few seconds
			// if (_en_mx) cdir = (_en_mx & 0x80) ? 0 : 2;
			// else cdir = (_en_my & 0x80) ? 1 : 3;
			ld  a, (__en_mx)
			or  a
			jr  z, yca_v
			rlca
			and 1
			xor 1
			add a, a
			jr  yca_d
		.yca_v
			ld  a, (__en_my)
			rlca
			and 1
			xor 1
			add a, a
			inc a
		.yca_d
			ld  (_cdir), a

			// if (en_ct [enit]) en_ct [enit] --; else {
			ld  a, (ix + YE_CT)
			or  a
			jr  z, yca_fire
			dec (ix + YE_CT)
			jr  yca_fr
		.yca_fire
			// en_ct [enit] = cannon_wait ();
			call ycw_do
			ld  (ix + YE_CT), a

			// rdx = _en_x; rdy = _en_y; rda = cdir; coco_straight (); peta_el_beeper (4); }
			ld  a, (__en_x)
			ld  (_rdx), a
			ld  a, (__en_y)
			ld  (_rdy), a
			ld  a, (_cdir)
			ld  (_rda), a
			call ycs_do
			ld  l, 4
			call _peta_el_beeper

		.yca_fr
			// switch (cdir) { case 0: sprite_29_a; case 1: sprite_30_a; case 2: sprite_31_a; default: sprite_32_a }
			ld  a, (_cdir)
			ld  hl, yca_tab
			call yb_tab
			jp  ye_frame

		.yca_tab
			defw YS29, YS30, YS31, YS32

			// Fanty, one axis: HL -> position, DE -> speed index, C = target, B = max position
		.yfz_axis
			// if (gpx > _en_x) { if (_en_mx < FZ_MAX) _en_mx ++; }
			// else if (gpx < _en_x) { if (_en_mx > -FZ_MAX) _en_mx --; }
			ld  a, c
			cp  (hl)
			jr  z, yfz_m
			ld  a, (de)
			jr  c, yfz_dec
			bit 7, a
			jr  nz, yfz_inc
			cp  FZ_MAX
			jr  nc, yfz_m
		.yfz_inc
			inc a
			ld  (de), a
			jr  yfz_m
		.yfz_dec
			bit 7, a
			jr  z, yfz_dd
			cp  256 - FZ_MAX + 1
			jr  c, yfz_m
		.yfz_dd
			dec a
			ld  (de), a
		.yfz_m
			// if (_en_mx < 0) {
			ld  a, (de)
			bit 7, a
			jr  z, yfz_pos

			// fi = fz_incs [-_en_mx];
			// if (_en_x < fi) { _en_x = 0; _en_mx = -_en_mx; } else _en_x -= fi;
			neg
			call yfz_lk
			ld  c, a
			ld  a, (hl)
			sub c
			jr  c, yfz_lo
			ld  (hl), a
			ret
		.yfz_lo
			ld  (hl), 0
			jr  yfz_flip

		.yfz_pos
			// } else { fi = fz_incs [_en_mx];
			// if (_en_x > 224 - fi) { _en_x = 224; _en_mx = -_en_mx; } else _en_x += fi; }
			call yfz_lk
			ld  c, a
			ld  a, b
			sub c
			cp  (hl)
			jr  c, yfz_hi
			ld  a, (hl)
			add c
			ld  (hl), a
			ret
		.yfz_hi
			ld  (hl), b
		.yfz_flip
			ld  a, (de)
			neg
			ld  (de), a
			ret

			// A = fz_incs [A]
		.yfz_lk
			push hl
			ld  hl, _fz_incs
			add a, l
			ld  l, a
			jr  nc, yfz_lk1
			inc h
		.yfz_lk1
			ld  a, (hl)
			pop hl
			ret

		.yfz_do
			// NES precalc fanty at the ZX rate: speed index -FZ_MAX..FZ_MAX in
			// _en_mx / _en_my, one step per frame, displacements from fz_incs.
			// 8 bits all the way. x against gpx up to 224, y against gpy up to 144.
			ld  hl, __en_x
			ld  de, __en_mx
			ld  a, (_gpx)
			ld  c, a
			ld  b, 224
			call yfz_axis
			ld  hl, __en_y
			ld  de, __en_my
			ld  a, (_gpy)
			ld  c, a
			ld  b, 144
			call yfz_axis

			// en_an_next_frame [enit] = (_en_x & 8) ? sprite_14_a : sprite_13_a;
			ld  hl, YS13
			ld  a, (__en_x)
			and 8
			jp  z, ye_frame
			ld  hl, YS14
			jp  ye_frame

		.ymc_do
			// NES enem_monococo. Phase in _en_mx as (phase + 1) & 3 so that the
			// 0 of a fresh screen means "hiding, about to be hidden"; counter in _en_my.
			// mc_phase = (_en_mx + 3) & 3;
			ld  a, (__en_mx)
			add 3
			and 3
			ld  (_mc_phase), a

			// if (_en_my) _en_my --;
			// if (_en_my == 0) {
			ld  a, (__en_my)
			or  a
			jr  z, ymc_0
			dec a
			ld  (__en_my), a
			jr  nz, ymc_1
		.ymc_0
			// mc_phase = (mc_phase + 1) & 3;
			ld  a, (_mc_phase)
			inc a
			and 3
			ld  (_mc_phase), a

			// _en_my = mc_times [mc_phase] - (rand () & 3);
			ld  e, a
			ld  d, 0
			ld  hl, _mc_times
			add hl, de
			ld  b, (hl)
			push bc
			call _rand
			pop bc
			ld  a, l
			and 3
			ld  c, a
			ld  a, b
			sub c
			ld  (__en_my), a

			// _en_mx = (mc_phase + 1) & 3; }
			ld  a, (_mc_phase)
			inc a
			and 3
			ld  (__en_mx), a
		.ymc_1
			// if (mc_phase == 2 && _en_my == MC_FIRE_AT) { rdx = _en_x; rdy = _en_y; coco_new (); peta_el_beeper (4); }
			ld  a, (_mc_phase)
			cp  2
			jr  nz, ymc_fr
			ld  a, (__en_my)
			cp  MC_FIRE_AT
			jr  nz, ymc_fr
			ld  a, (__en_x)
			ld  (_rdx), a
			ld  a, (__en_y)
			ld  (_rdy), a
			call ycn_do
			ld  l, 4
			call _peta_el_beeper
		.ymc_fr
			// switch (mc_phase) {
			//     case 0: en_an_next_frame [enit] = _en_t == 8 ? sprite_23_a : sprite_17_a; break;	// bush / wall
			//     case 1: en_an_next_frame [enit] = _en_t == 8 ? sprite_24_a : sprite_18_a; break;	// coming out
			//     default: en_an_next_frame [enit] = sprite_19_a; }							// out
			ld  a, (_mc_phase)
			ld  hl, YS19
			cp  2
			jp  nc, ye_frame
			ld  c, a
			ld  hl, ymc_t6
			ld  a, (__en_t)
			cp  8
			jr  nz, ymc_2
			ld  hl, ymc_t8
		.ymc_2
			ld  a, c
			call yb_tab
			jp  ye_frame

		.ymc_t6
			defw YS17, YS18
		.ymc_t8
			defw YS23, YS24
	#endasm
}

void extra_enems_move (void) {
	#asm
			call ye_ix

			// Trapped: stay put, drop to the floor, upside down, bubble around it.
			// if (en_cap [enit]) {
			ld  a, (ix + YE_CAP)
			or  a
			jr  z, yem_types

			// bj = en_cap [enit] - 1; _en_x = cap_x [enit];
			dec a
			ld  (_bj), a
			ld  a, (ix + YE_CX)
			ld  (__en_x), a

			// if (cap_y [enit] < 144) {
			ld  a, (ix + YE_CY)
			cp  144
			jr  nc, yem_y

			// if (attr ((_en_x + 8) >> 4, (cap_y [enit] + 16 + BUB_FALL - 1) >> 4) & 12)
			add 16 + BUB_FALL - 1
			rrca
			rrca
			rrca
			rrca
			and 15
			ld  b, a
			push bc
			ld  a, (__en_x)
			add 8
			rrca
			rrca
			rrca
			rrca
			and 15
			ld  c, a
			ld  a, b
			call _attr_2
			pop bc
			ld  a, l
			and 12
			jr  z, yem_fall

			// cap_y [enit] = (((cap_y [enit] + 16 + BUB_FALL - 1) >> 4) << 4) - 16;
			ld  a, b
			add a, a
			add a, a
			add a, a
			add a, a
			sub 16
			ld  (ix + YE_CY), a
			jr  yem_y

			// else cap_y [enit] += BUB_FALL;
		.yem_fall
			ld  a, (ix + YE_CY)
			add BUB_FALL
			ld  (ix + YE_CY), a

		.yem_y
			// _en_y = cap_y [enit]; bub_x [bj] = _en_x; bub_y [bj] = _en_y;
			ld  a, (ix + YE_CY)
			ld  (__en_y), a
			ld  a, (_bj)
			ld  e, a
			ld  d, 0
			ld  hl, _bub_x
			add hl, de
			ld  a, (__en_x)
			ld  (hl), a
			ld  hl, _bub_y
			add hl, de
			ld  a, (__en_y)
			ld  (hl), a

			// en_an_next_frame [enit] = ud_cells [((_en_t - 1) << 1) + ((maincounter & YA_UD) ? 1 : 0)];
			ld  a, (__en_t)
			dec a
			add a, a
			ld  c, a
			ld  a, (_maincounter)
			and YA_UD
			jr  z, yem_ud
			ld  a, 1
		.yem_ud
			add c
			ld  hl, _ud_cells
			call yb_tab
			jp  ye_frame

		.yem_types
			// } else if (_en_t == 5) fanty_move ();
			// else if (_en_t == 6) monococo_move ();
			// else if (_en_t == 8) monococo_move ();
			// else if (_en_t == 7) pezon_move ();
			// else if (_en_t == 9) gyrosaw_move ();
			// else if (_en_t == 10) cannon_move ();
			ld  a, (__en_t)
			cp  5
			jp  z, yfz_do
			cp  6
			jp  z, ymc_do
			cp  8
			jp  z, ymc_do
			cp  7
			jp  z, ypz_do
			cp  9
			jp  z, ygy_do
			cp  10
			jp  z, yca_do
	#endasm
}

void extra_enems_checks (void) {
	#asm
			call ye_ix

			// if (en_cap [enit]) {
			ld  a, (ix + YE_CAP)
			or  a
			jr  z, yec_free

			// Touch a trapped enemy: pop it
			// if (collide_enem ()) {
			call _collide_enem
			ld  a, l
			or  a
			ret z

			// bj = en_cap [enit] - 1; bub_ct [bj] = 0; bub_lock [bj] = 0xff;
			// if (bub_ride == bj) bub_ride = 0xff;
			call ye_ix
			ld  a, (ix + YE_CAP)
			dec a
			ld  (_bj), a
			ld  e, a
			ld  d, 0
			ld  hl, _bub_ct
			add hl, de
			ld  (hl), 0
			ld  hl, _bub_lock
			add hl, de
			ld  (hl), 255
			ld  hl, _bub_ride
			cp  (hl)
			jr  nz, yec_1
			ld  (hl), 255
		.yec_1
			// en_cap [enit] = 0;
			ld  (ix + YE_CAP), 0

			// enems_kill (1);
			ld  hl, 1
			push hl
			call _enems_kill
			pop hl

			// player.vy = -PLAYER_VY_FROM_ENEM;
			ld  hl, 65536 - PLAYER_VY_FROM_ENEM
			ld  (_player + 8), hl
			ret

		.yec_free
			// Linear enemies, fanties, pezons and monococos out of their bush can be trapped
			// if (_en_t < 1 || _en_t > 8 || _en_t == 4) return;
			ld  a, (__en_t)
			or  a
			ret z
			cp  9
			ret nc
			cp  4
			ret z

			// if ((_en_t == 6 || _en_t == 8) && ((_en_mx + 3) & 3) == 0) return;
			cp  6
			jr  z, yec_mc
			cp  8
			jr  nz, yec_go
		.yec_mc
			ld  a, (__en_mx)
			add 3
			and 3
			ret z

		.yec_go
			// for (bj = 0; bj < BUBS_MAX; bj ++) {
			xor a
		.yec_l
			ld  (_bj), a
			ld  e, a
			ld  d, 0
			ld  iy, _bub_ct
			add iy, de

			// if (bub_ct [bj] && bub_lock [bj] == 0xff && bub_ride != bj
			//     && bub_x [bj] + 12 >= _en_x && bub_x [bj] <= _en_x + 12
			//     && bub_y [bj] + 12 >= _en_y && bub_y [bj] <= _en_y + 12) {
			ld  a, (iy + YB_CT)
			or  a
			jr  z, yec_n
			ld  a, (iy + YB_LK)
			inc a
			jr  nz, yec_n
			ld  a, (_bub_ride)
			cp  e
			jr  z, yec_n
			ld  a, (__en_x)
			ld  c, a
			ld  a, (iy + YB_X)
			add 12
			cp  c
			jr  c, yec_n
			ld  a, (__en_x)
			add 12
			cp  (iy + YB_X)
			jr  c, yec_n
			ld  a, (__en_y)
			ld  c, a
			ld  a, (iy + YB_Y)
			add 12
			cp  c
			jr  c, yec_n
			ld  a, (__en_y)
			add 12
			cp  (iy + YB_Y)
			jr  nc, yec_trap
		.yec_n
			ld  a, (_bj)
			inc a
			cp  BUBS_MAX
			jr  c, yec_l
			ret

		.yec_trap
			// bub_lock [bj] = enit; en_cap [enit] = bj + 1;
			// cap_x [enit] = _en_x; cap_y [enit] = _en_y;
			// peta_el_beeper (6); break;
			ld  a, (_enit)
			ld  (iy + YB_LK), a
			ld  a, (_bj)
			inc a
			ld  (ix + YE_CAP), a
			ld  a, (__en_x)
			ld  (ix + YE_CX), a
			ld  a, (__en_y)
			ld  (ix + YE_CY), a
			ld  l, 6
			jp  _peta_el_beeper
	#endasm
}

void extra_enems_killed (void) {
}

unsigned char this_enemy_kills (void) {
	#asm
			call ye_ix
			ld  hl, 0

			// Trapped enemies and monococos in their bush don't hurt
			// if (en_cap [enit]) return 0;
			ld  a, (ix + YE_CAP)
			or  a
			ret nz

			// if (_en_t == 6 && ((_en_mx + 3) & 3) == 0) return 0;
			// if (_en_t == 8 && ((_en_mx + 3) & 3) == 0) return 0;
			ld  a, (__en_t)
			cp  6
			jr  z, ytk_mc
			cp  8
			jr  nz, ytk_1
		.ytk_mc
			ld  a, (__en_mx)
			add 3
			and 3
			ret z
			jr  ytk_step

		.ytk_1
			// if (_en_t == 9) return 1; 		// no bouncing off a saw
			cp  9
			jr  z, ytk_yes

		.ytk_step
			// Stepping on an enemy bounces Yun (Yun v5: PLAYER_STEPS_ON_ENEMIES)
			// if (gpy + 10 <= _en_y && player.vy > 0) { player.vy = -PLAYER_VY_FROM_ENEM; return 0; }
			ld  a, (_gpy)
			add 10
			ld  c, a
			ld  a, (__en_y)
			cp  c
			jr  c, ytk_yes
			ld  de, (_player + 8)
			bit 7, d
			jr  nz, ytk_yes
			ld  a, d
			or  e
			jr  z, ytk_yes
			ld  de, 65536 - PLAYER_VY_FROM_ENEM
			ld  (_player + 8), de
			ret

			// return 1;
		.ytk_yes
			ld  l, 1
	#endasm
}

#endif

#ifdef ENABLE_CUSTOM_HOTSPOTS
unsigned char custom_hotspots () {
	#asm
			// Stars: the NES counts one per level for the ending; split into loads
			// that can't carry over, so here a star is an extra life.
		#ifdef YUN_DARK_LEVEL
			// if (hotspot_t == HOTSPOT_TYPE_BULB) {
			//     (lights the room for good; the engine takes the hotspot away)
			//     lit [n_pant] = 1; lightning = 0; dark_set (0); peta_el_beeper (7); }
			ld  a, (_hotspot_t)
			cp  HOTSPOT_TYPE_BULB
			jr  nz, ych_1
			ld  a, (_n_pant)
			ld  e, a
			ld  d, 0
			ld  hl, _lit
			add hl, de
			ld  (hl), 1
			xor a
			ld  (_lightning), a
			call yds_do
			ld  l, 7
			call _peta_el_beeper
		.ych_1
		#endif

			// if (hotspot_t == HOTSPOT_TYPE_OBJECT) {
			//     player.life ++; if (player.life > 99) player.life = 99;
			//     peta_el_beeper (7); hotspot_t = 0; }
			ld  a, (_hotspot_t)
			cp  HOTSPOT_TYPE_OBJECT
			jr  nz, ych_end
			ld  hl, (_player + 29)
			inc hl
			ld  a, l
			cp  100
			jr  c, ych_2
			ld  l, 99
		.ych_2
			ld  (_player + 29), hl
			ld  l, 7
			call _peta_el_beeper
			xor a
			ld  (_hotspot_t), a

			// return 0;
		.ych_end
			ld  hl, 0
	#endasm
}
#endif
