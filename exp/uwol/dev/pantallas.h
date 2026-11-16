// MTE MK1 v3.2
// Copyleft 2011, 2026 by The Mojon Twins

// Customise title / game over / ending here

// Uwol: title, pyramid, endings (one drawing, two texts) and game over.

extern unsigned char s_title [];
extern unsigned char s_marco [];
extern unsigned char s_ending [];

#ifdef CPC
	#asm
		._s_title
			BINARY "titlec_cpc.bin"
		._s_marco
			BINARY "marcoc_cpc.bin"
		._s_ending
			BINARY "endingc_cpc.bin"
	#endasm
#else
	#asm
		._s_title
			BINARY "titlec.bin"
		._s_marco
			BINARY "marcoc.bin"
		._s_ending
			BINARY "endingc.bin"
	#endasm
#endif

#ifdef CPC
	#define TXT(c)
	#define SCREEN_UPDATE cpc_UpdateNow (0)
#else
	#define TXT(c) c,
	#define SCREEN_UPDATE sp_UpdateNow ()
#endif

void title_screen (void) {
	#if defined CPC && defined AUTO_SPLIT
		#ifdef ALWAYS_SPLIT
			#asm
					ld  hl, pal_general
					ld  (inject_pal + 1), hl
			#endasm
		#else
			do_split = 0;
			#asm
					call pal_general
			#endasm
		#endif
	#endif

	#ifndef CPC
		#asm
			call SPUpdateNow
		#endasm
	#endif

	blackout ();
	#asm
			ld  hl, _s_title
			call _unpack_screen
	#endasm
	#ifdef CPC
		cpc_ShowTileMap (1);
	#endif

	#ifdef CPC
		draw_text (5, 21, "1 KEYBOARD  2 JOYSTICK");
	#else
		draw_text (5, 21, 71, "1 KEYBOARD  2 KEMPSTON");
		draw_text (11, 22, 71, "3 SINCLAIR");
	#endif

	#ifdef CPC
		cpc_UpdateNow (0);
		AY_PLAY_MUSIC (0);
	#else
		#asm
				call SPUpdateNow
		#endasm
		#ifdef MODE_128K_DUAL
			if (is128k) ay_play_music (MUSIC_TITLE); else
		#endif
		#asm
				call musicstart
		#endasm
	#endif

	select_controls ();
}

void uwol_marco (void) {
	// Back to the game frame after a full screen.
	blackout ();
	#asm
			ld  hl, _s_marco
			call _unpack_screen
	#endasm
	#ifdef CPC
		cpc_ShowTileMap (1);
	#endif
	uwol_hud ();
}

// The pyramid bricks are tiles 22-24's top halves: a 2x1 chars strip per room.
#define UWOL_BRICK_CHAR 	(64 + (22 << 2))

unsigned char uwol_brick_attr [] = { 78, 86, 94 }; 	// Not visited, visited, current

void uwol_brick (unsigned char k) {
	// Prints room k's brick at (rdx, rdy).
	#ifdef CPC
		// CPC: tiles 22, 23, 24 carry the colours
		_x = rdx; _y = rdy; _t = UWOL_BRICK_CHAR + (k << 2);
		#asm
				call __tile_address 		// DE = buffer address
				ld  a, (__t)
				ld  (de), a
				inc de
				inc a
				ld  (de), a

				ld  a, (__x)
				ld  e, a
				ld  a, (__y)
				ld  d, a
				call cpc_UpdTileTable
				inc e
				call cpc_UpdTileTable
		#endasm
	#else
		sp_PrintAtInv (rdy, rdx, uwol_brick_attr [k], UWOL_BRICK_CHAR);
		sp_PrintAtInv (rdy, rdx + 1, uwol_brick_attr [k], UWOL_BRICK_CHAR + 1);
	#endif
}

