// MTE MK1 v3.2
// Copyleft 2011, 2026 by The Mojon Twins

// Nin Nin (CPC) - custom.h
//
// Everything Nin Nin-specific lives here and in the nn_*.h it includes:
//
//  * Levels (ENABLE_CUSTOM_LEVELS): NN_LEVELS per load (nn_load.h), tables
//    for all of them in nn_levels.h (generated). custom_level_setup unpacks
//    the level area (locks, enemies, hotspots, RLE44 map) and the level's
//    sprite cells, points tileset_mappings at its 16 tiles of the 48 and sets
//    its 4 inks.
//  * The map renderer (CUSTOM_RENDERER, nn_renderer.h): the NES rules that
//    add detail to the RLE44 screens.
//  * The jump (Nin Nin's: a strong kick, light gravity while UP is held for a
//    few frames, a cut when it's released) and swim engines
//    (PLAYER_CUSTOM_VENG), chosen per level and, in DE NADAL, per screen.
//  * Frames (PLAYER_CUSTOM_FRAME).
//
// Every function is written in assembly; its C is in the comments. No
// locals, no parameters: everything goes through globals. Results in L (HL).
// Calling conventions used from here:
//   peta_el_beeper, enems_en_an_calc: fastcall, the argument in L (HL)
//   draw_coloured_tile: set __x, __y, __t, call _draw_coloured_tile_do
//   other C functions: arguments pushed left to right, popped after the call
// Constants used in the asm must not start with a bracket: z80asm reads
// "ld a, (x)" as a memory read.

// Engine functions defined after this file is included
unsigned char attr (unsigned char x, unsigned char y);
unsigned char rand (void);
void draw_coloured_tile (unsigned char x, unsigned char y, unsigned char t);
void draw_2_digits (unsigned char x, unsigned char y, unsigned char value);
void enems_kill (unsigned char damage);

// Levels
unsigned char level, level_cont, nn_l;
signed int nn_saved_life;
unsigned char level_scr_ini, level_ini_x, level_ini_y;
unsigned char *level_src;
unsigned char *tileset_mappings;
unsigned char nn_swimming;				// the vertical engine: 0 jump, 1 swim
unsigned char nn_alt_bg, nn_stalactites;
unsigned char nn_ci, nn_cj;
unsigned char *nn_p, *nn_q;

char nn_level_text [] = "LEVEL 10";

// Coins (tile beh 128): 50 = one more life
unsigned char nn_coins, nn_coins_old;
#define NN_COINS_X 			29
#define NN_COINS_Y 			0
#define NN_COINS_LIFE 		50

#define NN_PAD_LRUD 		sp_LEFT + sp_RIGHT + sp_UP + sp_DOWN

#asm
	// HL = HL + A
	.nn_add_a
		add a, l
		ld  l, a
		ret nc
		inc h
		ret

	// A = 15 * _y (B = _y)
	.nn_y15
		ld  a, (__y)
		ld  b, a
		add a, a
		add a, a
		add a, a
		add a, a
		sub b
		ret

	// A = _x + 15 * _y: a tile of the screen (B = 15 * _y)
	.nn_xy
		call nn_y15
		ld  b, a
		ld  a, (__x)
		add a, b
		ret

	// A = (A + C) >> 4, the sum as 9 bits
	.nn_cell
		add a, c
		rra
		srl a
		srl a
		srl a
		ret

	// HL = -HL
	.nn_neg_hl
		xor a
		sub l
		ld  l, a
		sbc a, a
		sub h
		ld  h, a
		ret

#endasm

// Level blobs (zx0) and palettes, made by comp_cpc.sh
#include "nn_blobs.h"

#include "nn_renderer.h"
#ifdef NN_CASTLE
	unsigned char nn_castle_fire (void);
	unsigned char nn_castle_rays (void);
	unsigned char nn_fire_zoned (void);
#endif
#include "nn_play.h"
#include "nn_enems.h"
#ifdef NN_CASTLE
	#include "nn_castle.h"
#endif

// ----------------------------------------------------------------------------
// Boot

