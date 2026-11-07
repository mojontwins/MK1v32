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
// No locals, no parameters: z88dk 1.10 likes globals.

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

char nn_level_text [] = "LEVEL 1";

// Coins (tile beh 128): 50 = one more life
unsigned char nn_coins, nn_coins_old;
#define NN_COINS_X 			29
#define NN_COINS_Y 			0
#define NN_COINS_LIFE 		50

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
	nn_p = tile_map;
	for (nn_ci = 64; nn_ci; nn_ci ++) {
		nn_q = (unsigned char *) (CD_PAT + (nn_ci << 4));
		for (nn_cj = 0; nn_cj < 16; nn_cj ++) if (nn_q [nn_cj] != ((unsigned char *) CD_PAT) [nn_cj]) break;
		*nn_p ++ = (nn_cj == 16) ? 0 : nn_ci;
	}
}

// ----------------------------------------------------------------------------
// Vertical engines

void player_custom_veng (void) {
	if (nn_swimming) {
		// NES: DOWN (or FIRE) sinks, accelerating; otherwise float up slowly
		if ((pad0 & sp_DOWN) == 0) {
			player.vy += PLAYER_AY_SWIM;
			if (player.vy > PLAYER_VY_SWIM_MAX) player.vy = PLAYER_VY_SWIM_MAX;
		} else {
			player.vy -= PLAYER_AY_SWIM >> 1;
			if ((pad0 & sp_UP) == 0) player.vy -= PLAYER_AY_SWIM;
			if (player.vy < -PLAYER_VY_SWIM_MAX) player.vy = -PLAYER_VY_SWIM_MAX;
		}
		return;
	}

	// Gravity, lighter while the jump is held
	player.vy += player.saltando ? PLAYER_G_JUMPING : PLAYER_G;
	if (player.vy > PLAYER_MAX_VY_CAYENDO) player.vy = PLAYER_MAX_VY_CAYENDO;

	// Quicksand: sinking slowly, a jump gets out (but no long jump)
	if (nn_sand) {
		if (player.vy < 0) player.saltando = 0;
		else player.vy = NN_VY_SINKING;
	}

	// Jump start
	if ((pad_this_frame & sp_UP) == 0 && player.saltando == 0 && (player.possee || player.gotten || nn_sand)) {
		player.cont_salto = 0;
		player.saltando = 1;
		player.just_jumped = 1;
		player.vy = -PLAYER_VY_JUMP_INITIAL;
		peta_el_beeper (3);
	}

	if ((pad0 & sp_UP) == 0) {
		if (player.saltando) {
			player.cont_salto ++;
			if (player.cont_salto == PLAYER_VY_JUMP_STEPS) player.saltando = 0;
		}
	} else if (player.saltando) {
		// released: cut the jump
		if (player.vy < -PLAYER_VY_JUMP_RELEASE) player.vy = -PLAYER_VY_JUMP_RELEASE;
		player.saltando = 0;
	}
}

// ----------------------------------------------------------------------------
// Frames: see custom_sprite_cells.h

unsigned char nn_walk [] = { 0, 1, 2, 1 };

unsigned char player_custom_frame (void) {
	rda = (player.facing == SIDEVIEW_FACING_LEFT) ? 4 : 0;
	if (nn_swimming) {
		rdb = ((pad0 & (sp_LEFT | sp_RIGHT | sp_UP | sp_DOWN)) != (sp_LEFT | sp_RIGHT | sp_UP | sp_DOWN)) ?
			nn_walk [(maincounter >> 1) & 3] : 1;
		return NN_CELL_SWIM + (rda ? 3 : 0) + rdb;
	}
	if (player.possee || player.gotten) {
		rdb = player.vx ? nn_walk [(gpx >> 3) & 3] : 1;
		return rda + rdb;
	}
	return rda + 3;
}

// ----------------------------------------------------------------------------
// Levels

