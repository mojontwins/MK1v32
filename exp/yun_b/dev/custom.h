// MTE MK1 v3.2
// Copyleft 2011, 2026 by The Mojon Twins

// Yun (ZX prototype) - custom.h
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
//  * Fanty (type 5, Yun 3x): flies after Yun, the NES precalc fanty with its
//    increment table resampled to the ZX rate (8 bit, one step per frame). Monococo (type 6, Yun 9x): hides in a bush, comes out,
//    shoots one aimed coco, hides again. Both can be trapped.
//
// No locals, no parameters: z88dk 1.10 likes globals.

#include "yun_luts.h"
#include "yun_load.h"

#define BUBS_MAX 			3
#define BUB_COOLDOWN 		2		// frames between bubbles
#define BUB_FALL 			4		// px/frame a trapped enemy falls

// Engine functions defined after this file is included
unsigned char attr (unsigned char x, unsigned char y);
unsigned char collide_enem (void);
void enems_kill (unsigned char damage);
unsigned char rand (void);

// Bubbles
#ifndef CPC
	struct sp_SS *sp_bub [BUBS_MAX];
#endif
unsigned char *bub_cur [BUBS_MAX];
unsigned char bub_ct [BUBS_MAX];		// life, 0 = free
unsigned char bub_x [BUBS_MAX], bub_y [BUBS_MAX];
unsigned char bub_ix [BUBS_MAX], bub_iy [BUBS_MAX];
unsigned char bub_state [BUBS_MAX];		// 0 rising, 1 bobbing at the top
unsigned char bub_fc [BUBS_MAX];		// 0 right, 1 left
signed char bub_mx [BUBS_MAX];			// last horizontal displacement
unsigned char bub_lock [BUBS_MAX];		// enemy index or 0xff
unsigned char bub_shown [BUBS_MAX];

unsigned char bub_cool, bub_ride = 0xff;
unsigned char bi, bj, bt;
signed int bsi;
int bub_off;
unsigned char *bub_ptr, *bub_next;

// Trapped enemies
unsigned char en_cap [MAX_ENEMS];		// 0 or bubble index + 1
unsigned char cap_x [MAX_ENEMS], cap_y [MAX_ENEMS];


// Levels
unsigned char level, level_cont, saved_life;
unsigned char level_scr_ini, level_ini_x, level_ini_y, level_kills;
unsigned char *level_src;

// Fanty / monococo
unsigned char fi, mc_phase;

#ifdef YUN_EXTRA_ENEMS
	// pezon, gyrosaw, cannon: counter and state per enemy of the screen
	unsigned char en_ct [MAX_ENEMS], en_st [MAX_ENEMS];
	signed char gdelta;
	unsigned char cdir;
	unsigned char cannon_wait (void);
#endif

#ifdef YUN_DARK_LEVEL
	// Dark rooms (NES level 8): lit once their bulb is taken, flashes of lightning
	unsigned char lit [MAX_SCREENS];
	unsigned char scr_dark, lightning;
	void dark_set (unsigned char d);
#endif

// Coco (one at a time)
#ifndef CPC
	struct sp_SS *sp_coco;
#endif
unsigned char coco_on, coco_shown, crx, cry;
signed int coco_x, coco_y, coco_vx, coco_vy;
unsigned int cdist;
unsigned char cda, cdb;

// ----------------------------------------------------------------------------
// Bubbles

#ifdef CPC
void custom_sprites_create (void) {
	for (bi = 0; bi < BUBS_MAX; bi ++) {
		bub_ct [bi] = 0;
		bub_lock [bi] = 0xff;
	}
	coco_on = 0;
}
#else
void custom_sprites_create (void) {
	for (bi = 0; bi < BUBS_MAX; bi ++) {
		sp_bub [bi] = sp_CreateSpr (NO_MASKS, 3, sprite_9_a, 2);
		sp_AddColSpr (sp_bub [bi], sprite_9_b);
		sp_AddColSpr (sp_bub [bi], sprite_9_b);	// dummy, nulled by fix_sprites
		bub_cur [bi] = sprite_9_a;
		bub_ptr = (unsigned char *) sp_bub [bi];
		#asm
				ld  hl, (_bub_ptr)
				ld  b, 6
				call _fix_sprites
		#endasm
		bub_ct [bi] = 0;
		bub_lock [bi] = 0xff;
		bub_shown [bi] = 1;		// so the first render moves it out of the way
	}
	sp_coco = sp_CreateSpr (NO_MASKS, 3, sprite_21_a, 2);
	sp_AddColSpr (sp_coco, sprite_21_b);
	sp_AddColSpr (sp_coco, sprite_21_b);
	bub_ptr = (unsigned char *) sp_coco;
	#asm
			ld  hl, (_bub_ptr)
			ld  b, 6
			call _fix_sprites
	#endasm
	coco_on = 0;
	coco_shown = 1;
}
#endif

