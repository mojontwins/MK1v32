// MTE MK1 v3.2
// Copyleft 2011, 2026 by The Mojon Twins

// Custom flick (Punchy: the map is 4 x 1 screens, nothing out of it)
// Return 0 to prevent screen flick.

unsigned char flick_left (void) {
	return n_pant > 0;
}

unsigned char flick_right (void) {
	return n_pant < MAP_W - 1;
}

unsigned char flick_up (void) {
	return 1;
}

unsigned char flick_down (void) {
	return 0;
}