void nn_screen_setup (void) {
	// What the NES sets per screen: the engine, the decoration, the inks
	nn_swimming = nn_swim [nn_l];
	nn_alt_bg = nn_alt_bg_l [nn_l];
	nn_stalactites = nn_stalac [nn_l];
	cd_pal_bg = nn_pal [nn_l];
	if (nn_l == 3) {
		// DE NADAL 1: screen 7 is the surface, the rest is under water
		if (n_pant == 7) { nn_swimming = 0; nn_alt_bg = 34; cd_pal_bg = nn_pal_surface; }
		else nn_alt_bg = 42;
	} else if (nn_l == 6) {
		// DE NADAL 2: screens 0 and 1 are the surface
		if (n_pant < 2) { nn_swimming = 0; nn_alt_bg = 34; cd_pal_bg = nn_pal_surface; }
		else nn_alt_bg = 42;
	} else if (nn_l == 9) {
		// EL CASTLER: bushes outdoors only
		nn_alt_bg = ((n_pant & 1) == 0 && n_pant < 16) ? 34 : 0;
	} else if (nn_l == 7) {
		// MARSHY GOO: stalactites in the lower half
		nn_stalactites = (n_pant >= 10);
	}
}

void custom_level_setup (void) {
	#ifdef NN_DEBUG_LEVEL
		if (level_cont == 0) level = NN_DEBUG_LEVEL;		// testing: -DNN_DEBUG_LEVEL=n
	#else
		if (level_cont == 0) level = 0;
	#endif
	nn_l = NN_FIRST + level;

	// the level area: locks, enemies, hotspots, map (nn_load.h)
	level_src = lvl_area [level];
	#asm
			ld  hl, (_level_src)
			ld  de, _level_area
			call dzx0_standard
	#endasm
	// the level's sprite cells
	level_src = lvl_spr [level];
	#asm
			ld  hl, (_level_src)
			ld  de, NN_SPR_BASE
			call dzx0_standard
	#endasm

	tileset_mappings = nn_tsmaps + (nn_l << 4);
	map_w = nn_map_w [nn_l];
	level_scr_ini = nn_scr_ini [nn_l];
	level_ini_x = nn_ini_x [nn_l];
	level_ini_y = nn_ini_y [nn_l];

	// enemy speeds: NES pixels per 60 Hz frame -> 25 Hz
	for (nn_ci = 0; nn_ci < MAP_W * MAP_H * 3; nn_ci ++) {
		malotes [nn_ci].mx <<= 1; malotes [nn_ci].my <<= 1;
	}
	// nothing broken yet
	for (nn_p = (unsigned char *) NN_CLEAR; nn_p < (unsigned char *) (NN_CLEAR + MAP_W * MAP_H * 19); nn_p ++) *nn_p = 0;

	n_pant = level_scr_ini;
	nn_screen_setup ();
	cd_set_palette ();

	nn_coins_old = 0xff;

	// LEVEL n and its name, for a moment
	nn_level_text [6] = '1' + nn_l;
	nn_p = nn_names [nn_l];
	for (nn_ci = 0; nn_p [nn_ci]; nn_ci ++);
	draw_text (12, 10, nn_level_text);
	draw_text (16 - (nn_ci >> 1), 13, nn_p);
	cd_update (0);
	espera_activa (150);
}

unsigned char custom_level_won (void) {
	nn_saved_life = player.life;
	if (level + 1 < NN_LEVELS) {
		level ++;
		level_cont = 1;
		return 1;
	}
	level_cont = 0;
	return 0;
}

void custom_game_start (void) {
	if (level_cont == 0) nn_coins = 0; else player.life = nn_saved_life;
	player.facing = nn_facing [nn_l] ? SIDEVIEW_FACING_RIGHT : SIDEVIEW_FACING_LEFT;
}

