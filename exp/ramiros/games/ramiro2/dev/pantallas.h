// MTE MK1 v3.2
// Copyleft 2011, 2026 by The Mojon Twins

// Customise title / game over / ending here

extern unsigned char s_title [];
extern unsigned char s_marco [];
extern unsigned char s_ending [];

#asm
	._s_title
		BINARY "titlec.bin"
	._s_marco

	#ifndef DIRECT_TO_PLAY
			BINARY "marcoc.bin"
	#endif
			
	._s_ending
		BINARY "endingc.bin"
#endasm

// Ramiro: the title offers 1 = keys W A S D, 2 = keys O P Q A,
// 3 = Kempston, 4 = Sinclair. Jump is UP or FIRE (Ramiro 4's BOTH_KEYS_JUMP),
// DOWN is hover and action.
void ramiro_select_controls (void) {
	#ifdef CPC
		select_controls ();
	#else
		while (1) {
			if (sp_KeyPressed (key_1)) {
				joyfunc = sp_JoyKeyboard;
				keys.up = 0x02fb; keys.down = 0x02fd;	// W S
				keys.left = 0x01fd; keys.right = 0x04fd;	// A D
				keys.fire = 0x047f; 						// M
				break;
			} else if (sp_KeyPressed (key_2)) {
				joyfunc = sp_JoyKeyboard;
				keys.up = 0x01fb; keys.down = 0x01fd;	// Q A
				keys.left = 0x02df; keys.right = 0x01df;	// O P
				keys.fire = 0x017f; 						// SPACE
				break;
			} else if (sp_KeyPressed (key_3)) {
				joyfunc = sp_JoyKempston; break;
			} else if (sp_KeyPressed (0x08f7)) {
				joyfunc = sp_JoySinclair1; break;
			}
		}
	#endif
}

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
		cpc_UpdateNow (0);
		AY_PLAY_MUSIC (0);
	#else 
		#asm 
				call SPUpdateNow
				call musicstart
		#endasm
	#endif

	ramiro_select_controls ();
}

void game_ending (void) {
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

	#ifdef CPC
		cpc_ShowTileMap (1);
		AY_PLAY_MUSIC (0);
	#else
		#asm 
				call SPUpdateNow
		#endasm
		for (gpit = 0; gpit < 4; gpit ++) {
			peta_el_beeper (7);
			peta_el_beeper (2);
		}
		peta_el_beeper (9);
	#endif
	
	no_break ();
	espera_activa (5000);

	#ifdef CPC
		AY_STOP_SOUND ();
	#endif
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
		peta_el_beeper (5);
	#endif

	no_break ();
	espera_activa (5000);

	#ifdef CPC
		AY_STOP_SOUND ();
	#endif
}
