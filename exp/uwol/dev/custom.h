// MTE MK1 v3.2
// Copyleft 2011, 2026 by The Mojon Twins

// Uwol, Quest for Money - custom code

// ============================================================================
// Game state
// ============================================================================

unsigned char rand (void);

#define UWOL_FANTY_T 		6 		// Custom enemy type for the fanty
#define UWOL_COIN_TILE 		12 		// Coins are tiles 12-15 (one per background)
#define UWOL_ARROW_L 		20
#define UWOL_ARROW_R 		21
#define UWOL_DOOR 			18
#define UWOL_SHIRT 			HOTSPOT_TYPE_OBJECT 	// Tile 17

#define UWOL_ROOMS 			55

// Music: the original's (wyz/songs_ids.h, MUSIC_*): a song per zone, the
// fanty's, the pyramid's, game over and the two endings. CPC, and ZX 128K.
#ifdef CPC
	#define UWOL_MUSIC(n) 		AY_PLAY_MUSIC (n)
#else
	#include "wyz/songs_ids.h"
	#ifdef MODE_128K_DUAL
		#define UWOL_MUSIC(n) 	ay_play_music (n)
	#else
		#define UWOL_MUSIC(n)
	#endif
#endif

// Sound effects: sfx.h (ZX: beeper and, on a 128K, the AY ones with the same
// numbers), wyz/efectos.h (CPC), else MK1's numbers.
#if !defined CPC && !defined SFX_IDLE_PLAYER
	#define SFX_COIN 			8
	#define SFX_SHIRT 			7
	#define SFX_DEATH 			0
	#define SFX_FANTY 			4
	#define SFX_EXITS 			9
	#define SFX_EXTRA_LIFE 		7
#endif

#define UWOL_LAST_LEVEL 	10
#define UWOL_COINS_TO_WIN 	256

unsigned char uwol_level; 					// 1-10, row of the pyramid
unsigned char uwol_zone; 					// Background tile (0-3), depends on the level
unsigned char uwol_attr; 					// This room's background colour
unsigned char uwol_coins; 					// Coins left in this room
unsigned int uwol_total_coins;
unsigned int uwol_score;
signed char uwol_time; 						// -1 = the fanty is out
unsigned char uwol_nude;
unsigned char uwol_exit; 					// Exits are shown
unsigned char uwol_bonus; 					// Extra lives given
unsigned char uwol_good; 					// For game_ending ()
unsigned char uwol_visited [UWOL_ROOMS];
unsigned int uwol_fanty_idx = 0xffff; 				// malotes [] index of the fanty, 0xffff = none
unsigned char uwol_fanty_t; 				// Its original type
unsigned char uwol_num_buf [6];
unsigned int uwol_max;
unsigned char uwol_i;

void uwol_set_zone (void) {
	if (uwol_level < 4) uwol_zone = 0;
	else if (uwol_level < 7) uwol_zone = 1;
	else if (uwol_level < 9) uwol_zone = 2;
	else uwol_zone = 3;
}

// ============================================================================
// HUD
// ============================================================================

void draw_num (unsigned char x, unsigned char y, unsigned int n, unsigned char d) {
	uwol_num_buf [d] = 0;
	while (d) {
		d --;
		uwol_num_buf [d] = '0' + n % 10;
		n /= 10;
	}
	draw_text (x, y,
		#ifndef CPC
			69,
		#endif
		uwol_num_buf);
}

void uwol_hud_time (void) {
	draw_num (17, 1, uwol_time < 0 ? 0 : uwol_time, 3);
}

void uwol_hud_coins (void) {
	draw_num (25, 1, uwol_total_coins, 3);
}

void uwol_hud_score (void) {
	draw_num (23, 23, uwol_score, 5);
}

void uwol_hud (void) {
	uwol_hud_time ();
	uwol_hud_coins ();
	uwol_hud_score ();
	draw_num (10, 23, uwol_level, 2);
	life_old = 0xff;
}

// ============================================================================
// Player
// ============================================================================

