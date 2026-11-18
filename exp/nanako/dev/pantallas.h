// MTE MK1 v3.2
// Copyleft 2011, 2026 by The Mojon Twins

// Customise title / game over / ending here

// Nanako: title with password entry, game over in the game area, ending
// with the original's two texts.

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

void nk_title_draw (void) {
	blackout ();
	#asm
			ld  hl, _s_title
			call _unpack_screen
	#endasm
	#ifdef CPC
		cpc_ShowTileMap (1);
		draw_text (5, NK_TITLE_Y, "1 KEYBOARD  2 JOYSTICK");
	#else
		draw_text (5, NK_TITLE_Y, 71, "1 KEYBOARD  2 KEMPSTON");
		draw_text (5, NK_TITLE_Y + 1, 71, "3 SINCLAIR  P PASSWORD");
	#endif
	#ifdef CPC
		draw_text (11, NK_TITLE_Y + 1, "P PASSWORD");
	#endif
	if (nk_start_level) {
		nk_password_text (nk_start_level);
		draw_text (8, NK_TITLE_Y + 3, TXT (70) "START AT");
		draw_text (17, NK_TITLE_Y + 3, TXT (71) nk_buf);
	}
	SCREEN_UPDATE;
}

void nk_read_pw_keys (void) {
	// rdi = bits 0-4: O, P, Q, A, SPACE
	#asm
		halt
	#endasm
	nk_key_code = NK_KEY_O; rdi = nk_key ();
	nk_key_code = NK_KEY_P; if (nk_key ()) rdi |= 2;
	nk_key_code = NK_KEY_Q; if (nk_key ()) rdi |= 4;
	nk_key_code = NK_KEY_A; if (nk_key ()) rdi |= 8;
	nk_key_code = NK_KEY_SPACE; if (nk_key ()) rdi |= 16;
}

void nk_wait_no_key (void) {
	do nk_read_pw_keys (); while (rdi);
}

void nk_enter_password (void) {
	// The original's: O / P move the cursor, Q / A change the letter, SPACE
	// accepts. A wrong password starts at level 0.
	nk_buf [0] = nk_buf [1] = nk_buf [2] = nk_buf [3] = 'A'; nk_buf [4] = 0;
	nk_j = 0;
	draw_text (8, NK_TITLE_Y + 3, TXT (70) "PASSWORD        ");
	nk_wait_no_key ();
	while (1) {
		draw_text (17, NK_TITLE_Y + 3, TXT (71) nk_buf);
		draw_text (17, NK_TITLE_Y + 4, TXT (70) "    ");
		draw_text (17 + nk_j, NK_TITLE_Y + 4, TXT (70) "]");
		SCREEN_UPDATE;

		do nk_read_pw_keys (); while (rdi == 0);
		if (rdi & 16) break;
		if (rdi & 1) nk_j = (nk_j - 1) & 3;
		if (rdi & 2) nk_j = (nk_j + 1) & 3;
		if (rdi & 4) { nk_buf [nk_j] ++; if (nk_buf [nk_j] > 'Z') nk_buf [nk_j] = 'A'; }
		if (rdi & 8) { nk_buf [nk_j] --; if (nk_buf [nk_j] < 'A') nk_buf [nk_j] = 'Z'; }
		nk_wait_no_key ();
	}
	nk_wait_no_key ();

	nk_start_level = 0;
	for (nk_i = 0; nk_i < NK_LEVELS; nk_i ++) {
		nk_p = passwords + (nk_i << 2);
		if ((nk_p [0] ^ 63) == nk_buf [0] && (nk_p [1] ^ 63) == nk_buf [1] && (nk_p [2] ^ 63) == nk_buf [2] && (nk_p [3] ^ 63) == nk_buf [3]) {
			nk_start_level = nk_i;
		}
	}
}

void title_screen (void) {
	#ifndef CPC
		#asm
			call SPUpdateNow
		#endasm
	#endif

	nk_title_draw ();

	#ifdef CPC
		AY_PLAY_MUSIC (MUSIC_TITLE);
	#else
		#ifdef MODE_128K_DUAL
			if (is128k) ay_play_music (MUSIC_TITLE); else
		#endif
		#asm
				call musicstart
		#endasm
	#endif

	// 1, 2, (3): controls, P: password
	while (1) {
		nk_key_code = NK_KEY_PASSWORD;
		if (nk_key ()) {
			nk_enter_password ();
			nk_title_draw ();
		}
		#ifdef CPC
			pad_read ();
			if ((pad_this_frame & (sp_AUX3 | sp_AUX4)) != (sp_AUX3 | sp_AUX4)) break;
		#else
			if (sp_KeyPressed (key_1) || sp_KeyPressed (key_2) || sp_KeyPressed (key_3)) break;
		#endif
		#ifndef CPC
			// The beeper tune returns on a key: go on with it
			#ifdef MODE_128K_DUAL
				if (is128k == 0)
			#endif
			{
				#asm
					call musicresume
				#endasm
			}
		#endif
	}

	#ifdef CPC
		pad1 = 0xff; 		// So select_controls sees the key that broke the loop
	#endif
	select_controls ();
	NK_MUSIC_STOP ();
}

void game_ending (void) {
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

	draw_text (11, 8, TXT (70) "CONGRATS!");
	draw_text (11, 10, TXT (71) "NOW MYA IS");
	draw_text (13, 11, TXT (71) "SAFE!");

	#ifdef CPC
		cpc_ShowTileMap (1);
	#endif
	SCREEN_UPDATE;
	NK_MUSIC (MUSIC_TITLE);
	#ifdef SFX_IDLE_PLAYER
		peta_el_beeper (SFX_ENDING);
	#endif

	no_break ();
	espera_activa (250);

	draw_text (11, 13, TXT (70) "CYA IN HELL!");
	SCREEN_UPDATE;
	espera_activa (5000);
	NK_MUSIC_STOP ();
}

void game_over (void) {
	NK_MUSIC_STOP ();

	// The original's: three lines in the game area
	draw_text (VIEWPORT_X + 9, VIEWPORT_Y + 7, TXT (2) "            ");
	draw_text (VIEWPORT_X + 9, VIEWPORT_Y + 8, TXT (66) " GAME  OVER ");
	draw_text (VIEWPORT_X + 9, VIEWPORT_Y + 9, TXT (2) "            ");
	SCREEN_UPDATE;
	peta_el_beeper (SFX_GAME_OVER);

	no_break ();
	espera_activa (250);
}
