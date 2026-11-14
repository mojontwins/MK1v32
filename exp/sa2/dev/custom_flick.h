// MTE MK1 v3.2
// Copyleft 2011, 2026 by The Mojon Twins

// Sir Ababol 2: a 1 x 20 tower. No screens to the sides; no going up while
// frozen (the original's game loop).

unsigned char flick_left (void) {
	return 0;
}

unsigned char flick_right (void) {
	return 0;
}

unsigned char flick_up (void) {
	return (sa2_state != SA2_ST_ICE);
}

unsigned char flick_down (void) {
	return 1;
}