void custom_game_init (void) {
	// tile_map: tile t = patterns 64 + t * 4 .. + 3, but 0 (filled faster
	// by cpc_dirty.h) where the pattern is all black, like pattern 0
	// nn_p = tile_map;
	// for (nn_ci = 64; nn_ci; nn_ci ++) {
	//	nn_q = (unsigned char *) (CD_PAT + (nn_ci << 4));
	//	for (nn_cj = 0; nn_cj < 16; nn_cj ++) if (nn_q [nn_cj] != ((unsigned char *) CD_PAT) [nn_cj]) break;
	//	*nn_p ++ = (nn_cj == 16) ? 0 : nn_ci;
	// }
	#asm
			ld  de, _tile_map
			ld  c, 64
		.nn_gi_loop
			ld  l, c
			ld  h, 0
			add hl, hl
			add hl, hl
			add hl, hl
			add hl, hl
			push de
			ld  de, CD_PAT
			add hl, de
			ld  b, 16
		.nn_gi_cmp
			ld  a, (de)
			cp  (hl)
			jr  nz, nn_gi_diff
			inc hl
			inc de
			djnz nn_gi_cmp
			pop de
			xor a 						// all black: 0
			jr  nn_gi_store
		.nn_gi_diff
			pop de
			ld  a, c
		.nn_gi_store
			ld  (de), a
			inc de
			inc c
			jr  nz, nn_gi_loop
			ld  (_nn_p), de
			xor a
			ld  (_nn_ci), a
	#endasm
}

// ----------------------------------------------------------------------------
// Vertical engines

void player_custom_veng (void) {
	#asm
			// if (nn_swimming) {
			//	// NES: DOWN (or FIRE) sinks, accelerating; otherwise float up slowly
			//	if ((pad0 & sp_DOWN) == 0) {
			//		player.vy += PLAYER_AY_SWIM;
			//		if (player.vy > PLAYER_VY_SWIM_MAX) player.vy = PLAYER_VY_SWIM_MAX;
			//	} else {
			//		player.vy -= PLAYER_AY_SWIM >> 1;
			//		if ((pad0 & sp_UP) == 0) player.vy -= PLAYER_AY_SWIM;
			//		if (player.vy < -PLAYER_VY_SWIM_MAX) player.vy = -PLAYER_VY_SWIM_MAX;
			//	}
			//	return;
			// }
			ld  a, (_nn_swimming)
			or  a
			jr  z, nn_vg_jump
			ld  hl, (_player + 8) 		// vy
			ld  a, (_pad0)
			ld  c, a
			and sp_DOWN
			jr  nz, nn_vg_float
			ld  de, PLAYER_AY_SWIM
			add hl, de
			ld  de, PLAYER_VY_SWIM_MAX
			call nn_vg_max
			jr  nn_vg_sst
		.nn_vg_float
			ld  de, -PLAYER_AY_SWIM / 2
			add hl, de
			ld  a, c
			and sp_UP
			jr  nz, nn_vg_fl1
			ld  de, -PLAYER_AY_SWIM
			add hl, de
		.nn_vg_fl1
			ld  de, PLAYER_VY_SWIM_MAX
			call nn_vg_min
		.nn_vg_sst
			ld  (_player + 8), hl
			ret

		.nn_vg_jump
			// Gravity, lighter while the jump is held
			// player.vy += player.saltando ? PLAYER_G_JUMPING : PLAYER_G;
			// if (player.vy > PLAYER_MAX_VY_CAYENDO) player.vy = PLAYER_MAX_VY_CAYENDO;
			ld  hl, (_player + 8)
			ld  de, PLAYER_G
			ld  a, (_player + 19) 		// saltando
			or  a
			jr  z, nn_vg_g
			ld  de, PLAYER_G_JUMPING
		.nn_vg_g
			add hl, de
			ld  de, PLAYER_MAX_VY_CAYENDO
			call nn_vg_max

			// Quicksand: sinking slowly, a jump gets out (but no long jump)
			// if (nn_sand) { if (player.vy < 0) player.saltando = 0;
			//	else player.vy = NN_VY_SINKING; }
			ld  a, (_nn_sand)
			or  a
			jr  z, nn_vg_ns
			bit 7, h
			jr  z, nn_vg_sink
			xor a
			ld  (_player + 19), a
			jr  nn_vg_ns
		.nn_vg_sink
			ld  hl, NN_VY_SINKING
		.nn_vg_ns
			ld  (_player + 8), hl

			// Jump start
			// if ((pad_this_frame & sp_UP) == 0 && player.saltando == 0 &&
			//	(player.possee || player.gotten || nn_sand)) {
			//	player.cont_salto = 0; player.saltando = 1; player.just_jumped = 1;
			//	player.vy = -PLAYER_VY_JUMP_INITIAL; peta_el_beeper (3); }
			ld  a, (_pad_this_frame)
			and sp_UP
			jr  nz, nn_vg_j2
			ld  a, (_player + 19)
			or  a
			jr  nz, nn_vg_j2
			ld  a, (_player + 26) 		// possee
			ld  b, a
			ld  a, (_player + 25) 		// gotten
			or  b
			ld  b, a
			ld  a, (_nn_sand)
			or  b
			jr  z, nn_vg_j2
			xor a
			ld  (_player + 14), a 		// cont_salto
			inc a
			ld  (_player + 19), a 		// saltando
			ld  (_player + 43), a 		// just_jumped
			ld  hl, -PLAYER_VY_JUMP_INITIAL
			ld  (_player + 8), hl
			ld  l, 3
			call _peta_el_beeper
		.nn_vg_j2

			// if ((pad0 & sp_UP) == 0) {
			//	if (player.saltando) {
			//		player.cont_salto ++;
			//		if (player.cont_salto == PLAYER_VY_JUMP_STEPS) player.saltando = 0; }
			// } else if (player.saltando) {
			//	// released: cut the jump
			//	if (player.vy < -PLAYER_VY_JUMP_RELEASE) player.vy = -PLAYER_VY_JUMP_RELEASE;
			//	player.saltando = 0; }
			ld  a, (_player + 19)
			or  a
			ret z
			ld  a, (_pad0)
			and sp_UP
			jr  nz, nn_vg_rel
			ld  hl, _player + 14
			inc (hl)
			ld  a, (hl)
			cp  PLAYER_VY_JUMP_STEPS
			ret nz
			xor a
			ld  (_player + 19), a
			ret
		.nn_vg_rel
			ld  hl, (_player + 8)
			ld  de, PLAYER_VY_JUMP_RELEASE
			call nn_vg_min
			ld  (_player + 8), hl
			xor a
			ld  (_player + 19), a
			ret

			// HL = min (HL, DE)
		.nn_vg_max
			push hl
			ex  de, hl
			push hl
			or  a
			sbc hl, de 					// DE - HL
			ld  a, h
			pop de
			pop hl
			rlca
			ret nc
			ex  de, hl
			ret

			// HL = max (HL, -DE)
		.nn_vg_min
			push hl
			add hl, de
			ld  a, h
			pop hl
			rlca
			ret nc
			ex  de, hl
			jp  nn_neg_hl
	#endasm
}