// ----------------------------------------------------------------------------
// Coco

void coco_new (void) {
	// from (rdx, rdy) towards Yun, NES simplecoco_aimed_new
	if (coco_on) return;
	cda = rdx > gpx ? rdx - gpx : gpx - rdx;
	cdb = rdy > gpy ? rdy - gpy : gpy - rdy;
	rda = cda < cdb ? cda : cdb;
	cdist = cda + cdb - (rda >> 1) - (rda >> 2) + (rda >> 4);
	if (cdist == 0) return;
	coco_x = rdx << 4; coco_y = rdy << 4;
	coco_vx = (COCO_V * cda) / cdist; if (gpx < rdx) coco_vx = -coco_vx;
	coco_vy = (COCO_V * cdb) / cdist; if (gpy < rdy) coco_vy = -coco_vy;
	coco_on = 1;
}

#ifdef YUN_EXTRA_ENEMS
	void coco_straight (void) {
		// from (rdx, rdy), direction rda: 0 left, 1 up, 2 right, 3 down
		if (coco_on) return;
		coco_x = rdx << 4; coco_y = rdy << 4;
		coco_vx = coco_vy = 0;
		switch (rda) {
			case 0: coco_vx = -COCO_V; break;
			case 1: coco_vy = -COCO_V; break;
			case 2: coco_vx = COCO_V; break;
			default: coco_vy = COCO_V;
		}
		coco_on = 1;
	}
#endif

void coco_move (void) {
	if (coco_on == 0) return;
	coco_x += coco_vx; coco_y += coco_vy;
	if (coco_x < 0 || coco_x > (232 << 4) || coco_y < 0 || coco_y > (152 << 4)) { coco_on = 0; return; }
	crx = coco_x >> 4; cry = coco_y >> 4;
	// the coco is 8x8 at (crx + 4, cry + 4) in its cell
	if (crx + 12 >= gpx + 4 && crx + 4 <= gpx + 11 && cry + 12 >= gpy + 2 && cry + 4 <= gpy + 15) {
		coco_on = 0;
		if (player.estado == EST_NORMAL) {
			player.drain_amount = 1;
			player.is_dead = PLAYER_KILLED_BY_ENEM;
			peta_el_beeper (2);
		}
	}
}

void bubble_new (void) {
	for (bi = 0; bi < BUBS_MAX; bi ++) if (bub_ct [bi] == 0) break;
	if (bi == BUBS_MAX) return;

	bub_ct [bi] = BUB_LIFE;
	bub_lock [bi] = 0xff;
	bub_fc [bi] = player.facing ? 1 : 0;
	bub_x [bi] = gpx;
	bub_y [bi] = gpy;
	bub_ix [bi] = 0;
	bub_iy [bi] = 0;
	bub_state [bi] = 0;
	bub_mx [bi] = 0;
	bub_cool = BUB_COOLDOWN;
	peta_el_beeper (5);
}

