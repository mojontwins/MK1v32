// MTE MK1 v3.2
// Copyleft 2011, 2026 by The Mojon Twins

// Custom flick.
// This file is used if you #define ENABLE_CUSTOM_FLICK
// Do your thing, return 0 to prevent screen flick.

// Uwol: leaving by a side means coming in by the other one (the player warps
// when it is fully inside the black column) unless there is an obstacle
// there. Falling off the bottom kills.

unsigned char uwol_warp_blocked (unsigned char x) {
	// Normal collision: can't warp into an obstacle on the other side.
	if ((attr (x, gpy >> 4) | attr (x, (gpy + 15) >> 4)) & 8) {
		player.vx = 0;
		return 1;
	}
	return 0;
}

unsigned char flick_left (void) {
	if (uwol_warp_blocked (14) == 0) {
		gpx = 224;
		player.x = 224 << 6;
	}
	return 0;
}

unsigned char flick_right (void) {
	if (uwol_warp_blocked (0) == 0) {
		gpx = 0;
		player.x = 0;
	}
	return 0;
}

unsigned char flick_up (void) {
	return 0;
}

unsigned char flick_down (void) {
	if ((player.estado & EST_MUR) == 0) uwol_die ();
	return 0;
}