// ----------------------------------------------------------------------------
// Frames: see custom_sprite_cells.h

unsigned char nn_walk [] = { 0, 1, 2, 1 };

unsigned char player_custom_frame (void) {
	// rda = (player.facing == SIDEVIEW_FACING_LEFT) ? 4 : 0;
	// if (nn_swimming) {
	//	rdb = ((pad0 & (sp_LEFT | sp_RIGHT | sp_UP | sp_DOWN)) != (sp_LEFT | sp_RIGHT | sp_UP | sp_DOWN)) ?
	//		nn_walk [(maincounter >> 1) & 3] : 1;
	//	return NN_CELL_SWIM + (rda ? 3 : 0) + rdb;
	// }
	// if (player.possee || player.gotten) {
	//	rdb = player.vx ? nn_walk [(gpx >> 3) & 3] : 1;
	//	return rda + rdb;
	// }
	// return rda + 3;
	#asm
			ld  a, (_player + 22) 		// facing
			cp  SIDEVIEW_FACING_LEFT
			ld  a, 0
			jr  nz, nn_pf_r
			ld  a, 4
		.nn_pf_r
			ld  (_rda), a
			ld  a, (_nn_swimming)
			or  a
			jr  z, nn_pf_land
			ld  a, (_pad0)
			and NN_PAD_LRUD
			cp  NN_PAD_LRUD
			ld  a, 1
			jr  z, nn_pf_sw1
			ld  a, (_maincounter)
			rrca
			and 3
			ld  hl, _nn_walk
			call nn_add_a
			ld  a, (hl)
		.nn_pf_sw1
			ld  (_rdb), a
			ld  b, a
			ld  a, (_rda)
			or  a
			ld  a, NN_CELL_SWIM
			jr  z, nn_pf_sw2
			add a, 3
		.nn_pf_sw2
			add a, b
			ld  l, a
			ld  h, 0
			ret
		.nn_pf_land
			ld  a, (_player + 26) 		// possee
			ld  b, a
			ld  a, (_player + 25) 		// gotten
			or  b
			jr  z, nn_pf_air
			ld  hl, (_player + 6) 		// vx
			ld  a, h
			or  l
			ld  a, 1
			jr  z, nn_pf_w1
			ld  a, (_gpx)
			rrca
			rrca
			rrca
			and 3
			ld  hl, _nn_walk
			call nn_add_a
			ld  a, (hl)
		.nn_pf_w1
			ld  (_rdb), a
			ld  b, a
			ld  a, (_rda)
			add a, b
			ld  l, a
			ld  h, 0
			ret
		.nn_pf_air
			ld  a, (_rda)
			add a, 3
			ld  l, a
			ld  h, 0
	#endasm
}

// ----------------------------------------------------------------------------
// Levels