void bubbles_move (void) {
	for (bi = 0; bi < BUBS_MAX; bi ++) {
		if (bub_ct [bi] == 0) continue;

		bub_ct [bi] --;
		if (bub_ct [bi] == 0) {
			// Pop! Free whatever was inside.
			if (bub_lock [bi] != 0xff) {
				en_cap [bub_lock [bi]] = 0;
				bub_lock [bi] = 0xff;
			}
			if (bub_ride == bi) bub_ride = 0xff;
			continue;
		}

		// Trapped enemies move their bubble (extra_enems_move)
		if (bub_lock [bi] != 0xff) continue;

		// Horizontal: decelerating, bounce on screen edges
		bt = bub_dx [bub_ix [bi]];
		if (bub_ix [bi] < BUB_DX_MAX) bub_ix [bi] ++;
		bsi = bub_x [bi];
		if (bub_fc [bi]) bsi -= bt; else bsi += bt;
		if (bsi < 0) { bsi = 0; bub_fc [bi] = 0; }
		else if (bsi > 224) { bsi = 224; bub_fc [bi] = 1; }
		bub_mx [bi] = bsi - bub_x [bi];
		bub_x [bi] = bsi;

		// Vertical: rise faster and faster, then bob at the top
		bsi = bub_y [bi];
		if (bub_state [bi] == 0) {
			bsi -= bub_dy [bub_iy [bi]];
			if (bub_iy [bi] < BUB_DY_MAX) bub_iy [bi] ++;
			if (bsi < 0) {
				bsi = 0;
				bub_state [bi] = 1;
				bub_iy [bi] = 0;
			}
		} else {
			bsi += bub_bob [bub_iy [bi]];
			bub_iy [bi] ++;
			if (bub_iy [bi] == BUB_BOB_MAX) {
				bub_state [bi] = 0;
				bub_iy [bi] = 0;
			}
		}
		bub_y [bi] = bsi;
	}
}

void bubble_ride (void) {
	if (bub_ride != 0xff) {
		bi = bub_ride;
		// Still on it? Jumping, being stopped by a wall or ceiling drops her.
		if (bub_ct [bi] == 0 || bub_lock [bi] != 0xff || player.saltando
			|| gpx + 14 < bub_x [bi] || gpx > bub_x [bi] + 14
			|| gpy + 20 < bub_y [bi]) {
			bub_ride = 0xff;
			return;
		}
	} else {
		// Land on a free bubble (only while falling or standing)
		if (player.vy < 0) return;
		for (bi = 0; bi < BUBS_MAX; bi ++) {
			if (bub_ct [bi] && bub_lock [bi] == 0xff && bub_y [bi] > 16
				&& gpx + 12 >= bub_x [bi] && gpx <= bub_x [bi] + 12
				&& gpy + 16 >= bub_y [bi] && gpy + 6 <= bub_y [bi]) break;	// NES uses a 5 px window at 1-2 px/frame; here bubbles rise up to 7 px/frame
		}
		if (bi == BUBS_MAX) return;
		bub_ride = bi;
	}

	// Carry her. This runs at the start of move (): the engine adds gravity to vy, then
	// applies vy and ptgmx with collisions, and flicks the screen up when gpy
	// hits 0 with vy < 0. gotten lets her jump off.
	bsi = bub_y [bi];
	bsi = bsi - 16 - gpy;
	player.vy = bsi * 64 - PLAYER_G;
	ptgmx = bub_mx [bi] * 64;
	player.gotten = 1;
}

// ----------------------------------------------------------------------------
// Hooks

#ifdef CPC
// Bubbles and the coco are sp_sw [SP_CUSTOM_BASE + n]; bubbles byte-aligned
void custom_sprites_render (void) {
	for (bi = 0; bi < BUBS_MAX; bi ++) {
		bt = SP_CUSTOM_BASE + bi;
		if (bub_ct [bi] && (bub_ct [bi] > BUB_BLINK || half_life)) {
			sp_sw [bt].cx = ((bub_x [bi] + VIEWPORT_X * 8) >> 1) & 0xfe;
			sp_sw [bt].cy = bub_y [bi] + VIEWPORT_Y * 8;
			sp_sw [bt].sp0 = (int) ((maincounter & 2) ? sprite_10_a : sprite_9_a);
		} else sp_sw [bt].sp0 = (int) spr_empty;
	}
	bt = SP_CUSTOM_BASE + BUBS_MAX;
	if (coco_on) {
		sp_sw [bt].cx = (crx + VIEWPORT_X * 8) >> 1;
		sp_sw [bt].cy = cry + VIEWPORT_Y * 8;
		sp_sw [bt].sp0 = (int) sprite_21_a;
	} else sp_sw [bt].sp0 = (int) spr_empty;
}

