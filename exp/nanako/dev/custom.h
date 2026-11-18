// MTE MK1 v3.2
// Copyleft 2011, 2026 by The Mojon Twins

// custom.h

// Nanako in Classic Japanese Monster Castle. Everything the CPC original
// (2009, orig/dev/ncjmccpc.c) does on top of the vanilla engine:
//
//  * Levels: screens 1-25 (level = n_pant - 1), drawn from the original data
//    (levels.h) by custom_draw_scr_background. 12x8 tiles game area at map
//    columns 1-12, rows 0-7; columns 0, 13, 14 and rows 8, 9 are solid
//    (tile 12, never drawn); the top is open: jumping off it is the exit
//    (custom flick up).
//  * Boxes (tile 5): DOWN takes the one under the feet, UP puts it where
//    Nanako stands and lifts her one tile. On the 16 pixels grid.
//  * Karakasa: enemy slots 0 (head, a vanilla moving platform, type 4) and
//    1 (body, type 5, kills), random walk as the original. Slot 2: the puff.
//  * Death: puff, lose a life, level box, the level starts again. R retries
//    (costs a life), H (ZX) / ESC (CPC) quits.
//  * HUD: 5 lives icons (font chars '[' empty and ']' full).

unsigned char rand (void);
unsigned char attr (unsigned char x, unsigned char y);

#define NK_LEVELS 			25

#define NK_T_BG 			0
#define NK_T_BOX 			5
#define NK_T_BLACK 			12
#define NK_T_BOXFILL 		13

#define NK_HEAD_T 			4
#define NK_BODY_T 			5
#define NK_PUFF_T 			6

// Sprite cells (custom_sprite_cells.h)
#define NK_CELL_HEAD 		8
#define NK_CELL_BODY 		12
#define NK_CELL_PUFF 		16

// Lives icons, title texts
#ifdef CPC
	#define NK_LIVES_X 		12
	#define NK_LIVES_Y 		20
	#define NK_TITLE_Y 		19
#else
	#define NK_LIVES_X 		11
	#define NK_LIVES_Y 		21
	#define NK_TITLE_Y 		14
#endif

// Music (CPC, ZX 128K): see ogt/README.md
#define MUSIC_TITLE 		0
#define MUSIC_INGAME 		1
#define MUSIC_JINGLE 		2

#ifdef CPC
	#define NK_MUSIC(n) 		AY_PLAY_MUSIC (n)
	#define NK_MUSIC_STOP() 	AY_STOP_SOUND ()
	#define SCREEN_UPDATE 		cpc_UpdateNow (0)
	#define TXT(c)
	#include "wyz/efectos_ids.h"
#else
	#ifdef MODE_128K_DUAL
		#define NK_MUSIC(n) 	ay_play_music (n)
		#define NK_MUSIC_STOP() ay_stop_sound ()
	#else
		#define NK_MUSIC(n)
		#define NK_MUSIC_STOP()
	#endif
	#define SCREEN_UPDATE 		sp_UpdateNow ()
	#define TXT(c) 				c,
#endif

// Keys not in the pad: retry, quit (ZX scancodes, CPC matrix line | bit)
#ifdef CPC
	#define NK_KEY_RETRY 		0x4604 	// R
	#define NK_KEY_QUIT 		0x4804 	// ESC
	#define NK_KEY_PASSWORD 	0x4308 	// P
	#define NK_KEY_O 			0x4404
	#define NK_KEY_P 			0x4308
	#define NK_KEY_Q 			0x4808
	#define NK_KEY_A 			0x4820
	#define NK_KEY_SPACE 		0x4580
#else
	#define NK_KEY_RETRY 		0x08fb 	// R
	#define NK_KEY_QUIT 		0x10bf 	// H
	#define NK_KEY_PASSWORD 	0x01df 	// P
	#define NK_KEY_O 			0x02df
	#define NK_KEY_P 			0x01df
	#define NK_KEY_Q 			0x01fb
	#define NK_KEY_A 			0x01fd
	#define NK_KEY_SPACE 		0x017f
#endif