void nn_screen_setup (void) {
	// What the NES sets per screen: the engine, the decoration, the inks
	// nn_swimming = nn_swim [nn_l];
	// nn_alt_bg = nn_alt_bg_l [nn_l];
	// nn_stalactites = nn_stalac [nn_l];
	// cd_pal_bg = nn_pal [nn_l];
	// if (nn_l == 3) {
	//	// DE NADAL 1: screen 7 is the surface, the rest is under water
	//	if (n_pant == 7) { nn_swimming = 0; nn_alt_bg = 34; cd_pal_bg = nn_pal_surface; }
	//	else nn_alt_bg = 42;
	// } else if (nn_l == 6) {
	//	// DE NADAL 2: screens 0 and 1 are the surface
	//	if (n_pant < 2) { nn_swimming = 0; nn_alt_bg = 34; cd_pal_bg = nn_pal_surface; }
	//	else nn_alt_bg = 42;
	// } else if (nn_l == 9) {
	//	// EL CASTLER: bushes outdoors only
	//	nn_alt_bg = ((n_pant & 1) == 0 && n_pant < 16) ? 34 : 0;
	// } else if (nn_l == 7) {
	//	// MARSHY GOO: stalactites in the lower half
	//	nn_stalactites = (n_pant >= 10);
	// }
	#asm
			ld  a, (_nn_l)
			ld  c, a
			ld  hl, _nn_swim
			call nn_add_a
			ld  a, (hl)
			ld  (_nn_swimming), a
			ld  hl, _nn_alt_bg_l
			ld  a, c
			call nn_add_a
			ld  a, (hl)
			ld  (_nn_alt_bg), a
			ld  hl, _nn_stalac
			ld  a, c
			call nn_add_a
			ld  a, (hl)
			ld  (_nn_stalactites), a
			ld  hl, _nn_pal
			ld  a, c
			add a, a
			call nn_add_a
			ld  e, (hl)
			inc hl
			ld  d, (hl)
			ld  (_cd_pal_bg), de

			ld  a, (_n_pant)
			ld  b, a
			ld  a, c
			cp  3
			jr  z, nn_ss_nadal1
			cp  6
			jr  z, nn_ss_nadal2
			cp  9
			jr  z, nn_ss_castle
			cp  7
			ret nz
			ld  a, b
			cp  10
			ld  a, 0
			jr  c, nn_ss_st
			inc a
		.nn_ss_st
			ld  (_nn_stalactites), a
			ret
		.nn_ss_nadal1
			ld  a, b
			cp  7
			jr  z, nn_ss_surface
			jr  nn_ss_under
		.nn_ss_nadal2
			ld  a, b
			cp  2
			jr  c, nn_ss_surface
		.nn_ss_under
			ld  a, 42
			ld  (_nn_alt_bg), a
			ret
		.nn_ss_surface
			xor a
			ld  (_nn_swimming), a
			ld  a, 34
			ld  (_nn_alt_bg), a
			ld  hl, _lvl0_pal 			// nn_pal_surface
			ld  (_cd_pal_bg), hl
			ret
		.nn_ss_castle
			ld  a, b
			and 1
			jr  nz, nn_ss_c0
			ld  a, b
			cp  16
			jr  nc, nn_ss_c0
			ld  a, 34
			jr  nn_ss_c1
		.nn_ss_c0
			xor a
		.nn_ss_c1
			ld  (_nn_alt_bg), a
	#endasm
}