void custom_sprites_hide (void) {
	for (bi = 0; bi <= BUBS_MAX; bi ++) sp_sw [SP_CUSTOM_BASE + bi].sp0 = (int) spr_empty;
}
#else
void custom_sprites_render (void) {

	for (bi = 0; bi < BUBS_MAX; bi ++) {
		bub_ptr = (unsigned char *) sp_bub [bi];
		if (bub_ct [bi] && (bub_ct [bi] > BUB_BLINK || half_life)) {
			rdx = bub_x [bi]; rdy = bub_y [bi];
			bub_next = (maincounter & 2) ? sprite_10_a : sprite_9_a;
			bub_off = (int) bub_next - (int) bub_cur [bi];
			bub_cur [bi] = bub_next;
			#asm
					ld  ix, (_bub_ptr)
					ld  iy, vpClipStruct
					ld  bc, (_bub_off)

					ld  a, (_rdy)
					srl a
					srl a
					srl a
					add VIEWPORT_Y
					ld  h, a

					ld  a, (_rdx)
					srl a
					srl a
					srl a
					add VIEWPORT_X
					ld  l, a

					ld  a, (_rdx)
					and 7
					ld  d, a

					ld  a, (_rdy)
					and 7
					ld  e, a

					call SPMoveSprAbs
			#endasm
			bub_shown [bi] = 1;
		} else if (bub_shown [bi]) {
			#asm
					ld  ix, (_bub_ptr)
					ld  iy, vpClipStruct
					ld  bc, 0
					ld  hl, 0xfefe
					ld  de, 0
					call SPMoveSprAbs
			#endasm
			bub_shown [bi] = 0;
		}
	}

	bub_ptr = (unsigned char *) sp_coco;
	if (coco_on) {
		rdx = crx; rdy = cry;
		#asm
				ld  ix, (_bub_ptr)
				ld  iy, vpClipStruct
				ld  bc, 0
				ld  a, (_rdy)
				srl a
				srl a
				srl a
				add VIEWPORT_Y
				ld  h, a
				ld  a, (_rdx)
				srl a
				srl a
				srl a
				add VIEWPORT_X
				ld  l, a
				ld  a, (_rdx)
				and 7
				ld  d, a
				ld  a, (_rdy)
				and 7
				ld  e, a
				call SPMoveSprAbs
		#endasm
		coco_shown = 1;
	} else if (coco_shown) {
		#asm
				ld  ix, (_bub_ptr)
				ld  iy, vpClipStruct
				ld  bc, 0
				ld  hl, 0xfefe
				ld  de, 0
				call SPMoveSprAbs
		#endasm
		coco_shown = 0;
	}
}

void custom_sprites_hide (void) {
	for (bi = 0; bi < BUBS_MAX; bi ++) {
		bub_ptr = (unsigned char *) sp_bub [bi];
		#asm
				ld  ix, (_bub_ptr)
				ld  iy, vpClipStruct
				ld  bc, 0
				ld  hl, 0xfefe
				ld  de, 0
				call SPMoveSprAbs
		#endasm
		bub_shown [bi] = 0;
	}
	bub_ptr = (unsigned char *) sp_coco;
	#asm
			ld  ix, (_bub_ptr)
			ld  iy, vpClipStruct
			ld  bc, 0
			ld  hl, 0xfefe
			ld  de, 0
			call SPMoveSprAbs
	#endasm
	coco_shown = 0;
}
#endif

void before_vsync (void) {
}

void custom_game_start (void) {
	for (bi = 0; bi < BUBS_MAX; bi ++) {
		bub_ct [bi] = 0;
		bub_lock [bi] = 0xff;
	}
	bub_ride = 0xff;
	bub_cool = 0;
	coco_on = 0;
	if (level_cont) {
		player.life = saved_life;
		level_cont = 0;
	}
}

// ----------------------------------------------------------------------------
// Levels