void custom_screen_enter (void) {
	nn_clear_apply ();
	nn_face_count = nn_face_cleared = 0;
	#ifdef NN_CASTLE
		nn_castle_screen ();
	#endif
	for (nn_ci = 0; nn_ci < NN_COCOS; nn_ci ++) coco_on [nn_ci] = 0;
	for (nn_ci = 0; nn_ci < NN_BRK_MAX; nn_ci ++) brk_f [nn_ci] = 0;
	hit_on = 0;
	#ifdef NN_CASTLE
		if (nn_screen_is_safe ())
	#endif
	nn_set_safe ();

	rda = nn_swimming;
	nn_screen_setup ();
	cd_set_palette ();
	// DE NADAL: a push when going between the surface and the water
	if (rda != nn_swimming && on_pant != 0xff) {
		if (nn_swimming) player.vy = 461; else player.vy = -614;
	}
}

// ----------------------------------------------------------------------------
// Every frame

void custom_pre_move (void) {
	if (nn_coins != nn_coins_old) {
		draw_2_digits (NN_COINS_X, NN_COINS_Y, nn_coins);
		nn_coins_old = nn_coins;
	}

	nn_floor ();

	// a safe place: standing on something solid, not on a conveyor
	if (player.possee && player.gotten == 0 && nn_sand == 0 && ptgmx == 0) nn_set_safe ();

	// FIRE: a puzzle tile under the feet, or the punch
	if ((pad_this_frame & sp_FIRE) == 0
		#ifdef NN_CASTLE
			|| nn_fire_zoned ()
		#endif
	) {
		if (nn_fire_at_screen () == 0 && hit_on == 0) { hit_on = 1; hit_f = 0; }
	}
	nn_springs ();
	if (hit_on) nn_hitter_do ();

	nn_breakables_do ();
	#ifdef NN_BENCH
		// testing: 3 cocos crossing the room all the time, the punch always out
		for (nn_ci = 0; nn_ci < NN_COCOS; nn_ci ++) if (coco_on [nn_ci] == 0) {
			_x = 8; _y = 16 + nn_ci * 40; _t = 2; cocos_add ();
		}
		player.estado = EST_PARP; player.ct_estado = 50;
		if (hit_on == 0) { hit_on = 1; hit_f = 0; }
	#endif
	cocos_do ();
}

void before_vsync (void) {
}

// ----------------------------------------------------------------------------
// Custom sprites: SP_CUSTOM_BASE + 0-2 cocos, + 3 the punch

void custom_sprites_create (void) {
	custom_game_init ();
	for (nn_ci = 0; nn_ci < MAX_CUSTOM_SW_SPRITES; nn_ci ++) sp_sw [SP_CUSTOM_BASE + nn_ci].sp0 = (int) spr_empty;
}

void custom_sprites_render (void) {
	for (nn_ci = 0; nn_ci < NN_COCOS; nn_ci ++) {
		rda = SP_CUSTOM_BASE + nn_ci;
		if (coco_on [nn_ci]) {
			sp_sw [rda].cx = coco_x [nn_ci] + VIEWPORT_X * 8;
			sp_sw [rda].cy = coco_y [nn_ci] + VIEWPORT_Y * 8;
			sp_sw [rda].sp0 = (int) spr_coco;
		} else sp_sw [rda].sp0 = (int) spr_empty;
	}
	rda = SP_CUSTOM_BASE + 3;
	if (hit_on) {
		sp_sw [rda].cx = hit_x + VIEWPORT_X * 8;
		sp_sw [rda].cy = hit_y + VIEWPORT_Y * 8;
		sp_sw [rda].sp0 = (int) spr_punch;
	} else sp_sw [rda].sp0 = (int) spr_empty;
}

void custom_sprites_hide (void) {
	for (nn_ci = 0; nn_ci < MAX_CUSTOM_SW_SPRITES; nn_ci ++) sp_sw [SP_CUSTOM_BASE + nn_ci].sp0 = (int) spr_empty;
}

void custom_clear_tile (void) {
	// an opened lock: the level's background
	_t = tileset_mappings [0]; _n = comportamiento_tiles [_t];
}