void custom_level_setup (void) {
	// if (level_cont == 0) level = 0; 		// (NN_DEBUG_LEVEL when testing)
	// nn_l = NN_FIRST + level;
	// // the level area: locks, enemies, hotspots, map (nn_load.h)
	// dzx0_standard (lvl_area [level], level_area);
	// // the level's sprite cells
	// dzx0_standard (lvl_spr [level], NN_SPR_BASE);
	// tileset_mappings = nn_tsmaps + (nn_l << 4);
	// map_w = nn_map_w [nn_l];
	// level_scr_ini = nn_scr_ini [nn_l];
	// level_ini_x = nn_ini_x [nn_l]; level_ini_y = nn_ini_y [nn_l];
	// // enemy speeds: NES pixels per 60 Hz frame -> 25 Hz
	// for (nn_ci = 0; nn_ci < MAP_W * MAP_H * 3; nn_ci ++) {
	//	malotes [nn_ci].mx <<= 1; malotes [nn_ci].my <<= 1; }
	// // nothing broken yet
	// for (nn_p = NN_CLEAR; nn_p < NN_CLEAR + MAP_W * MAP_H * 19; nn_p ++) *nn_p = 0;
	// n_pant = level_scr_ini;
	// nn_screen_setup (); cd_set_palette ();
	// nn_coins_old = 0xff;
	// // the playfield blank (DIRECT_TO_PLAY: the title's logo, or the last level)
	// saca_a_todo_el_mundo_de_aqui ();
	// for (_y = VIEWPORT_Y; _y < VIEWPORT_Y + 20; _y ++)
	//	for (_x = VIEWPORT_X; _x < VIEWPORT_X + 30; _x ++) { _t = 0; cd_char (); }
	// // LEVEL n and its name, for a moment
	// if (nn_l == 9) { nn_level_text [6] = '1'; nn_level_text [7] = '0'; }
	// else { nn_level_text [6] = '1' + nn_l; nn_level_text [7] = 0; }
	// nn_p = nn_names [nn_l];
	// for (nn_ci = 0; nn_p [nn_ci]; nn_ci ++);
	// draw_text (12, 10, nn_level_text);
	// draw_text (16 - (nn_ci >> 1), 13, nn_p);
	// cd_update (1); 		// (with the sprites, so they go)
	// espera_activa (150);
	#asm
			ld  a, (_level_cont)
			or  a
			jr  nz, nn_ls_cont
		#ifdef NN_DEBUG_LEVEL
			ld  a, NN_DEBUG_LEVEL 		// testing: -DNN_DEBUG_LEVEL=n
		#endif
			ld  (_level), a
		.nn_ls_cont
			ld  a, (_level)
			add a, NN_FIRST
			ld  (_nn_l), a

			ld  a, (_level)
			add a, a
			ld  hl, _lvl_area
			call nn_add_a
			ld  a, (hl)
			inc hl
			ld  h, (hl)
			ld  l, a
			ld  de, _level_area
			call dzx0_standard
			ld  a, (_level)
			add a, a
			ld  hl, _lvl_spr
			call nn_add_a
			ld  a, (hl)
			inc hl
			ld  h, (hl)
			ld  l, a
			ld  (_level_src), hl
			ld  de, NN_SPR_BASE
			call dzx0_standard

			ld  a, (_nn_l)
			ld  c, a
			ld  l, a
			ld  h, 0
			add hl, hl
			add hl, hl
			add hl, hl
			add hl, hl
			ld  de, _nn_tsmaps
			add hl, de
			ld  (_tileset_mappings), hl
			ld  hl, _nn_map_w
			ld  a, c
			call nn_add_a
			ld  a, (hl)
			ld  (_map_w), a
			ld  hl, _nn_scr_ini
			ld  a, c
			call nn_add_a
			ld  a, (hl)
			ld  (_level_scr_ini), a
			ld  hl, _nn_ini_x
			ld  a, c
			call nn_add_a
			ld  a, (hl)
			ld  (_level_ini_x), a
			ld  hl, _nn_ini_y
			ld  a, c
			call nn_add_a
			ld  a, (hl)
			ld  (_level_ini_y), a

			ld  hl, _malotes + 6 		// mx, my
			ld  de, 8
			ld  b, MAP_W * MAP_H * 3
		.nn_ls_speed
			sla (hl)
			inc hl
			sla (hl)
			add hl, de
			djnz nn_ls_speed
			ld  a, MAP_W * MAP_H * 3
			ld  (_nn_ci), a

			ld  hl, NN_CLEAR
			ld  de, NN_CLEAR + 1
			ld  bc, MAP_W * MAP_H * 19 - 1
			ld  (hl), 0
			ldir
			ld  (_nn_p), de

			ld  a, (_level_scr_ini)
			ld  (_n_pant), a
			call _nn_screen_setup
			call _cd_set_palette
			ld  a, 0xff
			ld  (_nn_coins_old), a

			// DIRECT_TO_PLAY: the playfield still has the title's logo or the
			// last level (and its sprites): blank it (pattern 0) under the text
			call _saca_a_todo_el_mundo_de_aqui
			ld  a, VIEWPORT_Y
		.nn_ls_cly
			ld  (__y), a
			ld  a, VIEWPORT_X
		.nn_ls_clx
			ld  (__x), a
			xor a
			ld  (__t), a
			call _cd_char
			ld  a, (__x)
			inc a
			cp  VIEWPORT_X + 30
			jr  c, nn_ls_clx
			ld  a, (__y)
			inc a
			cp  VIEWPORT_Y + 20
			jr  c, nn_ls_cly

			ld  a, (_nn_l)
			ld  hl, _nn_level_text + 6
			cp  9
			jr  z, nn_ls_10
			add a, 49 					// '1'
			ld  (hl), a
			inc hl
			ld  (hl), 0
			jr  nn_ls_name
		.nn_ls_10
			ld  (hl), 49 				// "10"
			inc hl
			ld  (hl), 48
		.nn_ls_name
			ld  a, (_nn_l)
			add a, a
			ld  hl, _nn_names
			call nn_add_a
			ld  a, (hl)
			inc hl
			ld  h, (hl)
			ld  l, a
			ld  (_nn_p), hl
			ld  c, 0
		.nn_ls_len
			ld  a, (hl)
			or  a
			jr  z, nn_ls_lend
			inc hl
			inc c
			jr  nn_ls_len
		.nn_ls_lend
			ld  a, c
			ld  (_nn_ci), a

			ld  hl, 12
			push hl
			ld  hl, 10
			push hl
			ld  hl, _nn_level_text
			push hl
			call _draw_text
			pop hl
			pop hl
			pop hl
			ld  a, (_nn_ci)
			srl a
			ld  b, a
			ld  a, 16
			sub b
			ld  l, a
			ld  h, 0
			push hl
			ld  hl, 13
			push hl
			ld  hl, (_nn_p)
			push hl
			call _draw_text
			pop hl
			pop hl
			pop hl
			ld  hl, 1 					// (with the sprites: they go)
			push hl
			call _cd_update
			pop hl
			ld  hl, 150
			push hl
			call _espera_activa
			pop hl
	#endasm
}

