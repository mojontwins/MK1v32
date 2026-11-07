// Nin Nin - EL CASTLER's puzzles (load 3, NN_CASTLE). Hand-written, from the
// NES game's my/hooks.h (level 11). The coordinates are the NES ones moved to
// the 15x10 screens (tools/nn_assets.py's reduction: the rows and columns the
// converter deleted in each screen); where a NES tile fell in a deleted line,
// the next one is used.
//
//  * Screens 1, 3: entering them is not a safe place to come back to.
//  * Screens 5, 7, 23 and 15: spring traps (behaviour 48: the air over a floor
//    turns into spikes when stepped on); 5 and 7 also have a hidden way
//    through (behaviour 0) at the right.
//  * Screen 13: the moving platform (enemy 0) is off until FIRE on the face
//    at the bottom left.
//  * Screen 17: walking into the column at x 5 (rows 1-4) breaks the wall
//    in the middle (row 5, x 6-9), once.
//  * Screen 18: FIRE on the face at the top (8, 3) builds a column of
//    breakable blocks at x 9, rows 4-6.
//  * Screen 20: when its rays are off, two bricks (44) appear instead of the
//    fences opening.

#define NN_CASTLE_LEVEL 	9
unsigned char nn_cf0, nn_fzone;

// behaviour changes: x, y, behaviour; 0xff ends
unsigned char nn_alm5 [] = { 4, 2, 48, 8, 2, 48, 10, 2, 48, 13, 7, 0, 13, 8, 0, 0xff };
unsigned char nn_alm7 [] = { 4, 3, 48, 8, 3, 48, 10, 3, 48, 13, 7, 0, 13, 8, 0, 0xff };
unsigned char nn_alm15 [] = { 2, 3, 48, 0xff };
unsigned char nn_alm23 [] = { 6, 4, 48, 4, 4, 48, 0xff };

void nn_attr_list (void) {
	// nn_p: x, y, behaviour...
	while (*nn_p != 0xff) {
		map_attr [nn_p [0] + (nn_p [1] << 4) - nn_p [1]] = nn_p [2];
		nn_p += 3;
	}
}

#define NN_PLAYER_AT(x,y) (((gpx + 8) >> 4) == (x) && ((gpy + 8) >> 4) == (y))

void nn_castle_screen (void) {
	nn_cf0 = 0; nn_fzone = 0;
	if (nn_l != NN_CASTLE_LEVEL) return;
	nn_p = 0;
	switch (n_pant) {
		case 5: nn_p = nn_alm5; break;
		case 7: nn_p = nn_alm7; break;
		case 15: nn_p = nn_alm15; break;
		case 23: nn_p = nn_alm23; break;
		case 13:
			// the platform is off
			malotes [enoffs].t = 0;
			en_an_next_frame [0] = spr_empty;
			break;
		case 17:
			nn_fzone = 1;
			break;
	}
	if (nn_p) nn_attr_list ();
}

unsigned char nn_fire_zoned (void) {
	// screen 17: x 80-95, y 16-79 fires by itself
	if (nn_fzone && gpx + 8 >= 80 && gpx + 8 <= 95 && gpy + 8 >= 16 && gpy + 8 <= 79) return 1;
	return 0;
}

unsigned char nn_castle_fire (void) {
	if (nn_l != NN_CASTLE_LEVEL) return 0;
	if (nn_fzone && nn_fire_zoned ()) {
		peta_el_beeper (5);
		_y = 5; for (_x = 6; _x < 10; _x ++) { _t = tileset_mappings [0]; nn_set_tile (); }
		nn_fzone = 0;
		return 1;
	}
	if (nn_cf0) return 0;
	if (n_pant == 18 && NN_PLAYER_AT (8, 2)) {
		peta_el_beeper (5);
		_x = 9; for (_y = 4; _y < 7; _y ++) { _t = 29; nn_set_tile (); }
		nn_cf0 = 1;
		return 1;
	}
	if (n_pant == 13 && NN_PLAYER_AT (3, 8)) {
		// the platform comes back
		peta_el_beeper (8);
		malotes [enoffs].t = 4;
		enit = 0; enems_en_an_calc (3);
		nn_cf0 = 1;
		return 0; 		// (the face's own puzzle goes on, as on the NES)
	}
	return 0;
}

unsigned char nn_castle_rays (void) {
	// all the rays of the screen are off: 1 if handled here
	if (nn_l == NN_CASTLE_LEVEL && n_pant == 20) {
		_x = 8; _y = 3; _t = 44; nn_set_tile ();
		_x = 5; _y = 1; _t = 44; nn_set_tile ();
		return 1;
	}
	return 0;
}

unsigned char nn_screen_is_safe (void) {
	return !(nn_l == NN_CASTLE_LEVEL && (n_pant == 1 || n_pant == 3));
}