void uwol_die (void) {
	// Original death sequence: dead frame, jump up, fall off the screen.
	player.estado = EST_MUR;
	player.vy = -7 * 64;
	player.vx = 0;
	peta_el_beeper (SFX_DEATH);
}

#ifdef PLAYER_CUSTOM_FRAME
	unsigned char player_custom_frame (void) {
		// Vanilla still / walk / jump, + 8 when naked.
		if (player.possee || player.gotten) {
			if (thrusting && player.vx) {
				gpit = (gpx >> 3) & 3;
				if (gpit == 3) gpit = 1;
			} else gpit = 1;
		} else gpit = 3;

		return player.facing + gpit + (uwol_nude ? CELL_NAKED : 0);
	}
#endif

#ifdef PLAYER_MOVE_OVERRIDE
	unsigned char player_move_override (void) {
		// Return 1 if call to `move` should be skipped.
		if ((player.estado & EST_MUR) == 0) return 0;

		// Death sequence. No collisions, falls until it leaves the screen.
		if (player.vy < 1024) player.vy += PLAYER_G; else player.vy = 1024;
		player.y += player.vy;
		if (player.y < 0) player.y = 0;
		player.next_frame = sprite_cells [CELL_DEAD];

		if (player.y > (160 << 6)) {
			// Back to the room's start, blinking and dressed.
			player.x = (PLAYER_INI_X << 4) << 6;
			player.y = (PLAYER_INI_Y << 4) << 6;
			player.vy = 0;
			player.facing = SIDEVIEW_FACING_RIGHT;
			player.estado = EST_PARP;
			player.ct_estado = 32;
			player.next_frame = sprite_cells [1];
			uwol_nude = 0;
			player.life --;
		}

		gpx = player.x >> 6;
		gpy = player.y >> 6;

		return 1;
	}
#endif

// ============================================================================
// Enemies: the fanty
// ============================================================================

void uwol_fanty_create (void) {
	// The original takes the last empty slot and puts the fanty in the middle
	// of the room. If there's none, enemy 2 becomes the fanty where it is.
	gpit = 2; rdi = 0;
	for (uwol_i = 0; uwol_i < 3; uwol_i ++) if (malotes [enoffs + uwol_i].t == 0) {
		gpit = uwol_i; rdi = 1;
	}

	uwol_fanty_idx = enoffs + gpit;
	uwol_fanty_t = malotes [uwol_fanty_idx].t;
	malotes [uwol_fanty_idx].t = UWOL_FANTY_T;
	if (rdi) {
		malotes [uwol_fanty_idx].x = 111;
		malotes [uwol_fanty_idx].y = 71;
	}

	en_an_x [gpit] = malotes [uwol_fanty_idx].x << 6;
	en_an_y [gpit] = malotes [uwol_fanty_idx].y << 6;
	en_an_vx [gpit] = en_an_vy [gpit] = 0;
	en_an_state [gpit] = CELL_FANTY_R;
	peta_el_beeper (SFX_FANTY);
	UWOL_MUSIC (MUSIC_FANTY);
}

void uwol_fanty_restore (void) {
	if (uwol_fanty_idx != 0xffff) {
		malotes [uwol_fanty_idx].t = uwol_fanty_t;
		uwol_fanty_idx = 0xffff;
	}
}