unsigned char custom_level_won (void) {
	// nn_saved_life = player.life;
	// if (level + 1 < NN_LEVELS) { level ++; level_cont = 1; return 1; }
	// level_cont = 0; return 0;
	#asm
			ld  hl, (_player + 29) 		// life
			ld  (_nn_saved_life), hl
			ld  hl, 0
			ld  a, (_level)
			inc a
			cp  NN_LEVELS
			jr  nc, nn_lw_last
			ld  (_level), a
			inc l
			ld  a, l
			ld  (_level_cont), a
			ret
		.nn_lw_last
			xor a
			ld  (_level_cont), a
	#endasm
}

void custom_game_start (void) {
	// if (level_cont == 0) nn_coins = 0; else player.life = nn_saved_life;
	// player.facing = nn_facing [nn_l] ? SIDEVIEW_FACING_RIGHT : SIDEVIEW_FACING_LEFT;
	#asm
			ld  a, (_level_cont)
			or  a
			jr  nz, nn_gs_cont
			ld  (_nn_coins), a
			jr  nn_gs_face
		.nn_gs_cont
			ld  hl, (_nn_saved_life)
			ld  (_player + 29), hl
		.nn_gs_face
			ld  a, (_nn_l)
			ld  hl, _nn_facing
			call nn_add_a
			ld  a, (hl)
			or  a
			ld  a, SIDEVIEW_FACING_RIGHT
			jr  nz, nn_gs_f
			ld  a, SIDEVIEW_FACING_LEFT
		.nn_gs_f
			ld  (_player + 22), a
	#endasm
}

void custom_screen_enter (void) {
	// nn_clear_apply ();
	// nn_face_count = nn_face_cleared = 0;
	// nn_castle_screen (); 			// (NN_CASTLE)
	// for (nn_ci = 0; nn_ci < NN_COCOS; nn_ci ++) coco_on [nn_ci] = 0;
	// for (nn_ci = 0; nn_ci < NN_BRK_MAX; nn_ci ++) brk_f [nn_ci] = 0;
	// hit_on = 0;
	// if (nn_screen_is_safe ()) 		// (NN_CASTLE)
	// nn_set_safe ();
	// rda = nn_swimming;
	// nn_screen_setup (); cd_set_palette ();
	// // DE NADAL: a push when going between the surface and the water
	// if (rda != nn_swimming && on_pant != 0xff) {
	//	if (nn_swimming) player.vy = 461; else player.vy = -614; }
	#asm
			call _nn_clear_apply
			xor a
			ld  (_nn_face_count), a
			ld  (_nn_face_cleared), a
		#ifdef NN_CASTLE
			call _nn_castle_screen
		#endif
			xor a
			ld  hl, _coco_on
			ld  b, NN_COCOS
		.nn_se_c
			ld  (hl), a
			inc hl
			djnz nn_se_c
			ld  hl, _brk_f
			ld  b, NN_BRK_MAX
		.nn_se_b
			ld  (hl), a
			inc hl
			djnz nn_se_b
			ld  (_hit_on), a
			ld  a, NN_BRK_MAX
			ld  (_nn_ci), a
		#ifdef NN_CASTLE
			call _nn_screen_is_safe
			ld  a, l
			or  a
			call nz, _nn_set_safe
		#else
			call _nn_set_safe
		#endif
			ld  a, (_nn_swimming)
			ld  (_rda), a
			call _nn_screen_setup
			call _cd_set_palette
			ld  a, (_nn_swimming)
			ld  b, a
			ld  a, (_rda)
			cp  b
			ret z
			ld  a, (_on_pant)
			cp  0xff
			ret z
			ld  hl, 461
			ld  a, b
			or  a
			jr  nz, nn_se_vy
			ld  hl, -614
		.nn_se_vy
			ld  (_player + 8), hl
	#endasm
}