unsigned char nk_level; 					// 0-24
unsigned char nk_start_level; 				// From the password
unsigned char nk_carry; 					// Carrying a box
unsigned char nk_dying; 					// Death sequence frame counter, 0 = alive
unsigned char nk_quit;
unsigned char nk_win;
unsigned char nk_frames;

unsigned char nk_puff_ct; 					// 0 = off
unsigned char nk_puff_x, nk_puff_y;

unsigned char nk_k_on; 						// There's a karakasa
unsigned char nk_k_x, nk_k_y; 				// The original's coordinates (body)
unsigned char nk_k_status, nk_k_frame, nk_k_ct, nk_k_step;

unsigned char nk_i, nk_j, nk_t, nk_old;
unsigned char *nk_p;
unsigned char nk_buf [12];
unsigned int nk_key_code;

#define K_IDLE 		0
#define K_LEFT 		1
#define K_RIGHT 	2

// ============================================================================
// Helpers
// ============================================================================

unsigned char nk_key (void) {
	// nk_key_code pressed? Not through the pad (any controller selected).
	#ifdef CPC
		#asm
				ld  a, (_nk_key_code + 1)
				call cpc_TestKeyboard
				ld  c, a
				ld  a, (_nk_key_code)
				ld  b, a
				and c
				cp  b
				ld  hl, 0
				ret nz
				inc l
		#endasm
	#else
		return sp_KeyPressed (nk_key_code);
	#endif
}

unsigned char nk_solid (unsigned char x, unsigned char y) {
	// Original char coordinates of the game area (24x16) -> obstacle?
	// The original's dureza map at char level, from map_attr's 16x16 tiles.
	return attr (1 + (x >> 1), y >> 1) & 8;
}

void nk_hud (void) {
	// 5 lives icons, empty first, as the original
	for (nk_i = 0; nk_i < 5; nk_i ++) {
		nk_buf [nk_i] = nk_i < 5 - player.life ? '[' : ']';
	}
	nk_buf [5] = 0;
	draw_text (NK_LIVES_X, NK_LIVES_Y, TXT (68) nk_buf);
}

void nk_puff (unsigned char x, unsigned char y) {
	nk_puff_x = x; nk_puff_y = y; nk_puff_ct = 1;
}

// ============================================================================
// The level box (between levels) and the game area curtain
// ============================================================================

void nk_curtain (void) {
	// Game area to black
	for (nk_j = 0; nk_j < 8; nk_j ++) for (nk_i = 0; nk_i < 12; nk_i ++) {
		draw_coloured_tile (VIEWPORT_X + 2 + (nk_i << 1), VIEWPORT_Y + (nk_j << 1), NK_T_BLACK);
	}
}

void nk_box (void) {
	// The level box, 6x4 tiles at the middle of the game area
	nk_curtain ();
	nk_p = levelbox;
	for (nk_j = 0; nk_j < 4; nk_j ++) for (nk_i = 0; nk_i < 6; nk_i ++) {
		draw_coloured_tile (VIEWPORT_X + 8 + (nk_i << 1), VIEWPORT_Y + 4 + (nk_j << 1), *nk_p ++);
	}
}

void nk_level_digits (unsigned char *s, unsigned char n) {
	s [0] = '0' + n / 10;
	s [1] = '0' + n % 10;
}

void nk_password_text (unsigned char n) {
	// "<XXXX<": the original draws the dots with '<'
	nk_buf [0] = '.';
	for (nk_i = 0; nk_i < 4; nk_i ++) nk_buf [1 + nk_i] = passwords [(n << 2) + nk_i] ^ 63;
	nk_buf [5] = '.';
	nk_buf [6] = 0;
}

void nk_level_box (void) {
	// Level number, good luck and the level's password; the jingle.
	saca_a_todo_el_mundo_de_aqui ();
	nk_box ();
	draw_text (VIEWPORT_X + 10, VIEWPORT_Y + 6, TXT (71) " LEVEL XX ");
	nk_buf [0] = 0;
	draw_text (VIEWPORT_X + 10, VIEWPORT_Y + 8, TXT (70) "GOOD LUCK!");
	nk_password_text (nk_level);
	draw_text (VIEWPORT_X + 12, VIEWPORT_Y + 10, TXT (71) nk_buf);
	nk_buf [0] = '0' + nk_level / 10; nk_buf [1] = '0' + nk_level % 10; nk_buf [2] = 0;
	draw_text (VIEWPORT_X + 17, VIEWPORT_Y + 6, TXT (71) nk_buf);
	nk_hud ();
	SCREEN_UPDATE;

	NK_MUSIC (MUSIC_JINGLE);
	#ifdef SFX_IDLE_PLAYER
		peta_el_beeper (SFX_START);
	#endif
	espera_activa (100);
	NK_MUSIC (MUSIC_INGAME);
}