void custom_level_setup (void) {
	if (level_cont == 0) level = 0;

	// the level area: map + locks, enemies + hotspots, tile map (yun_level.h)
	level_src = lvl_area [level];
	#asm
			ld  hl, (_level_src)
			ld  de, _level_area
			call dzx0_standard
	#endasm
	// the level's enemy sprite cells
	level_src = lvl_spr [level];
	#asm
			ld  hl, (_level_src)
			ld  de, SPR_VAR_BASE
			call dzx0_standard
	#endasm
	// tile behaviours
	level_src = lvl_behs + (level << 4);
	#asm
			ld  hl, (_level_src)
			ld  de, _comportamiento_tiles
			ld  bc, 16
			ldir
	#endasm

	#ifdef VARIABLE_MAP_W
		map_w = lvl_map_w [level];
	#endif
	#ifdef YUN_DARK_LEVEL
		for (bi = 0; bi < MAX_SCREENS; bi ++) lit [bi] = 0;
		lightning = 0;
		scr_dark = 0;
	#endif

	level_scr_ini = lvl_scr_ini [level];
	level_ini_x = lvl_ini_x [level];
	level_ini_y = lvl_ini_y [level];
	level_kills = lvl_kills [level];

	#ifdef CPC
		// split palette: the level's background inks, the sprite inks
		cd_pal_bg = lvl_pal [level];
		cd_pal_spr = spr_pal;
		cd_set_palette ();
	#endif

	// kills to win, after the '/' of the frame
	draw_2_digits (HUD_TOTAL_X, 0, level_kills);
}

unsigned char custom_level_won (void) {
	if (level + 1 < YUN_LEVELS) {
		level ++;
		level_cont = 1;
		saved_life = player.life;
		return 1;
	}
	level_cont = 0;
	return 0;
}

#if defined YUN_DARK_LEVEL && !defined CPC
	#ifndef HOTSPOTS_FIRST_TILE
		#define HOTSPOTS_FIRST_TILE 16
	#endif
	void redraw_room (void) {
		// the room again, from map_buff, with the current tile_attr_and / _or
		gpit = 0;
		for (cdb = 0; cdb < 20; cdb += 2) for (cda = 0; cda < 30; cda += 2) {
			draw_coloured_tile (VIEWPORT_X + cda, VIEWPORT_Y + cdb, map_buff [gpit ++]);
		}
		if (hotspot_y != 240) draw_coloured_tile (VIEWPORT_X + (hotspot_x >> 3), VIEWPORT_Y + (hotspot_y >> 3),
			hotspot_t_r == 3 ? HOTSPOTS_FIRST_TILE : HOTSPOTS_FIRST_TILE + hotspot_t_r);
	}
#endif

#ifdef YUN_DARK_LEVEL
	void dark_set (unsigned char d) {
		// ZX: the room's colours go (DARK_ATTR) or come back; CPC: the background inks
		#ifdef CPC
			cd_pal_bg = d ? (d == 2 ? light_pal : dark_pal) : lvl_pal [level];
			cd_set_palette ();
		#else
			if (d == 1) { tile_attr_and = 0; tile_attr_or = DARK_ATTR; }
			else { tile_attr_and = 0xff; tile_attr_or = 0; }
			redraw_room ();
		#endif
		scr_dark = d;
	}
#endif

void custom_screen_enter (void) {
	#ifdef YUN_DARK_LEVEL
		lightning = 0;
		rda = (level == YUN_DARK_LEVEL && lit [n_pant] == 0);
		if (rda != scr_dark) dark_set (rda);
	#endif

	// Bubbles burst and trapped enemies are reset...
	for (bi = 0; bi < BUBS_MAX; bi ++) {
		bub_ct [bi] = 0;
		bub_lock [bi] = 0xff;
	}
	for (bi = 0; bi < MAX_ENEMS; bi ++) en_cap [bi] = 0;
	coco_on = 0;

	// ...unless Yun rode a bubble here: give her a new one (NES does this).
	if (bub_ride != 0xff) {
		if (gpy >= 16) { gpy -= 16; player.y = gpy * 64; }
		bub_ct [0] = BUB_LIFE;
		bub_x [0] = gpx;
		bub_y [0] = gpy + 16;
		bub_ix [0] = BUB_DX_MAX;
		bub_iy [0] = BUB_DY_MAX;
		bub_state [0] = 0;
		bub_fc [0] = player.facing ? 1 : 0;
		bub_mx [0] = 0;
		bub_ride = 0;
	}
}