// ----------------------------------------------------------------------------
// Every frame

void custom_pre_move (void) {
	#asm
			// if (nn_coins != nn_coins_old) {
			//	draw_2_digits (NN_COINS_X, NN_COINS_Y, nn_coins); nn_coins_old = nn_coins; }
			ld  a, (_nn_coins_old)
			ld  b, a
			ld  a, (_nn_coins)
			cp  b
			jr  z, nn_pm_1
			ld  hl, NN_COINS_X
			push hl
			ld  hl, NN_COINS_Y
			push hl
			ld  l, a
			ld  h, 0
			push hl
			call _draw_2_digits
			pop hl
			pop hl
			pop hl
			ld  a, (_nn_coins)
			ld  (_nn_coins_old), a
		.nn_pm_1

			// nn_floor ();
			call _nn_floor

			// a safe place: standing on something solid, not on a conveyor
			// if (player.possee && player.gotten == 0 && nn_sand == 0 && ptgmx == 0) nn_set_safe ();
			ld  a, (_player + 26) 		// possee
			or  a
			jr  z, nn_pm_2
			ld  a, (_player + 25) 		// gotten
			ld  b, a
			ld  a, (_nn_sand)
			or  b
			jr  nz, nn_pm_2
			ld  hl, (_ptgmx)
			ld  a, h
			or  l
			call z, _nn_set_safe
		.nn_pm_2

			// FIRE: a puzzle tile under the feet, or the punch
			// if ((pad_this_frame & sp_FIRE) == 0 || nn_fire_zoned () [NN_CASTLE]) {
			//	if (nn_fire_at_screen () == 0 && hit_on == 0) { hit_on = 1; hit_f = 0; } }
			ld  a, (_pad_this_frame)
			and sp_FIRE
			jr  z, nn_pm_fire
		#ifdef NN_CASTLE
			call _nn_fire_zoned
			ld  a, l
			or  a
			jr  nz, nn_pm_fire
		#endif
			jr  nn_pm_3
		.nn_pm_fire
			call _nn_fire_at_screen
			ld  a, l
			or  a
			jr  nz, nn_pm_3
			ld  a, (_hit_on)
			or  a
			jr  nz, nn_pm_3
			inc a
			ld  (_hit_on), a
			xor a
			ld  (_hit_f), a
		.nn_pm_3

			// nn_springs (); if (hit_on) nn_hitter_do (); nn_breakables_do ();
			call _nn_springs
			ld  a, (_hit_on)
			or  a
			call nz, _nn_hitter_do
			call _nn_breakables_do

		#ifdef NN_BENCH
			// testing: 3 cocos crossing the room all the time, the punch always out
			// for (nn_ci = 0; nn_ci < NN_COCOS; nn_ci ++) if (coco_on [nn_ci] == 0) {
			//	_x = 8; _y = 16 + nn_ci * 40; _t = 2; cocos_add (); }
			// player.estado = EST_PARP; player.ct_estado = 50;
			// if (hit_on == 0) { hit_on = 1; hit_f = 0; }
			xor a
		.nn_pm_bench
			ld  (_nn_ci), a
			ld  hl, _coco_on
			call nn_add_a
			ld  a, (hl)
			or  a
			jr  nz, nn_pm_bn
			ld  a, 8
			ld  (__x), a
			ld  a, (_nn_ci)
			ld  b, a
			add a, a
			add a, a
			add a, b
			add a, a
			add a, a
			add a, a 					// * 40
			add a, 16
			ld  (__y), a
			ld  a, 2
			ld  (__t), a
			call _cocos_add
		.nn_pm_bn
			ld  a, (_nn_ci)
			inc a
			cp  NN_COCOS
			jr  c, nn_pm_bench
			ld  (_nn_ci), a
			ld  a, EST_PARP
			ld  (_player + 23), a
			ld  a, 50
			ld  (_player + 24), a
			ld  a, (_hit_on)
			or  a
			jr  nz, nn_pm_bh
			inc a
			ld  (_hit_on), a
			xor a
			ld  (_hit_f), a
		.nn_pm_bh
		#endif

			// cocos_do ();
			jp  _cocos_do
	#endasm
}

void before_vsync (void) {
}

// ----------------------------------------------------------------------------
// Custom sprites: SP_CUSTOM_BASE + 0-2 cocos, + 3 the punch

#define NN_SW_CUSTOM 		BASE_SPRITES + 16 * SP_CUSTOM_BASE

