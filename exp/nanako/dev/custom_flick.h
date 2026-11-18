// MTE MK1 v3.2
// Copyleft 2011, 2026 by The Mojon Twins

// Custom flick.
// This file is used if you #define ENABLE_CUSTOM_FLICK
// Do your thing, return 0 to prevent screen flick.

// Nanako: the sides and the bottom are solid. Jumping off the top of the
// game area is the exit: next level, or the ending after the last one.

unsigned char flick_left (void) {
	return 0;
}

unsigned char flick_right (void) {
	return 0;
}

unsigned char flick_up (void) {
	if (nk_dying) return 0;

	if (n_pant == NK_LEVELS) {
		saca_a_todo_el_mundo_de_aqui ();
		NK_MUSIC_STOP ();
		game_ending ();
		playing = 0;
	} else {
		n_pant ++;
		nk_level = n_pant - 1;
		nk_level_box ();
	}
	return 0;
}

unsigned char flick_down (void) {
	return 0;
}