void custom_pre_move (void) {
	coco_move ();
	bubbles_move ();

	if (bub_cool) bub_cool --;
	if ((pad_this_frame & sp_FIRE) == 0 && bub_cool == 0) bubble_new ();

	bubble_ride ();

	// Conveyors (NES CONVEYOR_BIT 32, the evil bit means rightwards)
	if (player.possee) {
		cda = attr ((gpx + 4) >> 4, (gpy + 16) >> 4);
		cdb = attr ((gpx + 11) >> 4, (gpy + 16) >> 4);
		if ((cda | cdb) & 32) {
			if ((cda & 32) == 0) cda = cdb;
			if (cda & 1) {
				if (gpx < 224 && (attr ((gpx + 16) >> 4, (gpy + 8) >> 4) & 8) == 0) player.x += CONVEYOR_V;
			} else {
				if (gpx > 0 && (attr ((gpx - 1) >> 4, (gpy + 8) >> 4) & 8) == 0) player.x -= CONVEYOR_V;
			}
		}
	}

	#ifdef YUN_DARK_LEVEL
		// Lightning in the dark rooms
		if (level == YUN_DARK_LEVEL && lit [n_pant] == 0) {
			if (lightning) {
				lightning --;
				if (lightning == 0) dark_set (1);
			} else if ((rand () & 31) == 1) {
				lightning = 3;
				dark_set (2);
				peta_el_beeper (5);
			}
		}
	#endif
}

#ifdef PLAYER_CUSTOM_HENG
	void player_custom_heng (void) {
	}
#endif

#ifdef PLAYER_CUSTOM_FRAME
	unsigned char player_custom_frame (void) {
		return 0;
	}
#endif

#ifdef PLAYER_CUSTOM_BG_HIT
	unsigned char custom_bg_hit (void) {
		return 0;
	}
#endif