void custom_sprites_hide (void) {
	// for (nn_ci = 0; nn_ci < MAX_CUSTOM_SW_SPRITES; nn_ci ++) sp_sw [SP_CUSTOM_BASE + nn_ci].sp0 = (int) spr_empty;
	#asm
			ld  hl, NN_SW_CUSTOM
			ld  b, MAX_CUSTOM_SW_SPRITES
		.nn_sh_loop
			ld  de, _spr_empty
			ld  (hl), e
			inc hl
			ld  (hl), d
			ld  de, 15
			add hl, de
			djnz nn_sh_loop
			ld  a, MAX_CUSTOM_SW_SPRITES
			ld  (_nn_ci), a
	#endasm
}

void custom_sprites_create (void) {
	// custom_game_init ();
	// for (nn_ci = 0; nn_ci < MAX_CUSTOM_SW_SPRITES; nn_ci ++) sp_sw [SP_CUSTOM_BASE + nn_ci].sp0 = (int) spr_empty;
	#asm
			call _custom_game_init
			jp  _custom_sprites_hide
	#endasm
}

void custom_sprites_render (void) {
	// for (nn_ci = 0; nn_ci < NN_COCOS; nn_ci ++) {
	//	rda = SP_CUSTOM_BASE + nn_ci;
	//	if (coco_on [nn_ci]) {
	//		sp_sw [rda].cx = coco_x [nn_ci] + VIEWPORT_X * 8;
	//		sp_sw [rda].cy = coco_y [nn_ci] + VIEWPORT_Y * 8;
	//		sp_sw [rda].sp0 = (int) spr_coco;
	//	} else sp_sw [rda].sp0 = (int) spr_empty;
	// }
	// rda = SP_CUSTOM_BASE + 3;
	// if (hit_on) {
	//	sp_sw [rda].cx = hit_x + VIEWPORT_X * 8;
	//	sp_sw [rda].cy = hit_y + VIEWPORT_Y * 8;
	//	sp_sw [rda].sp0 = (int) (player.facing == SIDEVIEW_FACING_LEFT ? spr_punch_l : spr_punch);
	// } else sp_sw [rda].sp0 = (int) spr_empty;
	#asm
			ld  hl, NN_SW_CUSTOM
			ld  c, 0
		.nn_sr_loop
			push hl
			ld  hl, _coco_on
			ld  a, c
			call nn_add_a
			ld  a, (hl)
			pop hl
			or  a
			ld  de, _spr_empty
			jr  z, nn_sr_set
			push hl
			ld  de, 8
			add hl, de
			ex  de, hl 					// DE -> cx
			ld  hl, _coco_x
			ld  a, c
			call nn_add_a
			ld  a, (hl)
			add a, VIEWPORT_X * 8
			ld  (de), a
			inc de
			ld  hl, _coco_y
			ld  a, c
			call nn_add_a
			ld  a, (hl)
			add a, VIEWPORT_Y * 8
			ld  (de), a
			pop hl
			ld  de, _spr_fixed + 9 * 64 	// spr_coco
		.nn_sr_set
			ld  (hl), e
			inc hl
			ld  (hl), d
			ld  de, 15
			add hl, de
			inc c
			ld  a, c
			cp  NN_COCOS
			jr  c, nn_sr_loop
			ld  (_nn_ci), a

			// the punch (HL -> its slot, SP_CUSTOM_BASE + 3)
			ld  a, 3 + SP_CUSTOM_BASE
			ld  (_rda), a
			ld  a, (_hit_on)
			or  a
			ld  de, _spr_empty
			jr  z, nn_sr_pset
			push hl
			ld  de, 8
			add hl, de
			ld  a, (_hit_x)
			add a, VIEWPORT_X * 8
			ld  (hl), a
			inc hl
			ld  a, (_hit_y)
			add a, VIEWPORT_Y * 8
			ld  (hl), a
			pop hl
			ld  de, _spr_fixed + 10 * 64 	// spr_punch (facing right)
			ld  a, (_player + 22) 		// facing
			cp  SIDEVIEW_FACING_LEFT
			jr  nz, nn_sr_pset
			ld  de, _spr_fixed + 11 * 64 	// spr_punch_l
		.nn_sr_pset
			ld  (hl), e
			inc hl
			ld  (hl), d
	#endasm
}

void custom_clear_tile (void) {
	// an opened lock: the level's background
	// _t = tileset_mappings [0]; _n = comportamiento_tiles [_t];
	#asm
			ld  hl, (_tileset_mappings)
			ld  a, (hl)
			ld  (__t), a
			ld  hl, _comportamiento_tiles
			call nn_add_a
			ld  a, (hl)
			ld  (__n), a
	#endasm
}