// ============================================================================
// Player
// ============================================================================

#ifdef PLAYER_CUSTOM_HENG
	void player_custom_heng (void) {
		// Vanilla's, with half the acceleration and friction when airborne:
		// jumps keep their speed but are harder to steer.
		#asm
				ld  bc, PLAYER_AX
				ld  de, PLAYER_RX
				ld  a, (_player + 26) 		// player.possee
				ld  hl, _player + 25 		// player.gotten
				or  (hl)
				jr  nz, nk_heng_ground
				ld  bc, PLAYER_AX / 2
				ld  de, PLAYER_RX / 2
			.nk_heng_ground
				ld  (_nk_heng_ax), bc
				ld  (_nk_heng_rx), de

				ld  a, (_pad0)
				ld  c, a

				and sp_LEFT
				jr  z, nk_heng_kp

				ld  a, c
				and sp_RIGHT
				jr  z, nk_heng_kp

			// Neither: friction

				ld  hl, (_player + 6) 		// player.vx
				ld  a, h
				or  l
				ret z

				bit 7, h
				jr  nz, nk_heng_rx_neg

				ld  de, (_nk_heng_rx)
				or  a
				sbc hl, de
				jr  c, nk_heng_zero
				jr  nk_heng_write

			.nk_heng_rx_neg
				ld  de, (_nk_heng_rx)
				add hl, de
				jr  c, nk_heng_zero
				jr  nk_heng_write

			.nk_heng_zero
				ld  hl, 0
				jr  nk_heng_write

			.nk_heng_kp
				ld  a, c
				and sp_LEFT
				jr  nz, nk_heng_right

				ld  a, THRUST_LEFT
				ld  (_thrusting), a
				ld  a, SIDEVIEW_FACING_LEFT
				ld  (_player + 22), a 		// player.facing

				ld  hl, (_player + 6)
				ld  de, (_nk_heng_ax)
				or  a
				sbc hl, de
				ld  de, -PLAYER_MAX_VX
				call l_gt 					// C if DE > HL
				jr  nc, nk_heng_write
				ex  de, hl
				jr  nk_heng_write

			.nk_heng_right
				ld  a, THRUST_RIGHT
				ld  (_thrusting), a
				ld  a, SIDEVIEW_FACING_RIGHT
				ld  (_player + 22), a

				ld  hl, (_player + 6)
				ld  de, (_nk_heng_ax)
				add hl, de
				ld  de, PLAYER_MAX_VX
				call l_lt 					// C if DE < HL
				jr  nc, nk_heng_write
				ex  de, hl

			.nk_heng_write
				ld  (_player + 6), hl
				ret

			._nk_heng_ax
				defw 0
			._nk_heng_rx
				defw 0
		#endasm
	}
#endif

void nk_take_box (void) {
	// DOWN: take the box under the feet. Under her centre first, then the
	// other column she's on.
	if (nk_carry || (gpy & 15) || (player.possee == 0)) return;
	rdy = (gpy >> 4) + 1;
	if (rdy > 7) return;
	rdx = (gpx + 8) >> 4;
	if (map_buff [rdy * 15 + rdx] != NK_T_BOX) {
		rdx = (gpx & 15) < 8 ? (gpx >> 4) + 1 : gpx >> 4;
		if ((gpx & 15) == 0 || map_buff [rdy * 15 + rdx] != NK_T_BOX) return;
	}

	set_map_tile (rdx, rdy, NK_T_BG, 0);
	nk_carry = 1;
	nk_puff (rdx << 4, rdy << 4);
	peta_el_beeper (SFX_TAKE);
}