void show_pyramid (void) {
	// The original's pyramid screen: logo, a brick per room (visited, current)
	// and how it's going.
	saca_a_todo_el_mundo_de_aqui ();
	SCREEN_UPDATE;
	blackout ();
	UWOL_MUSIC (MUSIC_PYRAMID);

	// Logo: tiles 27-30 are the original's 8x2 chars
	for (gpit = 0; gpit < 4; gpit ++) draw_coloured_tile (12 + (gpit << 1), 2, 27 + gpit);

	rdi = 0;
	for (uwol_i = 0; uwol_i < 10; uwol_i ++) {
		rdx = 14 - uwol_i;
		rdy = 6 + uwol_i;
		for (gpit = 0; gpit <= uwol_i; gpit ++) {
			uwol_brick (rdi == n_pant ? 2 : uwol_visited [rdi]);
			rdx += 2;
			rdi ++;
		}
	}

	draw_coloured_tile (3, 18, 25);
	draw_text (5, 19, TXT (71) "X");
	draw_num (6, 19, player.life, 2);

	draw_text (10, 19, TXT (4) "SCORE");
	draw_text (15, 19, TXT (7) "X");
	draw_num (16, 19, uwol_score, 5);

	draw_coloured_tile (23, 18, 26);
	draw_text (25, 19, TXT (71) "X");
	draw_num (26, 19, uwol_total_coins, 3);

	SCREEN_UPDATE;

	no_break ();
	espera_activa (150);

	uwol_marco ();
}

void game_ending (void) {
	// uwol_good: enough money (the end) or not (back to the top).
	#if defined CPC && defined AUTO_SPLIT
		#ifdef ALWAYS_SPLIT
			#asm
					ld  hl, pal_general
					ld  (inject_pal + 1), hl
			#endasm
		#else
			#asm
					call pal_general
			#endasm
		#endif
	#endif

	#ifdef SPECCY
		#asm
			call SPUpdateNow
		#endasm
	#endif

	blackout ();
	#asm
			ld  hl, _s_ending
			call _unpack_screen
	#endasm

	if (uwol_good) {
		draw_text (5, 18, TXT (68) "C'MON, GUYS, CHEER UP!%I'LL BUY YA SOME BEER!");
	} else {
		draw_text (8, 17, TXT (68) "NOT ENOUGH MONEY!%");
		draw_text (5, 18, TXT (68) "I THINK I'LL HAVE TO");
		draw_text (6, 19, TXT (68) "ENTER THE MANSION");
		draw_text (10, 20, TXT (68) "ONCE AGAIN!");
	}

	#ifdef CPC
		cpc_ShowTileMap (1);
		cpc_UpdateNow (0);
		AY_PLAY_MUSIC (uwol_good ? MUSIC_ENDING_OK : MUSIC_ENDING_KO);
	#else
		#asm
				call SPUpdateNow
		#endasm
		UWOL_MUSIC (uwol_good ? MUSIC_ENDING_OK : MUSIC_ENDING_KO);
		#ifdef SFX_IDLE_PLAYER
			peta_el_beeper (uwol_good ? SFX_ENDING : SFX_GAME_OVER); 	// Plays during the wait (48K)
		#else
			for (gpit = 0; gpit < 4; gpit ++) {
				peta_el_beeper (7);
				peta_el_beeper (2);
			}
			peta_el_beeper (9);
		#endif
	#endif

	no_break ();
	espera_activa (500);

	if (uwol_good) {
		blackout ();
		draw_text (12, 11, TXT (70) "THE END");
		SCREEN_UPDATE;
		no_break ();
		espera_activa (1000);
	}
}

void game_over (void) {
	#ifdef CPC
		AY_STOP_SOUND ();
	#endif

	#asm
			ld  a, 10
			ld  (__x), a
			ld  a, 11
			ld  (__y), a
			ld  a, 21
			ld  (__x2), a
			ld  a, 13
			ld  (__y2), a
			ld  a, 72
			ld  (__t), a
	#endasm
	draw_rectangle ();
	draw_text (11, 12,
		#ifndef CPC
			64+1*8+7,
		#endif
		"GAME OVER!");


	#ifdef CPC
		cpc_UpdateNow (0);
		AY_PLAY_MUSIC (2);
	#else
		#asm
				call SPUpdateNow
		#endasm
		UWOL_MUSIC (MUSIC_GOVER);
		peta_el_beeper (5);
	#endif

	no_break ();
	espera_activa (5000);

	#ifdef CPC
		AY_STOP_SOUND ();
	#endif
}