#ifdef ENABLE_CUSTOM_ENEMS
	void extra_enems_init (void) {
	}

	void extra_enems_move (void) {
		// The original fanty: accelerates 4 towards the player, max 120 (1/64 px)
		#asm
				ld  a, (__en_t)
				cp  UWOL_FANTY_T
				ret nz

				ld  a, (_enit)
				add a, a
				ld  c, a
				ld  b, 0 						// BC = enit * 2 (16 bits arrays)

			// Horizontal

				ld  hl, _en_an_x
				add hl, bc
				ld  e, (hl)
				inc hl
				ld  d, (hl) 					// DE = rx
				ld  hl, (_player) 				// player.x
				or  a
				sbc hl, de 						// player.x - rx
				jr  z, uef_h_done
				jp  m, uef_h_left

				ld  hl, _en_an_vx
				add hl, bc
				ld  a, (hl)
				xor 128
				cp  128 + 120
				jr  nc, uef_h_done 				// vx >= 120
				xor 128
				add 4
				ld  (hl), a
				ld  a, CELL_FANTY_R
				jr  uef_h_facing

			.uef_h_left
				ld  hl, _en_an_vx
				add hl, bc
				ld  a, (hl)
				xor 128
				cp  128 - 120 + 1
				jr  c, uef_h_done 				// vx <= -120
				xor 128
				sub 4
				ld  (hl), a
				ld  a, CELL_FANTY_L

			.uef_h_facing
				ld  de, (_enit)
				ld  d, 0
				ld  hl, _en_an_state
				add hl, de
				ld  (hl), a

			.uef_h_done

			// Vertical

				ld  hl, _en_an_y
				add hl, bc
				ld  e, (hl)
				inc hl
				ld  d, (hl) 					// DE = ry
				ld  hl, (_player + 2) 			// player.y
				or  a
				sbc hl, de 						// player.y - ry
				jr  z, uef_v_done
				jp  m, uef_v_up

				ld  hl, _en_an_vy
				add hl, bc
				ld  a, (hl)
				xor 128
				cp  128 + 120
				jr  nc, uef_v_done
				xor 128
				add 4
				ld  (hl), a
				jr  uef_v_done

			.uef_v_up
				ld  hl, _en_an_vy
				add hl, bc
				ld  a, (hl)
				xor 128
				cp  128 - 120 + 1
				jr  c, uef_v_done
				xor 128
				sub 4
				ld  (hl), a

			.uef_v_done

			// rx += vx, inside 0..224 px

				ld  hl, _en_an_vx
				add hl, bc
				ld  de, 224 * 64
				call uef_move
				call HLshr6_A
				ld  (__en_x), a

			// ry += vy, inside 0..144 px

				ld  hl, _en_an_vy
				add hl, bc
				ld  de, 144 * 64
				call uef_move
				call HLshr6_A
				ld  (__en_y), a

			// en_an_next_frame [enit] = sprite_cells [en_an_state [enit]]

				ld  de, (_enit)
				ld  d, 0
				ld  hl, _en_an_state
				add hl, de
				ld  a, (hl)
				add a, a
				ld  e, a
				ld  hl, _sprite_cells
				add hl, de
				ld  e, (hl)
				inc hl
				ld  d, (hl)
				ld  hl, _en_an_next_frame
				add hl, bc
				ld  (hl), e
				inc hl
				ld  (hl), d
				ret

			.uef_move
				// HL -> en_an_v? [enit], DE = max. The position (en_an_? [enit])
				// is MAX_ENEMS * 4 bytes before. position += (signed) speed,
				// clamped to 0..max, written back and returned in HL.
				ld  (_uwol_max), de
				ld  a, (hl)
				ld  e, a
				add a, a
				sbc a, a
				ld  d, a 						// DE = speed, sign extended
				push de
				ld  de, -(MAX_ENEMS * 4)
				add hl, de 						// HL -> position
				pop de
				push hl
				ld  a, (hl)
				inc hl
				ld  h, (hl)
				ld  l, a
				add hl, de 						// HL = new position
				bit 7, h
				jr  z, uef_move_max
				ld  hl, 0
				jr  uef_move_write

			.uef_move_max
				ld  de, (_uwol_max)
				push hl
				or  a
				sbc hl, de
				pop hl
				jr  c, uef_move_write
				ex  de, hl

			.uef_move_write
				ex  de, hl 						// DE = position
				pop hl
				ld  (hl), e
				inc hl
				ld  (hl), d
				ex  de, hl 						// HL = position
				ret
		#endasm
	}

	void extra_enems_checks (void) {
	}

	void extra_enems_killed (void) {
	}

	unsigned char this_enemy_kills (void) {
		return 1;
	}