void nk_drop_box (void) {
	// UP: the box goes where she is (the column under her centre) and she
	// goes up one tile. There must be room above her.
	if (nk_carry == 0 || (gpy & 15) || gpy < 16) return;
	rdy = gpy >> 4;
	if ((attr (gpx >> 4, rdy - 1) | attr ((gpx + 15) >> 4, rdy - 1)) & 8) return;
	rdx = (gpx + 8) >> 4;
	if (attr (rdx, rdy) & 8) return;

	gpy -= 16;
	player.y = gpy << 6;
	player.vy = 0;
	set_map_tile (rdx, rdy, NK_T_BOX, 8);
	nk_carry = 0;
	nk_puff (rdx << 4, rdy << 4);
	peta_el_beeper (SFX_DROP);
}

void nk_die (void) {
	// Start the death sequence (karakasa, retry or quit)
	nk_dying = 1;
	player.estado = EST_MUR;
	player.next_frame = spr_empty;
	nk_puff (gpx, gpy);
	if (nk_quit) player.life = 0;
}

#ifdef PLAYER_MOVE_OVERRIDE
	unsigned char player_move_override (void) {
		// Return 1 if call to `move` should be skipped.
		if (nk_dying) return 1;

		if ((pad0 & sp_DOWN) == 0) nk_take_box ();
		else if ((pad0 & sp_UP) == 0) nk_drop_box ();

		return 0;
	}
#endif

// ============================================================================
// Enemies: the karakasa (head + body) and the puff
// ============================================================================

#ifdef ENABLE_CUSTOM_ENEMS
	void extra_enems_init (void) {
	}

	unsigned char nk_k_can_walk (unsigned char x) {
		// The original's check at the char column x (24x16 chars): head,
		// body free, floor under it.
		nk_t = nk_k_y >> 3;
		return nk_solid (x, nk_t - 1) == 0 && nk_solid (x, nk_t) == 0 && nk_solid (x, nk_t + 1) == 0 && nk_solid (x, nk_t + 2) != 0;
	}

	void nk_k_move (void) {
		// mueve_karakasa (): random walk, 2 pixels per frame, never off an
		// edge or into a block. Frames: left 0, 1; right 2, 3.
		rdi = rand () & 15;
		if (rdi < 4) {
			rdi = rand () % 9;
			if (rdi < 2) nk_k_status = K_LEFT;
			else if (rdi < 4) nk_k_status = K_RIGHT;
			else nk_k_status = K_IDLE;
		}

		rdx = 0; 	// mx
		switch (nk_k_status) {
			case K_IDLE:
				if (nk_k_frame == 1) nk_k_frame = 0;
				else if (nk_k_frame == 3) nk_k_frame = 2;
				break;
			case K_LEFT:
			case K_RIGHT:
				nk_k_frame = (nk_k_status == K_LEFT ? 0 : 2) + nk_k_step;
				nk_k_ct = !nk_k_ct;
				if (nk_k_ct) nk_k_step = !nk_k_step;
				if (nk_k_status == K_LEFT) {
					if (nk_k_x > 0 && ((nk_k_x & 7) || nk_k_can_walk ((nk_k_x >> 3) - 1))) rdx = -2;
				} else {
					if (nk_k_x < 176 && ((nk_k_x & 7) || nk_k_can_walk ((nk_k_x >> 3) + 2))) rdx = 2;
				}
		}
		nk_k_x += rdx;
	}

	void extra_enems_move (void) {
		// Called for every enemy after the vanilla movement
		switch (_en_t) {
			case NK_HEAD_T:
				// Undo the vanilla linear move (it used last frame's mx),
				// move as the original, and leave mx in _en_mx so the
				// vanilla moving platform carries Nanako along.
				nk_k_move ();
				_en_x = nk_k_x + 16;
				_en_mx = rdx;
				en_an_next_frame [enit] = sprite_cells [NK_CELL_HEAD + nk_k_frame];
				break;
			case NK_BODY_T:
				_en_x = nk_k_x + 16;
				_en_y = nk_k_y;
				en_an_next_frame [enit] = sprite_cells [NK_CELL_BODY + nk_k_frame];
				break;
			case NK_PUFF_T:
				if (nk_puff_ct) {
					_en_x = nk_puff_x;
					_en_y = nk_puff_y;
					en_an_next_frame [enit] = sprite_cells [NK_CELL_PUFF + (nk_puff_ct < 8 ? 0 : nk_puff_ct < 12 ? 1 : 2)];
					nk_puff_ct ++;
					if (nk_puff_ct == 16) nk_puff_ct = 0;
				} else en_an_next_frame [enit] = spr_empty;
		}
	}

	void extra_enems_checks (void) {
		// The head is a moving platform that doesn't move vertically (its
		// my bounces between +1 and -1 so vanilla always checks it): no
		// vertical carry.
		if (_en_t == NK_HEAD_T) ptgmy = 0;
	}

	void extra_enems_killed (void) {
	}

	unsigned char this_enemy_kills (void) {
		return _en_t == NK_BODY_T;
	}
