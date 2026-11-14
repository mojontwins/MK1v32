#include "sa2_load.h"
#include "sa2_cuts.h"

// MTE MK1 v3.2
// Copyleft 2011, 2026 by The Mojon Twins
// Sir Ababol 2: the hooks. The game is in sa2_play.h.

// (level: sa2_play.h's renderer seeds with it)
unsigned char level, level_cont;

#include "sa2_play.h"

void before_vsync (void) {
}

void custom_game_start (void) {
	#asm
			xor a
			ld  (_sa2_state), a
			ld  (_sw_on), a
			ld  (_sa2_swording), a
			ld  a, PLAYER_AX
			ld  (_sa2_ax), a
			ld  a, PLAYER_RX
			ld  (_sa2_rx), a
			ld  a, 255
			ld  (_sa2_o_coins), a
			ld  (_sa2_o_keys), a
			// A new game, or the next level (life and keys carried)
			ld  a, (_sa2_carry)
			or  a
			jr  nz, sa2_gs_carry
			ld  (_sa2_coins), a
			ret
		.sa2_gs_carry
			xor a
			ld  (_sa2_carry), a
			ld  a, (_sa2_sv_life)
			ld  (_player + 29), a
			ld  a, (_sa2_sv_keys)
			ld  (_player + 28), a
	#endasm
}

void custom_screen_enter (void) {
	// height, no blocks breaking
	#asm
			xor a
			ld  (_brk_f), a
			ld  (_brk_f + 1), a
			ld  (_sa2_perro), a
			ld  a, SA2_HEIGHT_X
			ld  (__x), a
			ld  a, SA2_HUD_Y
			ld  (__y), a
			ld  a, (_n_pant)
			jp  draw_2_digits_shortcut
	#endasm
}

void custom_pre_move (void) {
	#asm
			call _sa2_hud
			call _sa2_perro_check
			jp  _sa2_breakables_do
	#endasm
}

#ifdef ENABLE_CUSTOM_LEVELS
	// Sir Ababol 2: SA2_LEVELS levels per load (sa2_level.h, sa2_load.h).
	// level: 0-2 in the load; level_cont: 1 while going on to the next one.

	void custom_level_setup (void) {
		#asm
				call sa2_level_load
				jp  _sa2_level_screen

			// The level: map, enemies, tiles, behaviours, song
			.sa2_level_load
				// if (level_cont == 0) level = 0; (or SA2_DEBUG_LEVEL)
				ld  a, (_level_cont)
				or  a
				jr  nz, sls_1
			#ifdef SA2_DEBUG_LEVEL
				ld  a, SA2_DEBUG_LEVEL 		// (SA2FLAGS=-DSA2_DEBUG_LEVEL=n: start at level n of the load)
			#endif
				ld  (_level), a
			.sls_1
				xor a
				ld  (_level_cont), a

				// the level area: map + lock, enemies + hotspots, tile map (sa2_level.h)
				// dzx0_standard (lvl_area [level], level_area)
				ld  a, (_level)
				ld  hl, _lvl_area
				call sls_tab
				ld  de, _level_area
				call dzx0_standard

				// comportamiento_tiles [0..15] = lvl_behs [level * 16 ..]
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

				// bosses: 3 lives, their home
				call _sa2_level_enems

				// song 1 is the level's: wyz_songs [1] = wyz_songs [4 + level]
				ld  a, (_level)
				add a, 4
				ld  hl, _wyz_songs
				call sls_tab
				ld  (_wyz_songs + 2), hl
				ret

				// HL = (HL + 2A)
			.sls_tab
				add a, a
				add a, l
				ld  l, a
				jr  nc, sls_tab_1
				inc h
			.sls_tab_1
				ld  a, (hl)
				inc hl
				ld  h, (hl)
				ld  l, a
		#endasm
	}

	unsigned char custom_level_won (void) {
		#asm
				// if (level + 1 < SA2_LEVELS) { level ++; level_cont = 1; return 1; }
				// life and keys go on to the next level (init_player resets them)
				ld  a, (_player + 29)
				ld  (_sa2_sv_life), a
				ld  a, (_player + 28)
				ld  (_sa2_sv_keys), a
				ld  a, 1
				ld  (_sa2_carry), a

				ld  a, (_level)
				inc a
				cp  SA2_LEVELS
				jr  nc, slw_end
				ld  (_level), a
				ld  a, 1
				ld  (_level_cont), a
				ld  l, a
				ld  h, 0
				ret
			.slw_end
				// level_cont = 0; return 0;
				xor a
				ld  (_level_cont), a
				ld  l, a
				ld  h, a
		#endasm
	}
#endif