#endif

// ============================================================================
// Map renderer
// ============================================================================

#ifdef ENABLE_CUSTOM_MAP_RENDERER
	void uwol_set_colours (void) {
		// This room's background colour for the background, its shaded
		// version and the coin of the current zone.
		#ifdef CPC
			// Pens 1 (paper) and 2 (ink)
			#asm
					ld  a, (_uwol_attr)
					rrca
					rrca
					rrca
					call uwol_set_colours_hw
					ld  bc, 0x7f01
					out (c), c
					out (c), a 						// Pen 1 = paper

					ld  a, (_uwol_attr)
					call uwol_set_colours_hw
					ld  bc, 0x7f02
					out (c), c
					out (c), a 						// Pen 2 = ink
					ret

				.uwol_set_colours_hw
					// A & 7 = ZX colour -> A = CPC hardware colour | 0x40
					and 7
					ld  e, a
					ld  d, 0
					ld  hl, uwol_set_colours_lut
					add hl, de
					ld  a, (hl)
					ret

				.uwol_set_colours_lut
					defb 0x54, 0x44, 0x5C, 0x58, 0x56, 0x46, 0x5E, 0x40
			#endasm
		#else
			#asm
					ld  a, (_uwol_zone)
					call uwol_set_colours_tile 		// background
					ld  a, (_uwol_zone)
					add 12
					call uwol_set_colours_tile 		// coin
					ld  a, (_uwol_zone)
					add 32
					call uwol_set_colours_tile 		// shaded background

				// Tile 46 (black wall, obstacle) hides sprites like 47
					xor a
					ld  (_uwol_attr), a
					ld  a, 46

				.uwol_set_colours_tile
					add a, a
					add a, a
					ld  e, a
					ld  d, 0
					ld  hl, _tileset + 2048 + 64
					add hl, de
					ld  a, (_uwol_attr)
					ld  (hl), a
					inc hl
					ld  (hl), a
					inc hl
					ld  (hl), a
					inc hl
					ld  (hl), a
			#endasm
		#endif
	}

	void custom_draw_scr_background () {
		// Builds map_buff from the room's objects and coins, then draws it with
		// the vanilla draw_and_advance (map_attr, auto tile shadows). Coins
		// cover the shadows, so they are redrawn unshaded afterwards.
		// The colours are set first: ZX attributes in the tileset, CPC pens.
		#asm
				ld  a, (_n_pant)
				ld  l, a
				ld  h, 0
				add hl, hl
				ld  de, _rooms_index
				add hl, de
				ld  e, (hl)
				inc hl
				ld  d, (hl)
				ex  de, hl 						// HL -> room

				ld  a, (hl)
				inc hl
				ld  (_uwol_attr), a
				push hl
				call _uwol_set_colours

			// Walls (47) at columns 0, 13, 14, background elsewhere

				ld  hl, _map_buff
				ld  b, 10
				ld  a, (_uwol_zone)
				ld  c, a

			.cdsb_fill_rows
				ld  (hl), 47
				inc hl
				ld  e, 12

			.cdsb_fill_cols
				ld  (hl), c
				inc hl
				dec e
				jr  nz, cdsb_fill_cols

				ld  (hl), 47
				inc hl
				ld  (hl), 47
				inc hl
				djnz cdsb_fill_rows

				pop hl

			// Objects: n, then n * (XXXXYYYY, LLLLTTTD)

				ld  a, (hl)
				inc hl
				or  a
				jr  z, cdsb_objs_done
				ld  b, a

			.cdsb_objs_loop
				push bc
				ld  a, (hl)
				inc hl
				ld  c, (hl)
				inc hl
				push hl

				call cdsb_map_ptr 				// HL -> map_buff [XXXXYYYY]

				ld  a, c
				srl a
				and 7
				add 4
				ld  e, a 						// E = tile, 4 + TTT

				ld  d, 1 						// Horizontal: next is +1
				bit 0, c
				jr  z, cdsb_obj_dir_set
				ld  d, 15 						// Vertical: next is +15

			.cdsb_obj_dir_set
				ld  a, c
				srl a
				srl a
				srl a
				srl a
				or  a
				jr  z, cdsb_obj_done
				ld  b, a 						// B = length

			.cdsb_obj_cells
				ld  (hl), e
				ld  a, l
				add d
				ld  l, a
				jr  nc, cdsb_obj_cells_nc
				inc h

			.cdsb_obj_cells_nc
				djnz cdsb_obj_cells

			.cdsb_obj_done
				pop hl
				pop bc
				djnz cdsb_objs_loop

			.cdsb_objs_done

			// A platform at either playfield edge makes that row's side columns
			// solid (tile 46) at both sides: floors / ceilings go on over the
			// dark columns, and the other side can't be entered (the player warps).

				push hl
				ld  hl, _map_buff
				ld  b, 10

			.cdsb_sides_loop
				ld  c, 0 						// C = 1 if solid
				inc hl 							// Column 1
				ld  a, (hl)
				sub 4
				cp  8
				jr  nc, cdsb_sides_l_done
				inc c

			.cdsb_sides_l_done
				ld  de, 11
				add hl, de 						// Column 12
				ld  a, (hl)
				sub 4
				cp  8
				jr  nc, cdsb_sides_r_done
				inc c

			.cdsb_sides_r_done
				inc hl 							// Column 13
				ld  a, c
				or  a
				jr  z, cdsb_sides_next

				ld  (hl), 46
				inc hl
				ld  (hl), 46 					// Column 14
				dec hl
				ld  de, -13
				add hl, de
				ld  (hl), 46 					// Column 0
				ld  de, 13
				add hl, de

			.cdsb_sides_next
				inc hl
				inc hl 							// Next row
				djnz cdsb_sides_loop

				pop hl

			// Coins: n, then n * XXXXYYYY. Not in visited rooms.

				xor a
				ld  (_uwol_coins), a

				ld  b, (hl)
				inc hl

				ld  a, (_n_pant)
				ld  e, a
				ld  d, 0
				push hl
				ld  hl, _uwol_visited
				add hl, de
				ld  a, (hl)
				pop hl
				or  a
				jr  nz, cdsb_coins_done

				ld  a, b
				or  a
				jr  z, cdsb_coins_done
				ld  (_uwol_coins), a

				ld  a, (_uwol_zone)
				add UWOL_COIN_TILE
				ld  e, a

			.cdsb_coins_loop
				ld  a, (hl)
				inc hl
				push hl
				call cdsb_map_ptr
				ld  (hl), e
				pop hl
				djnz cdsb_coins_loop

			.cdsb_coins_done

			// Draw everything (rdi, rdx, rdy are 0)

			.cdsb_draw_loop
				ld  bc, (_rdi)
				ld  b, 0
				ld  hl, _map_buff
				add hl, bc
				ld  a, (hl)
				ld  (__n), a
				call _draw_and_advance
				ld  a, (_rdi)
				cp  150
				jr  nz, cdsb_draw_loop

			// Tile 46 is drawn as behaviour 0 so it doesn't cast shadows over
			// the playfield; it is made an obstacle now.

				ld  hl, _map_buff
				ld  de, _map_attr
				ld  b, 150

			.cdsb_walls_loop
				ld  a, (hl)
				cp  46
				jr  nz, cdsb_walls_next
				ld  a, 8
				ld  (de), a

			.cdsb_walls_next
				inc hl
				inc de
				djnz cdsb_walls_loop

			// Coins again, unshaded

				ld  a, (_uwol_coins)
				or  a
				ret z

				ld  hl, _map_buff
				ld  d, VIEWPORT_Y

			.cdsb_coins_row
				ld  e, VIEWPORT_X

			.cdsb_coins_col
				ld  a, (hl)
				sub UWOL_COIN_TILE
				cp  4
				jr  nc, cdsb_coins_next

				ld  a, (hl)
				ld  (__t), a
				ld  a, e
				ld  (__x), a
				ld  a, d
				ld  (__y), a
				push hl
				push de

				// map_attr = 8 while drawing: obstacles don't get shadows
				ld  bc, _map_buff
				or  a
				sbc hl, bc
				ld  bc, _map_attr
				add hl, bc
				ld  (hl), 8
				push hl
				call _draw_coloured_tile_do
				pop hl
				ld  (hl), 0

				pop de
				pop hl

			.cdsb_coins_next
				inc hl
				inc e
				inc e
				ld  a, e
				cp  VIEWPORT_X + 30
				jr  nz, cdsb_coins_col

				inc d
				inc d
				ld  a, d
				cp  VIEWPORT_Y + 20
				jr  nz, cdsb_coins_row
				ret

			.cdsb_map_ptr
				// A = XXXXYYYY (playfield) -> HL -> map_buff [15 * Y + X + 1]
				push de
				ld  d, a
				and 15
				ld  e, a
				add a, a
				add a, a
				add a, a
				add a, a
				sub e 							// 15 * Y
				ld  e, a
				ld  a, d
				srl a
				srl a
				srl a
				srl a
				inc a
				add e
				ld  e, a
				ld  d, 0
				ld  hl, _map_buff
				add hl, de
				pop de
				ret
		#endasm
	}