#endif

void nk_k_place (void) {
	// The original's (draw_screen): 20 random tries at the 8 places, then
	// in order. Note: a random try on a platform can't succeed (it checks
	// the same tile twice), so random tries only find the floor; the
	// ordered search starts with the platforms. As the original.
	nk_k_on = 0;
	for (nk_i = 0; nk_i < 20; nk_i ++) {
		nk_k_y = 48 + (rand () & 1) * 64;
		nk_k_x = 16 + (rand () & 3) * 48;
		if (nk_solid (nk_k_x >> 3, nk_k_y >> 3) == 0 && (nk_k_y == 112 || (nk_k_y == 48 && nk_solid (nk_k_x >> 3, 1 + (nk_k_y >> 3))))) {
			nk_k_on = 1; break;
		}
	}

	if (nk_k_on == 0) {
		nk_k_on = 1; nk_k_y = 48; nk_k_x = 16;
		while (nk_k_on && (nk_solid (nk_k_x >> 3, nk_k_y >> 3) || (nk_k_y == 48 && nk_solid (nk_k_x >> 3, 2 + (nk_k_y >> 3)) == 0))) {
			nk_k_x += 48;
			if (nk_k_x == 208) {
				nk_k_x = 16;
				nk_k_y += 64;
				if (nk_k_y == 176) nk_k_on = 0;
			}
		}
	}

	nk_k_status = K_IDLE; nk_k_frame = 0; nk_k_ct = nk_k_step = 0;

	// malotes: head (vanilla moving platform), body, puff
	gpit = enoffs;
	malotes [gpit].t = nk_k_on ? NK_HEAD_T : 0;
	malotes [gpit].x = nk_k_x + 16;
	malotes [gpit].y = malotes [gpit].y1 = malotes [gpit].y2 = nk_k_y - 8;
	malotes [gpit].x1 = 0; malotes [gpit].x2 = 240;
	malotes [gpit].mx = 0; malotes [gpit].my = 1;
	gpit ++;
	malotes [gpit].t = nk_k_on ? NK_BODY_T : 0;
	malotes [gpit].x = nk_k_x + 16;
	malotes [gpit].y = nk_k_y;
	malotes [gpit].mx = malotes [gpit].my = 0;
	gpit ++;
	malotes [gpit].t = NK_PUFF_T;
	malotes [gpit].mx = malotes [gpit].my = 0;
	nk_puff_ct = 0;
}

// ============================================================================
// Map renderer
// ============================================================================