#ifdef ENABLE_CUSTOM_ENEMS

	void extra_enems_init (void) {
		#ifdef YUN_EXTRA_ENEMS
			en_ct [enit] = en_st [enit] = 0;
			switch (_en_t) {
				case 7:		// pezon: waits at (x1, y1)
					_en_x = _en_x1; _en_y = _en_y1;
					en_ct [enit] = PZ_WAIT;
					break;
				case 9:		// gyrosaw: from (x1, y1)
					_en_x = _en_x1; _en_y = _en_y1;
					break;
				case 10:	// cannon: first shot after a full wait
					en_ct [enit] = cannon_wait ();
					break;
			}
		#endif
	}

	#ifdef YUN_EXTRA_ENEMS
		unsigned char cannon_wait (void) {
			// seconds in the path's speed (mx or my), direction in its sign
			rda = _en_mx ? _en_mx : _en_my;
			if (rda & 0x80) rda = -rda;
			return rda * CANNON_SECOND;
		}

		void pezon_move (void) {
			// NES precalc pezon: wait at y1, jump (pz_y), back to y1
			if (en_st [enit] == 0) {
				if (en_ct [enit]) en_ct [enit] --; else en_st [enit] = 1;
			} else {
				rda = en_st [enit] - 1;
				rdb = -pz_y [rda];
				_en_y = _en_y1 > rdb ? _en_y1 - rdb : 0;
				if (rda == PZ_N - 1) {
					en_st [enit] = 0;
					en_ct [enit] = PZ_WAIT;
					_en_y = _en_y1;
				} else en_st [enit] ++;
			}
			en_an_next_frame [enit] = sprite_25_a;
		}

		void gyrosaw_move (void) {
			// NES gyrosaw: around a 32x32 square, clockwise if mx != 0 (state in en_st)
			rda = en_st [enit];
			gdelta = (_en_mx ? (rda >> 1) : (((rda + 1) & 3) >> 1)) ? GYRO_V : -GYRO_V;
			if (rda & 1) _en_y += gdelta; else _en_x += gdelta;
			en_ct [enit] = (en_ct [enit] + GYRO_V) & 31;
			if (en_ct [enit] == 0) en_st [enit] = (rda + 1) & 3;
			en_an_next_frame [enit] = (maincounter & 1) ? sprite_28_a : sprite_27_a;
		}

		void cannon_move (void) {
			// NES steady shooter: shoots straight every few seconds
			if (_en_mx) cdir = (_en_mx & 0x80) ? 0 : 2;
			else cdir = (_en_my & 0x80) ? 1 : 3;
			if (en_ct [enit]) en_ct [enit] --; else {
				en_ct [enit] = cannon_wait ();
				rdx = _en_x; rdy = _en_y; rda = cdir;
				coco_straight ();
				peta_el_beeper (4);
			}
			switch (cdir) {
				case 0: en_an_next_frame [enit] = sprite_29_a; break;
				case 1: en_an_next_frame [enit] = sprite_30_a; break;
				case 2: en_an_next_frame [enit] = sprite_31_a; break;
				default: en_an_next_frame [enit] = sprite_32_a;
			}
		}
	#endif

	void fanty_move (void) {
		// NES precalc fanty at the ZX rate: speed index -FZ_MAX..FZ_MAX in
		// _en_mx / _en_my, one step per frame, displacements from fz_incs.
		// 8 bits all the way.
		if (gpx > _en_x) { if (_en_mx < FZ_MAX) _en_mx ++; }
		else if (gpx < _en_x) { if (_en_mx > -FZ_MAX) _en_mx --; }
		if (_en_mx < 0) {
			fi = fz_incs [-_en_mx];
			if (_en_x < fi) { _en_x = 0; _en_mx = -_en_mx; } else _en_x -= fi;
		} else {
			fi = fz_incs [_en_mx];
			if (_en_x > 224 - fi) { _en_x = 224; _en_mx = -_en_mx; } else _en_x += fi;
		}
		if (gpy > _en_y) { if (_en_my < FZ_MAX) _en_my ++; }
		else if (gpy < _en_y) { if (_en_my > -FZ_MAX) _en_my --; }
		if (_en_my < 0) {
			fi = fz_incs [-_en_my];
			if (_en_y < fi) { _en_y = 0; _en_my = -_en_my; } else _en_y -= fi;
		} else {
			fi = fz_incs [_en_my];
			if (_en_y > 144 - fi) { _en_y = 144; _en_my = -_en_my; } else _en_y += fi;
		}
		en_an_next_frame [enit] = (_en_x & 8) ? sprite_14_a : sprite_13_a;
	}

	void monococo_move (void) {
		// NES enem_monococo. Phase in _en_mx as (phase + 1) & 3 so that the
		// 0 of a fresh screen means "hiding, about to be hidden"; counter in _en_my.
		mc_phase = (_en_mx + 3) & 3;
		if (_en_my) _en_my --;
		if (_en_my == 0) {
			mc_phase = (mc_phase + 1) & 3;
			_en_my = mc_times [mc_phase] - (rand () & 3);
			_en_mx = (mc_phase + 1) & 3;
		}
		if (mc_phase == 2 && _en_my == MC_FIRE_AT) {
			rdx = _en_x; rdy = _en_y; coco_new ();
			peta_el_beeper (4);
		}
		switch (mc_phase) {
			#ifdef YUN_EXTRA_ENEMS
				case 0: en_an_next_frame [enit] = _en_t == 8 ? sprite_23_a : sprite_17_a; break;	// bush / wall
				case 1: en_an_next_frame [enit] = _en_t == 8 ? sprite_24_a : sprite_18_a; break;	// coming out
			#else
				case 0: en_an_next_frame [enit] = sprite_17_a; break;		// bush
				case 1: en_an_next_frame [enit] = sprite_18_a; break;		// coming out
			#endif
			default: en_an_next_frame [enit] = sprite_19_a;				// out
		}
	}

	void extra_enems_move (void) {
		// Trapped: stay put, drop to the floor, upside down, bubble around it.
		if (en_cap [enit]) {
			bj = en_cap [enit] - 1;
			_en_x = cap_x [enit];
			if (cap_y [enit] < 144) {
				if (attr ((_en_x + 8) >> 4, (cap_y [enit] + 16 + BUB_FALL - 1) >> 4) & 12) {
					cap_y [enit] = (((cap_y [enit] + 16 + BUB_FALL - 1) >> 4) << 4) - 16;
				} else cap_y [enit] += BUB_FALL;
			}
			_en_y = cap_y [enit];
			bub_x [bj] = _en_x;
			bub_y [bj] = _en_y;
			en_an_next_frame [enit] = ud_cells [((_en_t - 1) << 1) + ((maincounter >> 2) & 1)];
		} else if (_en_t == 5) fanty_move ();
		else if (_en_t == 6) monococo_move ();
		#ifdef YUN_EXTRA_ENEMS
			else if (_en_t == 8) monococo_move ();
			else if (_en_t == 7) pezon_move ();
			else if (_en_t == 9) gyrosaw_move ();
			else if (_en_t == 10) cannon_move ();
		#endif
	}

	void extra_enems_checks (void) {
		if (en_cap [enit]) {
			// Touch a trapped enemy: pop it
			if (collide_enem ()) {
				bj = en_cap [enit] - 1;
				bub_ct [bj] = 0;
				bub_lock [bj] = 0xff;
				if (bub_ride == bj) bub_ride = 0xff;
				en_cap [enit] = 0;
				enems_kill (1);
				player.vy = -PLAYER_VY_FROM_ENEM;
			}
			return;
		}

		// Linear enemies, fanties, pezons and monococos out of their bush can be trapped
		#ifdef YUN_EXTRA_ENEMS
			if (_en_t < 1 || _en_t > 8 || _en_t == 4) return;
			if ((_en_t == 6 || _en_t == 8) && ((_en_mx + 3) & 3) == 0) return;
		#else
			if (_en_t < 1 || _en_t > 6 || _en_t == 4) return;
			if (_en_t == 6 && ((_en_mx + 3) & 3) == 0) return;
		#endif

		for (bj = 0; bj < BUBS_MAX; bj ++) {
			if (bub_ct [bj] && bub_lock [bj] == 0xff && bub_ride != bj
				&& bub_x [bj] + 12 >= _en_x && bub_x [bj] <= _en_x + 12
				&& bub_y [bj] + 12 >= _en_y && bub_y [bj] <= _en_y + 12) {
				bub_lock [bj] = enit;
				en_cap [enit] = bj + 1;
				cap_x [enit] = _en_x;
				cap_y [enit] = _en_y;
				peta_el_beeper (6);
				break;
			}
		}
	}

	void extra_enems_killed (void) {
	}

	unsigned char this_enemy_kills (void) {
		// Trapped enemies and monococos in their bush don't hurt
		if (en_cap [enit]) return 0;
		if (_en_t == 6 && ((_en_mx + 3) & 3) == 0) return 0;
		#ifdef YUN_EXTRA_ENEMS
			if (_en_t == 8 && ((_en_mx + 3) & 3) == 0) return 0;
			if (_en_t == 9) return 1; 		// no bouncing off a saw
		#endif

		// Stepping on an enemy bounces Yun (Yun v5: PLAYER_STEPS_ON_ENEMIES)
		if (gpy + 10 <= _en_y && player.vy > 0) {
			player.vy = -PLAYER_VY_FROM_ENEM;
			return 0;
		}

		return 1;
	}

#endif

#ifdef ENABLE_CUSTOM_HOTSPOTS
	unsigned char custom_hotspots () {
		// Stars: the NES counts one per level for the ending; split into loads
		// that can't carry over, so here a star is an extra life.
		#ifdef YUN_DARK_LEVEL
			if (hotspot_t == HOTSPOT_TYPE_BULB) {
				// lights the room for good; the engine takes the hotspot away
				lit [n_pant] = 1;
				lightning = 0;
				dark_set (0);
				peta_el_beeper (7);
			}
		#endif
		if (hotspot_t == HOTSPOT_TYPE_OBJECT) {
			player.life ++;
			if (player.life > 99) player.life = 99;
			peta_el_beeper (7);
			hotspot_t = 0;
		}
		return 0;
	}
#endif