#endif

// ============================================================================
// Hotspots: the t-shirt
// ============================================================================

void uwol_shirt (void) {
	// Somewhere in the background, rows 1-8.
	do {
		rdx = 1 + rand () % 12;
		rdy = 1 + (rand () & 7);
	} while (map_buff [15 * rdy + rdx] != uwol_zone);

	hotspots [n_pant].xy = (rdx << 4) | rdy;
	hotspots [n_pant].tipo = UWOL_SHIRT;
	hotspots [n_pant].act = 1;
	#asm
			call _hotspot_paint
	#endasm
}

#ifdef ENABLE_CUSTOM_HOTSPOTS
	unsigned char custom_hotspots () {
		// hotspot type is hotspot_t.
		// set rdi to prevent clearing the hotspot.
		uwol_nude = 0;
		player.estado = EST_PARP;
		player.ct_estado = 32;
		uwol_score += 15;
		peta_el_beeper (SFX_SHIRT); 	// Before the vanilla "item" sound, but more important
		uwol_hud_score ();
		return 0;
	}
#endif

// ============================================================================
// Exits
// ============================================================================

void uwol_open_exits (void) {
	// Arrows (or the doors at the bottom of the pyramid) on the floor, if
	// there's room above them. They are solid, like the original's.
	gpit = uwol_level < UWOL_LAST_LEVEL ? UWOL_ARROW_L : UWOL_DOOR;
	if (map_attr [15 * 8 + 1] == 0) set_map_tile (1, 9, gpit, 8);
	if (uwol_level < UWOL_LAST_LEVEL) gpit = UWOL_ARROW_R;
	if (map_attr [15 * 8 + 12] == 0) set_map_tile (12, 9, gpit, 8);

	// Everybody out but the fanty
	for (gpit = 0; gpit < 3; gpit ++) {
		enoffsmasi = enoffs + gpit;
		if (enoffsmasi != uwol_fanty_idx && malotes [enoffsmasi].t) {
			malotes [enoffsmasi].t |= 128;
			en_an_next_frame [gpit] = spr_empty;
		}
	}

	uwol_exit = 1;
	peta_el_beeper (SFX_EXITS);
}