#ifdef ENABLE_CUSTOM_MAP_RENDERER
	unsigned char nk_plat_tiles [] = { 6, 7, 8, 9, 11 };
	// Style 3: the CPC alternates two tiles (A B A, or B A B on odd platforms);
	// the ZX has three (A B C).
	#ifdef CPC
		unsigned char nk_plat3 [] = { 9, 10, 9, 10, 9, 10 };
	#else
		unsigned char nk_plat3 [] = { 9, 10, 26, 9, 10, 26 };
	#endif

	void custom_draw_scr_background () {
		nk_level = n_pant - 1;

		// Frame (solid, not drawn) and background
		for (rdi = 0; rdi < 150; rdi ++) {
			rdx = rdi % 15; rdy = rdi / 15;
			map_buff [rdi] = (rdy > 7 || rdx == 0 || rdx > 12) ? NK_T_BLACK : NK_T_BG;
		}

		// Cracks
		for (nk_i = 0; nk_i < 5; nk_i ++) {
			rdx = rand () % 12; rdy = rand () & 7;
			map_buff [rdy * 15 + rdx + 1] = 1;
		}

		// Platforms: bit n, 4 per row at rows 0 and 4, 3 tiles wide
		nk_p = levels + nk_level * 21;
		rdi = *nk_p ++;
		nk_t = nk_level % 5;
		for (nk_i = 0; nk_i < 8; nk_i ++) {
			if (rdi & (1 << nk_i)) {
				gpit = ((nk_i >> 2) << 2) * 15 + (nk_i & 3) * 3 + 1;
				for (nk_j = 0; nk_j < 3; nk_j ++) {
					map_buff [gpit + nk_j] = nk_t == 3 ? nk_plat3 [((nk_i & 1) * 3) + nk_j] : nk_plat_tiles [nk_t];
				}
			}
		}

		// Fixed objects: random graphic, never twice in a row
		nk_old = 4;
		for (nk_i = 0; nk_i < 10; nk_i ++) {
			do {
				nk_t = rand () % 3;
			} while (nk_t == nk_old);
			nk_old = nk_t;
			rdi = *nk_p ++;
			if (rdi) map_buff [(rdi >> 4) * 15 + (rdi & 15) + 1] = 2 + nk_t;
		}

		// Boxes
		for (nk_i = 0; nk_i < 10; nk_i ++) {
			rdi = *nk_p ++;
			if (rdi) map_buff [(rdi >> 4) * 15 + (rdi & 15) + 1] = NK_T_BOX;
		}

		// map_attr, and draw the game area from the bottom up, as the
		// original's curtain.
		for (rdi = 0; rdi < 150; rdi ++) map_attr [rdi] = comportamiento_tiles [map_buff [rdi]];
		for (rdy = 8; rdy; ) {
			rdy --;
			for (rdx = 1; rdx < 13; rdx ++) {
				draw_coloured_tile (VIEWPORT_X + (rdx << 1), VIEWPORT_Y + (rdy << 1), map_buff [rdy * 15 + rdx]);
			}
			SCREEN_UPDATE;
		}
	}
#endif

// ============================================================================
// Custom code
// ============================================================================

#ifdef ENABLE_CUSTOM_CODE
	void entering_game (void) {
		player.life = PLAYER_LIFE;
		n_pant = 1 + nk_start_level;
		nk_level = nk_start_level;
		nk_win = nk_quit = 0;
		nk_hud ();
		nk_level_box ();
	}

	void entering_screen (void) {
		// Level (re)start
		nk_carry = 0;
		nk_dying = 0;
		player.estado = EST_NORMAL;
		player.vx = player.vy = 0;
		player.saltando = 0;
		player.facing = SIDEVIEW_FACING_LEFT;

		// Nanako at column 11 of the game area, the lowest free place
		gpx = 192; gpy = 112;
		while (attr (12, gpy >> 4) & 8) gpy -= 16;
		player.x = gpx << 6; player.y = gpy << 6;

		// Something else every time
		seed += nk_frames;
		nk_k_place ();
	}

	void before_vsync (void) {
		nk_frames ++;

		// Karakasa (vanilla collision) or R / quit keys
		if (nk_dying == 0) {
			if (player.is_dead) {
				player.is_dead = 0;
				nk_die ();
			} else {
				nk_key_code = NK_KEY_RETRY;
				if (nk_key ()) { peta_el_beeper (SFX_PLAYER_HIT); nk_die (); }
				nk_key_code = NK_KEY_QUIT;
				if (nk_key ()) { peta_el_beeper (SFX_PLAYER_HIT); nk_quit = 1; nk_die (); }
			}
		} else {
			player.is_dead = 0;
			nk_dying ++;
			if (nk_dying == 16 + 12) {
				// After the puff and a wait: game over or the level again.
				if (player.life == 0) {
					player.life = -1; 			// Vanilla game over
				} else {
					player.life --;
					nk_hud ();
					nk_level_box ();
					on_pant = 0xff; 			// Redraw: draw_scr + entering_screen
				}
			}
		}
	}
#endif