void uwol_leave (unsigned char right) {
	uwol_score += 100 + 5 * (uwol_time > 0 ? uwol_time : 0);
	uwol_visited [n_pant] = 1;
	uwol_fanty_restore ();
	hotspots [n_pant].tipo = 0;

	if (uwol_level == UWOL_LAST_LEVEL) {
		saca_a_todo_el_mundo_de_aqui ();
		uwol_good = (uwol_total_coins >= UWOL_COINS_TO_WIN);
		#asm
				call _game_ending
		#endasm
		if (uwol_good) {
			playing = 0;
			on_pant = 0xff; 		// Don't update the screen
			return;
		}

		// Not enough money! back to the top.
		n_pant = 0;
		uwol_level = 1;
	} else {
		n_pant += uwol_level + right;
		uwol_level ++;
	}

	uwol_set_zone ();
	#asm
			call _show_pyramid
	#endasm
}

// ============================================================================
// Flicks: warp sideways, die at the bottom
// ============================================================================

// (custom_flick.h)

// ============================================================================
// Hooks
// ============================================================================

#ifdef ENABLE_CUSTOM_CODE
	void entering_game (void) {
		uwol_level = 1;
		uwol_set_zone ();
		uwol_total_coins = uwol_score = 0;
		uwol_bonus = 0;
		uwol_fanty_restore ();
		for (gpit = 0; gpit < UWOL_ROOMS; gpit ++) {
			uwol_visited [gpit] = 0;
			hotspots [gpit].tipo = 0;
		}

		#ifndef CPC
			clr2d = 69;
		#endif

		#asm
				call _show_pyramid
		#endasm
	}

	void entering_screen (void) {
		// Every room starts at the bottom left, dressed.
		player.x = (PLAYER_INI_X << 4) << 6;
		player.y = (PLAYER_INI_Y << 4) << 6;
		player.vx = player.vy = 0;
		player.facing = SIDEVIEW_FACING_RIGHT;
		player.estado = EST_NORMAL;
		gpx = PLAYER_INI_X << 4;
		gpy = PLAYER_INI_Y << 4;

		uwol_nude = 0;
		uwol_exit = 0;
		uwol_fanty_idx = 0xffff;
		uwol_time = uwol_coins << 1; 	// 0 in visited rooms: exits + fanty at once.

		// Zone songs: MUSIC_ZONE1, then MUSIC_ZONE2-4 (3-5)
		UWOL_MUSIC (uwol_zone ? 2 + uwol_zone : MUSIC_ZONE1);

		uwol_hud ();
	}

	void before_vsync (void) {
		if (player.estado & EST_MUR) return;

		// Hit: lose the clothes, or die if already naked.
		if (player.is_dead) {
			if (uwol_nude) {
				player.is_dead = 0;
				uwol_die ();
				return;
			}
			uwol_nude = 1;
			player.drain_amount = 0;
			if (uwol_time >= 0) uwol_shirt ();
		}

		// Time. When it runs out, the fanty comes.
		if ((maincounter & 31) == 0 && uwol_time >= 0) {
			if (uwol_time) {
				uwol_time --;
			} else {
				uwol_fanty_create ();
				uwol_time = -1;
			}
			uwol_hud_time ();
		}

		// Coins, then exits.
		if (uwol_coins) {
			rdx = (gpx + 8) >> 4;
			rdy = (gpy + 8) >> 4;
			if (rdy < 10) {
				gpit = map_buff [15 * rdy + rdx] - UWOL_COIN_TILE;
				if (gpit < 4) {
					set_map_tile (rdx, rdy, uwol_zone, 0);
					uwol_coins --;
					uwol_total_coins ++;
					uwol_score ++;
					uwol_hud_coins ();
					uwol_hud_score ();
					peta_el_beeper (SFX_COIN);
				}
			}
		} else if (uwol_exit == 0) {
			uwol_open_exits ();
		} else if ((pad0 & sp_DOWN) == 0) {
			rdx = (gpx + 8) >> 4;
			if (rdx == 1) uwol_leave (0);
			else if (rdx == 12) uwol_leave (1);
		}

		// Extra lives at 1000 and 2500
		if ((uwol_bonus == 0 && uwol_score >= 1000) || (uwol_bonus == 1 && uwol_score >= 2500)) {
			uwol_bonus ++;
			player.life ++;
			peta_el_beeper (SFX_EXTRA_LIFE);
		}
	}
#endif
